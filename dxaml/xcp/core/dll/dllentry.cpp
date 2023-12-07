// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#define DLL_PROCESS_ATTACH 1

#if !DEPENDENCY_VERIFICATION_BUILD
extern "C"
void __cdecl __security_init_cookie(void);

// from winbase.h
// WINBASEAPI
extern "C"
BOOL
WINAPI
DisableThreadLibraryCalls (
    _In_ HMODULE hLibModule
    );

#endif

//------------------------------------------------------------------------
//
//  Method:   PrimeAgCore
//
//  Synopsis:
//      Prefetches a portion of agcore to improve the cold startup time
//
//------------------------------------------------------------------------
byte g_accessResult;
void PrimeAgCore(_In_ HINSTANCE hinstDLL)
{

    byte* ptr = reinterpret_cast<byte*>(hinstDLL);
    // Touch Approximately 1.6 MB of the dll, BBT will put the hottest code here.  AgCore is 5.7 MB so
    // this should never fail.
    byte* endPtr = ptr + 0x18C400;    
    while(ptr < endPtr)
    { 
        g_accessResult = *ptr; 
        ptr += 0x1000;
    }
}

extern "C" 
BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ unsigned int fdwReason,
    _In_opt_ void *
)
{
    // Perform actions based on the reason for calling.
        switch( fdwReason ) 
        { 
            case DLL_PROCESS_ATTACH:
                // Initialize once for each new process.
                // Return FALSE to fail DLL load.
 
                PrimeAgCore(hinstDLL);

#if !DEPENDENCY_VERIFICATION_BUILD
                //
                // The /GS security cookie must be initialized before any exception
                // handling targetting the current image is registered.  No function
                // using exception handling can be called in the current image until
                // after __security_init_cookie has been called.
                //

                // DisableThreadLibraryCalls here is okay because we arent actually
                // making any calls into any MT crt functions
                DisableThreadLibraryCalls(hinstDLL);
                __security_init_cookie();
#endif
                break;
        }
    return true;
}

