/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2021-2024 OpenCFD Ltd.
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
    ideasUnvToFoam

Group
    grpMeshConversionUtilities

Description
    I-Deas unv format mesh conversion.

    Uses either
    - DOF sets (757) or
    - face groups (2452(Cubit), 2467)
    to do patching.
    Works without but then puts all faces in defaultFaces patch.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "polyMesh.H"
#include "Time.H"
#include "IFstream.H"
#include "cellModel.H"
#include "cellSet.H"
#include "faceSet.H"
#include "DynamicList.H"

#include <cassert>
#include "MeshedSurfaces.H"

using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

const string SEPARATOR("    -1");

bool isSeparator(const std::string& line)
{
    return line.substr(0, 6) == SEPARATOR;
}


// Reads past -1 and reads element type
label readTag(IFstream& is)
{
    string tag;
    do
    {
        if (!is.good())
        {
            return -1;
        }

        string line;

        is.getLine(line);

        if (line.size() < 6)
        {
            return -1;
        }

        tag = line.substr(0, 6);

    } while (tag == SEPARATOR);

    return readLabel(tag);
}


// Reads and prints header
void readHeader(IFstream& is)
{
    string line;

    while (is.good())
    {
        is.getLine(line);

        if (isSeparator(line))
        {
            break;
        }
        else
        {
            Info<< line << endl;
        }
    }
}


// Skip
void skipSection(IFstream& is)
{
    Info<< "Skipping section at line " << is.lineNumber() << '.' << endl;

    string line;

    while (is.good())
    {
        is.getLine(line);

        if (isSeparator(line))
        {
            break;
        }
    }
}


scalar readUnvScalar(const std::string& unvString)
{
    string s(unvString);

    s.replaceAll("d", "E");
    s.replaceAll("D", "E");

    return readScalar(s);
}


// Reads unit section
void readUnits
(
    IFstream& is,
    scalar& lengthScale,
    scalar& forceScale,
    scalar& tempScale,
    scalar& tempOffset
)
{
    Info<< "Starting reading units at line " << is.lineNumber() << '.' << endl;

    string line;
    is.getLine(line);

    label l = readLabel(line.substr(0, 10));
    Info<< "l:" << l << endl;

    string units(line.substr(10, 20));
    Info<< "units:" << units << endl;

    label unitType = readLabel(line.substr(30, 10));
    Info<< "unitType:" << unitType << endl;

    // Read lengthscales
    is.getLine(line);

    lengthScale = readUnvScalar(line.substr(0, 25));
    forceScale = readUnvScalar(line.substr(25, 25));
    tempScale = readUnvScalar(line.substr(50, 25));

    is.getLine(line);
    tempOffset = readUnvScalar(line.substr(0, 25));

    Info<< "Unit factors:" << nl
        << "    Length scale       : " << lengthScale << nl
        << "    Force scale        : " << forceScale << nl
        << "    Temperature scale  : " << tempScale << nl
        << "    Temperature offset : " << tempOffset << nl
        << endl;
}


// Reads points section. Read region as well?
void readPoints
(
    IFstream& is,
    DynamicList<point>& points,     // coordinates
    DynamicList<label>& unvPointID  // unv index
)
{
    Info<< "Starting reading points at line " << is.lineNumber() << '.' << endl;

    static bool hasWarned = false;

    while (true)
    {
        string line;
        is.getLine(line);

        label pointi = readLabel(line.substr(0, 10));

        if (pointi == -1)
        {
            break;
        }
        else if (pointi != points.size()+1 && !hasWarned)
        {
            hasWarned = true;

            IOWarningInFunction(is)
                << "Points not in order starting at point " << pointi
                << endl;
        }

        is.getLine(line);

        unvPointID.push_back(pointi);
        point& p = points.emplace_back();

        p.x() = readUnvScalar(line.substr(0, 25));
        p.y() = readUnvScalar(line.substr(25, 25));
        p.z() = readUnvScalar(line.substr(50, 25));
    }

    points.shrink();
    unvPointID.shrink();

    Info<< "Read " << points.size() << " points." << endl;
}

void addAndExtend
(
    DynamicList<label>& indizes,
    label celli,
    label val
)
{
    if (indizes.size() < (celli+1))
    {
        indizes.setSize(celli+1,-1);
    }
    indizes[celli] = val;
}

