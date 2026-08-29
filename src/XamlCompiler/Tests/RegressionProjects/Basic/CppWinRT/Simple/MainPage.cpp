// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

namespace winrt::Simple::implementation
{
    MainPage::MainPage()
        : MainPageT<MainPage>(hstring(L"This is MainPage"))
    {
        InitializeComponent();
    }

    void MainPage::ClickHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        Button1().Content(::winrt::box_value(::winrt::hstring(L"Bad things will happen now")));
    }

    void MainPage::TappedHandler(IInspectable const&, ::winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&)
    {}
}
