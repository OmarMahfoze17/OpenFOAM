/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2007-2019 PCOpt/NTUA
    Copyright (C) 2013-2019 FOSS GP
    Copyright (C) 2019 OpenCFD Ltd.
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

#include "linearUpwindNormal.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::linearUpwindNormal<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsfCorr
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "linearUpwind::correction(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            ),
            mesh,
            dimensioned<Type>(vf.dimensions(), Zero)
        )
    );

    GeometricField<Type, fvsPatchField, surfaceMesh>& sfCorr = tsfCorr();

    const surfaceScalarField& faceFlux = this->faceFlux_;

    const auto& owner = mesh.owner();
    const auto& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    tmp
    <
        GeometricField
        <
            typename outerProduct<vector, Type>::type,
            fvPatchField,
            volMesh
        >
    > tgradVf = gradScheme_().grad(vf, gradSchemeName_);

    GeometricField
    <
        typename outerProduct<vector, Type>::type,
        fvPatchField,
        volMesh
    >& gradVf = tgradVf();
    gradVf /= mag(gradVf) + 1.e-12;

    forAll(faceFlux, facei)
    {
        label celli = (faceFlux[facei] > 0) ? owner[facei] : neighbour[facei];
        sfCorr[facei] = (Cf[facei] - C[celli]) & gradVf[celli];
    }


    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
        GeometricBoundaryField& bSfCorr = sfCorr.boundaryField();

    forAll(bSfCorr, patchi)
    {
        fvsPatchField<Type>& pSfCorr = bSfCorr[patchi];

        if (pSfCorr.coupled())
        {
            const labelUList& pOwner =
                mesh.boundary()[patchi].faceCells();

            const vectorField& pCf = Cf.boundaryField()[patchi];

            const scalarField& pFaceFlux = faceFlux.boundaryField()[patchi];

            const Field<typename outerProduct<vector, Type>::type> pGradVfNei
            (
                gradVf.boundaryField()[patchi].patchNeighbourField()
            );

            // Build the d-vectors
            vectorField pd(Cf.boundaryField()[patchi].patch().delta());

            forAll(pOwner, facei)
            {
                label own = pOwner[facei];

                if (pFaceFlux[facei] > 0)
                {
                    pSfCorr[facei] = (pCf[facei] - C[own]) & gradVf[own];
                }
                else
                {
                    pSfCorr[facei] =
                        (pCf[facei] - pd[facei] - C[own]) & pGradVfNei[facei];
                }
            }
        }
    }

    return tsfCorr;
}


namespace Foam
{
    //makelimitedSurfaceInterpolationScheme(linearUpwindNormal)
    makelimitedSurfaceInterpolationTypeScheme(linearUpwindNormal, scalar)
    makelimitedSurfaceInterpolationTypeScheme(linearUpwindNormal, vector)
}

// ************************************************************************* //
