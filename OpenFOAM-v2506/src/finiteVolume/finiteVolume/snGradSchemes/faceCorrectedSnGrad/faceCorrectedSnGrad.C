/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

#include "faceCorrectedSnGrad.H"
#include "volPointInterpolation.H"
#include "triangle.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::fv::faceCorrectedSnGrad<Type>::fullGradCorrection
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    GeometricField<Type, pointPatchField, pointMesh> pvf
    (
        volPointInterpolation::New(mesh).interpolate(vf)
    );

    // construct GeometricField<Type, fvsPatchField, surfaceMesh>
    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsfCorr
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "snGradCorr("+vf.name()+')',
                vf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            vf.dimensions()*mesh.nonOrthDeltaCoeffs().dimensions()
        )
    );

    Field<Type>& sfCorr = tsfCorr.ref().primitiveFieldRef();

    const pointField& points = mesh.points();
    const faceList& faces = mesh.faces();
    const vectorField& Sf = mesh.Sf();
    const vectorField& C = mesh.C();
    const scalarField& magSf = mesh.magSf();
    const auto& owner = mesh.owner();
    const auto& neighbour = mesh.neighbour();

    forAll(sfCorr, facei)
    {
        typename outerProduct<vector, Type>::type fgrad
        (
            outerProduct<vector, Type>::type::zero
        );

        const face& fi = faces[facei];

        const vector nf(Sf[facei]/magSf[facei]);

        for (label pi = 0; pi < fi.size(); ++pi)
        {
            // Next point index
            const label pj = fi.fcIndex(pi);

            // Edge normal in plane of face
            const vector edgen(nf^(points[fi[pj]] - points[fi[pi]]));

            // Edge centre field value
            const Type pvfe(0.5*(pvf[fi[pj]] + pvf[fi[pi]]));

            // Integrate face gradient
            fgrad += edgen*pvfe;
        }

        // Finalize face-gradient by dividing by face area
        fgrad /= magSf[facei];

        // Calculate correction vector
        vector dCorr(C[neighbour[facei]] - C[owner[facei]]);
        dCorr /= (nf & dCorr);

        sfCorr[facei] = dCorr&fgrad;
    }

    tsfCorr.ref().boundaryFieldRef() = Zero;

    return tsfCorr;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::fv::faceCorrectedSnGrad<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    // construct GeometricField<Type, fvsPatchField, surfaceMesh>
    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tssf
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "snGradCorr("+vf.name()+')',
                vf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            vf.dimensions()*mesh.nonOrthDeltaCoeffs().dimensions()
        )
    );
    GeometricField<Type, fvsPatchField, surfaceMesh>& ssf = tssf.ref();

    for (direction cmpt = 0; cmpt < pTraits<Type>::nComponents; ++cmpt)
    {
        ssf.replace
        (
            cmpt,
            faceCorrectedSnGrad<typename pTraits<Type>::cmptType>(mesh)
           .fullGradCorrection(vf.component(cmpt))
        );
    }

    return tssf;
}


// ************************************************************************* //
