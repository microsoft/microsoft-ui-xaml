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
#include "winrt/Windows.Foundation.Collections.h"
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
// winrt::xaml_typename<T>() and the TypeName that DependencyProperty::Register takes are
// projected from Windows.UI.Xaml.Interop, not the Microsoft.UI.Xaml.Interop namespace above.
// MyItem.cpp needs them and, being a plain runtime class, gets no compiler-generated header
// that would pull them in.
#include "winrt/Windows.UI.Xaml.Interop.h"
