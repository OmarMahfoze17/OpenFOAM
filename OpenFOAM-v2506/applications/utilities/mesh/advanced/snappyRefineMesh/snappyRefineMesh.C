/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2024 OpenCFD Ltd.
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

Application
    snappyRefineMesh

Group
    grpMeshAdvancedUtilities

Description
    Refine cells near to a surface.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "polyMesh.H"
#include "twoDPointCorrector.H"
#include "OFstream.H"
#include "multiDirRefinement.H"

#include "triSurface.H"
#include "triSurfaceSearch.H"

#include "cellSet.H"
#include "pointSet.H"
#include "surfaceToCell.H"
#include "surfaceToPoint.H"
#include "cellToPoint.H"
#include "pointToCell.H"
#include "cellToCell.H"
#include "surfaceSets.H"
#include "polyTopoChange.H"
#include "polyTopoChanger.H"
#include "mapPolyMesh.H"
#include "labelIOList.H"
#include "emptyPolyPatch.H"
#include "removeCells.H"
#include "meshSearch.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


// Max cos angle for edges to be considered aligned with axis.
static const scalar edgeTol = 1e-3;


void writeSet(const cellSet& cells, const string& msg)
{
    Info<< "Writing " << msg << " (" << cells.size() << ") to cellSet "
        << cells.instance()/cells.local()/cells.name()
        << endl;
    cells.write();
}


direction getNormalDir(const twoDPointCorrector* correct2DPtr)
{
    if (correct2DPtr)
    {
        const vector& normal = correct2DPtr->planeNormal();

        if (mag(normal.x()) > 1-edgeTol)
        {
            return vector::X;
        }
        else if (mag(normal.y()) > 1-edgeTol)
        {
            return vector::Y;
        }
        else if (mag(normal.z()) > 1-edgeTol)
        {
            return vector::Z;
        }
    }

    return direction(3);
}



// Calculate some edge statistics on mesh. Return min. edge length over all
// directions but exclude component (0=x, 1=y, 2=z, other=none)
scalar getEdgeStats(const primitiveMesh& mesh, const direction excludeCmpt)
{
    label nAny(0);
    label nX(0);
    label nY(0);
    label nZ(0);

    scalarMinMax limitsAny(GREAT, -GREAT);
    scalarMinMax limitsX(limitsAny);
    scalarMinMax limitsY(limitsAny);
    scalarMinMax limitsZ(limitsAny);

    const edgeList& edges = mesh.edges();

    for (const edge& e : edges)
    {
        vector eVec(e.vec(mesh.points()));

        scalar eMag = mag(eVec);

        eVec /= eMag;

        if (mag(eVec.x()) > 1-edgeTol)
        {
            limitsX.add(eMag);
            nX++;
        }
        else if (mag(eVec.y()) > 1-edgeTol)
        {
            limitsY.add(eMag);
            nY++;
        }
        else if (mag(eVec.z()) > 1-edgeTol)
        {
            limitsZ.add(eMag);
            nZ++;
        }
        else
        {
            limitsAny.add(eMag);
            nAny++;
        }
    }

    Info<< "Mesh bounding box:" << boundBox(mesh.points()) << nl << nl
        << "Mesh edge statistics:" << nl
        << "    x aligned :  number:" << nX
        << "\tminLen:" << limitsX.min() << "\tmaxLen:" << limitsX.max() << nl
        << "    y aligned :  number:" << nY
        << "\tminLen:" << limitsY.min() << "\tmaxLen:" << limitsY.max() << nl
        << "    z aligned :  number:" << nZ
        << "\tminLen:" << limitsZ.min() << "\tmaxLen:" << limitsZ.max() << nl
        << "    other     :  number:" << nAny
        << "\tminLen:" << limitsAny.min()
        << "\tmaxLen:" << limitsAny.max() << nl << endl;

    if (excludeCmpt == vector::X)
    {
        return Foam::min
        (
            limitsAny.min(),
            Foam::min(limitsY.min(), limitsZ.min())
        );
    }
    else if (excludeCmpt == vector::Y)
    {
        return Foam::min
        (
            limitsAny.min(),
            Foam::min(limitsX.min(), limitsZ.min())
        );
    }
    else if (excludeCmpt == vector::Z)
    {
        return Foam::min
        (
            limitsAny.min(),
            Foam::min(limitsX.min(), limitsY.min())
        );
    }
    else
    {
        return Foam::min
        (
            limitsAny.min(),
            Foam::min
            (
                limitsX.min(),
                Foam::min(limitsY.min(), limitsZ.min())
            )
        );
    }
}


