/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

Application
    equilibriumCO

Group
    grpThermophysicalUtilities

Description
    Calculate the equilibrium level of carbon monoxide.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "Time.H"
#include "dictionary.H"
#include "IFstream.H"
#include "OSspecific.H"
#include "IOmanip.H"

#include "specie.H"
#include "perfectGas.H"
#include "thermo.H"
#include "janafThermo.H"
#include "absoluteEnthalpy.H"

#include "PtrDynList.H"
#include "IOdictionary.H"

using namespace Foam;

typedef species::thermo<janafThermo<perfectGas<specie>>, absoluteEnthalpy>
    thermo;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Calculate the equilibrium level of carbon monoxide."
    );

    argList::noParallel();
    argList::noFunctionObjects();  // Never use function objects

    #include "setRootCase.H"
    #include "createTime.H"

    Info<< nl << "Reading thermodynamic data IOdictionary" << endl;

    IOdictionary thermoData
    (
        IOobject
        (
            "thermoData",
            runTime.constant(),
            runTime,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        )
    );


    const scalar P = 1e5;
    const scalar T = 3000.0;

    // Oxidant (mole-based)
    thermo O2(thermoData.subDict("O2")); O2 *= O2.W();
    thermo N2(thermoData.subDict("N2")); N2 *= N2.W();

    // Intermediates (mole-based)
    thermo H2(thermoData.subDict("H2")); H2 *= H2.W();
    thermo OH(thermoData.subDict("OH")); OH *= OH.W();
    thermo H(thermoData.subDict("H")); H *= H.W();
    thermo O(thermoData.subDict("O")); O *= O.W();

    // Products (mole-based)
    thermo CO2(thermoData.subDict("CO2")); CO2 *= CO2.W();
    thermo H2O(thermoData.subDict("H2O")); H2O *= H2O.W();
    thermo CO(thermoData.subDict("CO")); CO *= CO.W();

    PtrDynList<thermo> EQreactions(4);

    EQreactions.emplace_back((CO2 == CO + 0.5*O2));
    EQreactions.emplace_back((O2 == 2*O));
    EQreactions.emplace_back((H2O == H2 + 0.5*O2));
    EQreactions.emplace_back((H2O == H + OH));


    for (const thermo& react : EQreactions)
    {
        Info<< "Kc(EQreactions) = " << react.Kc(P, T) << endl;
    }

    Info<< nl << "End" << nl << endl;

    return 0;
}


// ************************************************************************* //
