/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2021 OpenCFD Ltd.
    Copyright (C) YEAR AUTHOR, AFFILIATION
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

#include "functionObjectTemplate.H"
#define namespaceFoam  // Suppress <using namespace Foam;>
#include "fvCFD.H"
#include "unitConversion.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(MUICouplingFunctionObject, 0);

addRemovableToRunTimeSelectionTable
(
    functionObject,
    MUICouplingFunctionObject,
    dictionary
);


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

// dynamicCode:
// SHA1 = 1040b3e12ad0b3e226a07818e1f0094983067c7f
//
// unique function name that can be checked if the correct library version
// has been loaded
extern "C" void MUICoupling_1040b3e12ad0b3e226a07818e1f0094983067c7f(bool load)
{
    if (load)
    {
        // Code that can be explicitly executed after loading
    }
    else
    {
        // Code that can be explicitly executed before unloading
    }
}


// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

//{{{ begin localCode

//}}} end localCode

} // End namespace Foam


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

const Foam::fvMesh&
Foam::MUICouplingFunctionObject::mesh() const
{
    return refCast<const fvMesh>(obr_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::
MUICouplingFunctionObject::
MUICouplingFunctionObject
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    functionObjects::regionFunctionObject(name, runTime, dict)
{
    read(dict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::
MUICouplingFunctionObject::
~MUICouplingFunctionObject()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool
Foam::
MUICouplingFunctionObject::read(const dictionary& dict)
{
    if (false)
    {
        printMessage("read MUICoupling");
    }

//{{{ begin code
    
//}}} end code

    return true;
}


bool
Foam::
MUICouplingFunctionObject::execute()
{
    if (false)
    {
        printMessage("execute MUICoupling");
    }

//{{{ begin code
    #line 88 "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/OpenFOAM/OpenFOAM-v2506/tutorials/MUICoupling/runTimeFunctionCoupling/demo/system/controlDict/functions/MUICoupling"
double pushData = 100.0;
	    double fetchData = 100.0;
	    if (iterCount == 0) 
	    {
	        std::vector<std::string> ifsName;
	        ifsName.emplace_back("ifs");
	        Info <<  "OF creating MUI interface " << endl;
	        mui_ifs=mui::create_uniface<mui::mui_config>( "OpenFOAM", ifsName );	    
	        Info <<  "OF Finsihed  MUI interface " << endl;
	    }	
	    

            // Fetch the value from the interface (blocking until data at "t" exists according to temporal_sampler)
            fetch_point[0] = 0;fetch_point[1] = 0;fetch_point[2] = 0;
            fetchData = mui_ifs[0]->fetch( "dataFromPing", fetch_point, iterCount, spatial_sampler, temporal_sampler );
            pushData--;
            // Push value stored in "state" to the MUI interface
            push_point[0] = 0;push_point[1] = 0;push_point[2] = 0;
            mui_ifs[0]->push( "dataFromOF", push_point, pushData );
            // Commit (transmit by MPI) the value
            mui_ifs[0]->commit( iterCount );	
	
	    Info << "zzzzzzzzzzzzzz OpenFoam at time "<< iterCount << " fitched "<< fetchData << endl;
            Info<<"OpenFoam iterCount " << iterCount << endl;
            iterCount = iterCount+1;
//}}} end code

    return true;
}


bool
Foam::
MUICouplingFunctionObject::write()
{
    if (false)
    {
        printMessage("write MUICoupling");
    }

//{{{ begin code
    
//}}} end code

    return true;
}


bool
Foam::
MUICouplingFunctionObject::end()
{
    if (false)
    {
        printMessage("end MUICoupling");
    }

//{{{ begin code
    
//}}} end code

    return true;
}


// ************************************************************************* //

