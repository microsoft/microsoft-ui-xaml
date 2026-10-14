#pragma once
#include "EmptyPage.g.h"
namespace winrt::NuGetModules::implementation
{
    struct EmptyPage : EmptyPageT<EmptyPage>
    {
        EmptyPage() = default;
        std::int32_t MyProperty() const { return 0; }
        void MyProperty(std::int32_t) {}
    };
}
namespace winrt::NuGetModules::factory_implementation
{
    struct EmptyPage : EmptyPageT<EmptyPage, implementation::EmptyPage> {};
}