// Reads cells section. Read region as well? Not handled yet but should just
// be a matter of reading corresponding to boundaryFaces the correct property
// and sorting it later on.
void readCells
(
    IFstream& is,
    DynamicList<cellShape>& cellVerts,
    DynamicList<label>& cellMaterial,
    DynamicList<label>& boundaryFaceIndices,
    DynamicList<face>& boundaryFaces,
    DynamicList<label>& cellCorrespondence,
    DynamicList<label>& unvPointID  // unv index
)
{
    Info<< "Starting reading cells at line " << is.lineNumber() << '.' << endl;

    // Invert point numbering.
    label maxUnvPoint = 0;
    forAll(unvPointID, pointi)
    {
        maxUnvPoint = Foam::max(maxUnvPoint, unvPointID[pointi]);
    }
    labelList unvToFoam(invert(maxUnvPoint+1, unvPointID));


    const cellModel& hex = cellModel::ref(cellModel::HEX);
    const cellModel& prism = cellModel::ref(cellModel::PRISM);
    const cellModel& tet = cellModel::ref(cellModel::TET);

    labelHashSet skippedElements;

    labelHashSet foundFeType;

    while (true)
    {
        string line;
        is.getLine(line);

        if (isSeparator(line))
        {
            break;
        }

        label celli, feID, physProp, matProp, colour, nNodes;

        IStringStream lineStr(line);
        lineStr
            >> celli >> feID >> physProp >> matProp >> colour >> nNodes;

        if (foundFeType.insert(feID))
        {
            Info<< "First occurrence of element type " << feID
                << " for cell " << celli << " at line "
                << is.lineNumber() << endl;
        }

        if (feID == 11)
        {
            // Rod. Skip.
            is.getLine(line);
            is.getLine(line);
        }
        else if (feID == 171)
        {
            // Rod. Skip.
            is.getLine(line);
        }
        else if (feID == 41 || feID == 91)
        {
            // Triangle. Save - used for patching later on.
            is.getLine(line);

            face cVerts(3);
            IStringStream lineStr(line);
            lineStr
                >> cVerts[0] >> cVerts[1] >> cVerts[2];
            boundaryFaces.append(cVerts);
            boundaryFaceIndices.append(celli);
        }
        else if (feID == 44 || feID == 94)
        {
            // Quad. Save - used for patching later on.
            is.getLine(line);

            face cVerts(4);
            IStringStream lineStr(line);
            lineStr
                >> cVerts[0] >> cVerts[1] >> cVerts[2] >> cVerts[3];
            boundaryFaces.append(cVerts);
            boundaryFaceIndices.append(celli);
        }
        else if (feID == 111)
        {
            // Tet.
            is.getLine(line);

            labelList cVerts(4);
            IStringStream lineStr(line);
            lineStr
                >> cVerts[0] >> cVerts[1] >> cVerts[2] >> cVerts[3];

            cellVerts.append(cellShape(tet, cVerts, true));
            cellMaterial.append(physProp);
            addAndExtend(cellCorrespondence,celli,cellMaterial.size()-1);

            if (cellVerts.last().size() != cVerts.size())
            {
                Info<< "Line:" << is.lineNumber()
                    << " element:" << celli
                    << " type:" << feID
                    << " collapsed from " << cVerts << nl
                    << " to:" << cellVerts.last()
                    << endl;
            }
        }
        else if (feID == 112)
        {
            // Wedge.
            is.getLine(line);

            labelList cVerts(6);
            IStringStream lineStr(line);
            lineStr
                >> cVerts[0] >> cVerts[1] >> cVerts[2]
                >> cVerts[3] >> cVerts[4] >> cVerts[5];

            cellVerts.append(cellShape(prism, cVerts, true));
            cellMaterial.append(physProp);
            addAndExtend(cellCorrespondence,celli,cellMaterial.size()-1);

            if (cellVerts.last().size() != cVerts.size())
            {
                Info<< "Line:" << is.lineNumber()
                    << " element:" << celli
                    << " type:" << feID
                    << " collapsed from " << cVerts << nl
                    << " to:" << cellVerts.last()
                    << endl;
            }
        }
        else if (feID == 115)
        {
            // Hex.
            is.getLine(line);

            labelList cVerts(8);
            IStringStream lineStr(line);
            lineStr
                >> cVerts[0] >> cVerts[1] >> cVerts[2] >> cVerts[3]
                >> cVerts[4] >> cVerts[5] >> cVerts[6] >> cVerts[7];

            cellVerts.append(cellShape(hex, cVerts, true));
            cellMaterial.append(physProp);
            addAndExtend(cellCorrespondence,celli,cellMaterial.size()-1);

            if (cellVerts.last().size() != cVerts.size())
            {
                Info<< "Line:" << is.lineNumber()
                    << " element:" << celli
                    << " type:" << feID
                    << " collapsed from " << cVerts << nl
                    << " to:" << cellVerts.last()
                    << endl;
            }
        }
        else if (feID == 118)
        {
            // Parabolic Tet
            is.getLine(line);

            labelList cVerts(4);
            label dummy;
            {
                IStringStream lineStr(line);
                lineStr
                    >> cVerts[0] >> dummy >> cVerts[1] >> dummy >> cVerts[2];
            }
            is.getLine(line);
            {
                IStringStream lineStr(line);
                lineStr >> dummy>> cVerts[3];
            }

            cellVerts.append(cellShape(tet, cVerts, true));
            cellMaterial.append(physProp);
            addAndExtend(cellCorrespondence,celli,cellMaterial.size()-1);

            if (cellVerts.last().size() != cVerts.size())
            {
                Info<< "Line:" << is.lineNumber()
                    << " element:" << celli
                    << " type:" << feID
                    << " collapsed from " << cVerts << nl
                    << " to:" << cellVerts.last()
                    << endl;
            }
        }
        else
        {
            if (skippedElements.insert(feID))
            {
                IOWarningInFunction(is)
                    << "Cell type " << feID << " not supported" << endl;
            }

            is.getLine(line);
        }
    }

    cellVerts.shrink();
    cellMaterial.shrink();
    boundaryFaces.shrink();
    boundaryFaceIndices.shrink();
    cellCorrespondence.shrink();

    Info<< "Read " << cellVerts.size() << " cells"
        << " and " << boundaryFaces.size() << " boundary faces." << endl;

    if (!cellVerts.size())
    {
        WarningInFunction
            << "There are no cells in the mesh." << endl
            << "    Note: 2D meshes are not supported."<< endl;
    }
}


