/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2014-2016 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
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

#include "uint64.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

const char* const Foam::pTraits<uint64_t>::componentNames[] = { "" };

const uint64_t Foam::pTraits<uint64_t>::zero = 0;
const uint64_t Foam::pTraits<uint64_t>::one = 1;
const uint64_t Foam::pTraits<uint64_t>::min = 0;
const uint64_t Foam::pTraits<uint64_t>::max = UINT64_MAX;
const uint64_t Foam::pTraits<uint64_t>::rootMin = 0;
const uint64_t Foam::pTraits<uint64_t>::rootMax = UINT64_MAX;


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pTraits<uint64_t>::pTraits(Istream& is)
{
    is >> p_;
}


// ************************************************************************* //
