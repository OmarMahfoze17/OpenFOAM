/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2025 OpenCFD Ltd.
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

Note
    Included by global/globals.C

\*---------------------------------------------------------------------------*/

#include "error.H"
#include "dictionary.H"
#include "foamVersion.H"
#include "UPstream.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

// Default is 2 : report source file name and line number if available
int Foam::messageStream::level(Foam::debug::infoSwitch("outputLevel", 2));

int Foam::messageStream::redirect(0);

// Default is 1 : report to Info
int Foam::infoDetailLevel(1);


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::messageStream::messageStream
(
    errorSeverity severity,
    int maxErrors,
    bool use_stderr
)
:
    severity_(severity),
    maxErrors_(maxErrors),
    errorCount_(0)
{
    if (use_stderr)
    {
        severity_ |= errorSeverity::USE_STDERR;
    }
}


Foam::messageStream::messageStream
(
    const char* title,
    errorSeverity severity,
    int maxErrors,
    bool use_stderr
)
:
    messageStream(severity, maxErrors, use_stderr)
{
    if (title)
    {
        title_ = title;
    }
}


Foam::messageStream::messageStream
(
    string title,
    errorSeverity severity,
    int maxErrors,
    bool use_stderr
)
:
    messageStream(severity, maxErrors, use_stderr)
{
    title_ = std::move(title);
}


Foam::messageStream::messageStream(const dictionary& dict)
:
    messageStream(dict.get<string>("title"), errorSeverity::FATAL)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::OSstream& Foam::messageStream::stream
(
    OSstream* alternative,
    int communicator
)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    if (level)
    {
        // Master-only output?
        const bool masterOnly
        (
            !UPstream::parRun()
         || ((severity_ & ~errorSeverity::USE_STDERR) == errorSeverity::INFO)
         || ((severity_ & ~errorSeverity::USE_STDERR) == errorSeverity::WARNING)
        );

        if
        (
            masterOnly
         && (UPstream::parRun() && !UPstream::master(communicator))
        )
        {
            // Requested master-only output but is non-master (in parallel)
            // -> early exit
            return Snull;
        }


        // Use stderr instead of stdout:
        // - requested via static <redirect> variable
        // - explicit:  with USE_STDERR mask
        // - inferred:  WARNING -> stderr when infoDetailLevel == 0
        const bool use_stderr =
        (
            (redirect == 2)
         || (severity_ & errorSeverity::USE_STDERR)
         || (severity_ == errorSeverity::WARNING && Foam::infoDetailLevel == 0)
        );


        OSstream* osptr;

        if (masterOnly)
        {
            // Use supplied alternative? Valid for master-only output
            osptr =
            (
                alternative
              ? alternative
              : (use_stderr ? &Serr : &Sout)
            );
        }
        else
        {
            osptr = (use_stderr ? &Perr : &Pout);
        }

        if (!title_.empty())
        {
            (*osptr) << title_.c_str();
        }

        if (maxErrors_ && (++errorCount_ >= maxErrors_))
        {
            FatalErrorInFunction
                << "Too many errors..."
                << abort(FatalError);
        }

        return *osptr;
    }

    return Snull;
}


Foam::OSstream& Foam::messageStream::masterStream(int communicator)
{
    if (communicator < 0)
    {
        communicator = UPstream::worldComm;
    }

    if (UPstream::warnComm >= 0 && communicator != UPstream::warnComm)
    {
        Perr<< "** messageStream with comm:" << communicator << endl;
        error::printStack(Perr);
    }

    if (communicator == UPstream::worldComm || UPstream::master(communicator))
    {
        return this->stream(nullptr, communicator);
    }

    return Snull;
}


