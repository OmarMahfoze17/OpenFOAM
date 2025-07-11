/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2017-2023 OpenCFD Ltd.
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

#include "treeBoundBox.H"
#include "Random.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

inline Foam::treeBoundBox::treeBoundBox(Foam::zero_one)
:
    boundBox(Foam::zero_one{})
{}


inline Foam::treeBoundBox::treeBoundBox(const point& p)
:
    boundBox(p)
{}


inline Foam::treeBoundBox::treeBoundBox(const point& min, const point& max)
:
    boundBox(min, max)
{}


inline Foam::treeBoundBox::treeBoundBox(const Pair<point>& bb)
:
    boundBox(bb)
{}


inline Foam::treeBoundBox::treeBoundBox(Istream& is)
:
    boundBox(is)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline Foam::point Foam::treeBoundBox::corner(const direction octant) const
{
    return point
    (
        (octant & RIGHTHALF) ? max().x() : min().x(),
        (octant & TOPHALF)   ? max().y() : min().y(),
        (octant & FRONTHALF) ? max().z() : min().z()
    );
}


// Returns octant in which point resides. Reverse of subBbox.
inline Foam::direction Foam::treeBoundBox::subOctant(const point& pt) const
{
    return subOctant(centre(), pt);
}


// Returns octant in which point resides. Reverse of subBbox.
// Precalculated midpoint
inline Foam::direction Foam::treeBoundBox::subOctant
(
    const point& mid,
    const point& pt
)
{
    direction octant = 0;

    if (pt.x() > mid.x())
    {
        octant |= treeBoundBox::RIGHTHALF;
    }

    if (pt.y() > mid.y())
    {
        octant |= treeBoundBox::TOPHALF;
    }

    if (pt.z() > mid.z())
    {
        octant |= treeBoundBox::FRONTHALF;
    }

    return octant;
}


// Returns octant in which point resides. Reverse of subBbox.
// Flags point exactly on edge.
inline Foam::direction Foam::treeBoundBox::subOctant
(
    const point& pt,
    bool& onEdge
) const
{
    return subOctant(centre(), pt, onEdge);
}


// Returns octant in which point resides. Reverse of subBbox.
// Precalculated midpoint
inline Foam::direction Foam::treeBoundBox::subOctant
(
    const point& mid,
    const point& pt,
    bool& onEdge
)
{
    direction octant = 0;
    onEdge = false;

    if (pt.x() > mid.x())
    {
        octant |= treeBoundBox::RIGHTHALF;
    }
    else if (pt.x() == mid.x())
    {
        onEdge = true;
    }

    if (pt.y() > mid.y())
    {
        octant |= treeBoundBox::TOPHALF;
    }
    else if (pt.y() == mid.y())
    {
        onEdge = true;
    }

    if (pt.z() > mid.z())
    {
        octant |= treeBoundBox::FRONTHALF;
    }
    else if (pt.z() == mid.z())
    {
        onEdge = true;
    }

    return octant;
}


// Returns octant in which intersection resides.
// Precalculated midpoint. If the point is on the dividing line between
// the octants the direction vector determines which octant to use
// (i.e. in which octant the point would be if it were moved along dir)
inline Foam::direction Foam::treeBoundBox::subOctant
(
    const point& mid,
    const vector& dir,
    const point& pt,
    bool& onEdge
)
{
    direction octant = 0;
    onEdge = false;

    if (pt.x() > mid.x())
    {
        octant |= treeBoundBox::RIGHTHALF;
    }
    else if (pt.x() == mid.x())
    {
        onEdge = true;
        if (dir.x() > 0)
        {
            octant |= treeBoundBox::RIGHTHALF;
        }
    }

    if (pt.y() > mid.y())
    {
        octant |= treeBoundBox::TOPHALF;
    }
    else if (pt.y() == mid.y())
    {
        onEdge = true;
        if (dir.y() > 0)
        {
            octant |= treeBoundBox::TOPHALF;
        }
    }

    if (pt.z() > mid.z())
    {
        octant |= treeBoundBox::FRONTHALF;
    }
    else if (pt.z() == mid.z())
    {
        onEdge = true;
        if (dir.z() > 0)
        {
            octant |= treeBoundBox::FRONTHALF;
        }
    }

    return octant;
}


