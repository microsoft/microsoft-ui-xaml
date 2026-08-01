#include "pch.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

std::wstring g_launchMarker;

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    g_launchMarker = L"CustomMain";

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    Application::Start([](auto&&)
    {
        ::winrt::make<::winrt::DisableXamlGeneratedMainCpp::implementation::App>();
    });
    return 0;
}