/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2025 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "directionalPressureGradientExplicitSource.H"
#include "fvMatrices.H"
#include "DimensionedField.H"
#include "IFstream.H"
#include "addToRunTimeSelectionTable.H"
#include "transform.H"
#include "surfaceInterpolate.H"
#include "turbulenceModel.H"
#include "turbulentTransportModel.H"
#include "turbulentFluidThermoModel.H"
#include "vectorFieldIOField.H"
#include "FieldField.H"
#include "emptyFvPatchFields.H"

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(directionalPressureGradientExplicitSource, 0);
    addToRunTimeSelectionTable
    (
        option,
        directionalPressureGradientExplicitSource,
        dictionary
    );
}
}


// * * * * * * * * * * * * * Static Member Data  * * * * * * * * * * * * * * //

const Foam::Enum
<
    Foam::fv::directionalPressureGradientExplicitSource::pressureDropModel
>
Foam::fv::directionalPressureGradientExplicitSource::pressureDropModelNames_
({
    { pressureDropModel::pVolumetricFlowRateTable, "volumetricFlowRateTable" },
    { pressureDropModel::pConstant, "constant" },
    { pressureDropModel::pDarcyForchheimer, "DarcyForchheimer" },
});


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::fv::directionalPressureGradientExplicitSource::initialise()
{
    const faceZone& fZone = mesh_.faceZones()[zoneID_];

    // Total number of faces selected
    label numFaces = fZone.size();

    faceId_.resize_nocopy(numFaces);
    facePatchId_.resize_nocopy(numFaces);

    numFaces = 0;

    // TDB: handle multiple zones
    {
        forAll(fZone, i)
        {
            const label meshFacei = fZone[i];

            // Internal faces
            label faceId = meshFacei;
            label facePatchId = -1;

            // Boundary faces
            if (!mesh_.isInternalFace(meshFacei))
            {
                facePatchId = mesh_.boundaryMesh().whichPatch(meshFacei);
                const polyPatch& pp = mesh_.boundaryMesh()[facePatchId];

                if (isA<emptyPolyPatch>(pp))
                {
                    continue;  // Ignore empty patch
                }

                const auto* cpp = isA<coupledPolyPatch>(pp);

                if (cpp && !cpp->owner())
                {
                    continue;  // Ignore neighbour side
                }

                faceId = pp.whichFace(meshFacei);
            }

            if (faceId >= 0)
            {
                faceId_[numFaces] = faceId;
                facePatchId_[numFaces] = facePatchId;

                ++numFaces;
            }
        }
    }

    // Shrink to size used
    faceId_.resize(numFaces);
    facePatchId_.resize(numFaces);
}


