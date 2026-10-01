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
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "BlankUserControl.h"