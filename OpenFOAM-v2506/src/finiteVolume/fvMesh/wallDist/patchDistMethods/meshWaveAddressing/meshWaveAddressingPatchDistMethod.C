/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2023-2025 OpenCFD Ltd.
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

#include "meshWaveAddressingPatchDistMethod.H"
#include "wallDistAddressing.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace patchDistMethods
{
    defineTypeNameAndDebug(meshWaveAddressing, 0);
    addToRunTimeSelectionTable(patchDistMethod, meshWaveAddressing, dictionary);
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::patchDistMethods::meshWaveAddressing::meshWaveAddressing
(
    const dictionary& dict,
    const fvMesh& mesh,
    const labelHashSet& patchIDs
)
:
    patchDistMethod(mesh, patchIDs),
    correctWalls_(dict.getOrDefault("correctWalls", true))
{}


Foam::patchDistMethods::meshWaveAddressing::meshWaveAddressing
(
    const fvMesh& mesh,
    const labelHashSet& patchIDs,
    const bool correctWalls
)
:
    patchDistMethod(mesh, patchIDs),
    correctWalls_(correctWalls)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::patchDistMethods::meshWaveAddressing::correct(volScalarField& y)
{
    return correct(y, volVectorField::null().constCast());
}


bool Foam::patchDistMethods::meshWaveAddressing::correct
(
    volScalarField& y,
    volVectorField& n
)
{
    // If not yet constructed, construct with our options.
    // Note that registration name is still 'wallDistAddressing'
    // since the supplied options (wall patchIDs and correctWalls)
    // are not assumed to be different from the defaults of other usage.

    const labelList patchIds(patchIDs_.sortedToc());

    // Calculate distance and addressing
    const auto& wDist = wallDistAddressing::New
    (
        "wall",
        mesh_,
        patchIds,
        correctWalls_
    );

    y = wDist.y();

    // Note: copying value only so might not be consistent with supplied
    // patch types (e.g. zeroGradient when called from wallDist). Assume
    // only affected ones are the supplied patches ...

    y.boundaryFieldRef().evaluateSelected(patchIds);


    // Only calculate n if the field is defined
    if (notNull(n))
    {
        auto& bfld = n.boundaryFieldRef();

        for (const label patchi : patchIds)
        {
            bfld[patchi] == bfld[patchi].patch().nf();
        }

        // No problem with inconsistency as for y (see above) since doing
        // correctBoundaryConditions on actual n field.
        wDist.map(n, mapDistribute::transform());
    }

    return true;
}


// ************************************************************************* //
