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

\*---------------------------------------------------------------------------*/

#include "SphericalTensor.H"
#include "SymmTensor.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Cmpt>
inline Foam::DiagTensor<Cmpt>::DiagTensor(Foam::zero)
:
    VectorSpace<DiagTensor<Cmpt>, Cmpt, 3>(Foam::zero{})
{}


template<class Cmpt>
template<class Cmpt2>
inline Foam::DiagTensor<Cmpt>::DiagTensor
(
    const VectorSpace<DiagTensor<Cmpt2>, Cmpt2, 3>& vs
)
:
    VectorSpace<DiagTensor<Cmpt>, Cmpt, 3>(vs)
{}


template<class Cmpt>
inline Foam::DiagTensor<Cmpt>::DiagTensor(const SphericalTensor<Cmpt>& st)
{
    this->v_[XX] = st.ii();
    this->v_[YY] = st.ii();
    this->v_[ZZ] = st.ii();
}


template<class Cmpt>
inline Foam::DiagTensor<Cmpt>::DiagTensor
(
    const Cmpt& vxx,
    const Cmpt& vyy,
    const Cmpt& vzz
)
{
    this->v_[XX] = vxx;
    this->v_[YY] = vyy;
    this->v_[ZZ] = vzz;
}


template<class Cmpt>
inline Foam::DiagTensor<Cmpt>::DiagTensor(Istream& is)
:
    VectorSpace<DiagTensor<Cmpt>, Cmpt, 3>(is)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Cmpt>
inline Foam::Vector<Cmpt> Foam::DiagTensor<Cmpt>::diag() const
{
    return Vector<Cmpt>(this->xx(), this->yy(), this->zz());
}


