// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlControlsResources.h"
#include "RevealBrush.h"
#include "MUXControlsFactory.h"
#include "FrameworkUdk/Containment.h"

// Bug 62471884: Don't force AcrylicBrush objects defined in style to be realized
#define WINAPPSDK_CHANGEID_62471884 62471884
//
// Bug 62644600: [2.0 servicing] Enable build of perf2026 XAML and switch styles to Setter
#define WINAPPSDK_CHANGEID_62644600 62644600

static constexpr auto c_AcrylicBackgroundFillColorDefaultBrush = L"AcrylicBackgroundFillColorDefaultBrush"sv;
static constexpr auto c_AcrylicInAppFillColorDefaultBrush = L"AcrylicInAppFillColorDefaultBrush"sv;
static constexpr auto c_AcrylicBackgroundFillColorDefaultInverseBrush = L"AcrylicBackgroundFillColorDefaultInverseBrush"sv;
static constexpr auto c_AcrylicInAppFillColorDefaultInverseBrush = L"AcrylicInAppFillColorDefaultInverseBrush"sv;
static constexpr auto c_AcrylicBackgroundFillColorBaseBrush = L"AcrylicBackgroundFillColorBaseBrush"sv;
static constexpr auto c_AcrylicInAppFillColorBaseBrush = L"AcrylicInAppFillColorBaseBrush"sv;
static constexpr auto c_AccentAcrylicBackgroundFillColorDefaultBrush = L"AccentAcrylicBackgroundFillColorDefaultBrush"sv;
static constexpr auto c_AccentAcrylicInAppFillColorDefaultBrush = L"AccentAcrylicInAppFillColorDefaultBrush"sv;
static constexpr auto c_AccentAcrylicBackgroundFillColorBaseBrush = L"AccentAcrylicBackgroundFillColorBaseBrush"sv;
static constexpr auto c_AccentAcrylicInAppFillColorBaseBrush = L"AccentAcrylicInAppFillColorBaseBrush"sv;

static bool AreDefaultStyleOptimizationsEnabled()
{
    try
    {
        return winrt::Microsoft::UI::Xaml::Settings::XamlOptionalChanges::IsChangeEnabled(
            winrt::Microsoft::UI::Xaml::Settings::XamlChangeId::DefaultStyleOptimizations);
    }
    catch (...)
    {
        return false;
    }
}

XamlControlsResources::XamlControlsResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
}

void XamlControlsResources::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_UseCompactResourcesProperty)
    {
        // UseCompactResources was intended to be an experimental-only api.
        // However it accidentally shipped as a non-experimental api in a broken state.
        // Since it has already shipped we cannot remove the api, so instead we add an error message.
#ifndef MUX_PRERELEASE
        bool useCompactResources = UseCompactResources();
        if (useCompactResources)
        {
            throw winrt::hresult_invalid_argument(L"XamlControlsResources.UseCompactResources property is only supported in Experimental releases of WinUI");
        }
#endif
        // Source link depends on UseCompactResources flag, we need to update it when the property changes
        UpdateSource();
    }
}

void XamlControlsResources::UpdateSource()
{
    const bool useCompactResources = UseCompactResources();
    const bool arePerf2026StylesEnabled = AreDefaultStyleOptimizationsEnabled();

    // We choose the URI to use at runtime based on whether we want compact resources
    winrt::Uri uri{
        [useCompactResources, arePerf2026StylesEnabled]() -> hstring {
            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";

            if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62644600>())
            {
                hstring postfix = arePerf2026StylesEnabled && !useCompactResources ? L"themeresources_perf2026.xaml" : L"themeresources.xaml";
                return packagePrefix + compactPrefix + postfix;
            }
            else
            {
                return packagePrefix + compactPrefix + L"themeresources.xaml";
            }
        }()
    };
    
    // Because of Compact, UpdateSource may be executed twice, but there is a bug in XAML and manually clear theme dictionaries here:
    // Prior to RS5, when ResourceDictionary.Source property is changed, XAML forgot to clear ThemeDictionaries.
    ThemeDictionaries().Clear();
    Source(uri);

    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62471884>())
    {
        // FUTURE: This remaining AcrylicBrush lookup is a workaround to force the Nullable<Double> type to be
        // registered as a known type, which somehow doesn't otherwise get registered for some apps which need it.
        // At some point, the underlying issue should be investigated and fixed.
        ThemeDictionaries().Lookup(box_value(L"Default")).as<winrt::ResourceDictionary>().Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush));
    }
    else
    {
        // Original (pre-fix) behavior: look up all AcrylicBrush resources and set
        // TintLuminosityOpacity programmatically, which forces all brushes to be realized.
        UpdateAcrylicBrushesDarkTheme(ThemeDictionaries().Lookup(box_value(L"Default")));
        UpdateAcrylicBrushesLightTheme(ThemeDictionaries().Lookup(box_value(L"Light")));
    }
}