void readSets
(
    IFstream& is,
    DynamicList<word>& patchNames,
    DynamicList<labelList>& patchFaceIndices
)
{
    Info<< "Starting reading patches at line " << is.lineNumber() << '.'
        << endl;

    while (true)
    {
        string line;
        is.getLine(line);

        if (isSeparator(line))
        {
            break;
        }

        IStringStream lineStr(line);
        label group, constraintSet, restraintSet, loadSet, dofSet,
            tempSet, contactSet, nFaces;
        lineStr
            >> group >> constraintSet >> restraintSet >> loadSet
            >> dofSet >> tempSet >> contactSet >> nFaces;

        is.getLine(line);
        const word groupName = word::validate(line);

        Info<< "For group " << group
            << " named " << groupName
            << " trying to read " << nFaces << " patch face indices."
            << endl;

        labelList groupIndices(nFaces);
        label groupType = -1;
        nFaces = 0;

        while (nFaces < groupIndices.size())
        {
            is.getLine(line);
            IStringStream lineStr(line);

            // Read one (for last face) or two entries from line.
            label nRead = 2;
            if (nFaces == groupIndices.size()-1)
            {
                nRead = 1;
            }

            for (label i = 0; i < nRead; i++)
            {
                label tag, nodeLeaf, component;

                lineStr >> groupType >> tag >> nodeLeaf >> component;

                groupIndices[nFaces++] = tag;
            }
        }


        // Store
        if (groupType == 8)
        {
            patchNames.append(groupName);
            patchFaceIndices.append(groupIndices);
        }
        else
        {
            IOWarningInFunction(is)
                << "When reading patches expect entity type code 8"
                << nl << "    Skipping group code " << groupType
                << endl;
        }
    }

    patchNames.shrink();
    patchFaceIndices.shrink();
}



