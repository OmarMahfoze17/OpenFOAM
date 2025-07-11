/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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
    Foam::cyclicAMIGAMGInterface

Description
    GAMG agglomerated cyclic AMI interface.

SourceFiles
    cyclicAMIGAMGInterface.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_cyclicAMIGAMGInterface_H
#define Foam_cyclicAMIGAMGInterface_H

#include "GAMGInterface.H"
#include "cyclicAMILduInterface.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class cyclicAMIGAMGInterface Declaration
\*---------------------------------------------------------------------------*/

class cyclicAMIGAMGInterface
:
    public GAMGInterface,
    virtual public cyclicAMILduInterface
{
    // Private data

        label neighbPatchID_;

        bool owner_;

        tensorField forwardT_;

        tensorField reverseT_;

        //- AMI interface
        autoPtr<AMIPatchToPatchInterpolation> amiPtr_;

        //- rank in AMI with respect to parent communicator.
        //- Used for combining contributions from different processors if the
        //- destination processor was not in the original set.
        label myProcNo_;


    // Private Member Functions

        //- No copy construct
        cyclicAMIGAMGInterface(const cyclicAMIGAMGInterface&) = delete;

        //- No copy assignment
        void operator=(const cyclicAMIGAMGInterface&) = delete;


public:

    //- Runtime type information
    TypeName("cyclicAMI");


    // Constructors

        //- Construct from fine level interface,
        //- local and neighbour restrict addressing
        cyclicAMIGAMGInterface
        (
            const label index,
            const lduInterfacePtrsList& coarseInterfaces,
            const lduInterface& fineInterface,
            const labelField& restrictAddressing,
            const labelField& neighbourRestrictAddressing,
            const label fineLevelIndex,
            const label coarseComm
        );

        //- Construct from Istream
        cyclicAMIGAMGInterface
        (
            const label index,
            const lduInterfacePtrsList& coarseInterfaces,
            Istream& is
        );

        //- Construct from fine level interface,
        //- local and neighbour restrict addressing
        cyclicAMIGAMGInterface
        (
            const label index,
            const lduInterfacePtrsList& coarseInterfaces,
            const lduInterface& fineInterface,
            const labelList& interfaceMap,
            const labelUList& faceCells,
            const labelUList& faceRestrictAddresssing,
            const labelUList& faceOffsets,
            const lduInterfacePtrsList& allInterfaces,
            const label coarseComm,
            const label myProcNo,
            const labelList& procAgglomMap
        );

        //- Construct by assembling and returning a clone.
        virtual autoPtr<GAMGInterface> clone
        (
            const label index,
            const lduInterfacePtrsList& coarseInterfaces,
            const labelList& interfaceMap,
            const labelUList& faceCells,
            const labelUList& faceRestrictAddresssing,
            const labelUList& faceOffsets,
            const lduInterfacePtrsList& allInterfaces,
            const label coarseComm,
            const label myProcNo,
            const labelList& procAgglomMap
        ) const
        {
            return autoPtr<GAMGInterface>
            (
                new cyclicAMIGAMGInterface
                (
                    index,
                    coarseInterfaces,
                    *this,
                    interfaceMap,
                    faceCells,
                    faceRestrictAddresssing,
                    faceOffsets,
                    allInterfaces,
                    coarseComm,
                    myProcNo,
                    procAgglomMap
                )
            );
        }


    //- Destructor
    virtual ~cyclicAMIGAMGInterface() = default;


    // Member Functions

        // Interface transfer functions

            //- Transfer and return internal field adjacent to the interface
            virtual tmp<labelField> internalFieldTransfer
            (
                const Pstream::commsTypes commsType,
                const labelUList& iF
            ) const;


        //- Cyclic interface functions

            //- Return neighbour processor number
            virtual label neighbPatchID() const
            {
                return neighbPatchID_;
            }

            virtual bool owner() const
            {
                return owner_;
            }

            virtual const cyclicAMIGAMGInterface& neighbPatch() const
            {
                return dynamic_cast<const cyclicAMIGAMGInterface&>
                (
                    coarseInterfaces_[neighbPatchID()]
                );
            }

            virtual const AMIPatchToPatchInterpolation& AMI() const
            {
                return *amiPtr_;
            }

            //- Return face transformation tensor
            virtual const tensorField& forwardT() const
            {
                return forwardT_;
            }

            //- Return neighbour-cell transformation tensor
            virtual const tensorField& reverseT() const
            {
                return reverseT_;
            }

            //- -1 or old local rank
            virtual label myProcNo() const
            {
                return myProcNo_;
            }


        // I/O

            //- Write to stream
            virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
