/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2025 OpenCFD Ltd.
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

#include "calculatedProcessorFvPatchField.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::calculatedProcessorFvPatchField<Type>::calculatedProcessorFvPatchField
(
    const lduInterface& interface,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    coupledFvPatchField<Type>(p, iF),
    procInterface_(refCast<const lduPrimitiveProcessorInterface>(interface)),
    sendRequest_(-1),
    recvRequest_(-1)
{}


template<class Type>
Foam::calculatedProcessorFvPatchField<Type>::calculatedProcessorFvPatchField
(
    const calculatedProcessorFvPatchField<Type>& ptf
)
:
    coupledFvPatchField<Type>(ptf),
    procInterface_(ptf.procInterface_),
    sendRequest_(-1),
    recvRequest_(-1)
{}


template<class Type>
Foam::calculatedProcessorFvPatchField<Type>::calculatedProcessorFvPatchField
(
    const calculatedProcessorFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    coupledFvPatchField<Type>(ptf, iF),
    procInterface_(ptf.procInterface_),
    sendRequest_(-1),
    recvRequest_(-1)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
bool Foam::calculatedProcessorFvPatchField<Type>::all_ready() const
{
    return UPstream::finishedRequestPair(recvRequest_, sendRequest_);
}


template<class Type>
bool Foam::calculatedProcessorFvPatchField<Type>::ready() const
{
    const bool ok = UPstream::finishedRequest(recvRequest_);
    if (ok)
    {
        recvRequest_ = -1;
        if (UPstream::finishedRequest(sendRequest_)) sendRequest_ = -1;
    }
    return ok;
}


template<class Type>
Foam::tmp<Foam::Field<Type>>
Foam::calculatedProcessorFvPatchField<Type>::patchNeighbourField() const
{
    if (!this->ready())
    {
        FatalErrorInFunction
            << "Outstanding request on patch of size "
            << procInterface_.faceCells().size()
            << " between proc " << procInterface_.myProcNo()
            << " and " << procInterface_.neighbProcNo()
            << abort(FatalError);
    }
    return *this;
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::patchNeighbourField
(
    UList<Type>& pnf
) const
{
    if (!this->ready())
    {
        FatalErrorInFunction
            << "Outstanding request on patch of size "
            << procInterface_.faceCells().size()
            << " between proc " << procInterface_.myProcNo()
            << " and " << procInterface_.neighbProcNo()
            << abort(FatalError);
    }
    pnf.deepCopy(*this);
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::initEvaluate
(
    const Pstream::commsTypes commsType
)
{
    if (UPstream::parRun())
    {
        if constexpr (!is_contiguous_v<Type>)
        {
            FatalErrorInFunction
                << "Invalid for non-contiguous data types"
                << abort(FatalError);
        }

        // Bypass patchInternalField since uses fvPatch addressing

        Field<Type>& self = *this;  // Receive straight into *this

        {
            const Field<Type>& psiInternal = this->internalField();
            const labelUList& faceCells = procInterface_.faceCells();

            sendBuf_.resize_nocopy(faceCells.size());
            self.resize_nocopy(faceCells.size());

            forAll(faceCells, i)
            {
                sendBuf_[i] = psiInternal[faceCells[i]];
            }
        }

        recvRequest_ = UPstream::nRequests();
        UIPstream::read
        (
            UPstream::commsTypes::nonBlocking,
            procInterface_.neighbProcNo(),
            self,
            procInterface_.tag(),
            procInterface_.comm()
        );

        sendRequest_ = UPstream::nRequests();
        UOPstream::write
        (
            UPstream::commsTypes::nonBlocking,
            procInterface_.neighbProcNo(),
            sendBuf_,
            procInterface_.tag(),
            procInterface_.comm()
        );
    }
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::evaluate
(
    const Pstream::commsTypes commsType
)
{
    if (UPstream::parRun())
    {
        // Require receive data.
        // Only update the send request state.
        UPstream::waitRequest(recvRequest_); recvRequest_ = -1;
        if (UPstream::finishedRequest(sendRequest_)) sendRequest_ = -1;
    }
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::initInterfaceMatrixUpdate
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
    if (!this->all_ready())
    {
        FatalErrorInFunction
            << "Outstanding request(s) on interface "
            //<< interface_.name()
            << abort(FatalError);
    }

    // Bypass patchInternalField since uses fvPatch addressing
    const labelUList& faceCells = lduAddr.patchAddr(patchId);

    {
        scalarSendBuf_.resize_nocopy(faceCells.size());
        scalarRecvBuf_.resize_nocopy(faceCells.size());

        forAll(faceCells, i)
        {
            scalarSendBuf_[i] = psiInternal[faceCells[i]];
        }
    }

    recvRequest_ = UPstream::nRequests();
    UIPstream::read
    (
        UPstream::commsTypes::nonBlocking,
        procInterface_.neighbProcNo(),
        scalarRecvBuf_,
        procInterface_.tag(),
        procInterface_.comm()
    );

    sendRequest_ = UPstream::nRequests();
    UOPstream::write
    (
        UPstream::commsTypes::nonBlocking,
        procInterface_.neighbProcNo(),
        scalarSendBuf_,
        procInterface_.tag(),
        procInterface_.comm()
    );

    this->updatedMatrix(false);
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::addToInternalField
(
    solveScalarField& result,
    const bool add,
    const scalarField& coeffs,
    const solveScalarField& vals
) const
{
    const labelUList& faceCells = this->procInterface_.faceCells();

    if (add)
    {
        forAll(faceCells, elemI)
        {
            result[faceCells[elemI]] += coeffs[elemI]*vals[elemI];
        }
    }
    else
    {
        forAll(faceCells, elemI)
        {
            result[faceCells[elemI]] -= coeffs[elemI]*vals[elemI];
        }
    }
}


template<class Type>
void Foam::calculatedProcessorFvPatchField<Type>::updateInterfaceMatrix
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
    if (this->updatedMatrix())
    {
        return;
    }

    if (UPstream::parRun())
    {
        // Require receive data.
        // Only update the send request state.
        UPstream::waitRequest(recvRequest_); recvRequest_ = -1;
        if (UPstream::finishedRequest(sendRequest_)) sendRequest_ = -1;
    }


    // Consume straight from receive buffer. Note use of our own
    // helper to avoid using fvPatch addressing
    addToInternalField(result, !add, coeffs, scalarRecvBuf_);

    this->updatedMatrix(true);
}


// ************************************************************************* //