// Adds empty patch if not yet there. Returns patchID.
label addPatch(polyMesh& mesh, const word& patchName)
{
    label patchi = mesh.boundaryMesh().findPatchID(patchName);

    if (patchi == -1)
    {
        const polyBoundaryMesh& patches = mesh.boundaryMesh();

        polyPatchList newPatches(patches.size() + 1);

        label nPatches = 0;

        // Add empty patch as 0th entry (Note: only since subsetMesh wants this)
        patchi = 0;
        {
            newPatches.set
            (
                nPatches,
                new emptyPolyPatch
                (
                    patchName,
                    0,
                    mesh.nInternalFaces(),
                    nPatches,
                    patches,
                    emptyPolyPatch::typeName
                )
            );
            ++nPatches;
        }

        for (const polyPatch& pp : patches)
        {
            newPatches.set
            (
                nPatches,
                pp.clone
                (
                    patches,
                    nPatches,
                    pp.size(),
                    pp.start()
                )
            );
            ++nPatches;
        }

        mesh.removeBoundary();
        mesh.addPatches(newPatches);

        Info<< "Created patch oldInternalFaces at " << patchi << endl;
    }
    else
    {
        Info<< "Reusing patch oldInternalFaces at " << patchi << endl;
    }


    return patchi;
}


// Take surface and select cells based on surface curvature.
void selectCurvatureCells
(
    const polyMesh& mesh,
    const fileName& surfName,
    const triSurfaceSearch& querySurf,
    const scalar nearDist,
    const scalar curvature,
    cellSet& cells
)
{
    // Use surfaceToCell to do actual calculation.

    // Since we're adding make sure set is on disk.
    cells.write();

    // Take centre of cell 0 as outside point since info not used.

    surfaceToCell cutSource
    (
        mesh,
        surfName,
        querySurf.surface(),
        querySurf,
        pointField(1, mesh.cellCentres()[0]),
        false,              // includeCut
        false,              // includeInside
        false,              // includeOutside
        false,              // geometricOnly
        nearDist,
        curvature
    );

    cutSource.applyToSet(topoSetSource::ADD, cells);
}


// cutCells contains currently selected cells to be refined. Add neighbours
// on the inside or outside to them.
void addCutNeighbours
(
    const polyMesh& mesh,
    const bool selectInside,
    const bool selectOutside,
    const labelHashSet& inside,
    const labelHashSet& outside,
    labelHashSet& cutCells
)
{
    // Pick up face neighbours of cutCells

    labelHashSet addCutFaces(cutCells.size());

    for (const label celli : cutCells)
    {
        const labelList& cFaces = mesh.cells()[celli];

        forAll(cFaces, i)
        {
            const label facei = cFaces[i];

            if (mesh.isInternalFace(facei))
            {
                label nbr = mesh.faceOwner()[facei];

                if (nbr == celli)
                {
                    nbr = mesh.faceNeighbour()[facei];
                }

                if (selectInside && inside.found(nbr))
                {
                    addCutFaces.insert(nbr);
                }
                else if (selectOutside && outside.found(nbr))
                {
                    addCutFaces.insert(nbr);
                }
            }
        }
    }

    Info<< "    Selected an additional " << addCutFaces.size()
        << " neighbours of cutCells to refine" << endl;

    for (const label facei : addCutFaces)
    {
        cutCells.insert(facei);
    }
}


