// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"

namespace winrt::CppWinRTExe::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
    };
}

namespace winrt::CppWinRTExe::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
