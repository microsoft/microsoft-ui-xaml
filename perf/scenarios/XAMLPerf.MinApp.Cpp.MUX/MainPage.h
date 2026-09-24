#pragma once

#include "MainPage.g.h"

namespace winrt::XAMLPerf_MinApp_Cpp_MUX::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage();
    };
}

namespace winrt::XAMLPerf_MinApp_Cpp_MUX::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
