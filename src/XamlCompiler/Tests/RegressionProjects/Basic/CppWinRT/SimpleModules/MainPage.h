// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPageBase.h"
#include "MainPage.g.h"

#ifdef WINRT_IMPORT_MODULE
import winrt.Simple.Models;
import winrt.Microsoft.UI.Xaml.Controls.Primitives;
import winrt.Microsoft.UI.Xaml.Documents;
import winrt.Microsoft.UI.Xaml.Input;
#else
#include "winrt/Simple.Models.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#endif

namespace winrt::Simple::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        friend struct MainPageT<MainPage>;

        hstring StringProperty() { return L""; }
        ::winrt::Simple::Models::BindModel Model() const noexcept { return m_model; }

    protected:
        void ClickHandler(IInspectable const& sender, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void TappedHandler(IInspectable const& sender, ::winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&e);

    private:
        ::winrt::Simple::Models::BindModel m_model{ nullptr };
    };
}

namespace winrt::Simple::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
