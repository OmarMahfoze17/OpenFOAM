/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2017 OpenFOAM Foundation
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

#include "basicSolidChemistryModel.H"
#include "fvMesh.H"
#include "Time.H"

/* * * * * * * * * * * * * * * private static data * * * * * * * * * * * * * */

namespace Foam
{
    defineTypeNameAndDebug(basicSolidChemistryModel, 0);
    defineRunTimeSelectionTable(basicSolidChemistryModel, thermo);
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::basicSolidChemistryModel::basicSolidChemistryModel
(
    solidReactionThermo& thermo
)
:
    basicChemistryModel(thermo),
    solidThermo_(thermo)
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::basicSolidChemistryModel::~basicSolidChemistryModel()
{}


const Foam::DimensionedField<Foam::scalar, Foam::volMesh>&
Foam::basicSolidChemistryModel::RR(const label i) const
{
    NotImplemented;
    return volScalarField::Internal::null();
}


Foam::DimensionedField<Foam::scalar, Foam::volMesh>&
Foam::basicSolidChemistryModel::RR(const label i)
{
    NotImplemented;
    return volScalarField::Internal::null().constCast();
}


Foam::tmp<Foam::DimensionedField<Foam::scalar, Foam::volMesh>>
Foam::basicSolidChemistryModel::calculateRR
(
    const label reactionI,
    const label speciei
) const
{
    NotImplemented;
    return nullptr;
}


// ************************************************************************* //
