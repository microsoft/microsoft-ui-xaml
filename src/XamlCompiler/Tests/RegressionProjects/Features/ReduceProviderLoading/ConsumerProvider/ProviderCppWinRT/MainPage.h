// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::ProviderCppWinRT::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();

        static void DoSomething();
        static hstring GetTextToShow();
    };
}

namespace winrt::ProviderCppWinRT::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
