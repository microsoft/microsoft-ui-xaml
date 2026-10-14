// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#ifdef XAML_FIXTURE_MODULES
#define WINRT_IMPORT_MODULE
import winrt.Windows.Foundation;
import winrt.Microsoft.UI.Xaml;
import StaticControlsModuleLib.Application_Xaml.BlankUserControl;
#endif
#include "BlankUserControl.h"

namespace winrt::StaticControlsModuleLib::implementation
{
    BlankUserControl::BlankUserControl()
    {
        InitializeComponent();
    }
}