std::ostream& Foam::messageStream::stdStream()
{
    // Currently do not need communicator != worldComm
    return this->stream().stdStream();
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

Foam::OSstream& Foam::messageStream::deprecated
(
    const int afterVersion,
    const char* functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber
)
{
    OSstream& os = this->stream();

    // No warning for 0 (unversioned) or -ve values (silent versioning).
    // Also no warning for (version >= foamVersion::api), which
    // can be used to denote future expiry dates of transition features.

    if (afterVersion > 0 && afterVersion < foamVersion::api)
    {
        const int months =
        (
            // YYMM -> months
            (12 * (foamVersion::api/100) + (foamVersion::api % 100))
          - (12 * (afterVersion/100)  + (afterVersion % 100))
        );

        os  << nl
            << ">>> DEPRECATED after version " << afterVersion;

        if (afterVersion < 1000)
        {
            // Predates YYMM versioning (eg, 240 for version 2.4)
            os  << ". This is very old! <<<" << nl;
        }
        else
        {
            os  << ". This is about " << months << " months old. <<<" << nl;
        }
    }


    os  << nl;
    if (functionName)  // nullptr check
    {
        {
            os  << "    From " << functionName << nl;
        }
        if (sourceFileName)  // nullptr check
        {
            os  << "    in file " << sourceFileName;

            if (sourceFileLineNumber >= 0)
            {
                os  << " at line " << sourceFileLineNumber;
            }
            os  << nl;
        }
    }
    os  << "    ";

    return os;
}


Foam::OSstream& Foam::messageStream::operator()
(
    const std::string& functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber
)
{
    OSstream& os = this->stream();

    if (!functionName.empty())
    {
        os  << nl
            << "    From " << functionName.c_str() << nl;
    }

    if (sourceFileName)  // nullptr check
    {
        os  << "    in file " << sourceFileName;

        if (sourceFileLineNumber >= 0)
        {
            os  << " at line " << sourceFileLineNumber;
        }
        os  << nl << "    ";
    }

    return os;
}


Foam::OSstream& Foam::messageStream::operator()
(
    const char* functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber
)
{
    OSstream& os = this->stream();

    if (functionName)  // nullptr check
    {
        os  << nl
            << "    From " << functionName << nl;
    }

    if (sourceFileName)  // nullptr check
    {
        os  << "    in file " << sourceFileName;

        if (sourceFileLineNumber >= 0)
        {
            os  << " at line " << sourceFileLineNumber;
        }
        os  << nl << "    ";
    }

    return os;
}


Foam::OSstream& Foam::messageStream::operator()
(
    const char* functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber,
    const std::string& ioFileName,
    const label ioStartLineNumber,
    const label ioEndLineNumber
)
{
    OSstream& os = operator()
    (
        functionName,
        sourceFileName,
        sourceFileLineNumber
    );

    os  << "Reading \"" << ioFileName.c_str() << '"';

    if (ioStartLineNumber >= 0)
    {
        os  << " at line " << ioStartLineNumber;

        if (ioStartLineNumber < ioEndLineNumber)
        {
            os  << " to " << ioEndLineNumber;
        }
    }

    os << endl  << "    ";

    return os;
}


Foam::OSstream& Foam::messageStream::operator()
(
    const char* functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber,
    const IOstream& ioStream
)
{
    return operator()
    (
        functionName,
        sourceFileName,
        sourceFileLineNumber,
        ioStream.relativeName(),
        ioStream.lineNumber(),
        -1  // No known endLineNumber
    );
}


Foam::OSstream& Foam::messageStream::operator()
(
    const char* functionName,
    const char* sourceFileName,
    const int sourceFileLineNumber,
    const dictionary& dict
)
{
    return operator()
    (
        functionName,
        sourceFileName,
        sourceFileLineNumber,
        dict.relativeName(),
        dict.startLineNumber(),
        dict.endLineNumber()
    );
}


// * * * * * * * * * * * * * * * Global Variables  * * * * * * * * * * * * * //

Foam::messageStream Foam::Info
(
    // No title
    Foam::messageStream::INFO
);

Foam::messageStream Foam::InfoErr
(
    // No title
    Foam::messageStream::INFO,
    0,
    true  // use_stderr = true
);

Foam::messageStream Foam::Warning
(
    "--> FOAM Warning : ",
    Foam::messageStream::WARNING
);

Foam::messageStream Foam::SeriousError
(
    "--> FOAM Serious Error : ",
    Foam::messageStream::SERIOUS,
    100
);


// ************************************************************************* //