// Return true if any cells had to be split to keep a difference between
// neighbouring refinement levels < limitDiff.
// Gets cells which will be refined (so increase the refinement level) and
// updates it.
bool limitRefinementLevel
(
    const primitiveMesh& mesh,
    const label limitDiff,
    const labelHashSet& excludeCells,
    const labelList& refLevel,
    labelHashSet& cutCells
)
{
    // Do simple check on validity of refinement level.
    forAll(refLevel, celli)
    {
        if (!excludeCells.found(celli))
        {
            const labelList& cCells = mesh.cellCells()[celli];

            forAll(cCells, i)
            {
                label nbr = cCells[i];

                if (!excludeCells.found(nbr))
                {
                    if (refLevel[celli] - refLevel[nbr] >= limitDiff)
                    {
                        FatalErrorInFunction
                            << "Level difference between neighbouring cells "
                            << celli << " and " << nbr
                            << " greater than or equal to " << limitDiff << endl
                            << "refLevels:" << refLevel[celli] << ' '
                            <<  refLevel[nbr] << abort(FatalError);
                    }
                }
            }
        }
    }


    labelHashSet addCutCells(cutCells.size());

    for (const label celli : cutCells)
    {
        // celli will be refined.
        const labelList& cCells = mesh.cellCells()[celli];

        forAll(cCells, i)
        {
            const label nbr = cCells[i];

            if (!excludeCells.found(nbr) && !cutCells.found(nbr))
            {
                if (refLevel[celli] + 1 - refLevel[nbr] >= limitDiff)
                {
                    addCutCells.insert(nbr);
                }
            }
        }
    }

    if (addCutCells.size())
    {
        // Add cells to cutCells.

        Info<< "Added an additional " << addCutCells.size() << " cells"
            << " to satisfy 1:" << limitDiff << " refinement level"
            << endl;

        for (const label celli : addCutCells)
        {
            cutCells.insert(celli);
        }
        return true;
    }

    Info<< "Added no additional cells"
        << " to satisfy 1:" << limitDiff << " refinement level"
        << endl;

    return false;
}


// Do all refinement (i.e. refCells) according to refineDict and update
// refLevel afterwards for added cells
void doRefinement
(
    polyMesh& mesh,
    const dictionary& refineDict,
    const labelHashSet& refCells,
    labelList& refLevel
)
{
    label oldCells = mesh.nCells();

    // Multi-iteration, multi-direction topology modifier.
    multiDirRefinement multiRef
    (
        mesh,
        refCells.toc(),
        refineDict
    );

    //
    // Update refLevel for split cells
    //

    refLevel.setSize(mesh.nCells());

    for (label celli = oldCells; celli < mesh.nCells(); celli++)
    {
        refLevel[celli] = 0;
    }

    const labelListList& addedCells = multiRef.addedCells();

    forAll(addedCells, oldCelli)
    {
        const labelList& added = addedCells[oldCelli];

        if (added.size())
        {
            // Give all cells resulting from split the refinement level
            // of the master.
            label masterLevel = ++refLevel[oldCelli];

            forAll(added, i)
            {
                refLevel[added[i]] = masterLevel;
            }
        }
    }
}


// Subset mesh and update refLevel and cutCells
void subsetMesh
(
    polyMesh& mesh,
    const label writeMesh,
    const label patchi,                 // patchID for exposed faces
    const labelHashSet& cellsToRemove,
    cellSet& cutCells,
    labelIOList& refLevel
)
{
    removeCells cellRemover(mesh);

    labelList cellLabels(cellsToRemove.toc());

    Info<< "Mesh has:" << mesh.nCells() << " cells."
        << " Removing:" << cellLabels.size() << " cells" << endl;

    // exposed faces
    labelList exposedFaces(cellRemover.getExposedFaces(cellLabels));

    polyTopoChange meshMod(mesh);
    cellRemover.setRefinement
    (
        cellLabels,
        exposedFaces,
        labelList(exposedFaces.size(), patchi),
        meshMod
    );

    // Do all changes
    Info<< "Morphing ..." << endl;

    const Time& runTime = mesh.time();

    autoPtr<mapPolyMesh> morphMap = meshMod.changeMesh(mesh, false);

    if (morphMap().hasMotionPoints())
    {
        mesh.movePoints(morphMap().preMotionPoints());
    }

    // Update topology on cellRemover
    cellRemover.updateMesh(morphMap());

    // Update refLevel for removed cells.
    const labelList& cellMap = morphMap().cellMap();

    labelList newRefLevel(cellMap.size());

    forAll(cellMap, i)
    {
        newRefLevel[i] = refLevel[cellMap[i]];
    }

    // Transfer back to refLevel
    refLevel.transfer(newRefLevel);

    if (writeMesh)
    {
        Info<< "Writing refined mesh to time " << runTime.timeName() << nl
            << endl;

        // More precision (for points data)
        const auto oldPrec = IOstream::minPrecision(10);

        mesh.write();
        refLevel.write();

        IOstream::defaultPrecision(oldPrec);
    }

    // Update cutCells for removed cells.
    cutCells.updateMesh(morphMap());
}


