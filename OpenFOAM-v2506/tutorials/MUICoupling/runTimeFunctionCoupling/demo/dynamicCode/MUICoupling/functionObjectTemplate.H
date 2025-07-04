/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019 OpenCFD Ltd.
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

Description
    Template for use with dynamic code generation of a functionObject.

SourceFiles
    functionObjectTemplate.C

\*---------------------------------------------------------------------------*/

#ifndef coded_functionObjectTemplate_H
#define coded_functionObjectTemplate_H

#include "regionFunctionObject.H"
#include "dictionaryContent.H"

//{{{ begin codeInclude
#line 66 "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/OpenFOAM/OpenFOAM-v2506/tutorials/MUICoupling/runTimeFunctionCoupling/demo/system/controlDict/functions/MUICoupling"
#include        "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/MUI/src/mui.h"
            #include        "../../system/muiconfig.h"
//}}} end codeInclude

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class fvMesh;

/*---------------------------------------------------------------------------*\
                         A templated functionObject
\*---------------------------------------------------------------------------*/

class MUICouplingFunctionObject
:
    public functionObjects::regionFunctionObject,
    public dictionaryContent
{
    // Private Data

//{{{ begin codeData
        #line 75 "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/OpenFOAM/OpenFOAM-v2506/tutorials/MUICoupling/runTimeFunctionCoupling/demo/system/controlDict/functions/MUICoupling"
int iterCount = 0 ; 
            std::vector<std::unique_ptr<mui::uniface<mui::mui_config>>> mui_ifs;
            mui::point3d push_point;
            mui::point3d fetch_point;
            mui::sampler_exact<mui::mui_config> spatial_sampler;
            mui::temporal_sampler_exact<mui::mui_config> temporal_sampler;
//}}} end codeData


    // Private Member Functions

        //- Report a message with the SHA1sum
        inline static void printMessage(const char* message)
        {
            Info<< message << " sha1: " << SHA1sum << '\n';
        }

        //- Cast reference of objectRegistry to fvMesh
        const fvMesh& mesh() const;

        //- No copy construct
        MUICouplingFunctionObject
        (
            const MUICouplingFunctionObject&
        ) = delete;

        //- No copy assignment
        void operator=
        (
            const MUICouplingFunctionObject&
        ) = delete;

public:

    //- SHA1 representation of the code content
    static constexpr const char* const SHA1sum = "1040b3e12ad0b3e226a07818e1f0094983067c7f";

    //- Runtime type information
    TypeName("MUICoupling");


    // Constructors

        //- Construct from Time and dictionary
        MUICouplingFunctionObject
        (
            const word& name,
            const Time& runTime,
            const dictionary& dict
        );


    //- Destructor
    virtual ~MUICouplingFunctionObject();


    // Member Functions

        //- Code context as a dictionary
        const dictionary& codeContext() const noexcept
        {
            return dictionaryContent::dict();
        }

        //- Read optional controls
        virtual bool read(const dictionary& dict);

        //- Execute (at time-step)
        virtual bool execute();

        //- Write (at write interval)
        virtual bool write();

        //- Executed at the final time-loop
        virtual bool end();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

