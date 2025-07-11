/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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
    Foam::enrichedPatch

Description
    The enriched patch contains a double set of faces from the two
    sides of the sliding interface before the cutting.

    The patch basically consists of two over-lapping sets of faces sitting
    on a common point support, where every edge may be shared by more than
    2 faces.  The patch points are collected in a map.  Additional
    information needed for cutting is the point insertion into every edge
    of master and slave.

    Note:
    If new points are created during master-slave edge cutting, they
    should be registered with the pointMap.

SourceFiles
    enrichedPatch.C
    enrichedPatchCutFaces.C
    enrichedPatchFaces.C
    enrichedPatchPointMap.C
    enrichedPatchPointMergeMap.C
    enrichedPatchPointPoints.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_enrichedPatch_H
#define Foam_enrichedPatch_H

#include "Map.H"
#include "primitivePatch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class enrichedPatch Declaration
\*---------------------------------------------------------------------------*/

class enrichedPatch
{
    // Private Data

        //- Reference to master patch
        const primitiveFacePatch& masterPatch_;

        //- Reference to slave patch
        const primitiveFacePatch& slavePatch_;

        //- Map of points supporting patch faces
        mutable Map<point> pointMap_;

        //- Has the point map been completed?
        mutable bool pointMapComplete_;

        //- Map of point merges
        mutable Map<label> pointMergeMap_;

        //- Slave point point hits
        const labelUList& slavePointPointHits_;

        //- Slave point edge hits
        const labelUList& slavePointEdgeHits_;

        //- Slave point face hits
        const UList<objectHit>& slavePointFaceHits_;


    // Demand-driven private data

        //- Enriched patch
        mutable std::unique_ptr<faceList> enrichedFacesPtr_;

        //- Mesh points
        mutable std::unique_ptr<labelList> meshPointsPtr_;

        //- Local faces
        mutable std::unique_ptr<faceList> localFacesPtr_;

        //- Local points
        mutable std::unique_ptr<pointField> localPointsPtr_;

        //- Point-point addressing
        mutable std::unique_ptr<labelListList> pointPointsPtr_;

        // Master point addressing
        mutable std::unique_ptr<Map<labelList>> masterPointFacesPtr_;


        // Cut faces and addressing

            //- Cut faces
            mutable std::unique_ptr<faceList> cutFacesPtr_;

            //- Cut face master
            //  - the face on the master patch for internal faces
            //  - the creator face for boundary face
            mutable std::unique_ptr<labelList> cutFaceMasterPtr_;

            //- Cut face slave
            //  - the face on the slave patch for internal faces
            //  - -1 for boundary face
            mutable std::unique_ptr<labelList> cutFaceSlavePtr_;


    // Private Member Functions

        //- No copy construct
        enrichedPatch(const enrichedPatch&) = delete;

        //- No copy assignment
        void operator=(const enrichedPatch&) = delete;

        // Creation of demand-driven private data

            //- Calculate point merge map
            void calcPointMergeMap() const;

            //- Complete point map
            void completePointMap() const;

            //- Calculate mesh points
            void calcMeshPoints() const;

            //- Calculate local points
            void calcLocalPoints() const;

            //- Calculate local faces
            void calcLocalFaces() const;

            //- Calculate point-point addressing
            void calcPointPoints() const;

            //- Calculate master point addressing
            void calcMasterPointFaces() const;

            //- Calculate cut faces
            void calcCutFaces() const;

            //- Clear cut faces
            void clearCutFaces();

            //- Clear out demand-driven data
            void clearOut();


    // Static Data Members

        //- Estimated ratio of original-to-enriched face size
        static const label enrichedFaceRatio_;

        //- Size of face on which the check is forced
        static const label maxFaceSizeDebug_;


public:

    // Static data members
    ClassName("enrichedPatch");


    // Constructors

        //- Construct from components
        enrichedPatch
        (
            const primitiveFacePatch& masterPatch,
            const primitiveFacePatch& slavePatch,
            const labelUList& slavePointPointHits,
                                            // -1 or common point snapped to
            const labelUList& slavePointEdgeHits,
                                            // -1 or common edge snapped to
            const UList<objectHit>& slavePointFaceHits
                                            // master face snapped to
        );


    //- Destructor
    ~enrichedPatch() = default;


    // Member Functions

        // Access

            //- Return map of points
            inline const Map<point>& pointMap() const;

            //- Return non-const access to point map to add points
            inline Map<point>& pointMap();

            //- Return map of point merges
            inline const Map<label>& pointMergeMap() const;

            //- Return map of point merges
            inline Map<label>& pointMergeMap();


        // Topological data

            //- Calculate enriched faces
            void calcEnrichedFaces
            (
                const labelListList& pointsIntoMasterEdges,
                const labelListList& pointsIntoSlaveEdges,
                const pointField& projectedSlavePoints
            );

            //- Return enriched faces
            const faceList& enrichedFaces() const;

            //- Return mesh points
            const labelList& meshPoints() const;

            //- Return local faces
            const faceList& localFaces() const;

            //- Return local points
            const pointField& localPoints() const;

            //- Return point-point addressing
            const labelListList& pointPoints() const;

            //- Master point face addressing
            const Map<labelList>& masterPointFaces() const;


        // Cut faces

            //- Return list of cut faces
            const faceList& cutFaces() const;

            //- Return cut face master list
            const labelList& cutFaceMaster() const;

            //- Return cut face slave list
            const labelList& cutFaceSlave() const;


        //- Check if the patch is fully supported
        bool checkSupport() const;


        //- Debugging: dump graphical representation to obj format file
        void writeOBJ(const fileName& fName) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "enrichedPatchI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
