#pragma once
#include "Views/PanelPage.g.h"
namespace winrt::Simple::Views::implementation
{
    struct PanelPage : PanelPageT<PanelPage>
    {
        PanelPage();
        winrt::hstring Marker() const { return L"Views"; }
    };
}
namespace winrt::Simple::Views::factory_implementation
{
    struct PanelPage : PanelPageT<PanelPage, implementation::PanelPage> {};
}
