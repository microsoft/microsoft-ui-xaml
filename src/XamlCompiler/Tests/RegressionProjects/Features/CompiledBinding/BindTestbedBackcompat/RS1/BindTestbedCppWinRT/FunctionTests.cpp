// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FunctionTests.h"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    FunctionTests::FunctionTests()
    {
        InitializeComponent();
        InitializeValues();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::FunctionTests>().Name);
    }

    void FunctionTests::InitializeValues()
    {
    }

    void FunctionTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
    }

    void FunctionTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
    }


    void FunctionTests::StopTrackingClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Bindings->StopTracking();
    }

    void FunctionTests::ReInitializeBindingsClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Bindings->Initialize();
    }
}