// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TwoWayTests.h"
#include "TwoWayTests.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    wux::DependencyProperty TwoWayTests::dpOnPageProperty =
        RegisterDependencyProperty(
            L"DPOnPage",
            xaml_typename<hstring>(),
            xaml_typename<BindTestbed::TwoWayTests>(),
            nullptr);

    TwoWayTests::TwoWayTests()
    {
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::TwoWayTests>().Name);
    }

    void TwoWayTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
    }

    void TwoWayTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
    }
}