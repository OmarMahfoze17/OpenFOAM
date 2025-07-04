/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2014 OpenFOAM Foundation
    Copyright (C) 2018-2021 OpenCFD Ltd.
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

\*---------------------------------------------------------------------------*/

#include "meshToMeshMethod.H"
#include "tetOverlapVolume.H"
#include "OFstream.H"
#include "Time.H"
#include "treeBoundBox.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(meshToMeshMethod, 0);
    defineRunTimeSelectionTable(meshToMeshMethod, components);
}

Foam::scalar Foam::meshToMeshMethod::tolerance_ = 1e-6;

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

Foam::labelList Foam::meshToMeshMethod::maskCells() const
{
    boundBox intersectBb(src_.bounds());
    intersectBb &= tgt_.bounds();
    intersectBb.inflate(0.01);

    DynamicList<label> cells(src_.nCells());

    for (label srcCelli = 0; srcCelli < src_.nCells(); ++srcCelli)
    {
        boundBox cellBb(src_.cellBb(srcCelli));
        if (intersectBb.overlaps(cellBb))
        {
            cells.append(srcCelli);
        }
    }

    if (debug)
    {
        Pout<< "participating source mesh cells: " << src_.nCells() << endl;
    }

    return cells;
}


bool Foam::meshToMeshMethod::intersect
(
    const label srcCelli,
    const label tgtCelli
) const
{
    scalar threshold = tolerance_*src_.cellVolumes()[srcCelli];

    tetOverlapVolume overlapEngine;

    treeBoundBox bbTgtCell(tgt_.cellBb(tgtCelli));

    return overlapEngine.cellCellOverlapMinDecomp
    (
        src_,
        srcCelli,
        tgt_,
        tgtCelli,
        bbTgtCell,
        threshold
    );
}


Foam::scalar Foam::meshToMeshMethod::interVol
(
    const label srcCelli,
    const label tgtCelli
) const
{
    tetOverlapVolume overlapEngine;

    treeBoundBox bbTgtCell(tgt_.cellBb(tgtCelli));

    scalar vol = overlapEngine.cellCellOverlapVolumeMinDecomp
    (
        src_,
        srcCelli,
        tgt_,
        tgtCelli,
        bbTgtCell
    );

    return vol;
}


Foam::Tuple2<Foam::scalar, Foam::point>
Foam::meshToMeshMethod::interVolAndCentroid
(
    const label srcCelli,
    const label tgtCelli
)
{
    tetOverlapVolume overlapEngine;

    treeBoundBox bbTgtCell(tgt_.cellBb(tgtCelli));

    Tuple2<scalar, point> volAndInertia =
    overlapEngine.cellCellOverlapMomentMinDecomp
    (
        src_,
        srcCelli,
        tgt_,
        tgtCelli,
        bbTgtCell
    );

    // Convert from inertia to centroid
    if (volAndInertia.first() <= ROOTVSMALL)
    {
        volAndInertia.first() = 0.0;
        volAndInertia.second() = Zero;
    }
    else
    {
        volAndInertia.second() /= volAndInertia.first();
    }

    return volAndInertia;
}


void Foam::meshToMeshMethod::appendNbrCells
(
    const label celli,
    const polyMesh& mesh,
    const labelUList& visitedCells,
    DynamicList<label>& nbrCellIDs
) const
{
    const labelList& nbrCells = mesh.cellCells()[celli];

    // filter out cells already visited from cell neighbours
    for (const label nbrCelli : nbrCells)
    {
        if (!visitedCells.contains(nbrCelli))
        {
            nbrCellIDs.push_uniq(nbrCelli);
        }
    }
}


bool Foam::meshToMeshMethod::initialise
(
    labelListList& srcToTgtAddr,
    scalarListList& srcToTgtWght,
    labelListList& tgtToSrcAddr,
    scalarListList& tgtToSrcWght
) const
{
    srcToTgtAddr.setSize(src_.nCells());
    srcToTgtWght.setSize(src_.nCells());
    tgtToSrcAddr.setSize(tgt_.nCells());
    tgtToSrcWght.setSize(tgt_.nCells());

    if (!src_.nCells())
    {
        return false;
    }
    else if (!tgt_.nCells())
    {
        if (debug)
        {
            Pout<< "mesh interpolation: have " << src_.nCells() << " source "
                << " cells but no target cells" << endl;
        }

        return false;
    }

    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::meshToMeshMethod::meshToMeshMethod
(
    const polyMesh& src,
    const polyMesh& tgt
)
:
    src_(src),
    tgt_(tgt),
    V_(0.0)
{
    if (!src_.nCells() || !tgt_.nCells())
    {
        if (debug)
        {
            Pout<< "mesh interpolation: cells not on processor: Source cells = "
                << src_.nCells() << ", target cells = " << tgt_.nCells()
                << endl;
        }
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::meshToMeshMethod::~meshToMeshMethod()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::meshToMeshMethod::writeConnectivity
(
    const polyMesh& mesh1,
    const polyMesh& mesh2,
    const labelListList& mesh1ToMesh2Addr
) const
{
    Pout<< "Source size = " << mesh1.nCells() << endl;
    Pout<< "Target size = " << mesh2.nCells() << endl;

    word fName("addressing_" + mesh1.name() + "_to_" + mesh2.name());

    if (Pstream::parRun())
    {
        fName = fName +  "_proc" + Foam::name(Pstream::myProcNo());
    }

    OFstream os(src_.time().path()/fName + ".obj");

    label vertI = 0;
    forAll(mesh1ToMesh2Addr, i)
    {
        const labelList& addr = mesh1ToMesh2Addr[i];
        forAll(addr, j)
        {
            label celli = addr[j];
            const vector& c0 = mesh1.cellCentres()[i];

            const cell& c = mesh2.cells()[celli];
            const pointField pts(c.points(mesh2.faces(), mesh2.points()));
            forAll(pts, j)
            {
                const point& p = pts[j];
                os  << "v " << p.x() << ' ' << p.y() << ' ' << p.z() << nl;
                vertI++;
                os  << "v " << c0.x() << ' ' << c0.y() << ' ' << c0.z()
                    << nl;
                vertI++;
                os  << "l " << vertI - 1 << ' ' << vertI << nl;
            }
        }
    }
}


// ************************************************************************* //
