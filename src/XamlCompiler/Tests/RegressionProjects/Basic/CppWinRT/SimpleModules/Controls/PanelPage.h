#pragma once
#include "Controls/PanelPage.g.h"
namespace winrt::Simple::Controls::implementation
{
    struct PanelPage : PanelPageT<PanelPage>
    {
        PanelPage();
        winrt::hstring Marker() const { return L"Controls"; }
    };
}
namespace winrt::Simple::Controls::factory_implementation
{
    struct PanelPage : PanelPageT<PanelPage, implementation::PanelPage> {};
}
