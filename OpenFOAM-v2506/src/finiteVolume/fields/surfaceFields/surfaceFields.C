/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2022 OpenCFD Ltd.
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

#include "surfaceFields.H"
#include "registerSwitch.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{

defineTemplateTypeNameAndDebug(surfaceScalarField::Internal, 0);
defineTemplateTypeNameAndDebug(surfaceVectorField::Internal, 0);
defineTemplateTypeNameAndDebug(surfaceSphericalTensorField::Internal, 0);
defineTemplateTypeNameAndDebug(surfaceSymmTensorField::Internal, 0);
defineTemplateTypeNameAndDebug(surfaceTensorField::Internal, 0);

defineTemplateTypeNameAndDebug(surfaceScalarField, 0);
defineTemplateTypeNameAndDebug(surfaceVectorField, 0);
defineTemplateTypeNameAndDebug(surfaceSphericalTensorField, 0);
defineTemplateTypeNameAndDebug(surfaceSymmTensorField, 0);
defineTemplateTypeNameAndDebug(surfaceTensorField, 0);

defineTemplateDebugSwitchWithName
(
    surfaceScalarField::Boundary,
    "surfaceScalarField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    surfaceVectorField::Boundary,
    "surfaceVectorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    surfaceSphericalTensorField::Boundary,
    "surfaceSphericalTensorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    surfaceSymmTensorField::Boundary,
    "surfaceSymmTensorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    surfaceTensorField::Boundary,
    "surfaceTensorField::Boundary",
    0
);

} // End namespace Foam


// * * * * * * * * * * * * * * * * Global Data * * * * * * * * * * * * * * * //

// Note hard-coded values are more reliable than other alternatives

const Foam::wordList Foam::fieldTypes::surface
({
    "surfaceScalarField",
    "surfaceVectorField",
    "surfaceSphericalTensorField",
    "surfaceSymmTensorField",
    "surfaceTensorField"
});


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