// Read dof set (757)
void readDOFS
(
    IFstream& is,
    DynamicList<word>& patchNames,
    DynamicList<labelList>& dofVertices
)
{
    Info<< "Starting reading constraints at line " << is.lineNumber() << '.'
        << endl;

    string line;
    is.getLine(line);
    label group;
    {
        IStringStream lineStr(line);
        lineStr >> group;
    }

    is.getLine(line);
    {
        IStringStream lineStr(line);
        word pName(lineStr);
        patchNames.append(pName);
    }

    Info<< "For DOF set " << group
        << " named " << patchNames.last()
        << " trying to read vertex indices."
        << endl;

    DynamicList<label> vertices;
    while (true)
    {
        string line;
        is.getLine(line);

        if (isSeparator(line))
        {
            break;
        }

        IStringStream lineStr(line);
        label pointi;
        lineStr >> pointi;

        vertices.append(pointi);
    }

    Info<< "For DOF set " << group
        << " named " << patchNames.last()
        << " read " << vertices.size() << " vertex indices." << endl;

    dofVertices.append(vertices.shrink());

    patchNames.shrink();
    dofVertices.shrink();
}


// Returns -1 or group that all of the vertices of f are in,
label findPatch(const List<labelHashSet>& dofGroups, const face& f)
{
    forAll(dofGroups, patchi)
    {
        if (dofGroups[patchi].found(f[0]))
        {
            bool allInGroup = true;

            // Check rest of face
            for (label fp = 1; fp < f.size(); fp++)
            {
                if (!dofGroups[patchi].found(f[fp]))
                {
                    allInGroup = false;
                    break;
                }
            }

            if (allInGroup)
            {
                return patchi;
            }
        }
    }
    return -1;
}



