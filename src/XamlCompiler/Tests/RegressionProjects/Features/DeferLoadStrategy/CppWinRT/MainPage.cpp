// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::CppWinRT::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
        DataContext(*this);
    }

    int32_t MainPage::CurrentYearAsInteger()
    {
        return 2015;
    }

    void MainPage::Button_Click(IInspectable const& /* sender */, RoutedEventArgs const& /* e */)
    {
        auto frameworkElement = contentcontrol().ContentTemplateRoot().try_as<FrameworkElement>();
        frameworkElement.FindName(L"deferred");
        frameworkElement.FindName(L"currentYearTextBlock");
    }

    void MainPage::deferred_Click(IInspectable const& /* sender */, RoutedEventArgs const& /* e */)
    {
        FindName(L"MainGrid");
    }
}
