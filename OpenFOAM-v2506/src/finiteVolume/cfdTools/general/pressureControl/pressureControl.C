/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017 OpenFOAM Foundation
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

#include "pressureControl.H"
#include "findRefCell.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pressureControl::pressureControl
(
    const volScalarField& p,
    const volScalarField& rho,
    const dictionary& dict,
    const bool pRefRequired
)
:
    refCell_(-1),
    refValue_(0),
    pMax_("pMax", dimPressure, GREAT),
    pMin_("pMin", dimPressure, Zero),
    limitMaxP_(false),
    limitMinP_(false)
{
    bool pLimits = false;
    scalar pMax = -GREAT;
    scalar pMin = GREAT;

    // Set the reference cell and value for closed domain simulations
    if (pRefRequired && setRefCell(p, dict, refCell_, refValue_))
    {
        pLimits = true;

        pMax = refValue_;
        pMin = refValue_;
    }

    if (dict.found("pMax") && dict.found("pMin"))
    {
        dict.readEntry("pMax", pMax_.value()); limitMaxP_ = true;
        dict.readEntry("pMin", pMin_.value()); limitMinP_ = true;
    }
    else
    {
        const volScalarField::Boundary& pbf = p.boundaryField();
        const volScalarField::Boundary& rhobf = rho.boundaryField();

        scalar rhoRefMax = -GREAT;
        scalar rhoRefMin = GREAT;
        bool rhoLimits = false;

        forAll(pbf, patchi)
        {
            if (pbf[patchi].fixesValue())
            {
                pLimits = true;
                rhoLimits = true;

                pMax = max(pMax, max(pbf[patchi]));
                pMin = min(pMin, min(pbf[patchi]));

                rhoRefMax = max(rhoRefMax, max(rhobf[patchi]));
                rhoRefMin = min(rhoRefMin, min(rhobf[patchi]));
            }
        }

        UPstream::reduceAnd(rhoLimits);
        if (rhoLimits)
        {
            reduce(pMax, maxOp<scalar>());
            reduce(pMin, minOp<scalar>());

            reduce(rhoRefMax, maxOp<scalar>());
            reduce(rhoRefMin, minOp<scalar>());
        }

        if (dict.readIfPresent("pMax", pMax_.value()))
        {
            limitMaxP_ = true;
        }
        else if (dict.found("pMaxFactor"))
        {
            if (!pLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'pMaxFactor' specified rather than 'pMax'" << nl
                    << "    but the corresponding reference pressure cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMax' rather than 'pMaxFactor'"
                    << exit(FatalIOError);
            }

            pMax_.value() = pMax * dict.get<scalar>("pMaxFactor");
            limitMaxP_ = true;
        }
        else if (dict.found("rhoMax"))
        {
            // For backward-compatibility infer the pMax from rhoMax

            IOWarningInFunction(dict)
                 << "'rhoMax' specified rather than 'pMax' or 'pMaxFactor'"
                 << nl
                 << "    This is supported for backward-compatibility but"
                    " 'pMax' or 'pMaxFactor' are more reliable." << endl;

            if (!pLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'rhoMax' specified rather than 'pMax'" << nl
                    << "    but the corresponding reference pressure cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMax' rather than 'rhoMax'"
                    << exit(FatalIOError);
            }

            if (!rhoLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'rhoMax' specified rather than 'pMaxFactor'" << nl
                    << "    but the corresponding reference density cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMaxFactor' rather than 'rhoMax'"
                    << exit(FatalIOError);
            }

            dimensionedScalar rhoMax("rhoMax", dimDensity, dict);

            pMax_.value() = max(rhoMax.value()/rhoRefMax, 1)*pMax;
            limitMaxP_ = true;
        }

        if (dict.readIfPresent("pMin", pMin_.value()))
        {
            limitMinP_ = true;
        }
        else if (dict.found("pMinFactor"))
        {
            if (!pLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'pMinFactor' specified rather than 'pMin'" << nl
                    << "    but the corresponding reference pressure cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMin' rather than 'pMinFactor'"
                    << exit(FatalIOError);
            }

            pMin_.value() = pMin * dict.get<scalar>("pMinFactor");
            limitMinP_ = true;
        }
        else if (dict.found("rhoMin"))
        {
            // For backward-compatibility infer the pMin from rhoMin

            IOWarningInFunction(dict)
                << "'rhoMin' specified rather than 'pMin' or 'pMinFactor'" << nl
                << "    This is supported for backward-compatibility but"
                   " 'pMin' or 'pMinFactor' are more reliable." << endl;

            if (!pLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'rhoMin' specified rather than 'pMin'" << nl
                    << "    but the corresponding reference pressure cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMin' rather than 'rhoMin'"
                    << exit(FatalIOError);
            }

            if (!rhoLimits)
            {
                FatalIOErrorInFunction(dict)
                    << "'rhoMin' specified rather than 'pMinFactor'" << nl
                    << "    but the corresponding reference density cannot"
                       " be evaluated from the boundary conditions." << nl
                    << "    Please specify 'pMinFactor' rather than 'rhoMin'"
                    << exit(FatalIOError);
            }

            dimensionedScalar rhoMin("rhoMin", dimDensity, dict);

            pMin_.value() = min(rhoMin.value()/rhoRefMin, 1)*pMin;
            limitMinP_ = true;
        }
    }

    if (limitMaxP_ || limitMinP_)
    {
        Info<< "pressureControl" << nl;

        if (limitMaxP_)
        {
            Info<< "    pMax " << pMax_.value() << nl;
        }

        if (limitMinP_)
        {
            Info<< "    pMin " << pMin_.value() << nl;
        }

        Info << endl;
    }
}


// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //

bool Foam::pressureControl::limit(volScalarField& p) const
{
    if (limitMaxP_ || limitMinP_)
    {
        if (limitMaxP_)
        {
            const scalar pMax = max(p).value();

            if (pMax > pMax_.value())
            {
                Info<< "pressureControl: p max " << pMax << endl;
                p = min(p, pMax_);
            }
        }

        if (limitMinP_)
        {
            const scalar pMin = min(p).value();

            if (pMin < pMin_.value())
            {
                Info<< "pressureControl: p min " << pMin << endl;
                p = max(p, pMin_);
            }
        }

        return true;
    }

    return false;
}


// ************************************************************************* //
