/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2020,2022 OpenCFD Ltd.
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

#include "totalFlowRateAdvectiveDiffusiveFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "turbulentFluidThermoModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::
totalFlowRateAdvectiveDiffusiveFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    mixedFvPatchField<scalar>(p, iF),
    phiName_("phi"),
    rhoName_("none"),
    massFluxFraction_(1.0)
{
    refValue() = 0.0;
    refGrad() = 0.0;
    valueFraction() = 0.0;
}


Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::
totalFlowRateAdvectiveDiffusiveFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    mixedFvPatchField<scalar>(p, iF),
    phiName_(dict.getOrDefault<word>("phi", "phi")),
    rhoName_(dict.getOrDefault<word>("rho", "none")),
    massFluxFraction_(dict.getOrDefault<scalar>("massFluxFraction", 1))
{

    refValue() = 1.0;
    refGrad() = 0.0;
    valueFraction() = 0.0;

    if (!this->readValueEntry(dict))
    {
        fvPatchField<scalar>::operator=(refValue());
    }
}


Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::
totalFlowRateAdvectiveDiffusiveFvPatchScalarField
(
    const totalFlowRateAdvectiveDiffusiveFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    mixedFvPatchField<scalar>(ptf, p, iF, mapper),
    phiName_(ptf.phiName_),
    rhoName_(ptf.rhoName_),
    massFluxFraction_(ptf.massFluxFraction_)
{}


Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::
totalFlowRateAdvectiveDiffusiveFvPatchScalarField
(
    const totalFlowRateAdvectiveDiffusiveFvPatchScalarField& tppsf
)
:
    mixedFvPatchField<scalar>(tppsf),
    phiName_(tppsf.phiName_),
    rhoName_(tppsf.rhoName_),
    massFluxFraction_(tppsf.massFluxFraction_)
{}


Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::
totalFlowRateAdvectiveDiffusiveFvPatchScalarField
(
    const totalFlowRateAdvectiveDiffusiveFvPatchScalarField& tppsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    mixedFvPatchField<scalar>(tppsf, iF),
    phiName_(tppsf.phiName_),
    rhoName_(tppsf.rhoName_),
    massFluxFraction_(tppsf.massFluxFraction_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    mixedFvPatchField<scalar>::autoMap(m);
}


void Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::rmap
(
    const fvPatchScalarField& ptf,
    const labelList& addr
)
{
    mixedFvPatchField<scalar>::rmap(ptf, addr);
}


void Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    const label patchi = patch().index();

    const auto& turbModel =
        db().lookupObject<compressible::turbulenceModel>
        (
            IOobject::groupName
            (
                turbulenceModel::propertiesName,
                internalField().group()
            )
        );

    const auto& phip = patch().lookupPatchField<surfaceScalarField>(phiName_);

    const scalarField alphap(turbModel.alphaEff(patchi));

    refValue() = massFluxFraction_;
    refGrad() = 0.0;

    valueFraction() =
        1.0
       /(
            1.0
          + alphap*patch().deltaCoeffs()*patch().magSf()/max(mag(phip), SMALL)
        );

    mixedFvPatchField<scalar>::updateCoeffs();

    if (debug)
    {
        scalar phi = -gWeightedSum(phip, *this);

        Info<< patch().boundaryMesh().mesh().name() << ':'
            << patch().name() << ':'
            << this->internalField().name() << " :"
            << " mass flux[Kg/s]:" << phi
            << endl;
    }
}


void Foam::totalFlowRateAdvectiveDiffusiveFvPatchScalarField::write
(
    Ostream& os
) const
{
    fvPatchField<scalar>::write(os);
    os.writeEntry("phi", phiName_);
    os.writeEntry("rho", rhoName_);
    os.writeEntry("massFluxFraction", massFluxFraction_);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        totalFlowRateAdvectiveDiffusiveFvPatchScalarField
    );

}

// ************************************************************************* //
