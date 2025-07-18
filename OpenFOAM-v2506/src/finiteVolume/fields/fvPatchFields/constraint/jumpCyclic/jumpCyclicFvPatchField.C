/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2019-2024 OpenCFD Ltd.
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

#include "jumpCyclicFvPatchField.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::jumpCyclicFvPatchField<Type>::jumpCyclicFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    cyclicFvPatchField<Type>(p, iF)
{}


template<class Type>
Foam::jumpCyclicFvPatchField<Type>::jumpCyclicFvPatchField
(
    const jumpCyclicFvPatchField<Type>& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    cyclicFvPatchField<Type>(ptf, p, iF, mapper)
{}


template<class Type>
Foam::jumpCyclicFvPatchField<Type>::jumpCyclicFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict,
    const bool needValue
)
:
    cyclicFvPatchField<Type>(p, iF, dict, needValue)
{}


template<class Type>
Foam::jumpCyclicFvPatchField<Type>::jumpCyclicFvPatchField
(
    const jumpCyclicFvPatchField<Type>& ptf
)
:
    cyclicFvPatchField<Type>(ptf)
{}


template<class Type>
Foam::jumpCyclicFvPatchField<Type>::jumpCyclicFvPatchField
(
    const jumpCyclicFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    cyclicFvPatchField<Type>(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::jumpCyclicFvPatchField<Type>::patchNeighbourField
(
    UList<Type>& pnf
) const
{
    const Field<Type>& iField = this->primitiveField();
    const labelUList& nbrFaceCells =
        this->cyclicPatch().neighbFvPatch().faceCells();

    Field<Type> jf(this->jump());
    if (!this->cyclicPatch().owner())
    {
        jf *= -1.0;
    }

    if (this->doTransform())
    {
        const auto& rot = this->forwardT()[0];

        forAll(pnf, i)
        {
            pnf[i] = (transform(rot, iField[nbrFaceCells[i]]) - jf[i]);
        }
    }
    else
    {
        forAll(pnf, i)
        {
            pnf[i] = (iField[nbrFaceCells[i]] - jf[i]);
        }
    }
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::jumpCyclicFvPatchField<Type>::patchNeighbourField() const
{
    auto tpnf = tmp<Field<Type>>::New(this->size());
    this->patchNeighbourField(tpnf.ref());
    return tpnf;
}


template<class Type>
void Foam::jumpCyclicFvPatchField<Type>::updateInterfaceMatrix
(
    solveScalarField& result,
    const bool add,
    const lduAddressing& lduAddr,
    const label patchId,
    const solveScalarField& psiInternal,
    const scalarField& coeffs,
    const direction cmpt,
    const Pstream::commsTypes
) const
{
    NotImplemented;
}


template<class Type>
void Foam::jumpCyclicFvPatchField<Type>::updateInterfaceMatrix
(
    Field<Type>& result,
    const bool add,
    const lduAddressing& lduAddr,
    const label patchId,
    const Field<Type>& psiInternal,
    const scalarField& coeffs,
    const Pstream::commsTypes
) const
{
    Field<Type> pnf(this->size());

    const labelUList& nbrFaceCells =
        lduAddr.patchAddr
        (
            this->cyclicPatch().neighbPatchID()
        );

    // only apply jump to original field
    if (&psiInternal == &this->primitiveField())
    {
        Field<Type> jf(this->jump());

        if (!this->cyclicPatch().owner())
        {
            jf *= -1.0;
        }

        forAll(pnf, facei)
        {
            pnf[facei] = psiInternal[nbrFaceCells[facei]] - jf[facei];
        }
    }
    else
    {
        forAll(pnf, facei)
        {
            pnf[facei] = psiInternal[nbrFaceCells[facei]];
        }
    }

    // Transform according to the transformation tensors
    this->transformCoupleField(pnf);

    const labelUList& faceCells = lduAddr.patchAddr(patchId);

    // Multiply the field by coefficients and add into the result
    this->addToInternalField(result, !add, faceCells, coeffs, pnf);
}


// ************************************************************************* //
