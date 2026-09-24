// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "EventTests.h"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    using namespace ::Windows::UI::Popups;

    EventTests::EventTests()
    {
        InitializeComponent();
        InitializeValues();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::EventTests>().Name);
    }

    void EventTests::InitializeValues()
    {
    }

    void EventTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
    }

    void EventTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
    }

    void EventTests::Click_RegularArgs(IInspectable const&, wux::RoutedEventArgs const&)
    {
        auto dlg = MessageDialog(L"Regular arguments clicked");
        auto t = dlg.ShowAsync();
    }

    void EventTests::Click_NoArgs()
    {
        auto dlg = MessageDialog(L"No argument Clicked");
        auto t = dlg.ShowAsync();
    }

    void EventTests::Click_BaseArgs(IInspectable const&, wux::RoutedEventArgs const&)
    {
        auto dlg = MessageDialog(L"Base argument clicked");
        auto t = dlg.ShowAsync();
    }

    void EventTests::Click_OverloadedArgs()
    {
        auto dlg = MessageDialog(L"Overloaded argument clicked");
        auto t = dlg.ShowAsync();
    }

    void EventTests::Click_OverloadedArgs(IInspectable const&, IInspectable const&)
    {
        auto dlg = MessageDialog(L"Overloaded argument clicked");
        auto t = dlg.ShowAsync();
    }
}