/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2016-2022 OpenCFD Ltd.
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

#include "activePressureForceBaffleVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "cyclicFvPatch.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::activePressureForceBaffleVelocityFvPatchVectorField::
activePressureForceBaffleVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(p, iF),
    pName_("p"),
    cyclicPatchName_(),
    cyclicPatchLabel_(-1),
    initWallSf_(0),
    initCyclicSf_(0),
    nbrCyclicSf_(0),
    openFraction_(0),
    openingTime_(0),
    maxOpenFractionDelta_(0),
    curTimeIndex_(-1),
    minThresholdValue_(0),
    fBased_(1),
    baffleActivated_(0),
    opening_(1)
{}


Foam::activePressureForceBaffleVelocityFvPatchVectorField::
activePressureForceBaffleVelocityFvPatchVectorField
(
    const activePressureForceBaffleVelocityFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchVectorField(ptf, p, iF, mapper),
    pName_(ptf.pName_),
    cyclicPatchName_(ptf.cyclicPatchName_),
    cyclicPatchLabel_(ptf.cyclicPatchLabel_),
    initWallSf_(ptf.initWallSf_),
    initCyclicSf_(ptf.initCyclicSf_),
    nbrCyclicSf_(ptf.nbrCyclicSf_),
    openFraction_(ptf.openFraction_),
    openingTime_(ptf.openingTime_),
    maxOpenFractionDelta_(ptf.maxOpenFractionDelta_),
    curTimeIndex_(-1),
    minThresholdValue_(ptf.minThresholdValue_),
    fBased_(ptf.fBased_),
    baffleActivated_(ptf.baffleActivated_),
    opening_(ptf.opening_)
{}


Foam::activePressureForceBaffleVelocityFvPatchVectorField::
activePressureForceBaffleVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchVectorField(p, iF, dict, IOobjectOption::NO_READ),
    pName_(dict.getOrDefault<word>("p", "p")),
    cyclicPatchName_(dict.lookup("cyclicPatch")),
    cyclicPatchLabel_(p.patch().boundaryMesh().findPatchID(cyclicPatchName_)),
    initWallSf_(0),
    initCyclicSf_(0),
    nbrCyclicSf_(0),
    openFraction_(dict.get<scalar>("openFraction")),
    openingTime_(dict.get<scalar>("openingTime")),
    maxOpenFractionDelta_(dict.get<scalar>("maxOpenFractionDelta")),
    curTimeIndex_(-1),
    minThresholdValue_(dict.get<scalar>("minThresholdValue")),
    fBased_(dict.get<bool>("forceBased")),
    baffleActivated_(0),
    opening_(dict.get<bool>("opening"))
{
    fvPatchVectorField::operator=(Zero);

    if (p.size() > 0)
    {
        initWallSf_ = p.Sf();
        initCyclicSf_ = p.boundaryMesh()[cyclicPatchLabel_].Sf();
        nbrCyclicSf_ =  refCast<const cyclicFvPatch>
        (
            p.boundaryMesh()[cyclicPatchLabel_],
            dict
        ).neighbFvPatch().Sf();
    }

    dict.readIfPresent("p", pName_);
}


Foam::activePressureForceBaffleVelocityFvPatchVectorField::
activePressureForceBaffleVelocityFvPatchVectorField
(
    const activePressureForceBaffleVelocityFvPatchVectorField& ptf
)
:
    fixedValueFvPatchVectorField(ptf),
    pName_(ptf.pName_),
    cyclicPatchName_(ptf.cyclicPatchName_),
    cyclicPatchLabel_(ptf.cyclicPatchLabel_),
    initWallSf_(ptf.initWallSf_),
    initCyclicSf_(ptf.initCyclicSf_),
    nbrCyclicSf_(ptf.nbrCyclicSf_),
    openFraction_(ptf.openFraction_),
    openingTime_(ptf.openingTime_),
    maxOpenFractionDelta_(ptf.maxOpenFractionDelta_),
    curTimeIndex_(-1),
    minThresholdValue_(ptf.minThresholdValue_),
    fBased_(ptf.fBased_),
    baffleActivated_(ptf.baffleActivated_),
    opening_(ptf.opening_)
{}


Foam::activePressureForceBaffleVelocityFvPatchVectorField::
activePressureForceBaffleVelocityFvPatchVectorField
(
    const activePressureForceBaffleVelocityFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchVectorField(ptf, iF),
    pName_(ptf.pName_),
    cyclicPatchName_(ptf.cyclicPatchName_),
    cyclicPatchLabel_(ptf.cyclicPatchLabel_),
    initWallSf_(ptf.initWallSf_),
    initCyclicSf_(ptf.initCyclicSf_),
    nbrCyclicSf_(ptf.nbrCyclicSf_),
    openFraction_(ptf.openFraction_),
    openingTime_(ptf.openingTime_),
    maxOpenFractionDelta_(ptf.maxOpenFractionDelta_),
    curTimeIndex_(-1),
    minThresholdValue_(ptf.minThresholdValue_),
    fBased_(ptf.fBased_),
    baffleActivated_(ptf.baffleActivated_),
    opening_(ptf.opening_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::activePressureForceBaffleVelocityFvPatchVectorField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchVectorField::autoMap(m);

    //- Note: cannot map field from cyclic patch anyway so just recalculate
    //  Areas should be consistent when doing autoMap except in case of
    //  topo changes.
    //- Note: we don't want to use Sf here since triggers rebuilding of
    //  fvMesh::S() which will give problems when mapped (since already
    //  on new mesh)
    forAll(patch().boundaryMesh().mesh().faceAreas(), i)
    {
        if (mag(patch().boundaryMesh().mesh().faceAreas()[i]) == 0)
        {
            Info << "faceArea[active] "<< i << endl;
        }
    }

    if (patch().size() > 0)
    {
        const vectorField& areas = patch().boundaryMesh().mesh().faceAreas();
        initWallSf_ = patch().patchSlice(areas);
        initCyclicSf_ =
            patch().boundaryMesh()[cyclicPatchLabel_].patchSlice(areas);
        nbrCyclicSf_ = refCast<const cyclicFvPatch>
        (
            patch().boundaryMesh()
            [
                cyclicPatchLabel_
            ]
        ).neighbFvPatch().patch().patchSlice(areas);
    }
}


void Foam::activePressureForceBaffleVelocityFvPatchVectorField::rmap
(
    const fvPatchVectorField& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchVectorField::rmap(ptf, addr);

    // See autoMap.
    const vectorField& areas = patch().boundaryMesh().mesh().faceAreas();
    initWallSf_ = patch().patchSlice(areas);
    initCyclicSf_ =
        patch().boundaryMesh()[cyclicPatchLabel_].patchSlice(areas);
    nbrCyclicSf_ = refCast<const cyclicFvPatch>
    (
        patch().boundaryMesh()
        [
            cyclicPatchLabel_
        ]
    ).neighbFvPatch().patch().patchSlice(areas);
}


void Foam::activePressureForceBaffleVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    // Execute the change to the openFraction only once per time-step
    if (curTimeIndex_ != this->db().time().timeIndex())
    {
        const volScalarField& p =
            db().lookupObject<volScalarField>(pName_);

        const fvPatch& cyclicPatch = patch().boundaryMesh()[cyclicPatchLabel_];
        const labelUList& cyclicFaceCells = cyclicPatch.patch().faceCells();
        const fvPatch& nbrPatch =
            refCast<const cyclicFvPatch>(cyclicPatch).neighbFvPatch();

        const labelUList& nbrFaceCells = nbrPatch.patch().faceCells();

        scalar valueDiff = 0;
        scalar area = 0;

        // Add this side (p*area)
        forAll(cyclicFaceCells, facei)
        {
            valueDiff +=p[cyclicFaceCells[facei]]*mag(initCyclicSf_[facei]);
            area += mag(initCyclicSf_[facei]);
        }

        // Remove other side
        forAll(nbrFaceCells, facei)
        {
            valueDiff -=p[nbrFaceCells[facei]]*mag(initCyclicSf_[facei]);
        }

        if (!fBased_) //pressure based then weighted by area
        {
            valueDiff = valueDiff/(area + VSMALL);
        }

        reduce(valueDiff, sumOp<scalar>());

        if (Pstream::master())
        {
            if (fBased_)
            {
                Info<< "Force difference (threshold) = " << valueDiff
                    << "(" << minThresholdValue_ << ")" << endl;
            }
            else
            {
                Info<< "Area-averaged pressure difference (threshold) = "
                    << valueDiff << "(" << minThresholdValue_ << ")" << endl;
            }
        }

        if (mag(valueDiff) > mag(minThresholdValue_) || baffleActivated_)
        {
            openFraction_ =
            (
                openFraction_
              + min
                (
                    this->db().time().deltaTValue()/openingTime_,
                    maxOpenFractionDelta_
                )
            );

            baffleActivated_ = true;
        }
        openFraction_ = clamp(openFraction_, scalar(1e-6), scalar(1 - 1e-6));

        if (Pstream::master())
        {
            Info<< "Open fraction = " << openFraction_ << endl;
        }

        const scalar areaFraction =
        (
            opening_ ? openFraction_ : (1 - openFraction_)
        );

        if (patch().boundaryMesh().mesh().moving())
        {
            // All areas have been recalculated already so are consistent
            // with the new points. Note we already only do this routine
            // once per timestep. The alternative is to use the upToDate
            // mechanism for regIOobject + polyMesh::upToDatePoints
            initWallSf_ = patch().Sf();
            initCyclicSf_ = patch().boundaryMesh()[cyclicPatchLabel_].Sf();
            nbrCyclicSf_ =  refCast<const cyclicFvPatch>
            (
                patch().boundaryMesh()[cyclicPatchLabel_]
            ).neighbFvPatch().Sf();
        }


        // Update this wall patch
        vectorField::subField Sfw = patch().patch().faceAreas();
        vectorField newSfw((1 - areaFraction)*initWallSf_);
        forAll(Sfw, facei)
        {
            Sfw[facei] = newSfw[facei];
        }
        const_cast<scalarField&>(patch().magSf()) = mag(patch().Sf());
        // Cache fraction
        const_cast<polyPatch&>(patch().patch()).areaFraction(1-areaFraction);


        // Update owner side of cyclic
        const_cast<vectorField&>(cyclicPatch.Sf()) = areaFraction*initCyclicSf_;
        const_cast<scalarField&>(cyclicPatch.magSf()) = mag(cyclicPatch.Sf());
        const_cast<polyPatch&>(cyclicPatch.patch()).areaFraction(areaFraction);


        // Update neighbour side of cyclic
        const_cast<vectorField&>(nbrPatch.Sf()) = areaFraction*nbrCyclicSf_;
        const_cast<scalarField&>(nbrPatch.magSf()) = mag(nbrPatch.Sf());
        const_cast<polyPatch&>(nbrPatch.patch()).areaFraction(areaFraction);

        curTimeIndex_ = this->db().time().timeIndex();
    }

    fixedValueFvPatchVectorField::updateCoeffs();
}


void Foam::activePressureForceBaffleVelocityFvPatchVectorField::
write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    os.writeEntryIfDifferent<word>("p", "p", pName_);
    os.writeEntry("cyclicPatch", cyclicPatchName_);
    os.writeEntry("openingTime", openingTime_);
    os.writeEntry("maxOpenFractionDelta", maxOpenFractionDelta_);
    os.writeEntry("openFraction", openFraction_);
    os.writeEntry("minThresholdValue", minThresholdValue_);
    os.writeEntry("forceBased", fBased_);
    os.writeEntry("opening", opening_);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        activePressureForceBaffleVelocityFvPatchVectorField
    );
}


// ************************************************************************* //
