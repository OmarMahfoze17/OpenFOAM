/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2016,2025 OpenCFD Ltd.
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
    polyDualMesh

Group
    grpMeshManipulationUtilities

Description
    Creates the dual of a polyMesh, adhering to all the feature and patch edges.

Usage
    \b polyDualMesh featureAngle

    Detects any boundary edge > angle and creates multiple boundary faces
    for it. Normal behaviour is to have each point become a cell
    (1.5 behaviour)

    Options:
      - \par -concaveMultiCells
        Creates multiple cells for each point on a concave edge. Might limit
        the amount of distortion on some meshes.

      - \par -splitAllFaces
        Normally only constructs a single face between two cells. This single
        face might be too distorted. splitAllFaces will create a single face for
        every original cell the face passes through. The mesh will thus have
        multiple faces in between two cells! (so is not strictly
        upper-triangular anymore - checkMesh will complain)

      - \par -doNotPreserveFaceZones:
        By default all faceZones are preserved by marking all faces, edges and
        points on them as features. The -doNotPreserveFaceZones disables this
        behaviour.

Note
    It is just a driver for meshDualiser. Substitute your own simpleMarkFeatures
    to have different behaviour.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "fvMesh.H"
#include "unitConversion.H"
#include "polyTopoChange.H"
#include "mapPolyMesh.H"
#include "bitSet.H"
#include "meshTools.H"
#include "OFstream.H"
#include "meshDualiser.H"
#include "ReadFields.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "topoSet.H"
#include "processorMeshes.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Naive feature detection. All boundary edges with angle > featureAngle become
// feature edges. All points on feature edges become feature points. All
// boundary faces become feature faces.
void simpleMarkFeatures
(
    const polyMesh& mesh,
    const bitSet& isBoundaryEdge,
    const scalar featureAngle,
    const bool concaveMultiCells,
    const bool doNotPreserveFaceZones,

    labelList& featureFaces,
    labelList& featureEdges,
    labelList& singleCellFeaturePoints,
    labelList& multiCellFeaturePoints
)
{
    scalar minCos = Foam::cos(degToRad(featureAngle));

    const polyBoundaryMesh& patches = mesh.boundaryMesh();

    // Working sets
    labelHashSet featureEdgeSet;
    labelHashSet singleCellFeaturePointSet;
    labelHashSet multiCellFeaturePointSet;


    // 1. Mark all edges between patches
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    forAll(patches, patchi)
    {
        const polyPatch& pp = patches[patchi];
        const labelList& meshEdges = pp.meshEdges();

        // All patch corner edges. These need to be feature points & edges!
        for (label edgeI = pp.nInternalEdges(); edgeI < pp.nEdges(); edgeI++)
        {
            label meshEdgeI = meshEdges[edgeI];
            featureEdgeSet.insert(meshEdgeI);
            singleCellFeaturePointSet.insert(mesh.edges()[meshEdgeI][0]);
            singleCellFeaturePointSet.insert(mesh.edges()[meshEdgeI][1]);
        }
    }



    // 2. Mark all geometric feature edges
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Make distinction between convex features where the boundary point becomes
    // a single cell and concave features where the boundary point becomes
    // multiple 'half' cells.

    // Addressing for all outside faces
    primitivePatch allBoundary(patches.faces(), mesh.points());

    // Check for non-manifold points (surface pinched at point)
    allBoundary.checkPointManifold(false, &singleCellFeaturePointSet);

    // Check for non-manifold edges (surface pinched at edge)
    const labelListList& edgeFaces = allBoundary.edgeFaces();
    const labelList& meshPoints = allBoundary.meshPoints();

    forAll(edgeFaces, edgeI)
    {
        const labelList& eFaces = edgeFaces[edgeI];

        if (eFaces.size() > 2)
        {
            const edge& e = allBoundary.edges()[edgeI];

            //Info<< "Detected non-manifold boundary edge:" << edgeI
            //    << " coords:"
            //    << allBoundary.points()[meshPoints[e[0]]]
            //    << allBoundary.points()[meshPoints[e[1]]] << endl;

            singleCellFeaturePointSet.insert(meshPoints[e[0]]);
            singleCellFeaturePointSet.insert(meshPoints[e[1]]);
        }
    }

    // Check for features.
    forAll(edgeFaces, edgeI)
    {
        const labelList& eFaces = edgeFaces[edgeI];

        if (eFaces.size() == 2)
        {
            label f0 = eFaces[0];
            label f1 = eFaces[1];

            // check angle
            const vector& n0 = allBoundary.faceNormals()[f0];
            const vector& n1 = allBoundary.faceNormals()[f1];

            if ((n0 & n1) < minCos)
            {
                const edge& e = allBoundary.edges()[edgeI];
                label v0 = meshPoints[e[0]];
                label v1 = meshPoints[e[1]];

                label meshEdgeI = meshTools::findEdge(mesh, v0, v1);
                featureEdgeSet.insert(meshEdgeI);

                // Check if convex or concave by looking at angle
                // between face centres and normal
                vector c1c0
                (
                    allBoundary[f1].centre(allBoundary.points())
                  - allBoundary[f0].centre(allBoundary.points())
                );

                if (concaveMultiCells && (c1c0 & n0) > SMALL)
                {
                    // Found concave edge. Make into multiCell features
                    Info<< "Detected concave feature edge:" << edgeI
                        << " cos:" << (c1c0 & n0)
                        << " coords:"
                        << allBoundary.points()[v0]
                        << allBoundary.points()[v1]
                        << endl;

                    singleCellFeaturePointSet.erase(v0);
                    multiCellFeaturePointSet.insert(v0);
                    singleCellFeaturePointSet.erase(v1);
                    multiCellFeaturePointSet.insert(v1);
                }
                else
                {
                    // Convex. singleCell feature.
                    if (!multiCellFeaturePointSet.found(v0))
                    {
                        singleCellFeaturePointSet.insert(v0);
                    }
                    if (!multiCellFeaturePointSet.found(v1))
                    {
                        singleCellFeaturePointSet.insert(v1);
                    }
                }
            }
        }
    }


    // 3. Mark all feature faces
    // ~~~~~~~~~~~~~~~~~~~~~~~~~

    // Face centres that need inclusion in the dual mesh
    labelHashSet featureFaceSet;
    featureFaceSet.reserve(mesh.nBoundaryFaces());

    // A. boundary faces.
    for (label facei = mesh.nInternalFaces(); facei < mesh.nFaces(); facei++)
    {
        featureFaceSet.insert(facei);
    }

    // B. face zones.
    const faceZoneMesh& faceZones = mesh.faceZones();

    if (doNotPreserveFaceZones)
    {
        if (faceZones.size() > 0)
        {
            WarningInFunction
                << "Detected " << faceZones.size()
                << " faceZones. These will not be preserved."
                << endl;
        }
    }
    else
    {
        if (faceZones.size() > 0)
        {
            Info<< "Detected " << faceZones.size()
                << " faceZones. Preserving these by marking their"
                << " points, edges and faces as features." << endl;
        }

        forAll(faceZones, zoneI)
        {
            const faceZone& fz = faceZones[zoneI];

            Info<< "Inserting all faces in faceZone " << fz.name()
                << " as features." << endl;

            forAll(fz, i)
            {
                label facei = fz[i];
                const face& f = mesh.faces()[facei];
                const labelList& fEdges = mesh.faceEdges()[facei];

                featureFaceSet.insert(facei);
                forAll(f, fp)
                {
                    // Mark point as multi cell point (since both sides of
                    // face should have different cells)
                    singleCellFeaturePointSet.erase(f[fp]);
                    multiCellFeaturePointSet.insert(f[fp]);

                    // Make sure there are points on the edges.
                    featureEdgeSet.insert(fEdges[fp]);
                }
            }
        }
    }

    // Transfer to arguments
    featureFaces = featureFaceSet.toc();
    featureEdges = featureEdgeSet.toc();
    singleCellFeaturePoints = singleCellFeaturePointSet.toc();
    multiCellFeaturePoints = multiCellFeaturePointSet.toc();
}