template<class Cmpt>
inline Foam::scalar Foam::DiagTensor<Cmpt>::diagSqr() const
{
    return
    (
        Foam::magSqr(this->xx())
      + Foam::magSqr(this->yy())
      + Foam::magSqr(this->zz())
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

//- Return the trace of a DiagTensor
template<class Cmpt>
inline Cmpt tr(const DiagTensor<Cmpt>& dt)
{
    return dt.xx() + dt.yy() + dt.zz();
}


//- Return the spherical part of a DiagTensor as a SphericalTensor
template<class Cmpt>
inline SphericalTensor<Cmpt> sph(const DiagTensor<Cmpt>& dt)
{
    return SphericalTensor<Cmpt>
    (
        (1.0/3.0)*tr(dt)
    );
}


//- Return the determinant of a DiagTensor
template<class Cmpt>
inline Cmpt det(const DiagTensor<Cmpt>& dt)
{
    return dt.xx()*dt.yy()*dt.zz();
}


//- Return the inverse of a DiagTensor as a DiagTensor
template<class Cmpt>
inline DiagTensor<Cmpt> inv(const DiagTensor<Cmpt>& dt)
{
    #ifdef FULLDEBUG
    if (mag(det(dt)) < VSMALL)
    {
        FatalErrorInFunction
            << "DiagTensor is not invertible due to the zero determinant:"
            << "det(DiagTensor) = " << det(dt)
            << abort(FatalError);
    }
    #endif

    return DiagTensor<Cmpt>(1/dt.xx(), 1/dt.yy(), 1/dt.zz());
}


//- Return the diagonal of a Tensor as a DiagTensor
template<class Cmpt>
inline DiagTensor<Cmpt> diag(const Tensor<Cmpt>& t)
{
    return DiagTensor<Cmpt>(t.xx(), t.yy(), t.zz());
}


//- Return the diagonal of a SymmTensor as a DiagTensor
template<class Cmpt>
inline DiagTensor<Cmpt> diag(const SymmTensor<Cmpt>& st)
{
    return DiagTensor<Cmpt>(st.xx(), st.yy(), st.zz());
}


template<class Cmpt>
inline Foam::scalar magSqr(const DiagTensor<Cmpt>& t)
{
    return t.diagSqr();
}


//- Linear interpolation of diagonal tensors a and b by factor t
template<class Cmpt>
inline DiagTensor<Cmpt> lerp
(
    const DiagTensor<Cmpt>& a,
    const DiagTensor<Cmpt>& b,
    const scalar t
)
{
    const scalar onet = (1-t);

    return DiagTensor<Cmpt>
    (
        onet*a.xx() + t*b.xx(),
        onet*a.yy() + t*b.yy(),
        onet*a.zz() + t*b.zz()
    );
}


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

//- Sum of a DiagTensor and a Tensor
template<class Cmpt>
inline Tensor<Cmpt>
operator+(const DiagTensor<Cmpt>& dt1, const Tensor<Cmpt>& t2)
{
    return Tensor<Cmpt>
    (
        dt1.xx() + t2.xx(), t2.xy(),            t2.xz(),
        t2.yx(),            dt1.yy() + t2.yy(), t2.yz(),
        t2.zx(),            t2.zy(),            dt1.zz() + t2.zz()
    );
}


//- Sum of a Tensor and a DiagTensor
template<class Cmpt>
inline Tensor<Cmpt>
operator+(const Tensor<Cmpt>& t1, const DiagTensor<Cmpt>& dt2)
{
    return Tensor<Cmpt>
    (
        t1.xx() + dt2.xx(), t1.xy(),            t1.xz(),
        t1.yx(),            t1.yy() + dt2.yy(), t1.yz(),
        t1.zx(),            t1.zy(),            t1.zz() + dt2.zz()
    );
}


//- Subtract a Tensor from a DiagTensor
template<class Cmpt>
inline Tensor<Cmpt>
operator-(const DiagTensor<Cmpt>& dt1, const Tensor<Cmpt>& t2)
{
    return Tensor<Cmpt>
    (
        dt1.xx() - t2.xx(), -t2.xy(),            -t2.xz(),
       -t2.yx(),             dt1.yy() - t2.yy(), -t2.yz(),
       -t2.zx(),            -t2.zy(),             dt1.zz() - t2.zz()
    );
}


//- Subtract a DiagTensor from a Tensor
template<class Cmpt>
inline Tensor<Cmpt>
operator-(const Tensor<Cmpt>& t1, const DiagTensor<Cmpt>& dt2)
{
    return Tensor<Cmpt>
    (
        t1.xx() - dt2.xx(), t1.xy(),            t1.xz(),
        t1.yx(),            t1.yy() - dt2.yy(), t1.yz(),
        t1.zx(),            t1.zy(),            t1.zz() - dt2.zz()
    );
}


//- Division of a Cmpt by a DiagTensor
template<class Cmpt>
inline DiagTensor<Cmpt>
operator/(const Cmpt s, const DiagTensor<Cmpt>& dt)
{
    #ifdef FULLDEBUG
    if (mag(det(dt)) < VSMALL)
    {
        FatalErrorInFunction
            << "Cmpt = " << s
            << " is not divisible by the DiagTensor due to a zero element:"
            << "DiagTensor = " << dt
            << abort(FatalError);
    }
    #endif

    return DiagTensor<Cmpt>(s/dt.xx(), s/dt.yy(), s/dt.zz());
}


//- Division of a DiagTensor by a Cmpt
template<class Cmpt>
inline DiagTensor<Cmpt>
operator/(const DiagTensor<Cmpt>& dt, const Cmpt s)
{
    #ifdef FULLDEBUG
    if (mag(s) < VSMALL)
    {
        FatalErrorInFunction
            << "DiagTensor = " << dt
            << " is not divisible due to a zero value in Cmpt:"
            << "Cmpt = " << s
            << abort(FatalError);
    }
    #endif

    return DiagTensor<Cmpt>(dt.xx()/s, dt.yy()/s, dt.zz()/s);
}


//- Division of a Vector by a DiagTensor
template<class Cmpt>
inline Vector<Cmpt>
operator/(const Vector<Cmpt> v, const DiagTensor<Cmpt>& dt)
{
    #ifdef FULLDEBUG
    if (mag(det(dt)) < VSMALL)
    {
        FatalErrorInFunction
            << "Vector = " << v
            << " is not divisible by the DiagTensor due to a zero element:"
            << "DiagTensor = " << dt
            << abort(FatalError);
    }
    #endif

    return Vector<Cmpt>(v.x()/dt.xx(), v.y()/dt.yy(), v.z()/dt.zz());
}


//- Inner-product of a DiagTensor and a DiagTensor
template<class Cmpt>
inline DiagTensor<Cmpt>
operator&(const DiagTensor<Cmpt>& dt1, const DiagTensor<Cmpt>& dt2)
{
    return DiagTensor<Cmpt>
    (
        dt1.xx()*dt2.xx(),
        dt1.yy()*dt2.yy(),
        dt1.zz()*dt2.zz()
    );
}


//- Inner-product of a DiagTensor and a Tensor
template<class Cmpt>
inline Tensor<Cmpt>
operator&(const DiagTensor<Cmpt>& dt1, const Tensor<Cmpt>& t2)
{
    return Tensor<Cmpt>
    (
        dt1.xx()*t2.xx(),
        dt1.xx()*t2.xy(),
        dt1.xx()*t2.xz(),

                          dt1.yy()*t2.yx(),
                          dt1.yy()*t2.yy(),
                          dt1.yy()*t2.yz(),

                                            dt1.zz()*t2.zx(),
                                            dt1.zz()*t2.zy(),
                                            dt1.zz()*t2.zz()
    );
}


//- Inner-product of a Tensor and a DiagTensor
template<class Cmpt>
inline Tensor<Cmpt>
operator&(const Tensor<Cmpt>& t1, const DiagTensor<Cmpt>& dt2)
{
    return Tensor<Cmpt>
    (
        t1.xx()*dt2.xx(),
                          t1.xy()*dt2.yy(),
                                            t1.xz()*dt2.zz(),

        t1.yx()*dt2.xx(),
                          t1.yy()*dt2.yy(),
                                            t1.yz()*dt2.zz(),

        t1.zx()*dt2.xx(),
                          t1.zy()*dt2.yy(),
                                            t1.zz()*dt2.zz()
    );
}


//- Inner-product of a DiagTensor and a Vector
template<class Cmpt>
inline Vector<Cmpt>
operator&(const DiagTensor<Cmpt>& dt, const Vector<Cmpt>& v)
{
    return Vector<Cmpt>
    (
        dt.xx()*v.x(),
                       dt.yy()*v.y(),
                                      dt.zz()*v.z()
    );
}


//- Inner-product of a Vector and a DiagTensor
template<class Cmpt>
inline Vector<Cmpt>
operator&(const Vector<Cmpt>& v, const DiagTensor<Cmpt>& dt)
{
    return Vector<Cmpt>
    (
        v.x()*dt.xx(),
                       v.y()*dt.yy(),
                                      v.z()*dt.zz()
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
