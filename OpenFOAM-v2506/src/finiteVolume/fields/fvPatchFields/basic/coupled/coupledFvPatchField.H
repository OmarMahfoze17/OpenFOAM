/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

Class
    Foam::coupledFvPatchField

Group
    grpCoupledBoundaryConditions

Description
    Abstract base class for coupled patches.

    The "value" entry is usually MUST_READ and always WRITE.

SourceFiles
    coupledFvPatchField.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_coupledFvPatchField_H
#define Foam_coupledFvPatchField_H

#include "LduInterfaceField.H"
#include "fvPatchField.H"
#include "coupledFvPatch.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class coupledFvPatch Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class coupledFvPatchField
:
    public LduInterfaceField<Type>,
    public fvPatchField<Type>
{
public:

    //- Runtime type information
    TypeName(coupledFvPatch::typeName_());


    // Constructors

        //- Construct from patch and internal field
        coupledFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&
        );

        //- Construct from patch and internal field and patch field
        coupledFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const Field<Type>&
        );

        //- Construct from patch, internal field and dictionary
        coupledFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const dictionary&,
            IOobjectOption::readOption requireValue = IOobjectOption::MUST_READ
        );

        //- Compatibility (prefer with readOption)
        coupledFvPatchField
        (
            const fvPatch& p,
            const DimensionedField<Type, volMesh>& iF,
            const dictionary& dict,
            const bool needValue
        )
        :
            coupledFvPatchField
            (
                p, iF, dict,
                (needValue? IOobjectOption::MUST_READ : IOobjectOption::NO_READ)
            )
        {}

        //- Construct by mapping the given coupledFvPatchField onto a new patch
        coupledFvPatchField
        (
            const coupledFvPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        coupledFvPatchField
        (
            const coupledFvPatchField<Type>&
        );

        //- Construct as copy setting internal field reference
        coupledFvPatchField
        (
            const coupledFvPatchField<Type>&,
            const DimensionedField<Type, volMesh>&
        );

        //- Return a clone
        virtual tmp<fvPatchField<Type>> clone() const = 0;

        //- Construct and return a clone
        virtual tmp<fvPatchField<Type>> clone
        (
            const DimensionedField<Type, volMesh>&
        ) const = 0;


    // Member Functions

        // Access

            //- True if this patch field is derived from coupledFvPatchField.
            virtual bool coupled() const
            {
                return true;
            }

            //- Return neighbour field of internal field
            virtual tmp<Field<Type>> patchNeighbourField() const = 0;

            //- Retrieve neighbour coupled data
            virtual void patchNeighbourField(UList<Type>&) const = 0;


        // Evaluation Functions

            //- Return patch-normal gradient
            virtual tmp<Field<Type>> snGrad
            (
                const scalarField& deltaCoeffs
            ) const;

            //- Return patch-normal gradient
            virtual tmp<Field<Type>> snGrad() const
            {
                NotImplemented;
                return *this;
            }

            //- Initialise the evaluation of the patch field
            virtual void initEvaluate
            (
                const Pstream::commsTypes commsType
            );

            //- Evaluate the patch field
            virtual void evaluate
            (
                const Pstream::commsTypes commsType
            );

            //- Initialise the evaluation of the patch field after a local
            //  operation
            virtual void initEvaluateLocal
            (
                const Pstream::commsTypes commsType =
                    Pstream::commsTypes::buffered
            )
            {
                initEvaluate(commsType);
            }

            //- Evaluate the patch field after a local operation (e.g. *=)
            virtual void evaluateLocal
            (
                const Pstream::commsTypes commsType =
                    Pstream::commsTypes::buffered
            )
            {
                evaluate(commsType);
            }

            //- Return the matrix diagonal coefficients corresponding to the
            //  evaluation of the value of this patchField with given weights
            virtual tmp<Field<Type>> valueInternalCoeffs
            (
                const tmp<scalarField>&
            ) const;

            //- Return the matrix source coefficients corresponding to the
            //  evaluation of the value of this patchField with given weights
            virtual tmp<Field<Type>> valueBoundaryCoeffs
            (
                const tmp<scalarField>&
            ) const;

            //- Return the matrix diagonal coefficients corresponding to the
            //  evaluation of the gradient of this patchField
            virtual tmp<Field<Type>> gradientInternalCoeffs
            (
                const scalarField& deltaCoeffs
            ) const;

            //- Return the matrix diagonal coefficients corresponding to the
            //  evaluation of the gradient of this patchField
            virtual tmp<Field<Type>> gradientInternalCoeffs() const;

            //- Return the matrix source coefficients corresponding to the
            //  evaluation of the gradient of this patchField
            virtual tmp<Field<Type>> gradientBoundaryCoeffs
            (
                const scalarField& deltaCoeffs
            ) const;

            //- Return the matrix source coefficients corresponding to the
            //  evaluation of the gradient of this patchField
            virtual tmp<Field<Type>> gradientBoundaryCoeffs() const;


        // Coupled interface functionality

            //- Update result field based on interface functionality
            virtual void updateInterfaceMatrix
            (
                solveScalarField& result,
                const bool add,
                const lduAddressing& lduAddr,
                const label patchId,
                const solveScalarField& psiInternal,
                const scalarField& coeffs,
                const direction,
                const Pstream::commsTypes commsType
            ) const = 0;

            //- Update result field based on interface functionality
            virtual void updateInterfaceMatrix
            (
                Field<Type>&,
                const bool add,
                const lduAddressing& lduAddr,
                const label patchId,
                const Field<Type>&,
                const scalarField&,
                const Pstream::commsTypes commsType
            ) const = 0;


        //- Write includes "value" entry
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "coupledFvPatchField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