// Divide the cells according to status compared to surface. Constructs sets:
// - cutCells : all cells cut by surface
// - inside   : all cells inside surface
// - outside  :   ,,      outside ,,
// and a combined set:
// - selected : sum of above according to selectCut/Inside/Outside flags.
void classifyCells
(
    const polyMesh& mesh,
    const fileName& surfName,
    const triSurfaceSearch& querySurf,
    const pointField& outsidePts,

    const bool selectCut,
    const bool selectInside,
    const bool selectOutside,

    const label nCutLayers,

    cellSet& inside,
    cellSet& outside,
    cellSet& cutCells,
    cellSet& selected
)
{
    // Cut faces with surface and classify cells
    surfaceSets::getSurfaceSets
    (
        mesh,
        surfName,
        querySurf.surface(),
        querySurf,
        outsidePts,

        nCutLayers,

        inside,
        outside,
        cutCells
    );

    // Combine wanted parts into selected
    if (selectCut)
    {
        selected.addSet(cutCells);
    }
    if (selectInside)
    {
        selected.addSet(inside);
    }
    if (selectOutside)
    {
        selected.addSet(outside);
    }

    Info<< "Determined cell status:" << endl
        << "    inside  :" << inside.size() << endl
        << "    outside :" << outside.size() << endl
        << "    cutCells:" << cutCells.size() << endl
        << "    selected:" << selected.size() << endl
        << endl;

    writeSet(inside, "inside cells");
    writeSet(outside, "outside cells");
    writeSet(cutCells, "cut cells");
    writeSet(selected, "selected cells");
}



