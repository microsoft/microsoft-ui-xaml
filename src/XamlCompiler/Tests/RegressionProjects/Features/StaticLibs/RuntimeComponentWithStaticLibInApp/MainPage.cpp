// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::RuntimeComponentWithStaticLibInApp::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
    }

    int32_t MainPage::Dummy()
    {
        throw hresult_not_implemented();
    }

    void MainPage::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainPage::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
