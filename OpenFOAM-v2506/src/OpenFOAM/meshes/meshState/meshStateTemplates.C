/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2016 OpenFOAM Foundation
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

#include "Time.H"
#include "solverPerformance.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void Foam::meshState::setSolverPerformance
(
    const word& name,
    const SolverPerformance<Type>& sp
) const
{
    dictionary& dict = const_cast<dictionary&>(solverPerformanceDict());

    Pair<SolverPerformance<Type>> perfs;

    const label timeIndex = this->time().timeIndex();

    if (prevTimeIndex_ != timeIndex)
    {
        // Reset solver performance between iterations
        prevTimeIndex_ = timeIndex;
        dict.clear();
    }
    else
    {
        dict.readIfPresent(name, perfs);
    }

    if (dict.found(name))
    {
        perfs.second() = sp;
    }
    else
    {
        perfs = Pair<SolverPerformance<Type>>(sp, sp);
    }

    dict.set(name, perfs);
}


template<class Type>
void Foam::meshState::setSolverPerformance
(
    const SolverPerformance<Type>& sp
) const
{
    setSolverPerformance(sp.fieldName(), sp);
}


// ************************************************************************* //
