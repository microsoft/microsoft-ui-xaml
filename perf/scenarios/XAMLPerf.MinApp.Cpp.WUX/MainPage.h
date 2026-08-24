#pragma once

#include "MainPage.g.h"

namespace winrt::XAMLPerf_MinApp_Cpp_WUX::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
    };
}

namespace winrt::XAMLPerf_MinApp_Cpp_WUX::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
