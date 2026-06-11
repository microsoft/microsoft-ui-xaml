// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.hpp"
#include <minerror.h>
#include <regex>

namespace std
{
    _Prhand _Raise_handler = 0;
        
#if !_HAS_EXCEPTIONS
    // When the Standard Template Library encounters certain truly erroneous conditions,
    // conditions that represent either memory exhaustion or client-programmer errors,
    // it calls into one of the following functions.  We here override the definitions
    // of those functions in the C++ runtime (which would throw exceptions) and instead
    // fail fast the process in our usual fashion.
    void __cdecl _Xbad_alloc() { IFCFAILFAST(E_OUTOFMEMORY); }
    void __cdecl _Xbad_function_call() { XAML_FAIL_FAST(); }
    void __cdecl _Xbad_function_call(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xinvalid_argument(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xlength_error(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xout_of_range(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xoverflow_error(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xregex_error(_In_z_ const char*) { XAML_FAIL_FAST(); }
    void __cdecl _Xregex_error(std::regex_constants::error_type) { XAML_FAIL_FAST(); }
    void __cdecl _Xruntime_error(_In_z_ const char*) { XAML_FAIL_FAST(); }
    const char* __cdecl _Syserror_map(int) { XAML_FAIL_FAST(); return nullptr; }
    int __cdecl _Winerror_map(int) { XAML_FAIL_FAST(); return 0; }
#endif
}