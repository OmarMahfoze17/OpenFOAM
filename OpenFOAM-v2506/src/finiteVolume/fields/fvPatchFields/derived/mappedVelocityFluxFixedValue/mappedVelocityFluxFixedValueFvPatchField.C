/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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

#include "mappedVelocityFluxFixedValueFvPatchField.H"
#include "fvPatchFieldMapper.H"
#include "mappedPatchBase.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "addToRunTimeSelectionTable.H"


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::mappedVelocityFluxFixedValueFvPatchField::
mappedVelocityFluxFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(p, iF),
    phiName_("phi")
{}


Foam::mappedVelocityFluxFixedValueFvPatchField::
mappedVelocityFluxFixedValueFvPatchField
(
    const mappedVelocityFluxFixedValueFvPatchField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchVectorField(ptf, p, iF, mapper),
    phiName_(ptf.phiName_)
{
    if (!isA<mappedPatchBase>(this->patch().patch()))
    {
        FatalErrorInFunction
            << "Patch type '" << p.type()
            << "' not type '" << mappedPatchBase::typeName << "'"
            << " for patch " << p.name()
            << " of field " << internalField().name()
            << " in file " << internalField().objectPath()
            << exit(FatalError);
    }
}


Foam::mappedVelocityFluxFixedValueFvPatchField::
mappedVelocityFluxFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchVectorField(p, iF, dict),
    phiName_(dict.getOrDefault<word>("phi", "phi"))
{
    if (!isA<mappedPatchBase>(this->patch().patch()))
    {
        FatalErrorInFunction
            << "Patch type '" << p.type()
            << "' not type '" << mappedPatchBase::typeName << "'"
            << " for patch " << p.name()
            << " of field " << internalField().name()
            << " in file " << internalField().objectPath()
            << exit(FatalError);
    }

    const mappedPatchBase& mpp = refCast<const mappedPatchBase>
    (
        this->patch().patch(),
        dict
    );
    if (mpp.mode() == mappedPolyPatch::NEARESTCELL)
    {
        FatalErrorInFunction
            << "Patch " << p.name()
            << " of type '" << p.type()
            << "' can not be used in 'nearestCell' mode"
            << " of field " << internalField().name()
            << " in file " << internalField().objectPath()
            << exit(FatalError);
    }
}


Foam::mappedVelocityFluxFixedValueFvPatchField::
mappedVelocityFluxFixedValueFvPatchField
(
    const mappedVelocityFluxFixedValueFvPatchField& ptf
)
:
    fixedValueFvPatchVectorField(ptf),
    phiName_(ptf.phiName_)
{}


Foam::mappedVelocityFluxFixedValueFvPatchField::
mappedVelocityFluxFixedValueFvPatchField
(
    const mappedVelocityFluxFixedValueFvPatchField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(ptf, iF),
    phiName_(ptf.phiName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::mappedVelocityFluxFixedValueFvPatchField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    // Since we're inside initEvaluate/evaluate there might be processor
    // comms underway. Change the tag we use.
    const int oldTag = UPstream::incrMsgType();

    // Get the mappedPatchBase
    const mappedPatchBase& mpp = refCast<const mappedPatchBase>
    (
        mappedVelocityFluxFixedValueFvPatchField::patch().patch()
    );
    const fvMesh& nbrMesh = refCast<const fvMesh>(mpp.sampleMesh());
    const word& fieldName = internalField().name();
    const volVectorField& UField =
        nbrMesh.lookupObject<volVectorField>(fieldName);

    const surfaceScalarField& phiField =
        nbrMesh.lookupObject<surfaceScalarField>(phiName_);

    vectorField newUValues;
    scalarField newPhiValues;

    switch (mpp.mode())
    {
        case mappedPolyPatch::NEARESTFACE:
        {
            vectorField allUValues(nbrMesh.nFaces(), Zero);
            scalarField allPhiValues(nbrMesh.nFaces(), Zero);

            forAll(UField.boundaryField(), patchi)
            {
                const fvPatchVectorField& Upf = UField.boundaryField()[patchi];
                const scalarField& phipf = phiField.boundaryField()[patchi];

                label faceStart = Upf.patch().start();

                forAll(Upf, facei)
                {
                    allUValues[faceStart + facei] = Upf[facei];
                    allPhiValues[faceStart + facei] = phipf[facei];
                }
            }

            mpp.distribute(allUValues);
            newUValues.transfer(allUValues);

            mpp.distribute(allPhiValues);
            newPhiValues.transfer(allPhiValues);

            break;
        }
        case mappedPolyPatch::NEARESTPATCHFACE:
        case mappedPolyPatch::NEARESTPATCHFACEAMI:
        {
            const label nbrPatchID =
                nbrMesh.boundaryMesh().findPatchID(mpp.samplePatch());

            newUValues = UField.boundaryField()[nbrPatchID];
            mpp.distribute(newUValues);

            newPhiValues = phiField.boundaryField()[nbrPatchID];
            mpp.distribute(newPhiValues);

            break;
        }
        default:
        {
            FatalErrorInFunction
                << "patch can only be used in NEARESTPATCHFACE, "
                << "NEARESTPATCHFACEAMI or NEARESTFACE mode" << nl
                << abort(FatalError);
        }
    }

    operator==(newUValues);
    phiField.constCast().boundaryFieldRef()[patch().index()] == newPhiValues;

    UPstream::msgType(oldTag);  // Restore tag

    fixedValueFvPatchVectorField::updateCoeffs();
}


void Foam::mappedVelocityFluxFixedValueFvPatchField::write
(
    Ostream& os
) const
{
    fvPatchField<vector>::write(os);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        mappedVelocityFluxFixedValueFvPatchField
    );
}


// ************************************************************************* //
