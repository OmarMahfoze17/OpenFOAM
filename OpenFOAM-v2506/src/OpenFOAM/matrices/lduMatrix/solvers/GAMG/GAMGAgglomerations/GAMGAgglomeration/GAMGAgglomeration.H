/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2023 OpenCFD Ltd.
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
    Foam::GAMGAgglomeration

Description
    Geometric agglomerated algebraic multigrid agglomeration class.

    Example of the specification in fvSolution:
    \verbatim
    {
        solver              GAMG;
        smoother            symGaussSeidel;

        // Whether to cache the agglomeration
        cacheAgglomeration  yes;
        // Optionally update every updateInterval mesh motion. Default is 1.
        updateInterval      10;
        // Optional name (default is GAMGAgglomeration)
        //name            pAgglomeration;

        // Optionally agglomerate coarsest-level across processors
        processorAgglomerator   masterCoarsest;
        nMasters            2;

        tolerance       1e-2;
        relTol          0;
    }
    \endverbatim

SourceFiles
    GAMGAgglomeration.C
    GAMGAgglomerationTemplates.C
    GAMGAgglomerateLduAddressing.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_GAMGAgglomeration_H
#define Foam_GAMGAgglomeration_H

#include "MeshObject.H"
#include "lduPrimitiveMesh.H"
#include "lduInterfacePtrsList.H"
#include "primitiveFields.H"
#include "runTimeSelectionTables.H"

#include "boolList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class lduMesh;
class lduMatrix;
class mapDistribute;
class GAMGProcAgglomeration;

/*---------------------------------------------------------------------------*\
                    Class GAMGAgglomeration Declaration
\*---------------------------------------------------------------------------*/

