// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPageBase.h"
#include "MainPage.g.h"

#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"

namespace winrt::Simple::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
        friend struct MainPageT<MainPage>;

        hstring StringProperty() { return L""; }

        void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e) override;
        static void ValidateStringNameLookup();

    protected:
        void ClickHandler(IInspectable const& sender, ::winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void TappedHandler(IInspectable const& sender, ::winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const&e);

    private:
        winrt::fire_and_forget RunTestScenario(hstring navigationParameter);
        void ValidateNamedStrings();
        static void HardAssert(bool condition, wchar_t const* message);
    };
}

namespace winrt::Simple::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
