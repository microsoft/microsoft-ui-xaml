// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define NOMINMAX

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

#include <wrl\implements.h>
#include <wrl\module.h>
#include <wrl\event.h>

#define MUX_ASSERT(X) _ASSERT(X) 
#define MUX_ASSERT_MSG(X, MSG) _ASSERT_EXPR(X, MSG)
#define MUX_ASSERT_NOASSUME(X) _ASSERT(X)

#define MUX_FAIL_FAST() RaiseFailFastException(nullptr, nullptr, 0);
#define MUX_FAIL_FAST_MSG(MSG) RaiseFailFastException(nullptr, nullptr, 0);

#ifdef BUILD_WINDOWS
#include <gsl/gsl_util>
#else
#include <gsl_util.h>
#endif

// windows.ui.xaml.h accesses LoadLibrary in its inline declaration of CreateXamlUiPresenter
// Accessing LoadLibrary is not always allowed (e.g. on phone), so we need to suppress that.
// We can do so by making this define prior to including the header.
#define CREATE_XAML_UI_PRESENTER_API
#include <Windows.UI.Xaml.Hosting.ReferenceTracker.h>

#include <WindowsNumerics.h>

#include <strsafe.h>
#include <robuffer.h>

// STL
#include <vector>
#include <map>
#include <functional>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef BUILD_WINDOWS
#include <staging.h>
#include <featurestaging-xaml.h>

#undef CATCH_RETURN // See our implementation in ErrorHandling.h

#else
#define WI_IS_FEATURE_PRESENT(FeatureName) 1
#endif

#undef GetCurrentTime

#include "CppWinRTIncludes.h"