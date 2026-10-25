// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define NOMINMAX

// By default C++/WinRT will include some extra source and type info in the exception structs that it creates.
// This is useful diagnostic info, but it comes at a high cost due to the fact that it breaks COMDAT folding
// due to template functions that would previously compile down to identical binary code (which gets folded by
// the compiler) now end up unique for each instantiation of the template.
// This bloats the file size of MUXC.dll by 50%, so we turn this C++/WinRT feature off.
#define WINRT_NO_SOURCE_LOCATION

#pragma warning(disable : 6221) // Disable implicit cast warning for C++/WinRT headers (tracked by Bug 17528784: C++/WinRT headers trigger C6221 comparing e.code() to int-typed things)

// Disable factory caching in CppWinRT as the global COM pointers that are released during dll/process
// unload are not safe. Setting this makes CppWinRT just call get_activation_factory directly every time.
#define WINRT_DISABLE_FACTORY_CACHE 1

#include "targetver.h"

#include "BuildMacros.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// Basic WinRT ABI things -- without including all of WRL
#include <inspectable.h>
#include <hstring.h>
#include <eventtoken.h>
#include <activation.h>
#include <weakreference.h>
#include <guiddef.h>

#ifdef DBG
// Copied from dxaml\xcp\components\base\inc\AssertMacros.h to avoid taking a dependency.
#define XCPW(x) L##x

#define ASSERT(cond, ...) \
    do { \
        _Analysis_assume_(!!(cond)); \
        __pragma(warning(suppress:4127)) \
        if(!(cond)) \
        { \
            FAIL_ASSERT_HERE(XCPW(#cond)); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

#define FAIL_ASSERT_HERE(x) \
    do { \
        __annotation(L"Debug", L"AssertFail", (x)); \
        DbgRaiseAssertionFailure(); \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

#define MUX_ASSERT(X) ASSERT(X) 
#define MUX_ASSERT_MSG(X, MSG) ASSERT(X)
#define MUX_ASSERT_NOASSUME(X) ASSERT(X)
#else
#define MUX_ASSERT(X)
#define MUX_ASSERT_MSG(X, MSG)
#define MUX_ASSERT_NOASSUME(X)
#endif //DBG

#define MUX_FAIL_FAST() RaiseFailFastException(nullptr, nullptr, 0);
#define MUX_FAIL_FAST_MSG(MSG) RaiseFailFastException(nullptr, nullptr, 0);

#include "gsl/gsl"

// Microsoft.UI.Xaml.h accesses LoadLibrary in its inline declaration of CreateXamlUiPresenter
// Accessing LoadLibrary is not always allowed (e.g. on phone), so we need to suppress that.
// We can do so by making this define prior to including the header.
#define CREATE_XAML_UI_PRESENTER_API
#include <Microsoft.UI.Xaml.Hosting.ReferenceTracker.h>
#include <Windows.Foundation.h>

#include <WindowsNumerics.h>

#include <strsafe.h>
#include <robuffer.h>

#include <TraceLoggingInterop.h>
#include <wil/common.h>

// STL
#include <vector>
#include <map>
#include <functional>

#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2

#define WI_IS_FEATURE_PRESENT(FeatureName) 1

#undef GetCurrentTime

#include "CppWinRTIncludes.h"

