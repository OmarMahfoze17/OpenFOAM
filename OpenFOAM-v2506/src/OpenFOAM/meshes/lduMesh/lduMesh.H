/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

Class
    Foam::lduMesh

Description
    Abstract base class for meshes which provide LDU addressing for the
    construction of lduMatrix and LDU-solvers.

\*---------------------------------------------------------------------------*/

#ifndef Foam_lduMesh_H
#define Foam_lduMesh_H

#include "lduAddressing.H"
#include "lduInterfacePtrsList.H"
#include "typeInfo.H"
#include "InfoProxy.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class objectRegistry;
class lduMesh;

Ostream& operator<<(Ostream&, const InfoProxy<lduMesh>&);


/*---------------------------------------------------------------------------*\
                           Class lduMesh Declaration
\*---------------------------------------------------------------------------*/

class lduMesh
{
public:

    //- Runtime type information
    TypeName("lduMesh");


    //- Destructor
    virtual ~lduMesh() = default;


    // Member Functions

        // Access

            //- Return true if thisDb() is a valid DB
            virtual bool hasDb() const = 0;

            //- Return the object registry
            virtual const objectRegistry& thisDb() const;

            //- Return ldu addressing
            virtual const lduAddressing& lduAddr() const = 0;

            //- Return a list of pointers for each patch
            //- with only those pointing to interfaces being set
            virtual lduInterfacePtrsList interfaces() const = 0;

            //- Return communicator used for parallel communication
            virtual label comm() const = 0;

            //- Helper: reduce with current communicator
            template<class T, class BinaryOp>
            void reduce(T& val, BinaryOp bop) const
            {
                Foam::reduce(val, bop, UPstream::msgType(), comm());
            }


    // Info

        //- Return info proxy,
        //- used to print mesh information to a stream
        InfoProxy<lduMesh> info() const noexcept { return *this; }


    // Ostream Operator

        friend Ostream& operator<<(Ostream&, const InfoProxy<lduMesh>&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