int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Convert I-Deas unv format to OpenFOAM"
    );
    argList::noParallel();
    argList::addArgument(".unv file");
    argList::addBoolOption
    (
        "dump",
        "Dump boundary faces as boundaryFaces.obj (for debugging)"
    );

    #include "setRootCase.H"
    #include "createTime.H"

    const auto ideasName = args.get<fileName>(1);
    IFstream inFile(ideasName);

    if (!inFile.good())
    {
        FatalErrorInFunction
            << "Cannot open file " << ideasName
            << exit(FatalError);
    }


    // Switch on additional debug info
    const bool verbose = false; //true;

    // Unit scale factors
    scalar lengthScale = 1;
    scalar forceScale = 1;
    scalar tempScale = 1;
    scalar tempOffset = 0;

    // Points
    DynamicList<point> points;
    // Original unv point label
    DynamicList<label> unvPointID;

    // Cells
    DynamicList<cellShape> cellVerts;
    DynamicList<label> cellMat;
    DynamicList<label> cellCorrespondence;

    // Boundary faces
    DynamicList<label> boundaryFaceIndices;
    DynamicList<face> boundaryFaces;

    // Patch names and patchFace indices.
    DynamicList<word> patchNames;
    DynamicList<labelList> patchFaceIndices;
    DynamicList<labelList> dofVertIndices;


    while (inFile.good())
    {
        label tag = readTag(inFile);

        if (tag == -1)
        {
            break;
        }

        Info<< "Processing tag:" << tag << endl;

        switch (tag)
        {
            case 151:
                readHeader(inFile);
            break;

            case 164:
                readUnits
                (
                    inFile,
                    lengthScale,
                    forceScale,
                    tempScale,
                    tempOffset
                );
            break;

            case 2411:
                readPoints(inFile, points, unvPointID);
            break;

            case 2412:
                readCells
                (
                    inFile,
                    cellVerts,
                    cellMat,
                    boundaryFaceIndices,
                    boundaryFaces,
                    cellCorrespondence,
                    unvPointID
                );
            break;

            case 2452:
            case 2467:
                readSets
                (
                    inFile,
                    patchNames,
                    patchFaceIndices
                );
            break;

            case 757:
                readDOFS
                (
                    inFile,
                    patchNames,
                    dofVertIndices
                );
            break;

            default:
                Info<< "Skipping tag " << tag << " on line "
                    << inFile.lineNumber() << endl;
                skipSection(inFile);
            break;
        }
        Info<< endl;
    }


    // Invert point numbering.
    label maxUnvPoint = 0;
    forAll(unvPointID, pointi)
    {
        maxUnvPoint = Foam::max(maxUnvPoint, unvPointID[pointi]);
    }
    labelList unvToFoam(invert(maxUnvPoint+1, unvPointID));


    // Renumber vertex numbers on cells

    forAll(cellVerts, celli)
    {
        labelList foamVerts
        (
            renumber
            (
                unvToFoam,
                static_cast<labelList&>(cellVerts[celli])
            )
        );

        if (foamVerts.found(-1))
        {
            FatalErrorInFunction
                << "Cell " << celli
                << " unv vertices " << cellVerts[celli]
                << " has some undefined vertices " << foamVerts
                << abort(FatalError);
        }

        // Bit nasty: replace vertex list.
        cellVerts[celli].transfer(foamVerts);
    }

    // Renumber vertex numbers on boundaryFaces

    forAll(boundaryFaces, bFacei)
    {
        labelList foamVerts(renumber(unvToFoam, boundaryFaces[bFacei]));

        if (foamVerts.found(-1))
        {
            FatalErrorInFunction
                << "Boundary face " << bFacei
                << " unv vertices " << boundaryFaces[bFacei]
                << " has some undefined vertices " << foamVerts
                << abort(FatalError);
        }

        // Bit nasty: replace vertex list.
        boundaryFaces[bFacei].transfer(foamVerts);
    }



    // Patchify = create patchFaceVerts for use in cellShape construction
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Works in one of two modes. Either has read boundaryFaces which
    // just need to be sorted according to patch. Or has read point constraint
    // sets (dofVertIndices).

    List<faceList> patchFaceVerts;

    labelList own(boundaryFaces.size(), -1);
    labelList nei(boundaryFaces.size(), -1);
    Pair<Map<label>> faceToCell;

    {
        // Can use face::symmHasher or use sorted indices instead
        // - choose the latter in case UNV has anything odd
        HashTable<label, face> faceToFaceID(2*boundaryFaces.size());

        forAll(boundaryFaces, bfacei)
        {
            face sortedVerts(boundaryFaces[bfacei]);
            Foam::sort(sortedVerts);
            faceToFaceID.insert(sortedVerts, bfacei);
        }

        forAll(cellVerts, celli)
        {
            const cellShape& shape = cellVerts[celli];

            for (const face& f : shape.faces())
            {
                face sortedVerts(f);
                Foam::sort(sortedVerts);
                const label bfacei = faceToFaceID.lookup(sortedVerts, -1);
                if (bfacei != -1)
                {
                    const int cmp = face::compare(f, boundaryFaces[bfacei]);

                    if (cmp == 1)
                    {
                        // Same orientation. Cell is owner.
                        own[bfacei] = celli;
                    }
                    else if (cmp == -1)
                    {
                        // Opposite orientation. Cell is neighbour.
                        nei[bfacei] = celli;
                    }
                }
            }
        }

        label nReverse = 0;
        forAll(own, facei)
        {
            if (own[facei] == -1 && nei[facei] != -1)
            {
                // Boundary face with incorrect orientation
                boundaryFaces[facei].flip();
                std::swap(own[facei], nei[facei]);
                nReverse++;
            }
        }
        if (nReverse > 0)
        {
            Info << "Found " << nReverse << " reversed boundary faces out of "
                << boundaryFaces.size() << endl;
        }


        label cnt = 0;
        forAll(own, facei)
        {
            if (own[facei] != -1 && nei[facei] != -1)
            {
                faceToCell[1].insert(facei, own[facei]);
                faceToCell[0].insert(facei, nei[facei]);
                cnt++;
            }
        }

        if (cnt > 0)
        {
            Info << "Of " << boundaryFaces.size() << " so-called"
                << " boundary faces " << cnt << " belong to two cells "
                << "and are therefore internal" << endl;
        }
    }

    HashTable<labelList> cellZones;
    HashTable<labelList> faceZones;
    List<bool> isAPatch(patchNames.size(),true);

    if (dofVertIndices.size())
    {
        // Use the vertex constraints to patch. Is of course bit dodgy since
        // face goes on patch if all its vertices are on a constraint.
        // Note: very inefficient since goes through all faces (including
        // internal ones) twice. Should do a construct without patches
        // and then repatchify.

        Info<< "Using " << dofVertIndices.size()
            << " DOF sets to detect boundary faces."<< endl;

        // Renumber vertex numbers on constraints
        forAll(dofVertIndices, patchi)
        {
            inplaceRenumber(unvToFoam, dofVertIndices[patchi]);
        }


        // Build labelHashSet of points per dofGroup/patch
        List<labelHashSet> dofGroups(dofVertIndices.size());

        forAll(dofVertIndices, patchi)
        {
            const labelList& foamVerts = dofVertIndices[patchi];
            dofGroups[patchi].insert(foamVerts);
        }

        List<DynamicList<face>> dynPatchFaces(dofVertIndices.size());

        forAll(cellVerts, celli)
        {
            const cellShape& shape = cellVerts[celli];

            for (const face& f : shape.faces())
            {
                label patchi = findPatch(dofGroups, f);

                if (patchi != -1)
                {
                    dynPatchFaces[patchi].append(f);
                }
            }
        }

        // Transfer
        patchFaceVerts.setSize(dynPatchFaces.size());

        forAll(dynPatchFaces, patchi)
        {
            patchFaceVerts[patchi].transfer(dynPatchFaces[patchi]);
        }
    }
    else
    {
        // Use the boundary faces.

        // Construct the patch faces by sorting the boundaryFaces according to
        // patch.
        patchFaceVerts.setSize(patchFaceIndices.size());

        Info<< "Sorting boundary faces according to group (patch)" << endl;

        // make sure that no face is used twice on the boundary
        // (possible for boundary-only faceZones)
        labelHashSet alreadyOnBoundary;

        // Construct map from boundaryFaceIndices
        Map<label> boundaryFaceToIndex(invertToMap(boundaryFaceIndices));

        forAll(patchFaceVerts, patchi)
        {
            Info << patchi << ": " << patchNames[patchi] << " is " << flush;

            faceList& patchFaces = patchFaceVerts[patchi];
            const labelList& faceIndices = patchFaceIndices[patchi];

            patchFaces.setSize(faceIndices.size());

            bool duplicateFaces = false;

            label cnt = 0;
            forAll(patchFaces, i)
            {
                if (boundaryFaceToIndex.found(faceIndices[i]))
                {
                    label bFacei = boundaryFaceToIndex[faceIndices[i]];

                    if (own[bFacei] != -1 && nei[bFacei] == -1)
                    {
                        patchFaces[cnt] = boundaryFaces[bFacei];
                        cnt++;
                        if (alreadyOnBoundary.found(bFacei))
                        {
                            duplicateFaces = true;
                        }
                    }
                }
            }

            if (cnt != patchFaces.size() || duplicateFaces)
            {
                isAPatch[patchi] = false;

                if (verbose)
                {
                    if (cnt != patchFaces.size())
                    {
                        WarningInFunction
                            << "For patch " << patchi << " there were "
                            << patchFaces.size()-cnt
                            << " faces not used because they seem"
                            << " to be internal. "
                            << "This seems to be a face or a cell-zone"
                            << endl;
                    }
                    else
                    {
                        WarningInFunction
                            << "Patch "
                            << patchi << " has faces that are already "
                            << " in use on other boundary-patches,"
                            << " Assuming faceZoneset." << endl;
                    }
                }

                patchFaces.clear();  // Assume that this is no patch at all

                if (cellCorrespondence[faceIndices[0]] >= 0)
                {
                    Info << "cellZone" << endl;
                    labelList theCells(faceIndices.size());
                    forAll(faceIndices, i)
                    {
                        if (cellCorrespondence[faceIndices[0]] < 0)
                        {
                            FatalErrorInFunction
                                << "The face index " << faceIndices[i]
                                << " was not found amongst the cells."
                                << " This kills the theory that "
                                << patchNames[patchi] << " is a cell zone"
                                << endl
                                << abort(FatalError);
                        }
                        theCells[i] = cellCorrespondence[faceIndices[i]];
                    }
                    cellZones.insert(patchNames[patchi], theCells);
                }
                else
                {
                    Info << "faceZone" << endl;
                    labelList theFaces(faceIndices.size());
                    forAll(faceIndices, i)
                    {
                        theFaces[i] = boundaryFaceToIndex[faceIndices[i]];
                    }
                    faceZones.insert(patchNames[patchi],theFaces);
                }
            }
            else
            {
                Info << "patch" << endl;

                forAll(patchFaces, i)
                {
                    label bFacei = boundaryFaceToIndex[faceIndices[i]];
                    alreadyOnBoundary.insert(bFacei);
                }
            }
        }
    }

    pointField polyPoints;
    polyPoints.transfer(points);

    // Length scaling factor
    polyPoints /= lengthScale;


    // For debugging: dump boundary faces as OBJ surface mesh
    if (args.found("dump"))
    {
        Info<< "Writing boundary faces to OBJ file boundaryFaces.obj"
            << nl << endl;

        // Create globally numbered surface
        meshedSurface rawSurface(polyPoints, boundaryFaces);

        // Write locally numbered surface
        meshedSurface
        (
            rawSurface.localPoints(),
            rawSurface.localFaces()
        ).write(runTime.path()/"boundaryFaces.obj");
    }


    Info<< "\nConstructing mesh with non-default patches of size:" << nl;
    DynamicList<word> usedPatchNames;
    DynamicList<faceList> usedPatchFaceVerts;

    forAll(patchNames, patchi)
    {
        if (isAPatch[patchi])
        {
            Info<< "    " << patchNames[patchi] << '\t'
                << patchFaceVerts[patchi].size() << nl;
            usedPatchNames.append(patchNames[patchi]);
            usedPatchFaceVerts.append(patchFaceVerts[patchi]);
        }
    }
    usedPatchNames.shrink();
    usedPatchFaceVerts.shrink();

    Info<< endl;

    // Construct mesh
    polyMesh mesh
    (
        IOobject
        (
            polyMesh::defaultRegion,
            runTime.constant(),
            runTime
        ),
        std::move(polyPoints),
        cellVerts,
        usedPatchFaceVerts,     // boundaryFaces,
        usedPatchNames,         // boundaryPatchNames,
        wordList(patchNames.size(), polyPatch::typeName), // boundaryPatchTypes,
        "defaultFaces",         // defaultFacesName
        polyPatch::typeName,    // defaultFacesType,
        wordList()              // boundaryPatchPhysicalTypes
    );

    // Remove files now, to ensure all mesh files written are consistent.
    mesh.removeFiles();

    if (faceZones.size() || cellZones.size())
    {
        Info << "Adding cell and face zones" << endl;

        List<pointZone*> pZones(0);
        List<faceZone*> fZones(faceZones.size());
        List<cellZone*> cZones(cellZones.size());

        if (cellZones.size())
        {
            forAll(cellZones.toc(), cnt)
            {
                word name = cellZones.toc()[cnt];
                Info<< " Cell Zone " << name << " " << tab
                    << cellZones[name].size() << endl;

                cZones[cnt] = new cellZone
                (
                    name,
                    cellZones[name],
                    cnt,
                    mesh.cellZones()
                );
            }
        }
        if (faceZones.size())
        {
            const labelList& own = mesh.faceOwner();
            const labelList& nei = mesh.faceNeighbour();
            const pointField& centers = mesh.faceCentres();
            const pointField& points = mesh.points();

            forAll(faceZones.toc(), cnt)
            {
                word name = faceZones.toc()[cnt];
                const labelList& oldIndizes = faceZones[name];
                labelList indizes(oldIndizes.size());

                Info<< " Face Zone " << name << " " << tab
                    << oldIndizes.size() << endl;

                forAll(indizes, i)
                {
                    const label old = oldIndizes[i];
                    label noveau = -1;

                    label c1 = faceToCell[0].lookup(old, -1);
                    label c2 = faceToCell[1].lookup(old, -1);

                    if (c1 < c2)
                    {
                        std::swap(c1, c2);
                    }
                    if (c2 == -1)
                    {
                        // Boundary face is part of the faceZone
                        forAll(own, j)
                        {
                            if (own[j] == c1)
                            {
                                const face& f = boundaryFaces[old];
                                if (mag(centers[j]- f.centre(points)) < SMALL)
                                {
                                    noveau = j;
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        forAll(nei, j)
                        {
                            if
                            (
                                (c1 == own[j] && c2 == nei[j])
                             || (c2 == own[j] && c1 == nei[j])
                            )
                            {
                                noveau = j;
                                break;
                            }
                        }
                    }
                    assert(noveau > -1);
                    indizes[i] = noveau;
                }
                fZones[cnt] = new faceZone
                (
                    faceZones.toc()[cnt],
                    indizes,
                    false, // none are flipped
                    cnt,
                    mesh.faceZones()
                );
            }
        }
        mesh.addZones(pZones, fZones, cZones);

        Info << endl;
    }

    // More precision (for points data)
    IOstream::minPrecision(10);

    mesh.write();

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
