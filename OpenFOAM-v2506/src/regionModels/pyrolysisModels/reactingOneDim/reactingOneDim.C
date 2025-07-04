/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2016-2021 OpenCFD Ltd.
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

#include "reactingOneDim.H"
#include "addToRunTimeSelectionTable.H"
#include "fvm.H"
#include "fvcDiv.H"
#include "fvcVolumeIntegrate.H"
#include "fvcLaplacian.H"
#include "absorptionEmissionModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace regionModels
{
namespace pyrolysisModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(reactingOneDim, 0);

addToRunTimeSelectionTable(pyrolysisModel, reactingOneDim, mesh);
addToRunTimeSelectionTable(pyrolysisModel, reactingOneDim, dictionary);

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void reactingOneDim::readReactingOneDimControls()
{
    const dictionary& solution = this->solution().subDict("SIMPLE");
    solution.readEntry("nNonOrthCorr", nNonOrthCorr_);
    time().controlDict().readEntry("maxDi", maxDiff_);
    coeffs().readEntry("minimumDelta", minimumDelta_);
    gasHSource_ = coeffs().getOrDefault("gasHSource", false);
    coeffs().readEntry("qrHSource", qrHSource_);
    useChemistrySolvers_ =
        coeffs().getOrDefault("useChemistrySolvers", true);
}


bool reactingOneDim::read()
{
    if (pyrolysisModel::read())
    {
        readReactingOneDimControls();
        return true;
    }

    return false;
}


bool reactingOneDim::read(const dictionary& dict)
{
    if (pyrolysisModel::read(dict))
    {
        readReactingOneDimControls();
        return true;
    }

    return false;
}


void reactingOneDim::updateqr()
{
    // Update local qr from coupled qr field
    qr_ == dimensionedScalar(qr_.dimensions(), Zero);

    // Retrieve field from coupled region using mapped boundary conditions
    qr_.correctBoundaryConditions();

    volScalarField::Boundary& qrBf = qr_.boundaryFieldRef();

    forAll(intCoupledPatchIDs_, i)
    {
        const label patchi = intCoupledPatchIDs_[i];

        // qr is positive going in the solid
        // If the surface is emitting the radiative flux is set to zero
        qrBf[patchi] = max(qrBf[patchi], scalar(0));
    }

    const vectorField& cellC = regionMesh().cellCentres();

    tmp<volScalarField> kappa = kappaRad();

    // Propagate qr through 1-D regions
    label localPyrolysisFacei = 0;
    forAll(intCoupledPatchIDs_, i)
    {
        const label patchi = intCoupledPatchIDs_[i];

        const scalarField& qrp = qr_.boundaryField()[patchi];
        const vectorField& Cf = regionMesh().Cf().boundaryField()[patchi];

        forAll(qrp, facei)
        {
            const scalar qr0 = qrp[facei];
            point Cf0 = Cf[facei];
            const labelList& cells = boundaryFaceCells_[localPyrolysisFacei++];
            scalar kappaInt = 0.0;
            forAll(cells, k)
            {
                const label celli = cells[k];
                const point& Cf1 = cellC[celli];
                const scalar delta = mag(Cf1 - Cf0);
                kappaInt += kappa()[celli]*delta;
                qr_[celli] = qr0*exp(-kappaInt);
                Cf0 = Cf1;
            }
        }
    }
}


void reactingOneDim::updatePhiGas()
{
    phiHsGas_ == dimensionedScalar(phiHsGas_.dimensions(), Zero);
    phiGas_ == dimensionedScalar(phiGas_.dimensions(), Zero);

    const speciesTable& gasTable = solidChemistry_->gasTable();

    forAll(gasTable, gasI)
    {
        tmp<volScalarField> tHsiGas =
            solidChemistry_->gasHs(solidThermo_->p(), solidThermo_->T(), gasI);

        const volScalarField& HsiGas = tHsiGas();

        const volScalarField::Internal& RRiGas = solidChemistry_->RRg(gasI);

        surfaceScalarField::Boundary& phiGasBf = phiGas_.boundaryFieldRef();

        label totalFaceId = 0;
        forAll(intCoupledPatchIDs_, i)
        {
            const label patchi = intCoupledPatchIDs_[i];

            scalarField& phiGasp = phiGasBf[patchi];
            const scalarField& cellVol = regionMesh().V();

            forAll(phiGasp, facei)
            {
                const labelList& cells = boundaryFaceCells_[totalFaceId++];
                scalar massInt = 0.0;
                forAllReverse(cells, k)
                {
                    const label celli = cells[k];
                    massInt += RRiGas[celli]*cellVol[celli];
                    phiHsGas_[celli] += massInt*HsiGas[celli];
                }

                phiGasp[facei] += massInt;

                if (debug)
                {
                    Info<< " Gas : " << gasTable[gasI]
                        << " on patch : " << patchi
                        << " mass produced at face(local) : "
                        <<  facei
                        << " is : " << massInt
                        << " [kg/s] " << endl;
                }
            }
        }
    }
}


void reactingOneDim::updateFields()
{
    if (qrHSource_)
    {
        updateqr();
    }

    updatePhiGas();
}


void reactingOneDim::updateMesh(const scalarField& deltaV)
{
    Info<< "Initial/final volumes = " << gSum(deltaV) << endl;

    // Move the mesh
    const labelList moveMap = moveMesh(deltaV, minimumDelta_);

    // Flag any cells that have not moved as non-reacting
    forAll(moveMap, i)
    {
        if (moveMap[i] == 1)
        {
            solidChemistry_->setCellReacting(i, false);
        }
    }
}


void reactingOneDim::solveContinuity()
{
    DebugInFunction << endl;

    if (!moveMesh_)
    {
        fvScalarMatrix rhoEqn
        (
            fvm::ddt(rho_) == -solidChemistry_->RRg()
        );

        rhoEqn.solve();
    }
    else
    {
        const scalarField deltaV
        (
            -solidChemistry_->RRg()*regionMesh().V()*time_.deltaT()/rho_
        );

        updateMesh(deltaV);
    }
}


void reactingOneDim::solveSpeciesMass()
{
    DebugInFunction << endl;

    volScalarField Yt(0.0*Ys_[0]);

    for (label i=0; i<Ys_.size()-1; i++)
    {
        volScalarField& Yi = Ys_[i];

        fvScalarMatrix YiEqn
        (
            fvm::ddt(rho_, Yi) == solidChemistry_->RRs(i)
        );

        if (regionMesh().moving())
        {
            surfaceScalarField phiYiRhoMesh
            (
                fvc::interpolate(Yi*rho_)*regionMesh().phi()
            );

            YiEqn -= fvc::div(phiYiRhoMesh);

        }

        YiEqn.solve(regionMesh().solver("Yi"));
        Yi.clamp_min(0);
        Yt += Yi;
    }

    Ys_[Ys_.size() - 1] = 1.0 - Yt;

}


void reactingOneDim::solveEnergy()
{
    DebugInFunction << endl;

    tmp<volScalarField> alpha(solidThermo_->alpha());

    fvScalarMatrix hEqn
    (
        fvm::ddt(rho_, h_)
      - fvm::laplacian(alpha, h_)
      + fvc::laplacian(alpha, h_)
      - fvc::laplacian(kappa(), T())
     ==
        chemistryQdot_
      + solidChemistry_->RRsHs()
    );

/*
    NOTE: gas Hs is included in hEqn

    if (gasHSource_)
    {
        const surfaceScalarField phiGas(fvc::interpolate(phiHsGas_));
        hEqn += fvc::div(phiGas);
    }
*/

    if (qrHSource_)
    {
        const surfaceScalarField phiqr(fvc::interpolate(qr_)*nMagSf());
        hEqn += fvc::div(phiqr);
    }

/*
    NOTE: The moving mesh option is only correct for reaction such as
    Solid -> Gas, thus the ddt term is compensated exactly by chemistrySh and
    the mesh flux is not necessary.

    if (regionMesh().moving())
    {
        surfaceScalarField phihMesh
        (
            fvc::interpolate(rho_*h_)*regionMesh().phi()
        );

        hEqn -= fvc::div(phihMesh);
    }
*/
    hEqn.relax();
    hEqn.solve();
}


void reactingOneDim::calculateMassTransfer()
{
    /*
    totalGasMassFlux_ = 0;
    forAll(intCoupledPatchIDs_, i)
    {
        const label patchi = intCoupledPatchIDs_[i];
        totalGasMassFlux_ += gSum(phiGas_.boundaryField()[patchi]);
    }
    */

    if (infoOutput_)
    {
        totalHeatRR_ = fvc::domainIntegrate(chemistryQdot_);

        addedGasMass_ +=
            fvc::domainIntegrate(solidChemistry_->RRg())*time_.deltaT();
        lostSolidMass_ +=
            fvc::domainIntegrate(solidChemistry_->RRs())*time_.deltaT();
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

reactingOneDim::reactingOneDim
(
    const word& modelType,
    const fvMesh& mesh,
    const word& regionType
)
:
    pyrolysisModel(modelType, mesh, regionType),
    solidThermo_(solidReactionThermo::New(regionMesh())),
    solidChemistry_(basicSolidChemistryModel::New(solidThermo_())),
    radiation_(radiation::radiationModel::New(solidThermo_->T())),
    rho_
    (
        IOobject
        (
            "rho",
            regionMesh().time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        solidThermo_->rho()
    ),
    Ys_(solidThermo_->composition().Y()),
    h_(solidThermo_->he()),
    nNonOrthCorr_(-1),
    maxDiff_(10),
    minimumDelta_(1e-4),

    phiGas_
    (
        IOobject
        (
            "phiGas",
            time().timeName(),
            regionMesh(),
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimMass/dimTime, Zero)
    ),

    phiHsGas_
    (
        IOobject
        (
            "phiHsGas",
            time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimEnergy/dimTime, Zero)
    ),

    chemistryQdot_
    (
        IOobject
        (
            "chemistryQdot",
            time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimEnergy/dimTime/dimVolume, Zero)
    ),

    qr_
    (
        IOobject
        (
            "qr",
            time().timeName(),
            regionMesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh()
    ),

    lostSolidMass_(dimensionedScalar(dimMass, Zero)),
    addedGasMass_(dimensionedScalar(dimMass, Zero)),
    totalGasMassFlux_(0.0),
    totalHeatRR_(dimensionedScalar(dimEnergy/dimTime, Zero)),
    gasHSource_(false),
    qrHSource_(false),
    useChemistrySolvers_(true)
{
    if (active_)
    {
        read();
    }
}


reactingOneDim::reactingOneDim
(
    const word& modelType,
    const fvMesh& mesh,
    const dictionary& dict,
    const word& regionType
)
:
    pyrolysisModel(modelType, mesh, dict, regionType),
    solidThermo_(solidReactionThermo::New(regionMesh())),
    solidChemistry_(basicSolidChemistryModel::New(solidThermo_())),
    radiation_(radiation::radiationModel::New(solidThermo_->T())),
    rho_
    (
        IOobject
        (
            "rho",
            regionMesh().time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        solidThermo_->rho()
    ),
    Ys_(solidThermo_->composition().Y()),
    h_(solidThermo_->he()),
    nNonOrthCorr_(-1),
    maxDiff_(10),
    minimumDelta_(1e-4),

    phiGas_
    (
        IOobject
        (
            "phiGas",
            time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimMass/dimTime, Zero)
    ),

    phiHsGas_
    (
        IOobject
        (
            "phiHsGas",
            time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimEnergy/dimTime, Zero)
    ),

    chemistryQdot_
    (
        IOobject
        (
            "chemistryQdot",
            time().timeName(),
            regionMesh(),
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh(),
        dimensionedScalar(dimEnergy/dimTime/dimVolume, Zero)
    ),

    qr_
    (
        IOobject
        (
            "qr",
            time().timeName(),
            regionMesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh()
    ),

    lostSolidMass_(dimensionedScalar(dimMass, Zero)),
    addedGasMass_(dimensionedScalar(dimMass, Zero)),
    totalGasMassFlux_(0.0),
    totalHeatRR_(dimensionedScalar(dimEnergy/dimTime, Zero)),
    gasHSource_(false),
    qrHSource_(false),
    useChemistrySolvers_(true)
{
    if (active_)
    {
        read(dict);
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

reactingOneDim::~reactingOneDim()
{}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

scalar reactingOneDim::addMassSources(const label patchi, const label facei)
{
    label index = 0;
    forAll(primaryPatchIDs_, i)
    {
        if (primaryPatchIDs_[i] == patchi)
        {
            index = i;
            break;
        }
    }

    const label localPatchId =  intCoupledPatchIDs_[index];

    const scalar massAdded = phiGas_.boundaryField()[localPatchId][facei];

    if (debug)
    {
        Info<< "\nPyrolysis region: " << type() << "added mass : "
            << massAdded << endl;
    }

    return massAdded;
}


scalar reactingOneDim::solidRegionDiffNo() const
{
    scalar DiNum = -GREAT;

    if (regionMesh().nInternalFaces() > 0)
    {
        surfaceScalarField KrhoCpbyDelta
        (
            sqr(regionMesh().surfaceInterpolation::deltaCoeffs())
           *fvc::interpolate(kappa())
           /fvc::interpolate(Cp()*rho_)
        );

        DiNum = max(KrhoCpbyDelta.primitiveField())*time().deltaTValue();
    }

    return returnReduce(DiNum, maxOp<scalar>());
}


scalar reactingOneDim::maxDiff() const
{
    return maxDiff_;
}


const volScalarField& reactingOneDim::rho() const
{
    return rho_;
}


const volScalarField& reactingOneDim::T() const
{
    return solidThermo_->T();
}


const tmp<volScalarField> reactingOneDim::Cp() const
{
    return solidThermo_->Cp();
}


tmp<volScalarField> reactingOneDim::kappaRad() const
{
    return radiation_->absorptionEmission().a();
}


tmp<volScalarField> reactingOneDim::kappa() const
{
    return solidThermo_->kappa();
}


const surfaceScalarField& reactingOneDim::phiGas() const
{
    return phiGas_;
}


void reactingOneDim::preEvolveRegion()
{
    pyrolysisModel::preEvolveRegion();
}


void reactingOneDim::evolveRegion()
{
    Info<< "\nEvolving pyrolysis in region: " << regionMesh().name() << endl;

    if (useChemistrySolvers_)
    {
        solidChemistry_->solve(time().deltaTValue());
    }
    else
    {
        solidChemistry_->calculate();
    }

    solveContinuity();

    chemistryQdot_ = solidChemistry_->Qdot()();

    updateFields();

    solveSpeciesMass();

    for (int nonOrth=0; nonOrth<=nNonOrthCorr_; nonOrth++)
    {
        solveEnergy();
    }

    calculateMassTransfer();

    solidThermo_->correct();

    auto limits = gMinMax(solidThermo_->T().primitiveField());

    Info<< "pyrolysis min/max(T) = "
        << limits.min() << ", " << limits.max() << endl;
}


void reactingOneDim::info()
{
    Info<< "\nPyrolysis in region: " << regionMesh().name() << endl;

    Info<< indent << "Total gas mass produced  [kg] = "
        << addedGasMass_.value() << nl
        << indent << "Total solid mass lost    [kg] = "
        << lostSolidMass_.value() << nl
        //<< indent << "Total pyrolysis gases  [kg/s] = "
        //<< totalGasMassFlux_ << nl
        << indent << "Total heat release rate [J/s] = "
        << totalHeatRR_.value() << nl;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam
} // End namespace regionModels
} // End namespace pyrolysisModels

// ************************************************************************* //
