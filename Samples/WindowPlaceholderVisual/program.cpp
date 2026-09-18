#include "pch.h"
#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Settings;

FeatureMode g_noRedirectionMode = FeatureMode::Off;
FeatureMode g_placeholderMode = FeatureMode::Off;

namespace
{
    constexpr auto SkipWindowRedirectionSurfaceChange =
        static_cast<XamlChangeId>(63530879);
    constexpr auto WindowPlaceholderVisualChange =
        static_cast<XamlChangeId>(63530880);

    struct ParsedModes
    {
        FeatureMode noRedirectionMode;
        FeatureMode placeholderMode;
        bool conflictingSwitches;
    };

    ParsedModes ParseModes(std::wstring_view commandLine)
    {
        const bool noRedirectionOn =
            commandLine.find(L"--no-redirection=on") != std::wstring_view::npos;
        const bool noRedirectionOff =
            commandLine.find(L"--no-redirection=off") != std::wstring_view::npos;
        const bool placeholderOn =
            commandLine.find(L"--placeholder=on") != std::wstring_view::npos;
        const bool placeholderOff =
            commandLine.find(L"--placeholder=off") != std::wstring_view::npos;

        return
        {
            noRedirectionOn ? FeatureMode::On : FeatureMode::Off,
            placeholderOn ? FeatureMode::On : FeatureMode::Off,
            (noRedirectionOn && noRedirectionOff) || (placeholderOn && placeholderOff),
        };
    }

    void SetChangeState(XamlChangeId changeId, FeatureMode mode)
    {
        if (mode == FeatureMode::On)
        {
            XamlOptionalChanges::EnableChange(changeId);
        }
        else
        {
            XamlOptionalChanges::DisableChange(changeId);
        }
    }
}

int __stdcall wWinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ PWSTR commandLine,
    _In_ int)
{
    const auto parsedModes = ParseModes(commandLine ? commandLine : L"");
    if (parsedModes.conflictingSwitches)
    {
        MessageBoxW(
            nullptr,
            L"Specify only one value for each of --no-redirection=on|off and --placeholder=on|off.",
            L"Window placeholder visual comparison",
            MB_OK | MB_ICONERROR);
        return 2;
    }

    g_noRedirectionMode = parsedModes.noRedirectionMode;
    g_placeholderMode = parsedModes.placeholderMode;

    winrt::init_apartment(winrt::apartment_type::single_threaded);
    SetChangeState(SkipWindowRedirectionSurfaceChange, g_noRedirectionMode);
    SetChangeState(WindowPlaceholderVisualChange, g_placeholderMode);

    Application::Start([](auto&&)
    {
        make<WindowPlaceholderVisual::implementation::App>();
    });
    return 0;
}
