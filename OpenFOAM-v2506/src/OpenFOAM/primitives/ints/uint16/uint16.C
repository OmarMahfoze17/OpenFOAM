/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2025 OpenCFD Ltd.
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

#include "uint16.H"
#include "int32.H"
#include "IOstreams.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

const char* const Foam::pTraits<uint16_t>::typeName = "uint16";
const char* const Foam::pTraits<uint16_t>::componentNames[] = { "" };

const uint16_t Foam::pTraits<uint16_t>::zero = 0;
const uint16_t Foam::pTraits<uint16_t>::one = 1;
const uint16_t Foam::pTraits<uint16_t>::min = 0;
const uint16_t Foam::pTraits<uint16_t>::max = UINT16_MAX;
const uint16_t Foam::pTraits<uint16_t>::rootMin = 0;
const uint16_t Foam::pTraits<uint16_t>::rootMax = UINT16_MAX;


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

Foam::Istream& Foam::operator>>(Istream& is, uint16_t& val)
{
    int32_t parsed;
    is >> parsed;

    val = uint16_t(parsed); // narrow
    return is;
}


Foam::Ostream& Foam::operator<<(Ostream& os, const uint16_t val)
{
    return (os << int32_t(val)); // widen (fits as int32 without sign problem)
}


// ************************************************************************* //
