#include "pch.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Settings;

PlaceholderMode g_placeholderMode = PlaceholderMode::Off;

namespace
{
    struct ParsedMode
    {
        PlaceholderMode mode;
        bool conflictingSwitches;
    };

    ParsedMode ParsePlaceholderMode(std::wstring_view commandLine)
    {
        const bool requestedOn = commandLine.find(L"--placeholder=on") != std::wstring_view::npos;
        const bool requestedOff = commandLine.find(L"--placeholder=off") != std::wstring_view::npos;
        return
        {
            requestedOn ? PlaceholderMode::On : PlaceholderMode::Off,
            requestedOn && requestedOff,
        };
    }
}

int __stdcall wWinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ PWSTR commandLine,
    _In_ int)
{
    const auto parsedMode = ParsePlaceholderMode(commandLine ? commandLine : L"");
    if (parsedMode.conflictingSwitches)
    {
        MessageBoxW(
            nullptr,
            L"Specify only one of --placeholder=on or --placeholder=off.",
            L"Window placeholder visual comparison",
            MB_OK | MB_ICONERROR);
        return 2;
    }

    g_placeholderMode = parsedMode.mode;

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    if (g_placeholderMode == PlaceholderMode::On)
    {
        XamlOptionalChanges::EnableChange(XamlChangeId::SkipWindowRedirectionSurface);
    }
    else
    {
        XamlOptionalChanges::DisableChange(XamlChangeId::SkipWindowRedirectionSurface);
    }

    Application::Start([](auto&&)
    {
        make<WindowPlaceholderVisual::implementation::App>();
    });
    return 0;
}
