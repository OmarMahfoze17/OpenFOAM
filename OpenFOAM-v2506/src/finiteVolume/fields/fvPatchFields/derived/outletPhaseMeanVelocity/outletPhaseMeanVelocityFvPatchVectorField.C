/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2017-2020 OpenCFD Ltd.
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

#include "outletPhaseMeanVelocityFvPatchVectorField.H"
#include "volFields.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::outletPhaseMeanVelocityFvPatchVectorField
::outletPhaseMeanVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    mixedFvPatchField<vector>(p, iF),
    Umean_(0),
    alphaName_("none")
{
    refValue() = Zero;
    refGrad() = Zero;
    valueFraction() = 0.0;
}


Foam::outletPhaseMeanVelocityFvPatchVectorField
::outletPhaseMeanVelocityFvPatchVectorField
(
    const outletPhaseMeanVelocityFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    mixedFvPatchField<vector>(ptf, p, iF, mapper),
    Umean_(ptf.Umean_),
    alphaName_(ptf.alphaName_)
{}


Foam::outletPhaseMeanVelocityFvPatchVectorField
::outletPhaseMeanVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    mixedFvPatchField<vector>(p, iF),
    Umean_(dict.get<scalar>("Umean")),
    alphaName_(dict.lookup("alpha"))
{
    fvPatchFieldBase::readDict(dict);

    refValue() = Zero;
    refGrad() = Zero;
    valueFraction() = 0;

    if (!this->readValueEntry(dict))
    {
        this->extrapolateInternal();
    }
}


Foam::outletPhaseMeanVelocityFvPatchVectorField
::outletPhaseMeanVelocityFvPatchVectorField
(
    const outletPhaseMeanVelocityFvPatchVectorField& ptf
)
:
    mixedFvPatchField<vector>(ptf),
    Umean_(ptf.Umean_),
    alphaName_(ptf.alphaName_)
{}


Foam::outletPhaseMeanVelocityFvPatchVectorField
::outletPhaseMeanVelocityFvPatchVectorField
(
    const outletPhaseMeanVelocityFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    mixedFvPatchField<vector>(ptf, iF),
    Umean_(ptf.Umean_),
    alphaName_(ptf.alphaName_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::outletPhaseMeanVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    scalarField alphap
    (
        patch().lookupPatchField<volScalarField>(alphaName_)
    );

    // Clamp to 0-1 range
    alphap.clamp_range(Foam::zero_one{});

    // Get the patchInternalField (zero-gradient field)
    vectorField Uzg(patchInternalField());

    // Calculate the phase mean zero-gradient velocity
    scalar Uzgmean =
        gSum(alphap*(patch().Sf() & Uzg))
       /gSum(alphap*patch().magSf());

    // Set the refValue and valueFraction to adjust the boundary field
    // such that the phase mean is Umean_
    if (Uzgmean >= Umean_)
    {
        refValue() = Zero;
        valueFraction() = 1.0 - Umean_/Uzgmean;
    }
    else
    {
        refValue() = (Umean_ + Uzgmean)*patch().nf();
        valueFraction() = 1.0 - Uzgmean/Umean_;
    }

    mixedFvPatchField<vector>::updateCoeffs();
}


void Foam::outletPhaseMeanVelocityFvPatchVectorField::write
(
    Ostream& os
) const
{
    fvPatchField<vector>::write(os);

    os.writeEntry("Umean", Umean_);
    os.writeEntry("alpha", alphaName_);
    fvPatchField<vector>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
   makePatchTypeField
   (
       fvPatchVectorField,
       outletPhaseMeanVelocityFvPatchVectorField
   );
}


// ************************************************************************* //