int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Refine cells near to a surface"
    );
    argList::noParallel();

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createPolyMesh.H"

    // If necessary add oldInternalFaces patch
    label newPatchi = addPatch(mesh, "oldInternalFaces");


    //
    // Read motionProperties dictionary
    //

    Info<< "Checking for motionProperties\n" << endl;

    IOobject motionObj
    (
        "motionProperties",
        runTime.constant(),
        mesh,
        IOobject::MUST_READ_IF_MODIFIED,
        IOobject::NO_WRITE
    );

    // corrector for mesh motion
    twoDPointCorrector* correct2DPtr = nullptr;

    if (motionObj.typeHeaderOk<IOdictionary>(true))
    {
        Info<< "Reading " << runTime.constant() / "motionProperties"
            << endl << endl;

        IOdictionary motionProperties(motionObj);

        if (motionProperties.get<bool>("twoDMotion"))
        {
            Info<< "Correcting for 2D motion" << endl << endl;
            correct2DPtr = new twoDPointCorrector(mesh);
        }
    }

    IOdictionary refineDict
    (
        IOobject
        (
            "snappyRefineMeshDict",
            runTime.system(),
            mesh,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE
        )
    );

    fileName surfName(refineDict.get<fileName>("surface"));
    surfName.expand();
    const label nCutLayers(refineDict.get<label>("nCutLayers"));
    const label cellLimit(refineDict.get<label>("cellLimit"));
    const bool selectCut(refineDict.get<bool>("selectCut"));
    const bool selectInside(refineDict.get<bool>("selectInside"));
    const bool selectOutside(refineDict.get<bool>("selectOutside"));
    const bool selectHanging(refineDict.get<bool>("selectHanging"));

    const scalar minEdgeLen(refineDict.get<scalar>("minEdgeLen"));
    const scalar maxEdgeLen(refineDict.get<scalar>("maxEdgeLen"));
    const scalar curvature(refineDict.get<scalar>("curvature"));
    const scalar curvDist(refineDict.get<scalar>("curvatureDistance"));
    pointField outsidePts(refineDict.lookup("outsidePoints"));
    const label refinementLimit(refineDict.get<label>("splitLevel"));
    const bool writeMesh(refineDict.get<bool>("writeMesh"));

    Info<< "Cells to be used for meshing (0=false, 1=true):" << nl
        << "    cells cut by surface            : " << selectCut << nl
        << "    cells inside of surface         : " << selectInside << nl
        << "    cells outside of surface        : " << selectOutside << nl
        << "    hanging cells                   : " << selectHanging << nl
        << endl;


    if (nCutLayers > 0 && selectInside)
    {
        WarningInFunction
            << "Illogical settings : Both nCutLayers>0 and selectInside true."
            << endl
            << "This would mean that inside cells get removed but should be"
            << " included in final mesh" << endl;
    }

    // Surface.
    triSurface surf(surfName);

    // Search engine on surface
    triSurfaceSearch querySurf(surf);

    // Search engine on mesh. No face decomposition since mesh unwarped.
    meshSearch queryMesh(mesh, polyMesh::FACE_PLANES);

    // Check all 'outside' points
    forAll(outsidePts, outsidei)
    {
        const point& outsidePoint = outsidePts[outsidei];

        if (queryMesh.findCell(outsidePoint, -1, false) == -1)
        {
            FatalErrorInFunction
                << "outsidePoint " << outsidePoint
                << " is not inside any cell"
                << exit(FatalError);
        }
    }



    // Current refinement level. Read if present.
    labelIOList refLevel
    (
        IOobject
        (
            "refinementLevel",
            runTime.timeName(),
            polyMesh::defaultRegion,
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        labelList(mesh.nCells(), Zero)
    );

    label maxLevel = max(refLevel);

    if (maxLevel > 0)
    {
        Info<< "Read existing refinement level from file "
            << refLevel.objectPath() << nl
            << "   min level : " << min(refLevel) << nl
            << "   max level : " << maxLevel << nl
            << endl;
    }
    else
    {
        Info<< "Created zero refinement level in file "
            << refLevel.objectPath() << nl
            << endl;
    }




    // Print edge stats on original mesh. Leave out 2d-normal direction
    direction normalDir(getNormalDir(correct2DPtr));
    scalar meshMinEdgeLen = getEdgeStats(mesh, normalDir);

    while (meshMinEdgeLen > minEdgeLen)
    {
        // Get inside/outside/cutCells cellSets.
        cellSet inside(mesh, "inside", mesh.nCells()/10);
        cellSet outside(mesh, "outside", mesh.nCells()/10);
        cellSet cutCells(mesh, "cutCells", mesh.nCells()/10);
        cellSet selected(mesh, "selected", mesh.nCells()/10);

        classifyCells
        (
            mesh,
            surfName,
            querySurf,
            outsidePts,

            selectCut,      // for determination of selected
            selectInside,   // for determination of selected
            selectOutside,  // for determination of selected

            nCutLayers,     // How many layers of cutCells to include

            inside,
            outside,
            cutCells,
            selected        // not used but determined anyway.
        );

        Info<< "    Selected " << cutCells.name() << " with "
            << cutCells.size() << " cells" << endl;

        if ((curvDist > 0) && (meshMinEdgeLen < maxEdgeLen))
        {
            // Done refining enough close to surface. Now switch to more
            // intelligent refinement where only cutCells on surface curvature
            // are refined.
            cellSet curveCells(mesh, "curveCells", mesh.nCells()/10);

            selectCurvatureCells
            (
                mesh,
                surfName,
                querySurf,
                maxEdgeLen,
                curvature,
                curveCells
            );

            Info<< "    Selected " << curveCells.name() << " with "
                << curveCells.size() << " cells" << endl;

            // Add neighbours to cutCells. This is if selectCut is not
            // true and so outside and/or inside cells get exposed there is
            // also refinement in them.
            if (!selectCut)
            {
                addCutNeighbours
                (
                    mesh,
                    selectInside,
                    selectOutside,
                    inside,
                    outside,
                    cutCells
                );
            }

            // Subset cutCells to only curveCells
            cutCells.subset(curveCells);

            Info<< "    Removed cells not on surface curvature. Selected "
                << cutCells.size() << endl;
        }


        if (nCutLayers > 0)
        {
            // Subset mesh to remove inside cells altogether. Updates cutCells,
            // refLevel.
            subsetMesh(mesh, writeMesh, newPatchi, inside, cutCells, refLevel);
        }


        // Added cells from 2:1 refinement level restriction.
        while
        (
            limitRefinementLevel
            (
                mesh,
                refinementLimit,
                labelHashSet(),
                refLevel,
                cutCells
            )
        )
        {}


        Info<< "    Current cells           : " << mesh.nCells() << nl
            << "    Selected for refinement :" << cutCells.size() << nl
            << endl;

        if (cutCells.empty())
        {
            Info<< "Stopping refining since 0 cells selected to be refined ..."
                << nl << endl;
            break;
        }

        if ((mesh.nCells() + 8*cutCells.size()) > cellLimit)
        {
            Info<< "Stopping refining since cell limit reached ..." << nl
                << "Would refine from " << mesh.nCells()
                << " to " << mesh.nCells() + 8*cutCells.size() << " cells."
                << nl << endl;
            break;
        }

        doRefinement
        (
            mesh,
            refineDict,
            cutCells,
            refLevel
        );

        Info<< "    After refinement:" << mesh.nCells() << nl
            << endl;

        if (writeMesh)
        {
            Info<< "    Writing refined mesh to time " << runTime.timeName()
                << nl << endl;

            // More precision (for points data)
            const auto oldPrec = IOstream::minPrecision(10);

            mesh.write();
            refLevel.write();

            IOstream::defaultPrecision(oldPrec);
        }

        // Update mesh edge stats.
        meshMinEdgeLen = getEdgeStats(mesh, normalDir);
    }


    if (selectHanging)
    {
        // Get inside/outside/cutCells cellSets.
        cellSet inside(mesh, "inside", mesh.nCells()/10);
        cellSet outside(mesh, "outside", mesh.nCells()/10);
        cellSet cutCells(mesh, "cutCells", mesh.nCells()/10);
        cellSet selected(mesh, "selected", mesh.nCells()/10);

        classifyCells
        (
            mesh,
            surfName,
            querySurf,
            outsidePts,

            selectCut,
            selectInside,
            selectOutside,

            nCutLayers,

            inside,
            outside,
            cutCells,
            selected
        );


        // Find any cells which have all their points on the outside of the
        // selected set and refine them
        labelHashSet hanging = surfaceSets::getHangingCells(mesh, selected);

        Info<< "Detected " << hanging.size() << " hanging cells"
            << " (cells with all points on"
            << " outside of cellSet 'selected').\nThese would probably result"
            << " in flattened cells when snapping the mesh to the surface"
            << endl;

        Info<< "Refining " << hanging.size() << " hanging cells" << nl
            << endl;

        // Added cells from 2:1 refinement level restriction.
        while
        (
            limitRefinementLevel
            (
                mesh,
                refinementLimit,
                labelHashSet(),
                refLevel,
                hanging
            )
        )
        {}

        doRefinement(mesh, refineDict, hanging, refLevel);

        Info<< "Writing refined mesh to time " << runTime.timeName() << nl
            << endl;

        // Write final mesh

        // More precision (for points data)
        const auto oldPrec = IOstream::minPrecision(10);

        mesh.write();
        refLevel.write();

        IOstream::defaultPrecision(oldPrec);
    }
    else if (!writeMesh)
    {
        // Write final mesh. (will have been written already if writeMesh=true)

        Info<< "Writing refined mesh to time " << runTime.timeName() << nl
            << endl;

        // More precision (for points data)
        const auto oldPrec = IOstream::minPrecision(10);

        mesh.write();
        refLevel.write();

        IOstream::defaultPrecision(oldPrec);
    }


    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