inline void Foam::treeBoundBox::searchOrder
(
    const point& pt,
    FixedList<direction, 8>& octantOrder
) const
{
    vector dist = centre() - pt;

    direction octant = 0;

    if (dist.x() < 0)
    {
        octant |= treeBoundBox::RIGHTHALF;
        dist.x() *= -1;
    }

    if (dist.y() < 0)
    {
        octant |= treeBoundBox::TOPHALF;
        dist.y() *= -1;
    }

    if (dist.z() < 0)
    {
        octant |= treeBoundBox::FRONTHALF;
        dist.z() *= -1;
    }

    direction min = 0;
    direction mid = 0;
    direction max = 0;

    if (dist.x() < dist.y())
    {
        if (dist.y() < dist.z())
        {
            min = treeBoundBox::RIGHTHALF;
            mid = treeBoundBox::TOPHALF;
            max = treeBoundBox::FRONTHALF;
        }
        else if (dist.z() < dist.x())
        {
            min = treeBoundBox::FRONTHALF;
            mid = treeBoundBox::RIGHTHALF;
            max = treeBoundBox::TOPHALF;
        }
        else
        {
            min = treeBoundBox::RIGHTHALF;
            mid = treeBoundBox::FRONTHALF;
            max = treeBoundBox::TOPHALF;
        }
    }
    else
    {
        if (dist.z() < dist.y())
        {
            min = treeBoundBox::FRONTHALF;
            mid = treeBoundBox::TOPHALF;
            max = treeBoundBox::RIGHTHALF;
        }
        else if (dist.x() < dist.z())
        {
            min = treeBoundBox::TOPHALF;
            mid = treeBoundBox::RIGHTHALF;
            max = treeBoundBox::FRONTHALF;
        }
        else
        {
            min = treeBoundBox::TOPHALF;
            mid = treeBoundBox::FRONTHALF;
            max = treeBoundBox::RIGHTHALF;
        }
    }

    // Primary subOctant
    octantOrder[0] = octant;
    // subOctants joined to the primary by faces.
    octantOrder[1] = octant ^ min;
    octantOrder[2] = octant ^ mid;
    octantOrder[3] = octant ^ max;
    // subOctants joined to the primary by edges.
    octantOrder[4] = octantOrder[1] ^ mid;
    octantOrder[5] = octantOrder[1] ^ max;
    octantOrder[6] = octantOrder[2] ^ max;
    // subOctants joined to the primary by corner.
    octantOrder[7] = octantOrder[4] ^ max;
}


inline Foam::FixedList<Foam::direction, 8>
Foam::treeBoundBox::searchOrder(const point& pt) const
{
    FixedList<direction, 8> octantOrder;
    searchOrder(pt, octantOrder);
    return octantOrder;
}


inline bool Foam::treeBoundBox::subOverlaps
(
    const direction octant,
    const point& centre,
    const scalar radiusSqr
) const
{
    // Accelerated version of
    //     subBbox(octant).overlaps(centre, radiusSqr)

    // NB: ordering of corners is irrelevant
    return box_sphere_overlaps
    (
        this->centre(),
        corner(octant),
        centre,
        radiusSqr
    );
}


inline bool Foam::treeBoundBox::intersects
(
    const linePointRef& ln,
    point& pt
) const
{
    return intersects(ln.start(), ln.end(), pt);
}


inline Foam::treeBoundBox Foam::treeBoundBox::extend
(
    Random& rndGen,
    const scalar factor
) const
{
    treeBoundBox bb(*this);

    bb.inflate(rndGen, factor);

    return bb;
}


inline Foam::treeBoundBox Foam::treeBoundBox::extend
(
    Random& rndGen,
    const scalar factor,
    const scalar delta
) const
{
    treeBoundBox bb(*this);

    bb.inflate(rndGen, factor, delta);

    return bb;
}


// * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * * //

inline bool Foam::operator==(const treeBoundBox& a, const treeBoundBox& b)
{
    return static_cast<const boundBox&>(a) == static_cast<const boundBox&>(b);
}


inline bool Foam::operator!=(const treeBoundBox& a, const treeBoundBox& b)
{
    return !(a == b);
}

// ************************************************************************* //
