// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This program exists for two reasons:
//
// 1)
// To overcome traditional DOS console command argument
// and variable length limitations. It takes 
// us from a measly 'some small length' (~2k) of arg chars to much 
// closer to the ~32k max imposed by the environment block.
//
// Upon invoking this program, it will transparently pass all 
// arguments it receives to TE.exe. It will additionally
// look for an optional file called teParams.txt. If this file
// exists it will append the entire file's contents to TE.exe's
// argument string.
//
// This allows us to pass much longer strings to TAEF, and allows
// for test selection strings to be built containing ~100 tests, 
// instead of the ~10-20 we can pass using the default argument
// length limitations.
//
// 2)
// To allow us to perform a prerun and postrun set of tests, even
// when using TAEF's loop modes. We can't do this using TAEF itself
// because it includes these tests in the loops.

#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <stdio.h>
#include <sstream>
#include <cstdio>

bool InvokeTaef(std::string& args, DWORD& exitCode)
{
    // Create the process... reuse the current
    // console handles so output is piped into the
    // current window.
    STARTUPINFO si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    auto result = CreateProcessA(
        "te.exe",
        &args[0],
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si, 
        &pi
    );
    if (result == 0) {
        std::cout << "CreateProcess returned 0 (LastError: " << GetLastError() << ")" << std::endl;
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    result = GetExitCodeProcess(pi.hProcess, &exitCode);
    if (result == 0) {
        std::cout << "GetExitCodeProcess returned 0 (LastError: " << GetLastError() << ")" << std::endl;
        return false;
    }
    return true;
}

int __cdecl main(int argc, char** argv)
{    
    std::cout << "Invoking TE.exe..." << std::endl;

    // For C/C++ argc/argv processing we expect
    // the first argument to be the command name.
    std::string baseArgs("te.exe");

    // Append all our arguments...
    for (int i = 1; i < argc; i++) {
        baseArgs = baseArgs + " " + argv[i];
    }

    std::string preRunArgStr;
    std::string runArgStr = baseArgs;
    std::string postRunArgStr;
    
    {
        bool appendToWtt = false;

        // And append the contents of teParams.txt
        // if it exists...
        // The arguments to append should all be on one line,
        // but we use std::getline to ensure we only get the
        // one line, in case somebody decided to append a stray
        // '\n'. (Not to name names, but let's just say it starts
        // with a 'P' and rhymes with "Flowershell's Set-Content cmdlet").
        std::ifstream preRunArgs("teParams.pre.txt");
        std::ifstream runArgs("teParams.txt");
        std::ifstream postRunArgs("teParams.post.txt");

        if (preRunArgs.is_open()) {
            std::string argString;
            std::getline(preRunArgs, argString);
            preRunArgStr = baseArgs + " " + argString;
            appendToWtt = true;
        }

        if (runArgs.is_open()) {
            std::string argString;
            std::getline(runArgs, argString);
            runArgStr = runArgStr + " " + argString;
        }
        if (appendToWtt) {
            runArgStr = runArgStr + " /appendWttLogging";
        }

        if (postRunArgs.is_open()) {
            std::string argString;
            std::getline(postRunArgs, argString);
            postRunArgStr = baseArgs + " " + argString + " /appendWttLogging";
        }
    }

    std::remove("teParams.pre.txt");
    std::remove("teParams.txt");
    std::remove("teParams.post.txt");

    // teParams.pre.txt run.
    // ---------------------------------------
    if (!preRunArgStr.empty()) {
        std::cout << std::endl << "Found prerun file, invoking with arguments from teParams.pre.txt:" << std::endl;
        std::cout << preRunArgStr << std::endl << std::endl;
        DWORD exitCode = 1;
        if (!InvokeTaef(preRunArgStr, exitCode)) {
            std::cout << "Terminating due to InvokeTaef() failure." << std::endl;
            return 1;
        }
    }

    // Main teParams.txt run.
    // ---------------------------------------
    std::cout << std::endl << "Invoking with arguments from teParams.txt:" << std::endl;
    std::cout << runArgStr << std::endl << std::endl;
    DWORD mainRunExitCode = 1;
    if (!InvokeTaef(runArgStr, mainRunExitCode)) {
        std::cout << "Terminating due to InvokeTaef() failure." << std::endl;
        return 1;
    }

    // teParams.post.txt run.
    // ---------------------------------------
    if (!postRunArgStr.empty()) {
        std::cout << std::endl << "Found postrun file, invoking with arguments from teParams.post.txt:" << std::endl;
        std::cout << postRunArgStr << std::endl << std::endl;
        DWORD exitCode = 1;
        if (!InvokeTaef(postRunArgStr, exitCode)) {
            std::cout << "Terminating due to InvokeTaef() failure." << std::endl;
            return 1;
        }
    }

    // And finally return TE.exe's exit code as if it
    // were our own.
    return mainRunExitCode;
}