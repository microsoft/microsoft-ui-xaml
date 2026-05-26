// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlControlsResources.h"
#include "RevealBrush.h"
#include "MUXControlsFactory.h"

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
    const bool isPerf2026Enabled = false; // TODO: Decide based on opt-in flag, task.ms/60958581

    // We choose the URI to use at runtime based on whether we want compact resources
    winrt::Uri uri{
        [useCompactResources, isPerf2026Enabled]() -> hstring {
            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = isPerf2026Enabled && !useCompactResources ? L"themeresources_perf2026.xaml" : L"themeresources.xaml";

            return packagePrefix + compactPrefix + postfix;
        }()
    };
    
    // Because of Compact, UpdateSource may be executed twice, but there is a bug in XAML and manually clear theme dictionaries here:
    // Prior to RS5, when ResourceDictionary.Source property is changed, XAML forgot to clear ThemeDictionaries.
    ThemeDictionaries().Clear();
    Source(uri);

    // FUTURE: This remaining AcrylicBrush lookup is a workaround to force the Nullable<Double> type to be
    // registered as a known type, which somehow doesn't otherwise get registered for some apps which need it.
    // At some point, the underlying issue should be investigated and fixed.
    constexpr auto c_AcrylicBackgroundFillColorDefaultBrush = L"AcrylicBackgroundFillColorDefaultBrush"sv;
    ThemeDictionaries().Lookup(box_value(L"Default")).as<winrt::ResourceDictionary>().Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush));
}

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className) 
{
    controlProtected.DefaultStyleKey(box_value(className));

    if (auto control = controlProtected.try_as<winrt::IControl>())
    {
        const bool isPerf2026Enabled = false; // TODO: Decide based on opt-in flag, task.ms/60958581
        winrt::Uri uri{isPerf2026Enabled
            ? L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic_perf2026.xaml"
            : L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/generic.xaml"};
        control.DefaultStyleResourceUri(uri);
    }
}

void XamlControlsResources::EnsureRevealLights(winrt::UIElement const& /*element*/)
{
    // This is a no-op now since it only applied to RS3 XAML, but since it's an API we shipped with, we need to keep it around.
}
