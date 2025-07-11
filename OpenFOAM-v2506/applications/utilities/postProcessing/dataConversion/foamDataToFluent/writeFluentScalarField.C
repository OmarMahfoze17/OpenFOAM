/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

Description
    Given a volScalarField and Fluent field identifier, write the field in
    Fluent data format


\*---------------------------------------------------------------------------*/

#include "writeFluentFields.H"
#include "emptyFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

void Foam::writeFluentField
(
    const volScalarField& phi,
    const label fluentFieldIdentifier,
    Ostream& stream
)
{
    const scalarField& phiInternal = phi;

    // Writing cells
    stream
        << "(300 ("
        << fluentFieldIdentifier << " "  // Field identifier
        << "1 "                  // Zone ID: (cells=1, internal faces=2,
                                 // patch faces=patchi+10)
        << "1 "                  // Number of components (scalar=1, vector=3)
        << "0 0 "                // Unused
        << "1 " << phiInternal.size() // Start and end of list
        << ")(" << nl;

    for (const scalar val : phiInternal)
    {
        stream << val << nl;
    }

    stream
        << "))" << nl;

    label nWrittenFaces = phiInternal.size();

    // Writing boundary faces
    forAll(phi.boundaryField(), patchi)
    {
        if (isType<emptyFvPatchScalarField>(phi.boundaryField()[patchi]))
        {
            // Form empty patch field repeat the internal field to
            // allow for the node interpolation in Fluent
            const scalarField& phiInternal = phi;

            // Get reference to internal cells
            const auto& emptyFaceCells =
                phi.boundaryField()[patchi].patch().patch().faceCells();

            // Writing cells for empty patch
            stream
                << "(300 ("
                << fluentFieldIdentifier << " "  // Field identifier
                << patchi + 10 << " "            // Zone ID: patchi+10
                << "1 "             // Number of components (scalar=1, vector=3)
                << "0 0 "                // Unused
                << nWrittenFaces + 1 << " "
                << nWrittenFaces + emptyFaceCells.size()// Start and end of list
                << ")(" << nl;

            nWrittenFaces += emptyFaceCells.size();

            forAll(emptyFaceCells, facei)
            {
                stream << phiInternal[emptyFaceCells[facei]] << nl;
            }

            stream
                << "))" << endl;
        }
        else
        {
            // Regular patch
            label nWrittenFaces = phiInternal.size();

            const scalarField& patchPhi = phi.boundaryField()[patchi];

            // Write header
            stream
                << "(300 ("
                << fluentFieldIdentifier << " "  // Field identifier
                << patchi + 10 << " "            // Zone ID: patchi+10
                << "1 "          // Number of components (scalar=1, vector=3)
                << "0 0 "            // Unused
                << nWrittenFaces + 1 << " " << nWrittenFaces + patchPhi.size()
                                 // Start and end of list
                << ")(" << nl;

            nWrittenFaces += patchPhi.size();

            for (const scalar val : patchPhi)
            {
                stream << val << nl;
            }

            stream
                << "))" << endl;
        }
    }
}


// ************************************************************************* //
