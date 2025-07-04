/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2025 OpenCFD Ltd.
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

#include "outletMappedUniformInletHeatAdditionFvPatchField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "basicThermo.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::outletMappedUniformInletHeatAdditionFvPatchField::
outletMappedUniformInletHeatAdditionFvPatchField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(p, iF),
    Qptr_(nullptr),
    outletPatchName_(),
    phiName_("phi"),
    TMin_(0),
    TMax_(5000)
{}


Foam::outletMappedUniformInletHeatAdditionFvPatchField::
outletMappedUniformInletHeatAdditionFvPatchField
(
    const outletMappedUniformInletHeatAdditionFvPatchField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchScalarField(ptf, p, iF, mapper),
    Qptr_(ptf.Qptr_.clone()),
    outletPatchName_(ptf.outletPatchName_),
    phiName_(ptf.phiName_),
    TMin_(ptf.TMin_),
    TMax_(ptf.TMax_)
{}


Foam::outletMappedUniformInletHeatAdditionFvPatchField::
outletMappedUniformInletHeatAdditionFvPatchField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchScalarField(p, iF, dict),
    Qptr_(Function1<scalar>::New("Q", dict, &db())),
    outletPatchName_(dict.get<word>("outletPatch")),
    phiName_(dict.getOrDefault<word>("phi", "phi")),
    TMin_(dict.getOrDefault<scalar>("TMin", 0)),
    TMax_(dict.getOrDefault<scalar>("TMax", 5000))
{}



Foam::outletMappedUniformInletHeatAdditionFvPatchField::
outletMappedUniformInletHeatAdditionFvPatchField
(
    const outletMappedUniformInletHeatAdditionFvPatchField& ptf
)
:
    fixedValueFvPatchScalarField(ptf),
    Qptr_(ptf.Qptr_.clone()),
    outletPatchName_(ptf.outletPatchName_),
    phiName_(ptf.phiName_),
    TMin_(ptf.TMin_),
    TMax_(ptf.TMax_)
{}



Foam::outletMappedUniformInletHeatAdditionFvPatchField::
outletMappedUniformInletHeatAdditionFvPatchField
(
    const outletMappedUniformInletHeatAdditionFvPatchField& ptf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(ptf, iF),
    Qptr_(ptf.Qptr_.clone()),
    outletPatchName_(ptf.outletPatchName_),
    phiName_(ptf.phiName_),
    TMin_(ptf.TMin_),
    TMax_(ptf.TMax_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


void Foam::outletMappedUniformInletHeatAdditionFvPatchField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    const volScalarField& vsf =
    (
        dynamic_cast<const volScalarField&>(this->internalField())
    );

    const fvPatch& fvp = this->patch();

    label outletPatchID =
        fvp.patch().boundaryMesh().findPatchID(outletPatchName_);

    if (outletPatchID < 0)
    {
        FatalErrorInFunction
            << "Unable to find outlet patch " << outletPatchName_
            << abort(FatalError);
    }

    const fvPatch& outletPatch = fvp.boundaryMesh()[outletPatchID];

    const fvPatchField<scalar>& outletPatchField =
        vsf.boundaryField()[outletPatchID];

    const surfaceScalarField& phi =
        db().lookupObject<surfaceScalarField>(phiName_);

    const scalarField& outletPatchPhi = phi.boundaryField()[outletPatchID];
    const scalar sumOutletPatchPhi = gSum(outletPatchPhi);

    if (sumOutletPatchPhi > SMALL)
    {
        const basicThermo& thermo =
            db().lookupObject<basicThermo>(basicThermo::dictName);

        const scalarField& pp = thermo.p().boundaryField()[outletPatchID];
        const scalarField& pT = thermo.T().boundaryField()[outletPatchID];

        scalar averageOutletField =
            gWeightedSum(outletPatchPhi, outletPatchField)/sumOutletPatchPhi;

        // Calculate Q as a function of average outlet temperature
        const scalar Q = Qptr_->value(averageOutletField);

        const scalarField Cpf(thermo.Cp(pp, pT, outletPatchID));

        scalar totalPhiCp = sumOutletPatchPhi*gAverage(Cpf);

        operator==(clamp(averageOutletField + Q/totalPhiCp, TMin_, TMax_));
    }
    else
    {
        scalar averageOutletField =
            gWeightedAverage(outletPatch.magSf(), outletPatchField);

        operator==(averageOutletField);
    }

    fixedValueFvPatchScalarField::updateCoeffs();
}


void Foam::outletMappedUniformInletHeatAdditionFvPatchField::write
(
    Ostream& os
) const
{
    fvPatchField<scalar>::write(os);
    os.writeEntry("outletPatch", outletPatchName_);
    os.writeEntryIfDifferent<word>("phi", "phi", phiName_);

    Qptr_->writeData(os);

    os.writeEntry("TMin", TMin_);
    os.writeEntry("TMax", TMax_);

    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        outletMappedUniformInletHeatAdditionFvPatchField
    );
}


// ************************************************************************* //
