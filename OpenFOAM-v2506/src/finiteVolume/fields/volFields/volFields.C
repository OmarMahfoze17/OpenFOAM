/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018,2023 OpenCFD Ltd.
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

#include "volFields.H"
#include "coupledFvPatchField.H"
#include "registerSwitch.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{

defineTemplateTypeNameAndDebug(volScalarField::Internal, 0);
defineTemplateTypeNameAndDebug(volVectorField::Internal, 0);
defineTemplateTypeNameAndDebug(volSphericalTensorField::Internal, 0);
defineTemplateTypeNameAndDebug(volSymmTensorField::Internal, 0);
defineTemplateTypeNameAndDebug(volTensorField::Internal, 0);

defineTemplateTypeNameAndDebug(volScalarField, 0);
defineTemplateTypeNameAndDebug(volVectorField, 0);
defineTemplateTypeNameAndDebug(volSphericalTensorField, 0);
defineTemplateTypeNameAndDebug(volSymmTensorField, 0);
defineTemplateTypeNameAndDebug(volTensorField, 0);

defineTemplateDebugSwitchWithName
(
    volScalarField::Boundary,
    "volScalarField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    volVectorField::Boundary,
    "volVectorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    volSphericalTensorField::Boundary,
    "volSphericalTensorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    volSymmTensorField::Boundary,
    "volSymmTensorField::Boundary",
    0
);
defineTemplateDebugSwitchWithName
(
    volTensorField::Boundary,
    "volTensorField::Boundary",
    0
);

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Specializations

namespace Foam
{

template<>
tmp<GeometricField<scalar, fvPatchField, volMesh>>
GeometricField<scalar, fvPatchField, volMesh>::component
(
    const direction
) const
{
    return *this;
}


template<>
void GeometricField<scalar, fvPatchField, volMesh>::replace
(
    const direction,
    const GeometricField<scalar, fvPatchField, volMesh>& gsf
)
{
    *this == gsf;
}


#undef  fieldChecks
#define fieldChecks(Type)                                                     \
template<>                                                                    \
bool GeometricBoundaryField<Type, fvPatchField, volMesh>::check() const       \
{                                                                             \
    return checkConsistency<coupledFvPatchField<Type>>                        \
    (                                                                         \
        FieldBase::localBoundaryTolerance_,                                   \
       !(debug&4)  /* make into warning if debug&4 */                         \
    );                                                                        \
}

fieldChecks(scalar);
fieldChecks(vector);
fieldChecks(sphericalTensor);
fieldChecks(symmTensor);
fieldChecks(tensor);

#undef fieldChecks

} // End namespace Foam


// * * * * * * * * * * * * * * * * Global Data * * * * * * * * * * * * * * * //

// Note hard-coded values are more reliable than other alternatives

const Foam::wordList Foam::fieldTypes::internal
({
    "volScalarField::Internal",
    "volVectorField::Internal",
    "volSphericalTensorField::Internal",
    "volSymmTensorField::Internal",
    "volTensorField::Internal"
});


const Foam::wordList Foam::fieldTypes::volume
({
    "volScalarField",
    "volVectorField",
    "volSphericalTensorField",
    "volSymmTensorField",
    "volTensorField"
});


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
