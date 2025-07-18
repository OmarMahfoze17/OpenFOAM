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

\*---------------------------------------------------------------------------*/

#include "error.H"
#include "products.H"
#include "VectorSpaceOps.H"
#include "ops.H"
#include <type_traits>

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Form, class Cmpt, Foam::direction Ncmpts>
inline Foam::VectorSpace<Form, Cmpt, Ncmpts>::VectorSpace(Foam::zero)
{
    VectorSpaceOps<Ncmpts>::fill_n(this->begin(), Cmpt(Foam::zero{}));
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline Foam::VectorSpace<Form, Cmpt, Ncmpts>::VectorSpace
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    VectorSpaceOps<Ncmpts>::copy_n(vs.cbegin(), this->begin());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
template<class Form2, class Cmpt2>
inline Foam::VectorSpace<Form, Cmpt, Ncmpts>::VectorSpace
(
    const VectorSpace<Form2, Cmpt2, Ncmpts>& vs
)
{
    VectorSpaceOps<Ncmpts>::copy_n(vs.cbegin(), this->begin());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
template<class SubVector, Foam::direction BStart>
inline Foam::VectorSpace<Form, Cmpt, Ncmpts>::ConstBlock<SubVector, BStart>
    ::ConstBlock
(
    const vsType& vs
)
:
    vs_(vs)
{
    static_assert
    (
        vsType::nComponents >= BStart + nComponents,
        "Requested block size > VectorSpace size"
    );
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Form, class Cmpt, Foam::direction Ncmpts>
inline const Cmpt& Foam::VectorSpace<Form, Cmpt, Ncmpts>::component
(
    const direction d
) const
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    return v_[d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline Cmpt& Foam::VectorSpace<Form, Cmpt, Ncmpts>::component
(
    const direction d
)
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    return v_[d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::component
(
    Cmpt& c,
    const direction d
) const
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    c = v_[d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::replace
(
    const direction d,
    const Cmpt& c
)
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    v_[d] = c;
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::fill(const Cmpt& s)
{
    VectorSpaceOps<Ncmpts>::fill_n(this->begin(), s);
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline Form Foam::VectorSpace<Form, Cmpt, Ncmpts>::uniform(const Cmpt& s)
{
    Form v;
    v.fill(s);
    return v;
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
template<class SubVector, Foam::direction BStart>
inline const typename Foam::VectorSpace<Form, Cmpt, Ncmpts>::template
    ConstBlock<SubVector, BStart>
Foam::VectorSpace<Form, Cmpt, Ncmpts>::block() const
{
    return *this;
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

template<class Form, class Cmpt, Foam::direction Ncmpts>
inline const Cmpt& Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator[]
(
    const direction d
) const
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    return v_[d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline Cmpt& Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator[]
(
    const direction d
)
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    return v_[d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
template<class SubVector, Foam::direction BStart>
inline const Cmpt&
Foam::VectorSpace<Form, Cmpt, Ncmpts>::
ConstBlock<SubVector, BStart>::operator[]
(
    const direction d
) const
{
    #ifdef FULLDEBUG
    if (d >= Ncmpts)
    {
        FatalErrorInFunction
            << "index out of range"
            << abort(FatalError);
    }
    #endif

    return vs_[BStart + d];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
template<class SubVector, Foam::direction BStart>
inline const Cmpt&
Foam::VectorSpace<Form, Cmpt, Ncmpts>::
ConstBlock<SubVector, BStart>::operator()
(
    const direction i,
    const direction j
) const
{
    #ifdef FULLDEBUG
    if (i >= Ncmpts)
    {
        FatalErrorInFunction
            << "index " << i << " out of range"
            << abort(FatalError);
    }

    if (j)
    {
        FatalErrorInFunction
            << "index " << j << " != 0"
            << abort(FatalError);
    }
    #endif

    return vs_[BStart + i];
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    VectorSpaceOps<Ncmpts>::copy_n(vs.cbegin(), this->begin());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator+=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    VectorSpaceOps<Ncmpts,0>::eqOp(*this, vs, plusEqOp<Cmpt>());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator-=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    VectorSpaceOps<Ncmpts,0>::eqOp(*this, vs, minusEqOp<Cmpt>());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator=(Foam::zero)
{
    VectorSpaceOps<Ncmpts>::fill_n(this->begin(), Cmpt(Foam::zero{}));
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator*=
(
    const scalar s
)
{
    VectorSpaceOps<Ncmpts,0>::eqOpS(*this, s, multiplyEqOp2<Cmpt, scalar>());
}


template<class Form, class Cmpt, Foam::direction Ncmpts>
inline void Foam::VectorSpace<Form, Cmpt, Ncmpts>::operator/=
(
    const scalar s
)
{
    VectorSpaceOps<Ncmpts,0>::eqOpS(*this, s, divideEqOp2<Cmpt, scalar>());
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt& setComponent
(
    VectorSpace<Form, Cmpt, Ncmpts>& vs,
    const direction d
)
{
    return vs.component(d);
}


template<class Form, class Cmpt, direction Ncmpts>
inline const Cmpt& component
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs,
    const direction d
)
{
    return vs.component(d);
}


// Powers of a Form
// Equivalent to outer-products between the Form and itself
// Form^0 = 1.0
template<class Form, class Cmpt, direction Ncmpts>
inline typename powProduct<Form, 0>::type pow
(
    const VectorSpace<Form, Cmpt, Ncmpts>&,
    typename powProduct<Form, 0>::type
    = pTraits<typename powProduct<Form, 0>::type>::zero
)
{
    return 1.0;
}


// Form^1 = Form
template<class Form, class Cmpt, direction Ncmpts>
inline typename powProduct<Form, 1>::type pow
(
    const VectorSpace<Form, Cmpt, Ncmpts>& v,
    typename powProduct<Form, 1>::type
  = pTraits<typename powProduct<Form, 1>::type>::zero
)
{
    return static_cast<const Form&>(v);
}


// Form^2 = sqr(Form)
template<class Form, class Cmpt, direction Ncmpts>
inline typename powProduct<Form, 2>::type pow
(
    const VectorSpace<Form, Cmpt, Ncmpts>& v,
    typename powProduct<Form, 2>::type
  = pTraits<typename powProduct<Form, 2>::type>::zero
)
{
    return sqr(static_cast<const Form&>(v));
}


template<class Form, class Cmpt, direction Ncmpts>
inline scalar magSqr
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    scalar ms = magSqr(vs.v_[0]);
    VectorSpaceOps<Ncmpts,1>::SeqOp(ms, vs, plusEqMagSqrOp2<scalar, Cmpt>());
    return ms;
}


template<class Form, class Cmpt, direction Ncmpts>
inline scalar mag
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    return ::sqrt(magSqr(static_cast<const Form&>(vs)));
}


//- Return the vector type normalised by its magnitude
//  For small magnitudes (less than ROOTVSMALL) return zero.
template<class Form, class Cmpt, direction Ncmpts>
inline VectorSpace<Form, Cmpt, Ncmpts> normalised
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    #ifdef __clang__
    volatile  // Use volatile to avoid aggressive branch optimization
    #endif
    const scalar s(mag(vs));
    return s < ROOTVSMALL ? Zero : vs/s;
}


template<class Form, class Cmpt, direction Ncmpts>
inline VectorSpace<Form, Cmpt, Ncmpts> cmptMultiply
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, cmptMultiplyOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline VectorSpace<Form, Cmpt, Ncmpts> cmptPow
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, cmptPowOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline VectorSpace<Form, Cmpt, Ncmpts> cmptDivide
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, cmptDivideOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline VectorSpace<Form, Cmpt, Ncmpts> stabilise
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs,
    const Cmpt& small
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::opVS(v, vs, small, stabiliseOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt cmptMax
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Cmpt cMax = vs.v_[0];
    VectorSpaceOps<Ncmpts,1>::SeqOp(cMax, vs, maxEqOp<Cmpt>());
    return cMax;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt cmptMin
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Cmpt cMin = vs.v_[0];
    VectorSpaceOps<Ncmpts,1>::SeqOp(cMin, vs, minEqOp<Cmpt>());
    return cMin;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt cmptSum
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Cmpt sum = vs.v_[0];
    VectorSpaceOps<Ncmpts,1>::SeqOp(sum, vs, plusEqOp<Cmpt>());
    return sum;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt cmptAv
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    return cmptSum(vs)/Ncmpts;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt cmptProduct
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Cmpt product = vs.v_[0];
    VectorSpaceOps<Ncmpts,1>::SeqOp(product, vs, multiplyEqOp<Cmpt>());
    return product;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form cmptSqr
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::eqOp(v, vs, eqSqrOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form cmptMag
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::eqOp(v, vs, eqMagOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form cmptMagSqr
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::eqOp(v, vs, eqMagSqrOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form max
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, maxOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form min
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, minOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form minMod
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, minModOp<Cmpt>());
    return v;
}


template<class Type>
inline Type dot(const scalar s, const Type& t)
{
    return s * t;
}


template<class Type>
inline Type dot(const Type& t, const scalar s)
{
    return t * s;
}


template
<
    class Form1, class Cmpt1, direction Ncmpts1,
    class Form2, class Cmpt2, direction Ncmpts2
>
inline typename innerProduct<Form1, Form2>::type dot
(
    const VectorSpace<Form1, Cmpt1, Ncmpts1>& t1,
    const VectorSpace<Form2, Cmpt2, Ncmpts2>& t2
)
{
    return static_cast<const Form1&>(t1) & static_cast<const Form2&>(t2);
}


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

template<class Form, class Cmpt, direction Ncmpts>
inline Form operator-
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::eqOp(v, vs, eqMinusOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form operator+
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, plusOp<Cmpt>());
    return v;
}

template<class Form, class Cmpt, direction Ncmpts>
inline Form operator-
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, minusOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form operator*
(
    scalar s,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::opSV(v, s, vs, multiplyOp3<Cmpt, scalar, Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form operator*
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs,
    scalar s
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::opVS(v, vs, s, multiplyOp3<Cmpt, Cmpt, scalar>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form operator/
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs,
    scalar s
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::opVS(v, vs, s, divideOp3<Cmpt, Cmpt, scalar>());
    return v;
}

/*
template<class Form, class Cmpt, direction Ncmpts>
inline Form operator/
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::op(v, vs1, vs2, divideOp<Cmpt>());
    return v;
}


template<class Form, class Cmpt, direction Ncmpts>
inline Form operator/
(
    scalar s,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs
)
{
    Form v;
    VectorSpaceOps<Ncmpts,0>::opSV(v, s, vs, divideOp2<scalar, Cmpt>());
    return v;
}
*/


template<class Form, class Cmpt, direction Ncmpts>
inline Cmpt operator&&
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    Cmpt ddProd = vs1.v_[0]*vs2.v_[0];
    for (direction i=1; i<Ncmpts; ++i)
    {
        ddProd += vs1.v_[i]*vs2.v_[i];
    }
    return ddProd;
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator==
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    for (direction i=0; i<Ncmpts; ++i)
    {
        if (!equal(vs1.v_[i], vs2.v_[i])) return false;
    }
    return true;
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator!=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    return !(vs1 == vs2);
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator<
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    // Compare all components, stop at first non less-than component
    for (direction i=0; i<Ncmpts; ++i)
    {
        if (!(vs1.v_[i] < vs2.v_[i])) return false;
    }
    return true;
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator<=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    return !(vs2 < vs1);
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator>
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    return (vs2 < vs1);
}


template<class Form, class Cmpt, direction Ncmpts>
inline bool operator>=
(
    const VectorSpace<Form, Cmpt, Ncmpts>& vs1,
    const VectorSpace<Form, Cmpt, Ncmpts>& vs2
)
{
    return !(vs1 < vs2);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
