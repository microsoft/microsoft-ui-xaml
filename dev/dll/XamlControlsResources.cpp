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
static constexpr auto c_AcrylicBackgroundFillColorBaseBrush = L"AcrylicBackgroundFillColorBaseBrush"sv;

// Controls knows nothing about XamlControlsResources, but we need a way to pass the new visual flag from XamlControlsResources to Controls
// Assume XamlControlsResources is one per Application resource, and application is per thread, 
// so it's OK to assume one instance of XamlControlsResources per thread.
thread_local bool s_tlsUseLatestStyle = true;

XamlControlsResources::XamlControlsResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
}

bool XamlControlsResources::UseLatestStyle()
{
    return Version() != winrt::StylesVersion::WinUI_2dot5;
}

void XamlControlsResources::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_UseCompactResourcesProperty || property == s_VersionProperty)
    {
        // Source link depends on Version and UseCompactResources flag, we need to update it when either property changed
        UpdateSource();
    }
}

void XamlControlsResources::UpdateSource()
{
    const bool useCompactResources = UseCompactResources();
    const bool useNewVisual = UseLatestStyle();

    // At runtime choose the URI to use. If we're in a framework package and/or running on a different OS, 
    // we need to choose a different version because the URIs they have internally are different and this 
    // is the best we can do without conditional markup.
    winrt::Uri uri {
        [useCompactResources, useNewVisual]() -> hstring {
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            const bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            const bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            const bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            const bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();
#ifdef USE_INTERNAL_SDK
            const bool is21H1OrHigher = SharedHelpers::Is21H1OrHigher() && SharedHelpers::IsSelectionIndicatorModeAvailable();
#endif

            const bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            const bool isInCBSPackage = SharedHelpers::IsInCBSPackage();

            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = useNewVisual ? L"themeresources.xaml" : L"themeresources_2dot5.xaml";

            if (isInFrameworkPackage)
            {
                packagePrefix = L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR  "/Themes/";
            }
            else if (isInCBSPackage)
            {
                packagePrefix = L"ms-appx://" MUXCONTROLS_CBS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR  "/Themes/";
            }

            hstring releasePrefix;

#ifdef USE_INTERNAL_SDK
            if (is21H1OrHigher && useNewVisual)
            {
                releasePrefix = L"21h1_";
            }
            else
#endif
            if (is19H1OrHigher)
            {
                releasePrefix = L"19h1_";
            }
            else if (isRS5OrHigher)
            {
                releasePrefix = L"rs5_";
            }
            else if (isRS4OrHigher)
            {
                releasePrefix = L"rs4_";
            }
            else if (isRS3OrHigher)
            {
                releasePrefix = L"rs3_";
            }
            else
            {
                releasePrefix = L"rs2_";
            }

            return packagePrefix + releasePrefix + compactPrefix + postfix;
        }()
    };

    // Because of Compact, UpdateSource may be executed twice, but there is a bug in XAML and manually clear theme dictionaries here:
    // Prior to RS5, when ResourceDictionary.Source property is changed, XAML forgot to clear ThemeDictionaries.
    ThemeDictionaries().Clear();

    Source(uri);

    // Hacky workaround for a XAML compiler bug:
    // Assigning nullable primitive types from XAML fails with disabled XAML metadata reflection on older versions.
    // The MUXC AcrylicBrush's TintLuminosityOpacity is a nullable double though and needs to be set.
    // Solution: Load theme resources and edit brushes manually.
    // Since something must go horribly wrong for those lookups to fail, we just assume they exist.
    if (SharedHelpers::Is19H1OrHigher())
    {
        UpdateAcrylicBrushesDarkTheme(ThemeDictionaries().Lookup(box_value(L"Default")));
        UpdateAcrylicBrushesLightTheme(ThemeDictionaries().Lookup(box_value(L"Light")));
    }

    s_tlsUseLatestStyle = useNewVisual;
}

void XamlControlsResources::UpdateAcrylicBrushesLightTheme(const winrt::IInspectable themeDictionary)
{
    const auto dictionary = themeDictionary.try_as<winrt::ResourceDictionary>();
    if (const auto acrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicBackgroundFillColorDefaultBrush.TintLuminosityOpacity(0.85);
    }
    if (const auto acrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicInAppFillColorDefaultBrush.TintLuminosityOpacity(0.85);
    }
    if (const auto acrylicBackgroundFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultInverseBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicBackgroundFillColorDefaultInverseBrush.TintLuminosityOpacity(0.96);
    }
    if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.9);
    }
}

