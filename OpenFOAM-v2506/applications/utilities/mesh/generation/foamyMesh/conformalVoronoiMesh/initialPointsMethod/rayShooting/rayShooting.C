/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
    Copyright (C) 2018-2022 OpenCFD Ltd.
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

#include "rayShooting.H"
#include "addToRunTimeSelectionTable.H"
#include "triSurfaceMesh.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(rayShooting, 0);
    addToRunTimeSelectionTable(initialPointsMethod, rayShooting, dictionary);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::rayShooting::splitLine
(
    const line<point, point>& l,
    const scalar& pert,
    DynamicList<Vb::Point>& initialPoints
) const
{
    Foam::point midPoint(l.centre());
    const scalar localCellSize(cellShapeControls().cellSize(midPoint));

    const scalar minDistFromSurfaceSqr
    (
        minimumSurfaceDistanceCoeffSqr_
       *sqr(localCellSize)
    );

    if
    (
        magSqr(midPoint - l.start()) > minDistFromSurfaceSqr
     && magSqr(midPoint - l.end()) > minDistFromSurfaceSqr
    )
    {
        // Add extra points if line length is much bigger than local cell size
//        const scalar lineLength(l.mag());
//
//        if (lineLength > 4.0*localCellSize)
//        {
//            splitLine
//            (
//                line<point, point>(l.start(), midPoint),
//                pert,
//                initialPoints
//            );
//
//            splitLine
//            (
//                line<point, point>(midPoint, l.end()),
//                pert,
//                initialPoints
//            );
//        }

        if (randomiseInitialGrid_)
        {
            Foam::point newPt
            (
                midPoint.x() + pert*(rndGen().sample01<scalar>() - 0.5),
                midPoint.y() + pert*(rndGen().sample01<scalar>() - 0.5),
                midPoint.z() + pert*(rndGen().sample01<scalar>() - 0.5)
            );

            if
            (
                !geometryToConformTo().findSurfaceAnyIntersection
                (
                    midPoint,
                    newPt
                )
            )
            {
                midPoint = newPt;
            }
            else
            {
                WarningInFunction
                    << "Point perturbation crosses a surface. Not inserting."
                    << endl;
            }
        }

        initialPoints.append(toPoint(midPoint));
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::rayShooting::rayShooting
(
    const dictionary& initialPointsDict,
    const Time& runTime,
    Random& rndGen,
    const conformationSurfaces& geometryToConformTo,
    const cellShapeControl& cellShapeControls,
    const autoPtr<backgroundMeshDecomposition>& decomposition
)
:
    initialPointsMethod
    (
        typeName,
        initialPointsDict,
        runTime,
        rndGen,
        geometryToConformTo,
        cellShapeControls,
        decomposition
    ),
    randomiseInitialGrid_(detailsDict().get<Switch>("randomiseInitialGrid")),
    randomPerturbationCoeff_
    (
        detailsDict().get<scalar>("randomPerturbationCoeff")
    )
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::List<Vb::Point> Foam::rayShooting::initialPoints() const
{
    // Loop over surface faces
    const searchableSurfaces& surfaces = geometryToConformTo().geometry();
    const labelList& surfacesToConformTo = geometryToConformTo().surfaces();

    const scalar maxRayLength(surfaces.bounds().mag());

    // Initialise points list
    label initialPointsSize = 0;
    forAll(surfaces, surfI)
    {
        initialPointsSize += surfaces[surfI].size();
    }

    DynamicList<Vb::Point> initialPoints(initialPointsSize);

    forAll(surfacesToConformTo, surfI)
    {
        const searchableSurface& s = surfaces[surfacesToConformTo[surfI]];

        tmp<pointField> faceCentresTmp(s.coordinates());
        const pointField& faceCentres = faceCentresTmp();

        Info<< "    Shoot rays from " << s.name() << nl
            << "        nRays = " << faceCentres.size() << endl;

        forAll(faceCentres, fcI)
        {
            const Foam::point& fC = faceCentres[fcI];

            if
            (
                Pstream::parRun()
             && !decomposition().positionOnThisProcessor(fC)
            )
            {
                continue;
            }

            const scalar pert
            (
                randomPerturbationCoeff_
               *cellShapeControls().cellSize(fC)
            );

            pointIndexHit surfHitStart;
            label hitSurfaceStart;

            // Face centres should be on the surface so search distance can be
            // small
            geometryToConformTo().findSurfaceNearest
            (
                 fC,
                 sqr(pert),
                 surfHitStart,
                 hitSurfaceStart
            );

            vectorField normStart(1, vector::min);
            geometryToConformTo().getNormal
            (
                hitSurfaceStart,
                List<pointIndexHit>(1, surfHitStart),
                normStart
            );

            pointIndexHit surfHitEnd;
            label hitSurfaceEnd;

            geometryToConformTo().findSurfaceNearestIntersection
            (
                fC - normStart[0]*pert,
                fC - normStart[0]*maxRayLength,
                surfHitEnd,
                hitSurfaceEnd
            );

            if (surfHitEnd.hit())
            {
                vectorField normEnd(1, vector::min);
                geometryToConformTo().getNormal
                (
                    hitSurfaceEnd,
                    List<pointIndexHit>(1, surfHitEnd),
                    normEnd
                );

                if ((normStart[0] & normEnd[0]) < 0)
                {
                    line<point, point> l(fC, surfHitEnd.point());

                    if (Pstream::parRun())
                    {
                        // Clip the line in parallel
                        pointIndexHit procIntersection =
                            decomposition().findLine
                            (
                                l.start(),
                                l.end()
                            );

                        if (procIntersection.hit())
                        {
                            l =
                                line<point, point>
                                (
                                    l.start(),
                                    procIntersection.point()
                                );
                        }
                    }

                    splitLine
                    (
                        l,
                        pert,
                        initialPoints
                    );
                }
            }
        }
    }

    return List<Vb::Point>(std::move(initialPoints));
}


// ************************************************************************* //