class GAMGAgglomeration
:
    public MeshObject<lduMesh, MoveableMeshObject, GAMGAgglomeration>
{
    // Private Typedefs

        typedef MeshObject
        <
            lduMesh,
            MoveableMeshObject,
            GAMGAgglomeration
        > MeshObject_type;


protected:

    // Protected Data

        //- Max number of levels
        const label maxLevels_;

        //- Update agglomeration every updateInterval_ steps
        const label updateInterval_;

        //- Does agglomeration require update
        mutable bool requireUpdate_;

        //- Number of cells in coarsest level
        label nCellsInCoarsestLevel_;

        //- Cached mesh interfaces
        const lduInterfacePtrsList meshInterfaces_;

        autoPtr<GAMGProcAgglomeration> procAgglomeratorPtr_;

        //- The number of cells in each level
        labelList nCells_;

        //- Cell restriction addressing array.
        //  Maps from the finer to the coarser level.
        PtrList<labelField> restrictAddressing_;

        //- The number of (coarse) faces in each level.
        //  max(faceRestrictAddressing)+1.
        labelList nFaces_;

        //- Face restriction addressing array.
        //  Maps from the finer to the coarser level.
        //  Positive indices map the finer faces which form part of the boundary
        //  of the coarser cells to the corresponding coarser cell face.
        //  Negative indices map the finer faces which are internal to the
        //  coarser cells to minus the corresponding coarser cell index minus 1.
        PtrList<labelList> faceRestrictAddressing_;

        //- Face flip: for faces mapped to internal faces stores whether
        //  the face is reversed or not. This is used to avoid having
        //  to access the coarse mesh at all when mapping
        PtrList<boolList> faceFlipMap_;

        //- The number of (coarse) patch faces in each level.
        //  max(patchFaceRestrictAddressing_)+1.
        PtrList<labelList> nPatchFaces_;

        //- Patch-local face restriction addressing array.
        //  Maps from the finer to the coarser level. Always positive.
        //  Extracted from GAMGInterfaces after agglomeration.
        PtrList<labelListList> patchFaceRestrictAddressing_;

        //- Hierarchy of mesh addressing
        PtrList<lduPrimitiveMesh> meshLevels_;


        // Processor agglomeration

            //- Per level, per processor the processor it agglomerates into
            mutable PtrList<labelList> procAgglomMap_;

            //- Per level the set of processors to agglomerate. Element 0 is
            //  the 'master' of the cluster.
            mutable PtrList<labelList> agglomProcIDs_;

            //- Communicator for given level
            mutable labelList procCommunicator_;

            //- Communicator for collecting contributions. Note self-contained
            //  communicator for ease of cleanup - this one gets allocated
            //  internally, not by the GAMGProcAgglomerations
            mutable PtrList<UPstream::communicator> procAgglomCommunicator_;

            //- Mapping from processor to procMeshLevel cells
            mutable PtrList<labelList> procCellOffsets_;

            //- Mapping from processor to procMeshLevel face
            mutable PtrList<labelListList> procFaceMap_;

            //- Mapping from processor to procMeshLevel boundary
            mutable PtrList<labelListList> procBoundaryMap_;

            //- Mapping from processor to procMeshLevel boundary face
            mutable PtrList<labelListListList> procBoundaryFaceMap_;


    // Protected Member Functions

        //- Does the agglomeration need to be fully updated?
        bool requiresUpdate() const;

        //- Assemble coarse mesh addressing
        void agglomerateLduAddressing(const label fineLevelIndex);

        //- Combine a level with the previous one
        void combineLevels(const label curLevel);

        //- Shrink the number of levels to that specified. Optionally do
        //  processor agglomeration
        void compactLevels
        (
            const label nCreatedLevels,
            const bool doProcessorAgglomerate
        );

        //- Check the need for further agglomeration
        bool continueAgglomerating
        (
            const label nCellsInCoarsestLevel,
            const label nCells,
            const label nCoarseCells,
            const label comm
        ) const;

        void clearLevel(const label leveli);

        //- Print level overview
        void printLevels() const;


        // Processor agglomeration

            //- Collect and combine processor meshes into allMesh:
            //
            //    - allMeshComm   : communicator for combined mesh.
            //    - procAgglomMap : per processor the new agglomerated
            //        processor (rank in allMeshComm!). Global information.
            //    - procIDs       : local information: same for all in
            //        agglomerated processor.
            void procAgglomerateLduAddressing
            (
                const label comm,
                const labelList& procAgglomMap,
                const labelList& procIDs,
                const label allMeshComm,
                const label levelIndex
            );

            //- Collect and combine basic restriction addressing:
            //
            //    - nCells_
            //    - restrictAddressing_
            void procAgglomerateRestrictAddressing
            (
                const label comm,
                const labelList& procIDs,
                const label levelIndex
            );


        //- No copy construct
        GAMGAgglomeration(const GAMGAgglomeration&) = delete;

        //- No copy assignment
        void operator=(const GAMGAgglomeration&) = delete;


public:

    //- Declare friendship with GAMGProcAgglomeration
    friend class GAMGProcAgglomeration;

    //- Runtime type information
    TypeName("GAMGAgglomeration");


    // Declare run-time constructor selection tables

        //- Runtime selection table for pure geometric agglomerators
        declareRunTimeSelectionTable
        (
            autoPtr,
            GAMGAgglomeration,
            lduMesh,
            (
                const lduMesh& mesh,
                const dictionary& controlDict
            ),
            (
                mesh,
                controlDict
            )
        );

        //- Runtime selection table for matrix or mixed geometric/matrix
        //- agglomerators
        declareRunTimeSelectionTable
        (
            autoPtr,
            GAMGAgglomeration,
            lduMatrix,
            (
                const lduMatrix& matrix,
                const dictionary& controlDict
            ),
            (
                matrix,
                controlDict
            )
        );

        //- Runtime selection table for matrix or mixed geometric/matrix
        //- agglomerators
        declareRunTimeSelectionTable
        (
            autoPtr,
            GAMGAgglomeration,
            geometry,
            (
                const lduMesh& mesh,
                const scalarField& cellVolumes,
                const vectorField& faceAreas,
                const dictionary& controlDict
            ),
            (
                mesh,
                cellVolumes,
                faceAreas,
                controlDict
            )
        );


    // Constructors

        //- Construct given mesh and controls
        GAMGAgglomeration
        (
            const lduMesh& mesh,
            const dictionary& controlDict
        );


    // Selectors

        //- Return the selected geometric agglomerator
        static const GAMGAgglomeration& New
        (
            const lduMesh& mesh,
            const dictionary& controlDict
        );

        //- Return the selected matrix agglomerator
        static const GAMGAgglomeration& New
        (
            const lduMatrix& matrix,
            const dictionary& controlDict
        );

        //- Return the selected geometric agglomerator
        static const GAMGAgglomeration& New
        (
            const lduMesh& mesh,
            const scalarField& cellVolumes,
            const vectorField& faceAreas,
            const dictionary& controlDict
        );


    //- Destructor
    ~GAMGAgglomeration();


    // Member Functions

            //- Update when the mesh moves
            virtual bool movePoints();


        // Access

            label size() const
            {
                return meshLevels_.size();
            }

            //- Return LDU mesh of given level
            const lduMesh& meshLevel(const label leveli) const;

            //- Do we have mesh for given level?
            bool hasMeshLevel(const label leveli) const;

            //- Return LDU interface addressing of given level
            const lduInterfacePtrsList& interfaceLevel
            (
                const label leveli
            ) const;

            //- Return cell restrict addressing of given level
            const labelField& restrictAddressing(const label leveli) const
            {
                return restrictAddressing_[leveli];
            }

            //- Return face restrict addressing of given level
            const labelList& faceRestrictAddressing(const label leveli) const
            {
                return faceRestrictAddressing_[leveli];
            }

            const labelListList& patchFaceRestrictAddressing(const label leveli)
            const
            {
                return patchFaceRestrictAddressing_[leveli];
            }

            //- Return face flip map of given level
            const boolList& faceFlipMap(const label leveli) const
            {
                return faceFlipMap_[leveli];
            }

            //- Return number of coarse cells (before processor agglomeration)
            label nCells(const label leveli) const
            {
                return nCells_[leveli];
            }

            //- Return number of coarse faces (before processor agglomeration)
            label nFaces(const label leveli) const
            {
                return nFaces_[leveli];
            }

            //- Return number of coarse patch faces (before processor
            //- agglomeration)
            const labelList& nPatchFaces(const label leveli) const
            {
                return nPatchFaces_[leveli];
            }


        // Restriction and prolongation

            //- Restrict (integrate by summation) cell field
            template<class Type>
            void restrictField
            (
                Field<Type>& cf,
                const Field<Type>& ff,
                const label fineLevelIndex,
                const bool procAgglom
            ) const;

            //- Restrict (integrate by summation) face field
            template<class Type>
            void restrictFaceField
            (
                Field<Type>& cf,
                const Field<Type>& ff,
                const label fineLevelIndex
            ) const;

            //- Restrict (integrate by summation) cell field
            template<class Type>
            void restrictField
            (
                Field<Type>& cf,
                const Field<Type>& ff,
                const labelList& fineToCoarse
            ) const;

            //- Prolong (interpolate by injection) cell field
            template<class Type>
            void prolongField
            (
                Field<Type>& ff,
                const Field<Type>& cf,
                const label coarseLevelIndex,
                const bool procAgglom
            ) const;

            //- Prolong (interpolate by injection) cell field. Return reference
            //  to (potentially scattered) coarse field
            template<class Type>
            const Field<Type>& prolongField
            (
                Field<Type>& ff,        // fine-level field
                Field<Type>& work,      // work storage
                const Field<Type>& cf,  // coarse-level field
                const label coarseLevelIndex
            ) const;


        // Processor agglomeration. Note that the mesh and agglomeration is
        // stored per fineLevel (even though it is the coarse level mesh that
        // has been agglomerated). This is just for convenience and consistency
        // with GAMGSolver notation.

            //- Given fine to coarse processor map determine:
            //
            //    - for each coarse processor a master (minimum of the fine
            //        processors)
            //    - for each coarse processor the set of fine processors
            //        (element 0 is the master processor)
            static void calculateRegionMaster
            (
                const label comm,
                const labelList& procAgglomMap,
                labelList& masterProcs,
                List<label>& agglomProcIDs
            );

            //- Whether to agglomerate across processors
            bool processorAgglomerate() const
            {
                return bool(procAgglomeratorPtr_);
            }

            //- Mapping from processor to agglomerated processor (global, all
            //- processors have the same information). Note that level is
            //- the fine, not the coarse, level index. This is to be
            //- consistent with the way the restriction is stored
            const labelList& procAgglomMap(const label fineLeveli) const;

            //- Set of processors to agglomerate. Element 0 is the
            //- master processor. (local, same only on those processors that
            //- agglomerate)
            const labelList& agglomProcIDs(const label fineLeveli) const;

            //- Check that level has combined mesh
            bool hasProcMesh(const label fineLeveli) const;

            //- Communicator for current level or -1
            label procCommunicator(const label fineLeveli) const;

            //- Communicator for collecting contributions
            label agglomCommunicator(const label fineLeveli) const;

            //- Mapping from processor to procMesh cells
            const labelList& cellOffsets(const label fineLeveli) const;

            //- Mapping from processor to procMesh face
            const labelListList& faceMap(const label fineLeveli) const;

            //- Mapping from processor to procMesh boundary
            const labelListList& boundaryMap(const label fineLeveli) const;

            //- Mapping from processor to procMesh boundary face
            const labelListListList& boundaryFaceMap(const label fineLeveli)
            const;


        // Helpers

            //- Agglomerate from a starting level. Starting level is usually 0
            //- (initial mesh) but sometimes >0 (restarting after processor
            //- agglomeration)
            virtual void agglomerate
            (
                const label nCellsInCoarsestLevel,
                const label startLevel,
                const scalarField& startFaceWeights,
                const bool doProcessorAgglomerate = true
            ) = 0;

            //- Given restriction determines if coarse cells are connected.
            //  Return ok is so, otherwise creates new restriction that is
            static bool checkRestriction
            (
                labelList& newRestrict,
                label& nNewCoarse,
                const lduAddressing& fineAddressing,
                const labelUList& restriction,
                const label nCoarse
            );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "GAMGAgglomerationTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