void XamlControlsResources::UpdateAcrylicBrushesDarkTheme(const winrt::IInspectable themeDictionary)
{
    if (const auto dictionary = themeDictionary.try_as<winrt::ResourceDictionary>())
    {
        if (const auto acrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicBackgroundFillColorDefaultBrush.TintLuminosityOpacity(0.96);
        }
        if (const auto acrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicInAppFillColorDefaultBrush.TintLuminosityOpacity(0.96);
        }
        if (const auto acrylicBackgroundFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorDefaultInverseBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicBackgroundFillColorDefaultInverseBrush.TintLuminosityOpacity(0.85);
        }
        if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.96);
        }
    }
}

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className) 
{
    controlProtected.DefaultStyleKey(box_value(className));

    if (auto control5 = controlProtected.try_as<winrt::IControl5>())
    {
        winrt::Uri uri{
            []() {
            
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            const bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            const bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            const bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            const bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();

            const bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            const bool isInCBSPackage = SharedHelpers::IsInCBSPackage();
            
            std::wstring postfix = s_tlsUseLatestStyle ? L"generic.xaml" : L"generic_2dot5.xaml";
            std::wstring releasePrefix = L"";
            
            if (isInFrameworkPackage)
            {
                if (is19H1OrHigher)
                {
                    releasePrefix = L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_";
                }
                else if (isRS5OrHigher)
                {
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_";
                }
                else if (isRS4OrHigher)
                {
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_";
                }
                else if (isRS3OrHigher)
                {
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_";
                }
                else
                {
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_";
                }
            }
            else if (isInCBSPackage)
            {
                if (is19H1OrHigher)
                {
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_CBS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_";
                }
                else
                {
                    MUX_FAIL_FAST_MSG("CBS package doesn't apply to old platforms");
                    releasePrefix =  L"ms-appx://" MUXCONTROLS_CBS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_";
                }
            }
            else
            {
                if (is19H1OrHigher)
                {
                    releasePrefix =  L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_";
                }
                else if (isRS5OrHigher)
                {
                    releasePrefix =  L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_";
                }
                else if (isRS4OrHigher)
                {
                    releasePrefix =  L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_";
                }
                else if (isRS3OrHigher)
                {
                    releasePrefix =  L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_";
                }
                else
                {
                    releasePrefix =  L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_";
                }
            }

            return releasePrefix + postfix;
        }()
        };
        // Choose a default resource URI based on whether we're running in a framework package scenario or not.
        control5.DefaultStyleResourceUri(uri);
    }
}

// Normally global reveal lights are attached automtically via the first call to
// RevealBrush::OnConnected for the current view. However, there are some corner cases
// where lights were lost or never connected. In those situations, apps will manually call this API.
// In general, it is ok to can call this multiple times on elements in the live tree.
// If RevealHoverLight's are already present on the root, we will not try to attach more lights.
//
// Currently known scenarios requiring this API :
// (1) Reveal on Full Window media controls (call when ME/MPE is in FullWindow and pass its MediaTransportControls instance)
// (2) App sets Window.Content (this action destroys the RootScrolLViewer's lights, and they need to be recreated. Pass any element in the main tree.)

void XamlControlsResources::EnsureRevealLights(winrt::UIElement const& element)
{
    // Ensure that ambient and border lights needed for reveal effects are set on tree root
    if (SharedHelpers::IsXamlCompositionBrushBaseAvailable()
        // If Xaml can apply a light on the root visual, then the app doesn't need to manually attach lights to some other root
        && !SharedHelpers::DoesXamlMoveRSVLightToRootVisual())
    {
        // Defer until next Rendering event. Otherwise, in the FullWindow media case 
        // VisualTreehelper may fail to find the FullWindowMediaRoot that had been created just prior to this call
        auto renderingEventToken = std::make_shared<winrt::event_token>();
        *renderingEventToken = winrt::Xaml::Media::CompositionTarget::Rendering(
            [renderingEventToken, element](auto&, auto&) {
                // Detach event or Rendering will keep calling us back.
                winrt::Xaml::Media::CompositionTarget::Rendering(*renderingEventToken);

                RevealBrush::AttachLightsToAncestor(element, false);
            });
    }
}
