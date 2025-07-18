/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2017 OpenFOAM Foundation
    Copyright (C) 2019,2024 OpenCFD Ltd.
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

#include "jumpCyclicAMIFvPatchField.H"
#include "transformField.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::jumpCyclicAMIFvPatchField<Type>::jumpCyclicAMIFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    cyclicAMIFvPatchField<Type>(p, iF)
{}


template<class Type>
Foam::jumpCyclicAMIFvPatchField<Type>::jumpCyclicAMIFvPatchField
(
    const jumpCyclicAMIFvPatchField<Type>& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    cyclicAMIFvPatchField<Type>(ptf, p, iF, mapper)
{}


template<class Type>
Foam::jumpCyclicAMIFvPatchField<Type>::jumpCyclicAMIFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    cyclicAMIFvPatchField<Type>(p, iF, dict)
{
    // Call this evaluation in derived classes
    //this->evaluate(Pstream::commsTypes::buffered);
}


template<class Type>
Foam::jumpCyclicAMIFvPatchField<Type>::jumpCyclicAMIFvPatchField
(
    const jumpCyclicAMIFvPatchField<Type>& ptf
)
:
    cyclicAMIFvPatchField<Type>(ptf)
{}


template<class Type>
Foam::jumpCyclicAMIFvPatchField<Type>::jumpCyclicAMIFvPatchField
(
    const jumpCyclicAMIFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    cyclicAMIFvPatchField<Type>(ptf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::jumpCyclicAMIFvPatchField<Type>::patchNeighbourField() const
{
    const Field<Type>& iField = this->primitiveField();
    const labelUList& nbrFaceCells =
        this->cyclicAMIPatch().cyclicAMIPatch().neighbPatch().faceCells();

    Field<Type> pnf(iField, nbrFaceCells);
    tmp<Field<Type>> tpnf;

    if (this->cyclicAMIPatch().applyLowWeightCorrection())
    {
        tpnf =
            this->cyclicAMIPatch().interpolate
            (
                pnf,
                this->patchInternalField()()
            );
    }
    else
    {
        tpnf = this->cyclicAMIPatch().interpolate(pnf);
    }

    if (this->doTransform())
    {
        transform(tpnf.ref(), this->forwardT(), tpnf());
    }

    tmp<Field<Type>> tjf = jump();
    if (!this->cyclicAMIPatch().owner())
    {
        tjf = -tjf;
    }

    return tpnf - tjf;
}


template<class Type>
void Foam::jumpCyclicAMIFvPatchField<Type>::initEvaluate
(
    const Pstream::commsTypes commsType
)
{
    // Bypass cyclicAMI to avoid caching
    coupledFvPatchField<Type>::initEvaluate(commsType);
}


template<class Type>
void Foam::jumpCyclicAMIFvPatchField<Type>::evaluate
(
    const Pstream::commsTypes commsType
)
{
    // Bypass cyclicAMI to avoid caching
    coupledFvPatchField<Type>::evaluate(commsType);
}


template<class Type>
void Foam::jumpCyclicAMIFvPatchField<Type>::initInterfaceMatrixUpdate
(
    solveScalarField& result,
    const bool add,
    const lduAddressing& lduAddr,
    const label patchId,
    const solveScalarField& psiInternal,
    const scalarField& coeffs,
    const direction cmpt,
    const Pstream::commsTypes commsType
) const
{
    // Bypass cyclicAMI to avoid caching
    coupledFvPatchField<Type>::initInterfaceMatrixUpdate
    (
        result,
        add,
        lduAddr,
        patchId,
        psiInternal,
        coeffs,
        cmpt,
        commsType
    );
}


template<class Type>
void Foam::jumpCyclicAMIFvPatchField<Type>::updateInterfaceMatrix
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
void Foam::jumpCyclicAMIFvPatchField<Type>::initInterfaceMatrixUpdate
(
    Field<Type>& result,
    const bool add,
    const lduAddressing& lduAddr,
    const label patchId,
    const Field<Type>& psiInternal,
    const scalarField& coeffs,
    const Pstream::commsTypes commsType
) const
{
    // Bypass cyclicAMI to avoid caching
    coupledFvPatchField<Type>::initInterfaceMatrixUpdate
    (
        result,
        add,
        lduAddr,
        patchId,
        psiInternal,
        coeffs,
        commsType
    );
}


template<class Type>
void Foam::jumpCyclicAMIFvPatchField<Type>::updateInterfaceMatrix
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
    const labelUList& nbrFaceCells =
        lduAddr.patchAddr
        (
            this->cyclicAMIPatch().neighbPatchID()
        );

    Field<Type> pnf(psiInternal, nbrFaceCells);

    if (this->cyclicAMIPatch().applyLowWeightCorrection())
    {
        pnf =
            this->cyclicAMIPatch().interpolate
            (
                pnf,
                this->patchInternalField()()
            );

    }
    else
    {
        pnf = this->cyclicAMIPatch().interpolate(pnf);
    }

    // only apply jump to original field
    if (&psiInternal == &this->primitiveField())
    {
        Field<Type> jf(this->jump());
        if (!this->cyclicAMIPatch().owner())
        {
            jf *= -1.0;
        }

        pnf -= jf;
    }

    // Transform according to the transformation tensors
    this->transformCoupleField(pnf);

    const labelUList& faceCells = lduAddr.patchAddr(patchId);

    // Multiply the field by coefficients and add into the result
    this->addToInternalField(result, !add,faceCells, coeffs, pnf);
}


// ************************************************************************* //
