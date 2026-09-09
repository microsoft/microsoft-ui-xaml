// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::EventHandling_968976::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        // Handlers attached in MainPage.xaml. Their signatures are what bug 968976 was
        // about: array and out parameters have to be generated correctly by the XAML
        // compiler when it wires up a XAML-declared event handler.
        void FirstHandler(winrt::array_view<uint32_t const> args);
        void SecondHandler(winrt::array_view<uint32_t> args);
        void ThirdHandler(winrt::com_array<uint32_t>& args);
        void FourthHandler(winrt::hstring& args);
    };
}

namespace winrt::EventHandling_968976::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
