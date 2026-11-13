// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// The native module the symbolization tests resolve with its PDB, and that the CPU profile ETL fixture sampled. It has
// no imports and no entry point, so loading it runs nothing. build.cmd builds it; rebuild the DLL and PDB together, then
// record the ETL fixture again, whose stacks hold this build's addresses.

// Runs iterations additions. The total is volatile, so the loop isn't optimized away.
__declspec(dllexport) __declspec(noinline) unsigned long long FixtureBeta(unsigned long long iterations)
{
    volatile unsigned long long total = 0;
    for (unsigned long long i = 0; i < iterations; i++)
    {
        total += i;
    }

    return total;
}

// Calls FixtureBeta, and adds to its result after the call, so the call returns here rather than being a tail call: a
// sample in FixtureBeta has FixtureAlpha as its caller.
__declspec(dllexport) __declspec(noinline) unsigned long long FixtureAlpha(unsigned long long iterations)
{
    return FixtureBeta(iterations) + 1;
}
