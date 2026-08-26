// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabularControlsResources.h"
#include "MUXControlsFactory.h"

TabularControlsResources::TabularControlsResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
}

void TabularControlsResources::UpdateSource()
{
#ifdef TABULAR_BINARY_EMITS_THEME_RESOURCES
    const bool isPerf2026Enabled = false; // TODO: Decide based on opt-in flag, task.ms/60958581

    winrt::Uri uri{
        [isPerf2026Enabled]() -> hstring {
            // Authority-less, matching MUXC (XamlControlsResources.cpp). A consuming app's build expands
            // every reference PRI into its own resources.pri, which erases component root map names, so
            // the alias authority does not exist outside this repo. The path must match AppxPriInitialPath.
            hstring packagePrefix = L"ms-appx:///" MUXTABULARROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = isPerf2026Enabled ? L"themeresources_perf2026.xaml" : L"themeresources.xaml";

            return packagePrefix + postfix;
        }()
    };

    // Workaround a pre-RS5 XAML bug: changing ResourceDictionary.Source didn't clear ThemeDictionaries.
    ThemeDictionaries().Clear();
    Source(uri);
#else
    // Keep scaffolding source-free so it doesn't bind or require MUXC theme resources.
#endif

    // FUTURE: This remaining AcrylicBrush lookup is a workaround to force the Nullable<Double> type to be
    // registered as a known type, which somehow doesn't otherwise get registered for some apps which need it.
    // At some point, the underlying issue should be investigated and fixed.
    // Guard the lookup because scaffolding has no Default theme dictionary.
    constexpr auto c_AcrylicBackgroundFillColorDefaultBrush = L"AcrylicBackgroundFillColorDefaultBrush"sv;
    auto themeDictionaries = ThemeDictionaries();
    if (themeDictionaries.HasKey(box_value(L"Default")))
    {
        if (auto defaultThemeDictionary = themeDictionaries.Lookup(box_value(L"Default")).try_as<winrt::ResourceDictionary>())
        {
            if (defaultThemeDictionary.HasKey(box_value(c_AcrylicBackgroundFillColorDefaultBrush)))
            {
                defaultThemeDictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush));
            }
        }
    }
}

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className) 
{
    controlProtected.DefaultStyleKey(box_value(className));

#ifdef TABULAR_BINARY_EMITS_THEME_RESOURCES
    if (auto control = controlProtected.try_as<winrt::IControl>())
    {
        const bool isPerf2026Enabled = false; // TODO: Decide based on opt-in flag, task.ms/60958581
        winrt::Uri uri{isPerf2026Enabled
            ? L"ms-appx:///" MUXTABULARROOT_NAMESPACE_STR "/Themes/generic_perf2026.xaml"
            : L"ms-appx:///" MUXTABULARROOT_NAMESPACE_STR "/Themes/generic.xaml"};
        control.DefaultStyleResourceUri(uri);
    }
#endif
}
