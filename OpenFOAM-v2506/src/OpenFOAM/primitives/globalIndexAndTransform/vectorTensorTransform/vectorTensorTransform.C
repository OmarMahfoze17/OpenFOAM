/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2025 OpenCFD Ltd.
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

#include "vectorTensorTransform.H"
#include "IOstreams.H"
#include "StringStream.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

const char* const Foam::vectorTensorTransform::typeName =
    "vectorTensorTransform";

const Foam::vectorTensorTransform Foam::vectorTensorTransform::zero
(
    Zero,
    Zero,
    false
);


const Foam::vectorTensorTransform Foam::vectorTensorTransform::I
(
    Zero,
    sphericalTensor::I,
    false
);


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::vectorTensorTransform::vectorTensorTransform(Istream& is)
{
    is  >> *this;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::vectorTensorTransform::checkRotation(const scalar tol)
{
    // Detect identity and zero rotations
    hasR_ = !(R_.is_identity(tol) || Foam::mag(R_) < tol);
}


Foam::word Foam::name(const vectorTensorTransform& s)
{
    OStringStream buf;

    buf << '(' << s.t() << ',' << s.R() << ')';

    return buf.str();
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

Foam::Istream& Foam::operator>>(Istream& is, vectorTensorTransform& tr)
{
    is.readBegin("vectorTensorTransform");

    is  >> tr.t_ >> tr.R_ >> tr.hasR_;

    is.readEnd("vectorTensorTransform");

    is.check(FUNCTION_NAME);

    return is;
}


Foam::Ostream& Foam::operator<<(Ostream& os, const vectorTensorTransform& tr)
{
    os  << token::BEGIN_LIST
        << tr.t() << token::SPACE << tr.R() << token::SPACE << tr.hasR()
        << token::END_LIST;

    return os;
}


// ************************************************************************* //