// Dump features to .obj files
void dumpFeatures
(
    const polyMesh& mesh,
    const labelList& featureFaces,
    const labelList& featureEdges,
    const labelList& singleCellFeaturePoints,
    const labelList& multiCellFeaturePoints
)
{
    {
        OFstream str("featureFaces.obj");
        Info<< "Dumping centres of featureFaces to obj file " << str.name()
            << endl;
        forAll(featureFaces, i)
        {
            meshTools::writeOBJ(str, mesh.faceCentres()[featureFaces[i]]);
        }
    }
    {
        OFstream str("featureEdges.obj");
        Info<< "Dumping featureEdges to obj file " << str.name() << endl;
        label vertI = 0;

        forAll(featureEdges, i)
        {
            const edge& e = mesh.edges()[featureEdges[i]];
            meshTools::writeOBJ(str, mesh.points()[e[0]]);
            vertI++;
            meshTools::writeOBJ(str, mesh.points()[e[1]]);
            vertI++;
            str<< "l " << vertI-1 << ' ' << vertI << nl;
        }
    }
    {
        OFstream str("singleCellFeaturePoints.obj");
        Info<< "Dumping featurePoints that become a single cell to obj file "
            << str.name() << endl;
        forAll(singleCellFeaturePoints, i)
        {
            meshTools::writeOBJ(str, mesh.points()[singleCellFeaturePoints[i]]);
        }
    }
    {
        OFstream str("multiCellFeaturePoints.obj");
        Info<< "Dumping featurePoints that become multiple cells to obj file "
            << str.name() << endl;
        forAll(multiCellFeaturePoints, i)
        {
            meshTools::writeOBJ(str, mesh.points()[multiCellFeaturePoints[i]]);
        }
    }
}


