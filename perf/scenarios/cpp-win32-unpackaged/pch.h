// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// This tells App.xaml.g.hpp the app will define its own WinMain
#define DISABLE_XAML_GENERATED_MAIN

// There's an API named GetCurrentTime in the Storyboard type.
#undef GetCurrentTime

// Com and WinRT headers
#include <Unknwn.h>

// Some generated files, like MainPage.xaml.g.h need files such as Microsoft.UI.Xaml.Markup.h
// to already be included.  Let's just include a bunch of stuff we know we'll need here in the PCH.
#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Hosting.h>
#include <winrt/Microsoft.UI.Xaml.Markup.h>
#include <winrt/Windows.Foundation.h>
