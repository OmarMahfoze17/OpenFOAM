/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
    Copyright (C) 2015-2025 OpenCFD Ltd.
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

#include "DESModelRegions.H"
#include "volFields.H"
#include "DESModelBase.H"
#include "turbulenceModel.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(DESModelRegions, 0);
    addToRunTimeSelectionTable(functionObject, DESModelRegions, dictionary);
}
}


// * * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * //

void Foam::functionObjects::DESModelRegions::writeFileHeader(Ostream& os) const
{
    writeHeader(os, "DES model region coverage (% volume)");

    writeCommented(os, "Time");
    writeTabbed(os, "LES");
    writeTabbed(os, "RAS");
    os  << endl;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::DESModelRegions::DESModelRegions
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    writeFile(obr_, name, typeName, dict),
    resultName_(scopedName("regions"))
{
    read(dict);

    auto tmodelRegions = tmp<volScalarField>::New
    (
        IOobject
        (
            resultName_,
            time_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            IOobject::REGISTER
        ),
        mesh_,
        dimensionedScalar(dimless, Zero)
    );

    store(resultName_, tmodelRegions);

    writeFileHeader(file());
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::DESModelRegions::read(const dictionary& dict)
{
    fvMeshFunctionObject::read(dict);
    writeFile::read(dict);

    dict.readIfPresent("result", resultName_);

    return true;
}


bool Foam::functionObjects::DESModelRegions::execute()
{
    Log << type() << " " << name() <<  " execute:" << nl;

    auto& regions = lookupObjectRef<volScalarField>(resultName_);

    if
    (
        const auto* modelp
      = cfindObject<DESModelBase>(turbulenceModel::propertiesName)
    )
    {
        regions == (*modelp).LESRegion();

        const scalar prc =
            gWeightedAverage(mesh_.V(), regions.primitiveField())*100.0;

        file() << time_.value()
            << token::TAB << prc
            << token::TAB << 100.0 - prc
            << endl;

        Log << "    LES = " << prc << " % (volume)" << nl
            << "    RAS = " << 100.0 - prc << " % (volume)" << nl
            << endl;
    }
    else
    {
        Log << "    No DES turbulence model found in database" << nl
            << endl;
    }

    return true;
}


bool Foam::functionObjects::DESModelRegions::write()
{
    const auto& regions = lookupObject<volScalarField>(resultName_);

    Log << type() << " " << name() <<  " output:" << nl
        << "    writing field " << regions.name() << nl
        << endl;

    regions.write();

    return true;
}


// ************************************************************************* //