int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Creates the dual of a polyMesh,"
        " adhering to all the feature and patch edges."
    );

    #include "addOverwriteOption.H"
    argList::noParallel();

    argList::addArgument
    (
        "featureAngle",
        "in degrees [0-180]"
    );

    argList::addBoolOption
    (
        "splitAllFaces",
        "Have multiple faces in between cells"
    );
    argList::addBoolOption
    (
        "concaveMultiCells",
        "Split cells on concave boundary edges into multiple cells"
    );
    argList::addBoolOption
    (
        "doNotPreserveFaceZones",
        "Disable the default behaviour of preserving faceZones by having"
        " multiple faces in between cells"
    );

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createNamedMesh.H"

    const word oldInstance = mesh.pointsInstance();

    // Mark boundary edges and points.
    // (Note: in 1.4.2 we can use the built-in mesh point ordering
    //  facility instead)
    bitSet isBoundaryEdge(mesh.nEdges());
    for (label facei = mesh.nInternalFaces(); facei < mesh.nFaces(); facei++)
    {
        const labelList& fEdges = mesh.faceEdges()[facei];

        forAll(fEdges, i)
        {
            isBoundaryEdge.set(fEdges[i]);
        }
    }

    const scalar featureAngle = args.get<scalar>(1);
    const scalar minCos = Foam::cos(degToRad(featureAngle));

    Info<< "Feature:" << featureAngle << endl
        << "minCos :" << minCos << endl
        << endl;


    const bool splitAllFaces = args.found("splitAllFaces");
    if (splitAllFaces)
    {
        Info<< "Splitting all internal faces to create multiple faces"
            << " between two cells." << nl
            << endl;
    }

    const bool overwrite = args.found("overwrite");
    const bool doNotPreserveFaceZones = args.found("doNotPreserveFaceZones");
    const bool concaveMultiCells = args.found("concaveMultiCells");
    if (concaveMultiCells)
    {
        Info<< "Generating multiple cells for points on concave feature edges."
            << nl << endl;
    }


    // Face(centre)s that need inclusion in the dual mesh
    labelList featureFaces;
    // Edge(centre)s  ,,
    labelList featureEdges;
    // Points (that become a single cell) that need inclusion in the dual mesh
    labelList singleCellFeaturePoints;
    // Points (that become a multiple cells)        ,,
    labelList multiCellFeaturePoints;

    // Sample implementation of feature detection.
    simpleMarkFeatures
    (
        mesh,
        isBoundaryEdge,
        featureAngle,
        concaveMultiCells,
        doNotPreserveFaceZones,

        featureFaces,
        featureEdges,
        singleCellFeaturePoints,
        multiCellFeaturePoints
    );

    // If we want to split all polyMesh faces into one dualface per cell
    // we are passing through we also need a point
    // at the polyMesh facecentre and edgemid of the faces we want to
    // split.
    if (splitAllFaces)
    {
        featureEdges = identity(mesh.nEdges());
        featureFaces = identity(mesh.nFaces());
    }

    // Write obj files for debugging
    dumpFeatures
    (
        mesh,
        featureFaces,
        featureEdges,
        singleCellFeaturePoints,
        multiCellFeaturePoints
    );



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


    // Topo change container
    polyTopoChange meshMod(mesh.boundaryMesh().size());

    // Mesh dualiser engine
    meshDualiser dualMaker(mesh);

    // Insert all commands into polyTopoChange to create dual of mesh. This does
    // all the hard work.
    dualMaker.setRefinement
    (
        splitAllFaces,
        featureFaces,
        featureEdges,
        singleCellFeaturePoints,
        multiCellFeaturePoints,
        meshMod
    );

    // Create mesh, return map from old to new mesh.
    autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh, false);

    // Update fields
    mesh.updateMesh(map());

    // Optionally inflate mesh
    if (map().hasMotionPoints())
    {
        mesh.movePoints(map().preMotionPoints());
    }

    if (!overwrite)
    {
        ++runTime;
    }
    else
    {
        mesh.setInstance(oldInstance);
    }

    Info<< "Writing dual mesh to " << runTime.timeName() << endl;

    mesh.write();
    topoSet::removeFiles(mesh);
    processorMeshes::removeFiles(mesh);

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
