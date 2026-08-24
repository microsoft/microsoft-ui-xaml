// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "BasicTests.h"
#include "BasicTests.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    wux::DependencyProperty BasicTests::dpOnPageProperty =
        RegisterDependencyProperty(
            L"DPOnPage",
            xaml_typename<hstring>(),
            xaml_typename<BindTestbed::BasicTests>(),
            nullptr);

    BasicTests::BasicTests()
    {
        InitializeComponent();
        InitializeValues();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::BasicTests>().Name);
    }

    void BasicTests::InitializeValues()
    {
        DPOnPage(L"DP on page");
        NonDPOnPage(L"Non DP on Page");
    }

    void BasicTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
        std::wstring s = DPOnPage().c_str();
        DPOnPage(s.append(L"-").c_str());
    }

    void BasicTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
        InitializeValues();
    }

    void BasicTests::StopTrackingClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Bindings->StopTracking();
    }

    void BasicTests::ReInitializeBindingsClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Bindings->Initialize();
    }
}