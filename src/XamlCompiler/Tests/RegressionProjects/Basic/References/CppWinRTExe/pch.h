// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// pch.h
// Header for platform projection include files
//

#pragma once

#include "hstring.h"
#include "windows.h"
// windows.h defines GetCurrentTime as a macro, which collides with the
// Microsoft.UI.Xaml.Media.Animation.Timeline.GetCurrentTime projection.
#undef GetCurrentTime

#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Foundation.Metadata.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Data.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"
#include "winrt/Microsoft.UI.Xaml.Shapes.h"

#include "winrt/CppWinRTComponent.h"
#include "winrt/CSharpWinrtComponent.h"
// This project deliberately does NOT include
// winrt/CSharpWinrtComponent.CSharpWinrtComponent_XamlTypeInfo.h by hand. The XAML compiler chains
// this component's generated metadata provider from XamlTypeInfo.g.cpp's OtherProviders(), and a
// managed component puts its provider in a nested <Ns>.<Ns>_XamlTypeInfo namespace that cppwinrt
// projects into its own header. Emitting that include is the compiler's job, and
// TypeInfoDefinition.LookupNeededCppWinRTProjectionHeaderFiles does it. Leaving it out here is what
// keeps this project a regression test for that behavior.
