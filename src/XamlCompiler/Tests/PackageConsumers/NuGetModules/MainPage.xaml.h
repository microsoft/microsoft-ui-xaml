#pragma once
#include "MainPage.g.h"
namespace winrt::NuGetModules::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        MainPage() = default;
        std::int32_t MyProperty() const { return 0; }
        void MyProperty(std::int32_t) {}
    };
}
namespace winrt::NuGetModules::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage> {};
}
