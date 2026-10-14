#include <unknwn.h>
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef WINRT_IMPORT_MODULE
import winrt.Microsoft.UI.Xaml.Input;
import winrt.Simple.Models;
#else
#include <winrt/Microsoft.UI.Xaml.Input.h>
#include <winrt/Simple.Models.h>
#endif

#ifdef WINRT_IMPORT_MODULE
import Simple.Application_Xaml.MainPage;
#endif
#include "MainPage.h"
#include "MainPage.g.cpp"

namespace winrt::Simple::implementation
{
    MainPage::MainPage()
        : MainPageT<MainPage>(hstring(L"This is MainPage"))
    {
        m_model = ::winrt::Simple::Models::BindModel();
        InitializeComponent();
    }

    void MainPage::ClickHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        Button1().Content(::winrt::box_value(::winrt::hstring(L"Bad things will happen now")));
    }

    void MainPage::TappedHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&)
    {}
}
