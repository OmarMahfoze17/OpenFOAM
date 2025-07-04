/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2020-2025 OpenCFD Ltd.
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

InClass
    Foam::dummyTransform

Description
    Dummy transform to be used with syncTools.

\*---------------------------------------------------------------------------*/

#ifndef Foam_dummyTransform_H
#define Foam_dummyTransform_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class dummyTransform Declaration
\*---------------------------------------------------------------------------*/

class dummyTransform
{
public:

    template<class T>
    void operator()
    (
        const vectorTensorTransform& vt,
        const bool forward,
        UList<T>& fld
    ) const
    {}

    template<class Type>
    void operator()
    (
        const vectorTensorTransform& vt,
        const bool forward,
        List<List<Type>>& flds
    ) const
    {}

    template<class T>
    void operator()(const coupledPolyPatch& cpp, UList<T>& fld) const
    {}

    template<class T, template<class> class Container>
    void operator()(const coupledPolyPatch& cpp, Container<T>& map) const
    {}
};


template<class T>
class pTraits<List<T>>
:
    public List<T>
{
public:
    typedef label cmptType;

    explicit pTraits(Istream& is)
    :
        List<T>(is)
    {}
};

template<class T>
class pTraits<UList<T>>
:
    public UList<T>
{
public:
    typedef label cmptType;

    explicit pTraits(Istream& is)
    :
        UList<T>(is)
    {}
};

template<class T>
class pTraits<Field<T>>
:
    public Field<T>
{
public:
    typedef label cmptType;

    explicit pTraits(Istream& is)
    :
        Field<T>(is)
    {}
};

template<>
class pTraits<face>
:
    public face
{
public:
    typedef label cmptType;

    explicit pTraits(Istream& is)
    :
        face(is)
    {}
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
