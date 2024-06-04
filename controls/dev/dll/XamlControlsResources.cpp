// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlControlsResources.h"
#include "RevealBrush.h"
#include "MUXControlsFactory.h"

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

    // We choose the URI to use at runtime based on whether we want compact resources
    winrt::Uri uri{
        [useCompactResources]() -> hstring {
            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = L"themeresources.xaml";

            return packagePrefix + compactPrefix + postfix;
        }()
    };
    
    // Because of Compact, UpdateSource may be executed twice, but there is a bug in XAML and manually clear theme dictionaries here:
    // Prior to RS5, when ResourceDictionary.Source property is changed, XAML forgot to clear ThemeDictionaries.
    ThemeDictionaries().Clear();
    Source(uri);
    
    UpdateAcrylicBrushesDarkTheme(ThemeDictionaries().Lookup(box_value(L"Default")));
    UpdateAcrylicBrushesLightTheme(ThemeDictionaries().Lookup(box_value(L"Light")));
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
        winrt::Uri uri{L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic.xaml"};
        control.DefaultStyleResourceUri(uri);
    }
}

void XamlControlsResources::EnsureRevealLights(winrt::UIElement const& /*element*/)
{
    // This is a no-op now since it only applied to RS3 XAML, but since it's an API we shipped with, we need to keep it around.
}
