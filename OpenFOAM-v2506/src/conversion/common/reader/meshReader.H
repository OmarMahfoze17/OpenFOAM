/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2016-2021 OpenCFD Ltd.
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
    Foam::meshReader

Description
    This class supports creating polyMeshes with baffles.

    The derived classes are responsible for providing the protected data.
    This implementation is somewhat messy, but could/should be restructured
    to provide a more generalized reader (at the moment it has been written
    for converting PROSTAR data).

    The meshReader supports cellTable information (see new user's guide entry).

Note
    The boundary definitions are given as cell/face.

SourceFiles
    calcPointCells.C
    createPolyBoundary.C
    createPolyCells.C
    meshReader.C
    meshReaderAux.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_meshReader_H
#define Foam_meshReader_H

#include "polyMesh.H"
#include "HashTable.H"
#include "IOstream.H"
#include "cellTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                        Class meshReader Declaration
\*---------------------------------------------------------------------------*/

class meshReader
{
protected:

    //- Identify cell faces in terms of cell Id and face Id
    class cellFaceIdentifier : public labelPair
    {
    public:

        // Constructors

            //- Construct null, as invalid pair
            cellFaceIdentifier() : labelPair(-1, -1) {}

            //- Construct from cell/face components
            cellFaceIdentifier(label c, label f) : labelPair(c, f) {}


        // Access

            //- The cell id (readonly)
            const label& cellId() const
            {
                return first();
            }

            //- The face id (readonly)
            const label& faceId() const
            {
                return second();
            }


        // Check

            //- Used if both cell or face are non-negative
            bool used() const
            {
                return (first() >= 0 && second() >= 0);
            }

            //- Unused if either cell or face are negative
            bool notUsed() const
            {
                return (first() < 0 || second() < 0);
            }
    };


private:

    // Private Data

        //- Point-cell addressing. Used for topological analysis
        // Warning. This point cell addressing list potentially contains
        // duplicate cell entries. Use additional checking
        mutable std::unique_ptr<labelListList> pointCellsPtr_;

        //- Association between two faces
        List<labelPair> interfaces_;

        //- List of cells/faces id pairs for each baffle
        List<List<cellFaceIdentifier>> baffleIds_;

        //- Cells as polyhedra for polyMesh
        cellList cellPolys_;

        //- Face sets for monitoring
        HashTable<labelList> monitoringSets_;


    // Private Member Functions

        //- No copy construct
        meshReader(const meshReader&) = delete;

        //- No copy assignment
        void operator=(const meshReader&) = delete;

        //- Calculate pointCells
        void calcPointCells() const;

        const labelListList& pointCells() const;

        //- Make polyhedral cells and global faces if the mesh is polyhedral
        void createPolyCells();

        //- Add in boundary face
        void addPolyBoundaryFace
        (
            const label cellId,
            const label cellFaceId,
            const label nCreatedFaces
        );

        //- Add in boundary face
        void addPolyBoundaryFace
        (
            const cellFaceIdentifier& identifier,
            const label nCreatedFaces
        );

        //- Add cellZones based on cellTable Id
        void addCellZones(polyMesh&) const;

        //- Add faceZones based on monitoring boundary conditions
        void addFaceZones(polyMesh&) const;

        //- Make polyhedral boundary from shape boundary
        // (adds more faces to the face list)
        void createPolyBoundary();

        //- Add polyhedral boundary
        polyPatchList polyBoundaryPatches(const polyMesh&);

        //- Clear extra storage before creation of the mesh to remove
        //  a memory peak
        void clearExtraStorage();

        void writeInterfaces(const objectRegistry&) const;

        //- Write labelList in constant/polyMesh
        void writeMeshLabelList
        (
            const objectRegistry& registry,
            const word& propertyName,
            const labelUList& list,
            IOstreamOption streamOpt
        ) const;

        //- Return list of faces for every cell
        faceListList& cellFaces() const
        {
            return const_cast<faceListList&>(cellFaces_);
        }


protected:

    // Protected Data

        //- Referenced filename
        fileName geometryFile_;

        //- Geometry scaling
        scalar scaleFactor_;

        //- Points supporting the mesh
        pointField points_;

        //- Lookup original Cell number for a given cell
        labelList origCellId_;

        //- Identify boundary faces by cells and their faces
        //  for each patch
        List<List<cellFaceIdentifier>> boundaryIds_;

        //- Boundary patch types
        wordList patchTypes_;

        //- Boundary patch names
        wordList patchNames_;

        //- Boundary patch physical types
        wordList patchPhysicalTypes_;

        //- Polyhedral mesh boundary patch start indices and dimensions
        labelList patchStarts_;
        labelList patchSizes_;

        //- Number of internal faces for polyMesh
        label nInternalFaces_;

        //- Global face list for polyMesh
        faceList meshFaces_;

        //- List of faces for every cell
        faceListList cellFaces_;

        //- List of each baffle face
        faceList baffleFaces_;

        //- Cell table id for each cell
        labelList cellTableId_;

        //- Cell table persistent data saved as a dictionary
        cellTable cellTable_;


    // Protected Member Functions

        //- Subclasses are required to supply this information
        virtual bool readGeometry(const scalar scaleFactor = 1.0) = 0;


public:

    // Static Members

        //- Warn about repeated names
        static void warnDuplicates(const word& context, const wordList&);


    // Constructors

        //- Construct from fileName
        meshReader(const fileName&, const scalar scaling = 1.0);


    //- Destructor
    virtual ~meshReader() = default;


    // Member Functions

        //- Create and return polyMesh
        virtual autoPtr<polyMesh> mesh(const objectRegistry&);

        //- Write auxiliary information
        void writeAux(const objectRegistry&) const;

        //- Write mesh
        void writeMesh
        (
            const polyMesh&,
            IOstreamOption streamOpt = IOstreamOption(IOstreamOption::BINARY)
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

#endif

// ************************************************************************* //
