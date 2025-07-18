/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2025 OpenCFD Ltd.
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

Class
    Foam::surfaceMesh

Description
    Mesh data needed to do the Finite Volume discretisation.

\*---------------------------------------------------------------------------*/

#ifndef Foam_surfaceMesh_H
#define Foam_surfaceMesh_H

#include "GeoMesh.H"
#include "fvMesh.H"
#include "primitiveMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class surfaceMesh Declaration
\*---------------------------------------------------------------------------*/

class surfaceMesh
:
    public GeoMesh<fvMesh>
{
public:

    // Constructors

        //- Construct surface mesh from fvMesh reference
        explicit surfaceMesh(const fvMesh& mesh)
        :
            GeoMesh<fvMesh>(mesh)
        {}


    // Static Functions

        //- The geometric (internal) size - number of internal faces.
        //  Method name expected by GeoMesh interface
        static label size(const polyMesh& mesh) noexcept
        {
            return mesh.nInternalFaces();
        }

        //- The geometric boundary size - number of boundary faces.
        //  Method name expected by GeoMesh interface
        static label boundary_size(const polyMesh& mesh) noexcept
        {
            return mesh.nBoundaryFaces();
        }


    // Member Functions

        //- The geometric mesh size - number of internal faces.
        //  Method name expected by GeoMesh interface
        label size() const noexcept
        {
            return size(mesh_);
        }


        //- Field of face centres
        const surfaceVectorField& C() const
        {
            return mesh_.Cf();
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