void Foam::fv::directionalPressureGradientExplicitSource::writeProps
(
    const vectorField& gradP
) const
{
    // Only write on output time
    if (mesh_.time().writeTime())
    {
        IOdictionary propsDict
        (
            IOobject
            (
                name_ + "Properties",
                mesh_.time().timeName(),
                "uniform",
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            )
        );
        propsDict.add("gradient", gradP);
        propsDict.regIOobject::write();
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::directionalPressureGradientExplicitSource::
directionalPressureGradientExplicitSource
(
    const word& sourceName,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    fv::cellSetOption(sourceName, modelType, dict, mesh),
    model_(pressureDropModelNames_.get("model", coeffs_)),
    gradP0_(cells_.size(), Zero),
    dGradP_(cells_.size(), Zero),
    gradPporous_(cells_.size(), Zero),
    flowDir_(coeffs_.get<vector>("flowDir")),
    invAPtr_(nullptr),
    D_(0),
    I_(0),
    length_(0),
    pressureDrop_(0),
    flowRate_(),
    faceZoneName_(coeffs_.get<word>("faceZone")),
    zoneID_(mesh_.faceZones().findZoneID(faceZoneName_)),
    faceId_(),
    facePatchId_(),
    relaxationFactor_(coeffs_.getOrDefault<scalar>("relaxationFactor", 0.3)),
    cellFaceMap_(cells_.size(), -1)
{
    coeffs_.readEntry("fields", fieldNames_);

    flowDir_.normalise();

    if (fieldNames_.size() != 1)
    {
        FatalErrorInFunction
            << "Source can only be applied to a single field.  Current "
            << "settings are:" << fieldNames_ << exit(FatalError);
    }

    if (zoneID_ < 0)
    {
        FatalErrorInFunction
            << type() << " " << this->name() << ": "
            << "    Unknown face zone name: " << faceZoneName_
            << ". Valid face zones are: " << mesh_.faceZones().names()
            << exit(FatalError);
    }

    if (model_ == pVolumetricFlowRateTable)
    {
        flowRate_ = interpolationTable<scalar>(coeffs_);
    }
    else if (model_ == pConstant)
    {
        coeffs_.readEntry("pressureDrop", pressureDrop_);
    }
    else if (model_ == pDarcyForchheimer)
    {
        coeffs_.readEntry("D", D_);
        coeffs_.readEntry("I", I_);
        coeffs_.readEntry("length", length_);
    }
    else
    {
        FatalErrorInFunction
            << "Did not find mode " << model_ << nl
            << "Please set 'model' to one of "
            << pressureDropModelNames_
            << exit(FatalError);
    }

    fv::option::resetApplied();

    // Read the initial pressure gradient from file if it exists
    IFstream propsFile
    (
        mesh_.time().timePath()/"uniform"/(name_ + "Properties")
    );

    if (propsFile.good())
    {
        Info<< "    Reading pressure gradient from file" << endl;
        dictionary propsDict(propsFile);
        propsDict.readEntry("gradient", gradP0_);
    }

    Info<< "    Initial pressure gradient = " << gradP0_ << nl << endl;

    initialise();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fv::directionalPressureGradientExplicitSource::correct
(
    volVectorField& U
)
{
    const scalarField& rAU = invAPtr_().internalField();

    const scalarField magUn(mag(U), cells_);

    const auto& phi = mesh().lookupObject<surfaceScalarField>("phi");

    switch (model_)
    {
        case pDarcyForchheimer:
        {
            if (phi.dimensions() == dimVolume/dimTime)
            {
                const auto& turbModel =
                    mesh().lookupObject<incompressible::turbulenceModel>
                    (
                        turbulenceModel::propertiesName
                    );

                const scalarField nu(turbModel.nu(), cells_);

                gradPporous_ = -flowDir_*(D_*nu + I_*0.5*magUn)*magUn*length_;
            }
            else
            {
                const auto& turbModel =
                    mesh().lookupObject<compressible::turbulenceModel>
                    (
                        turbulenceModel::propertiesName
                    );

                const scalarField mu(turbModel.mu(),cells_);

                const scalarField rho(turbModel.rho(),cells_);

                gradPporous_ =
                    - flowDir_*(D_*mu + I_*0.5*rho*magUn)*magUn*length_;
            }
            break;
        }
        case pConstant:
        {
            gradPporous_ = -flowDir_*pressureDrop_;
            break;
        }

        case pVolumetricFlowRateTable:
        {
            scalar volFlowRate = 0;
            scalar totalphi = 0;

            forAll(faceId_, i)
            {
                label faceI = faceId_[i];
                if (facePatchId_[i] != -1)
                {
                    label patchI = facePatchId_[i];
                    totalphi += phi.boundaryField()[patchI][faceI];
                }
                else
                {
                    totalphi += phi[faceI];
                }
            }
            reduce(totalphi, sumOp<scalar>());

            if (phi.dimensions() == dimVolume/dimTime)
            {
                volFlowRate = mag(totalphi);
            }
            else
            {
                const auto& turbModel =
                    mesh().lookupObject<compressible::turbulenceModel>
                    (
                        turbulenceModel::propertiesName
                    );
                const scalarField rho(turbModel.rho(),cells_);
                const scalarField cv(mesh_.V(), cells_);
                scalar rhoAve = gWeightedAverage(cv, rho);
                volFlowRate = mag(totalphi)/rhoAve;
            }

            gradPporous_ = -flowDir_*flowRate_(volFlowRate);
            break;
        }
    }

    const faceZone& fZone = mesh_.faceZones()[zoneID_];

    labelList meshToLocal(mesh_.nCells(), -1);
    forAll(cells_, i)
    {
        meshToLocal[cells_[i]] = i;
    }

    labelList faceToCellIndex(fZone.size(), -1);
    const labelList& mc = fZone.masterCells();
    const labelList& sc = fZone.slaveCells();

    forAll(fZone, i)
    {
        label masterCellI = mc[i];

        if (meshToLocal[masterCellI] != -1 && masterCellI != -1)
        {
            faceToCellIndex[i] = meshToLocal[masterCellI];
        }
        else if (meshToLocal[masterCellI] == -1)
        {
            FatalErrorInFunction
                << "Did not find  cell " << masterCellI
                << "in cellZone :" << zoneName()
                << exit(FatalError);
        }
    }

    // Accumulate 'upstream' velocity into cells
    vectorField UfCells(cells_.size(), Zero);
    scalarField UfCellWeights(cells_.size(), Zero);

    const polyBoundaryMesh& pbm = mesh_.boundaryMesh();

    FieldField<Field, vector> upwindValues(pbm.size());

    forAll(U.boundaryField(), patchI)
    {
        const fvPatchVectorField& pf = U.boundaryField()[patchI];

        if (pf.coupled())
        {
            upwindValues.set(patchI, pf.patchNeighbourField());
        }
        else if (!isA<emptyFvPatchScalarField>(pf))
        {
            upwindValues.set(patchI, new vectorField(pf));
        }
    }

    forAll(fZone, i)
    {
        label faceI = fZone[i];
        label cellId = faceToCellIndex[i];

        if (cellId != -1)
        {
            label sourceCellId = sc[i];
            if (mesh_.isInternalFace(faceI))
            {
                scalar w = mesh_.magSf()[faceI];
                UfCells[cellId] += U[sourceCellId]*w;
                UfCellWeights[cellId] += w;
            }
            else if (fZone.flipMap()[i])
            {
                const label patchI = pbm.patchID(faceI);
                label localFaceI = pbm[patchI].whichFace(faceI);

                scalar w = mesh_.magSf().boundaryField()[patchI][localFaceI];

                if (upwindValues.set(patchI))
                {
                    UfCells[cellId] += upwindValues[patchI][localFaceI]*w;
                    UfCellWeights[cellId] += w;
                }
            }
        }
    }

    UfCells /= UfCellWeights;

    forAll(cells_, i)
    {
        label cellI = cells_[i];

        const vector Ufnorm(UfCells[i]/(mag(UfCells[i]) + SMALL));

        const tensor D(rotationTensor(Ufnorm, flowDir_));

        dGradP_[i] +=
            relaxationFactor_*
            (
                (D & UfCells[i]) - U[cellI]
            )/rAU[cellI];


        if (debug)
        {
            Info<< "Difference mag(U) = "
                << mag(UfCells[i]) - mag(U[cellI])
                << endl;
            Info<< "Pressure drop in flowDir direction : "
                << gradPporous_[i] << endl;
            Info<< "UfCell:= " << UfCells[i] << "U : " << U[cellI] << endl;
        }
    }
    writeProps(gradP0_ + dGradP_);
}


void Foam::fv::directionalPressureGradientExplicitSource::addSup
(
    fvMatrix<vector>& eqn,
    const label fieldI
)
{
    auto tSu = DimensionedField<vector, volMesh>::New
    (
        name_ + fieldNames_[fieldI] + "Sup",
        IOobject::NO_REGISTER,
        mesh_,
        dimensionedVector(eqn.dimensions()/dimVolume, Zero)
    );
    auto& Su = tSu.ref();

    UIndirectList<vector>(Su, cells_) = gradP0_ + dGradP_ + gradPporous_;

    eqn += tSu;
}


void Foam::fv::directionalPressureGradientExplicitSource::addSup
(
    const volScalarField& rho,
    fvMatrix<vector>& eqn,
    const label fieldI
)
{
    this->addSup(eqn, fieldI);
}


void Foam::fv::directionalPressureGradientExplicitSource::constrain
(
    fvMatrix<vector>& eqn,
    const label
)
{
    if (!invAPtr_)
    {
        invAPtr_.reset
        (
            new volScalarField
            (
                mesh_.newIOobject(IOobject::scopedName(name_, "invA")),
                1.0/eqn.A()
            )
        );
    }
    else
    {
        invAPtr_() = 1.0/eqn.A();
    }

    gradP0_ += dGradP_;
    dGradP_ = Zero;
}


void Foam::fv::directionalPressureGradientExplicitSource::writeData
(
    Ostream& os
) const
{
    NotImplemented;
}


bool Foam::fv::directionalPressureGradientExplicitSource::read
(
    const dictionary& dict
)
{
    const dictionary coeffs(dict.subDict(typeName + "Coeffs"));

    relaxationFactor_ =
        coeffs.getOrDefault<scalar>("relaxationFactor", 0.3);

    coeffs.readEntry("flowDir", flowDir_);
    flowDir_.normalise();

    if (model_ == pConstant)
    {
        coeffs.readEntry("pressureDrop", pressureDrop_);
    }
    else if (model_ == pDarcyForchheimer)
    {
        coeffs.readEntry("D", D_);
        coeffs.readEntry("I", I_);
        coeffs.readEntry("length", length_);
    }

    return false;
}


// ************************************************************************* //
