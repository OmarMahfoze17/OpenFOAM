/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2014 OpenFOAM Foundation
    Copyright (C) 2016 OpenCFD Ltd.
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
    refineHexMesh

Group
    grpMeshAdvancedUtilities

Description
    Refine a hex mesh by 2x2x2 cell splitting for the specified cellSet.

\*---------------------------------------------------------------------------*/

#include "fvMesh.H"
#include "pointMesh.H"
#include "argList.H"
#include "Time.H"
#include "hexRef8.H"
#include "cellSet.H"
#include "Fstream.H"
#include "meshTools.H"
#include "polyTopoChange.H"
#include "mapPolyMesh.H"
#include "volMesh.H"
#include "surfaceMesh.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "pointFields.H"
#include "ReadFields.H"
#include "processorMeshes.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Refine a hex mesh by 2x2x2 cell splitting for the specified cellSet"
    );
    #include "addOverwriteOption.H"
    #include "addRegionOption.H"
    argList::addArgument("cellSet");
    argList::addBoolOption
    (
        "minSet",
        "Remove cells from input cellSet to keep to 2:1 ratio"
        " (default is to extend set)"
    );

    argList::noFunctionObjects();  // Never use function objects

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createNamedMesh.H"

    const word oldInstance = mesh.pointsInstance();

    word cellSetName(args[1]);
    const bool overwrite = args.found("overwrite");

    const bool minSet = args.found("minSet");

    Info<< "Reading cells to refine from cellSet " << cellSetName
        << nl << endl;

    cellSet cellsToRefine(mesh, cellSetName);

    Info<< "Read " << returnReduce(cellsToRefine.size(), sumOp<label>())
        << " cells to refine from cellSet " << cellSetName << nl
        << endl;


    // Read objects in time directory
    IOobjectList objects(mesh, runTime.timeName());

    // Read vol fields.

    PtrList<volScalarField> vsFlds;
    ReadFields(mesh, objects, vsFlds);

    PtrList<volVectorField> vvFlds;
    ReadFields(mesh, objects, vvFlds);

    PtrList<volSphericalTensorField> vstFlds;
    ReadFields(mesh, objects, vstFlds);

    PtrList<volSymmTensorField> vsymtFlds;
    ReadFields(mesh, objects, vsymtFlds);

    PtrList<volTensorField> vtFlds;
    ReadFields(mesh, objects, vtFlds);

    // Read surface fields.

    PtrList<surfaceScalarField> ssFlds;
    ReadFields(mesh, objects, ssFlds);

    PtrList<surfaceVectorField> svFlds;
    ReadFields(mesh, objects, svFlds);

    PtrList<surfaceSphericalTensorField> sstFlds;
    ReadFields(mesh, objects, sstFlds);

    PtrList<surfaceSymmTensorField> ssymtFlds;
    ReadFields(mesh, objects, ssymtFlds);

    PtrList<surfaceTensorField> stFlds;
    ReadFields(mesh, objects, stFlds);

    // Read point fields
    PtrList<pointScalarField> psFlds;
    ReadFields(pointMesh::New(mesh), objects, psFlds);

    PtrList<pointVectorField> pvFlds;
    ReadFields(pointMesh::New(mesh), objects, pvFlds);


    // Construct refiner without unrefinement. Read existing point/cell level.
    hexRef8 meshCutter(mesh);

    // Some stats
    {
        auto cellLimits = gMinMax(meshCutter.cellLevel());
        auto pointLimits = gMinMax(meshCutter.pointLevel());

        Info<< "Read mesh:" << nl
            << "    cells:" << mesh.globalData().nTotalCells() << nl
            << "    faces:" << mesh.globalData().nTotalFaces() << nl
            << "    points:" << mesh.globalData().nTotalPoints() << nl
            << "    cellLevel :"
            << " min:" << cellLimits.min()
            << " max:" << cellLimits.max() << nl
            << "    pointLevel :"
            << " min:" << pointLimits.min()
            << " max:" << pointLimits.max() << nl
            << endl;
    }


    // Maintain 2:1 ratio
    labelList newCellsToRefine
    (
        meshCutter.consistentRefinement
        (
            cellsToRefine.toc(),
            !minSet                 // extend set
        )
    );

    // Mesh changing engine.
    polyTopoChange meshMod(mesh);

    // Play refinement commands into mesh changer.
    meshCutter.setRefinement(newCellsToRefine, meshMod);

    if (!overwrite)
    {
        ++runTime;
    }

    // Create mesh, return map from old to new mesh.
    autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh, false);

    // Update fields
    mesh.updateMesh(map());

    // Update numbering of cells/vertices.
    meshCutter.updateMesh(map());

    // Optionally inflate mesh
    if (map().hasMotionPoints())
    {
        mesh.movePoints(map().preMotionPoints());
    }

    Info<< "Refined from " << returnReduce(map().nOldCells(), sumOp<label>())
        << " to " << mesh.globalData().nTotalCells() << " cells." << nl << endl;

    if (overwrite)
    {
        mesh.setInstance(oldInstance);
        meshCutter.setInstance(oldInstance);
    }
    Info<< "Writing mesh to " << runTime.timeName() << endl;

    mesh.write();
    meshCutter.write();
    topoSet::removeFiles(mesh);
    processorMeshes::removeFiles(mesh);

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
