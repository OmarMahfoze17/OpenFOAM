/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2021-2025 OpenCFD Ltd.
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
    Foam::edgeMesh

Description
    Mesh data needed to do the Finite Area discretisation.

Author
    Zeljko Tukovic, FMENA
    Hrvoje Jasak, Wikki Ltd.

\*---------------------------------------------------------------------------*/

#ifndef Foam_edgeFaMesh_H
#define Foam_edgeFaMesh_H

#include "GeoMesh.H"
#include "faMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class edgeMesh Declaration
\*---------------------------------------------------------------------------*/

class edgeMesh
:
    public GeoMesh<faMesh>
{
public:

    // Constructors

        //- Construct finite-area edge mesh faMesh reference
        explicit edgeMesh(const faMesh& mesh)
        :
            GeoMesh<faMesh>(mesh)
        {}


    // Static Functions

        //- The geometric (internal) size - number of internal edges.
        //  Method name expected by GeoMesh interface
        static label size(const faMesh& mesh) noexcept
        {
            return mesh.nInternalEdges();
        }

        //- The geometric boundary size - number of boundary edges.
        //  Method name expected by GeoMesh interface
        static label boundary_size(const faMesh& mesh) noexcept
        {
            return mesh.nBoundaryEdges();
        }


    // Member Functions

        //- The geometric (internal) size - number of internal edges.
        //  Method name expected by GeoMesh interface
        label size() const noexcept
        {
            return size(mesh_);
        }


        //- Field of edge centres
        const edgeVectorField& C() const { return mesh_.edgeCentres(); }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
