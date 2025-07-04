/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "pointLinear.H"
#include "fvMesh.H"
#include "volPointInterpolation.H"
#include "triangle.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::pointLinear<Type>::
correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    GeometricField<Type, pointPatchField, pointMesh> pvf
    (
        volPointInterpolation::New(mesh).interpolate(vf)
    );

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsfCorr =
        linearInterpolate(vf);

    Field<Type>& sfCorr = tsfCorr.ref().primitiveFieldRef();

    const pointField& points = mesh.points();
    const pointField& C = mesh.C();
    const faceList& faces = mesh.faces();
    const scalarField& w = mesh.weights();
    const auto& owner = mesh.owner();
    const auto& neighbour = mesh.neighbour();

    forAll(sfCorr, facei)
    {
        point pi =
            w[owner[facei]]*C[owner[facei]]
          + (1.0 - w[owner[facei]])*C[neighbour[facei]];

        const face& f = faces[facei];

        scalar at = triPointRef
        (
            pi,
            points[f[0]],
            points[f[f.size()-1]]
        ).mag();

        scalar sumAt = at;
        Type sumPsip = at*(1.0/3.0)*
        (
            sfCorr[facei]
          + pvf[f[0]]
          + pvf[f[f.size()-1]]
        );

        for (label pointi=1; pointi<f.size(); pointi++)
        {
            at = triPointRef
            (
                pi,
                points[f[pointi]],
                points[f[pointi-1]]
            ).mag();

            sumAt += at;
            sumPsip += at*(1.0/3.0)*
            (
                sfCorr[facei]
              + pvf[f[pointi]]
              + pvf[f[pointi-1]]
            );

        }

        sfCorr[facei] = sumPsip/sumAt - sfCorr[facei];
    }


    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
        Boundary& bSfCorr = tsfCorr.ref().boundaryFieldRef();

    forAll(bSfCorr, patchi)
    {
        fvsPatchField<Type>& pSfCorr = bSfCorr[patchi];

        if (pSfCorr.coupled())
        {
            const fvPatch& fvp = mesh.boundary()[patchi];
            const scalarField& pWghts = mesh.weights().boundaryField()[patchi];
            const labelUList& pOwner = fvp.faceCells();
            const vectorField& pNbrC = mesh.C().boundaryField()[patchi];

            forAll(pOwner, facei)
            {
                label own = pOwner[facei];

                point pi =
                    pWghts[facei]*C[own]
                  + (1.0 - pWghts[facei])*pNbrC[facei];

                const face& f = faces[facei+fvp.start()];

                scalar at = triPointRef
                (
                    pi,
                    points[f[0]],
                    points[f[f.size()-1]]
                ).mag();

                scalar sumAt = at;
                Type sumPsip = at*(1.0/3.0)*
                (
                    pSfCorr[facei]
                  + pvf[f[0]]
                  + pvf[f[f.size()-1]]
                );

                for (label pointi=1; pointi<f.size(); pointi++)
                {
                    at = triPointRef
                    (
                        pi,
                        points[f[pointi]],
                        points[f[pointi-1]]
                    ).mag();

                    sumAt += at;
                    sumPsip += at*(1.0/3.0)*
                    (
                        pSfCorr[facei]
                      + pvf[f[pointi]]
                      + pvf[f[pointi-1]]
                    );

                }

                pSfCorr[facei] = sumPsip/sumAt - pSfCorr[facei];
            }
        }
        else
        {
            pSfCorr = Zero;
        }
    }

    return tsfCorr;
}


namespace Foam
{
    makeSurfaceInterpolationScheme(pointLinear);
}

// ************************************************************************* //
