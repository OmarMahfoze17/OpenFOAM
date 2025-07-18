/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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
    Foam::pointPairs

Description
    HashSet of unique edges. The edges are stored as a pair of pairs:

        ( (local index, processor index) (local index, processor index) )

    e.g.,

        ( (0 1) (3 1) )
        ( (0 2) (5 1) )

\*---------------------------------------------------------------------------*/

#ifndef pointPairs_H
#define pointPairs_H

#include "labelPairHashes.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class pointPairs Declaration
\*---------------------------------------------------------------------------*/

template<class Triangulation>
class pointPairs
:
    public HashSet<labelPairPair>
{
    // Private Typedefs

    typedef HashSet<labelPairPair> StorageContainer;
    typedef typename Triangulation::Vertex_handle Vertex_handle;


    // Private Data

        const Triangulation& triangulation_;


    // Private Member Functions

        inline labelPairPair orderPointPair
        (
            const labelPair& vA,
            const labelPair& vB
        ) const;

        inline bool insertPointPair
        (
            const labelPair& vA,
            const labelPair& vB
        );

        inline bool findPointPair
        (
            const labelPair& vA,
            const labelPair& vB
        ) const;


public:

    // Constructors

        //- Construct from triangulation
        pointPairs(const Triangulation& triangulation);


    //- Destructor
    ~pointPairs();


    // Member Functions

        // Access

            inline bool isPointPair
            (
                const Vertex_handle& vA,
                const Vertex_handle& vB
            ) const;

            inline bool isPointPair
            (
                const labelPair& vA,
                const labelPair& vB
            ) const;


        // Edit

            inline bool addPointPair
            (
                const labelPair& vA,
                const labelPair& vB
            );

            inline bool addPointPair
            (
                const labelPair& master,
                const UList<labelPair>& slaves
            );

            inline bool addPointPair
            (
                const label vA,
                const label vB
            );

            void reIndex(const Map<label>& oldToNewIndices);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "pointPairs.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