void XamlControlsResources::UpdateAcrylicBrushesLightTheme(const winrt::IInspectable themeDictionary)
{
    const auto dictionary = themeDictionary.try_as<winrt::ResourceDictionary>();
    if (const auto acrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicBackgroundFillColorDefaultBrush, 0.85);
    }
    if (const auto acrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicInAppFillColorDefaultBrush, 0.85);
    }
    if (const auto acrylicBackgroundFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultInverseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicBackgroundFillColorDefaultInverseBrush, 0.96);
    }
    if (const auto acrylicInAppFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultInverseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicInAppFillColorDefaultInverseBrush, 0.96);
    }
    if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicBackgroundFillColorBaseBrush, 0.9);
    }
    if (const auto acrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorBaseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(acrylicInAppFillColorBaseBrush, 0.9);
    }
    if (const auto accentAcrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(accentAcrylicBackgroundFillColorDefaultBrush, 0.9);
    }
    if (const auto accentAcrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(accentAcrylicInAppFillColorDefaultBrush, 0.9);
    }
    if (const auto accentAcrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorBaseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(accentAcrylicBackgroundFillColorBaseBrush, 0.9);
    }
    if (const auto accentAcrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorBaseBrush)).as<winrt::AcrylicBrush>())
    {
        UpdateTintLuminosityOpacity(accentAcrylicInAppFillColorBaseBrush, 0.9);
    }
}

void XamlControlsResources::UpdateAcrylicBrushesDarkTheme(const winrt::IInspectable themeDictionary)
{
    if (const auto dictionary = themeDictionary.try_as<winrt::ResourceDictionary>())
    {
        if (const auto acrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicBackgroundFillColorDefaultBrush, 0.96);
        }
        if (const auto acrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicInAppFillColorDefaultBrush, 0.96);
        }
        if (const auto acrylicBackgroundFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultInverseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicBackgroundFillColorDefaultInverseBrush, 0.85);
        }
        if (const auto acrylicInAppFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultInverseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicInAppFillColorDefaultInverseBrush, 0.85);
        }
        if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicBackgroundFillColorBaseBrush, 0.96);
        }
        if (const auto acrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorBaseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(acrylicInAppFillColorBaseBrush, 0.96);
        }
        if (const auto accentAcrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(accentAcrylicBackgroundFillColorDefaultBrush, 0.8);
        }
        if (const auto accentAcrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorDefaultBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(accentAcrylicInAppFillColorDefaultBrush, 0.8);
        }
        if (const auto accentAcrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorBaseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(accentAcrylicBackgroundFillColorBaseBrush, 0.8);
        }
        if (const auto accentAcrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorBaseBrush)).as<winrt::AcrylicBrush>())
        {
            UpdateTintLuminosityOpacity(accentAcrylicInAppFillColorBaseBrush, 0.8);
        }
    }
}

void XamlControlsResources::UpdateTintLuminosityOpacity(winrt::Microsoft::UI::Xaml::Media::AcrylicBrush brush, double luminosityValue)
{
    brush.TintLuminosityOpacity(luminosityValue);
}

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className) 
{
    controlProtected.DefaultStyleKey(box_value(className));

    if (auto control = controlProtected.try_as<winrt::IControl>())
    {
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62644600>())
        {
            const bool arePerf2026StylesEnabled = AreDefaultStyleOptimizationsEnabled();
            winrt::Uri uri{arePerf2026StylesEnabled
                ? L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic_perf2026.xaml"
                : L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic.xaml"};
            control.DefaultStyleResourceUri(uri);
        }
        else
        {
            winrt::Uri uri{L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic.xaml"};
            control.DefaultStyleResourceUri(uri);
        }
    }
}

void XamlControlsResources::EnsureRevealLights(winrt::UIElement const& /*element*/)
{
    // This is a no-op now since it only applied to RS3 XAML, but since it's an API we shipped with, we need to keep it around.
}
