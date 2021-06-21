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

// Controls knows nothing about XamlControlsResources, but we need a way to pass the new visual flag from XamlControlsResources to Controls
// Assume XamlControlsResources is one per Application resource, and application is per thread, 
// so it's OK to assume one instance of XamlControlsResources per thread.
thread_local bool s_tlsIsControlsResourcesVersion2 = true;

XamlControlsResources::XamlControlsResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
}

bool XamlControlsResources::IsControlsResourcesVersion2()
{
    return ControlsResourcesVersion() != winrt::ControlsResourcesVersion::Version1;
}

void XamlControlsResources::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_UseCompactResourcesProperty || property == s_ControlsResourcesVersionProperty)
    {
        // Source link depends on ControlsResourcesVersion and UseCompactResources flag, we need to update it when either property changed
        UpdateSource();
    }

    // To be deleted
    // Version is going to be replaced with ControlsResourcesVersion
    else if (property == s_VersionProperty)
    {
        ControlsResourcesVersion(Version() == winrt::StylesVersion::Latest ? winrt::ControlsResourcesVersion::Version2 : winrt::ControlsResourcesVersion::Version1);
    }
}

void XamlControlsResources::UpdateSource()
{
    const bool useCompactResources = UseCompactResources();
    const bool useControlsResourcesVersion2 = IsControlsResourcesVersion2();

    // At runtime choose the URI to use. If we're in a framework package and/or running on a different OS, 
    // we need to choose a different version because the URIs they have internally are different and this 
    // is the best we can do without conditional markup.
    winrt::Uri uri {
        [useCompactResources, useControlsResourcesVersion2]() -> hstring {
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            const bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            const bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            const bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            const bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();
            const bool is21H1OrHigher = SharedHelpers::Is21H1OrHigher() && SharedHelpers::IsSelectionIndicatorModeAvailable();

            const bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            const bool isInCBSPackage = SharedHelpers::IsInCBSPackage();

            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = useControlsResourcesVersion2 ? L"themeresources.xaml" : L"themeresources_v1.xaml";

            if (isInFrameworkPackage)
            {
                packagePrefix = L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR  "/Themes/";
            }
            else if (isInCBSPackage)
            {
                packagePrefix = L"ms-appx://" MUXCONTROLS_CBS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR  "/Themes/";
            }

            hstring releasePrefix;

            if (is21H1OrHigher)
            {
                releasePrefix = L"21h1_";
            }
            else if (is19H1OrHigher)
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
        try
        {
            UpdateAcrylicBrushesDarkTheme(ThemeDictionaries().Lookup(box_value(L"Default")));
            UpdateAcrylicBrushesLightTheme(ThemeDictionaries().Lookup(box_value(L"Light")));
        }
        catch (winrt::hresult_error const& e)
        {
            MUX_FAIL_FAST_MSG("Fail to update acrylic brush");
        }
    }

    s_tlsIsControlsResourcesVersion2 = useControlsResourcesVersion2;
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
    if (const auto acrylicInAppFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultInverseBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicInAppFillColorDefaultInverseBrush.TintLuminosityOpacity(0.96);
    }
    if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.9);
    }
    if (const auto acrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
    {
        acrylicInAppFillColorBaseBrush.TintLuminosityOpacity(0.9);
    }
    if (const auto accentAcrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
    {
        accentAcrylicBackgroundFillColorDefaultBrush.TintLuminosityOpacity(0.9);
    }
    if (const auto accentAcrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
    {
        accentAcrylicInAppFillColorDefaultBrush.TintLuminosityOpacity(0.9);
    }
    if (const auto accentAcrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
    {
        accentAcrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.9);
    }
    if (const auto accentAcrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
    {
        accentAcrylicInAppFillColorBaseBrush.TintLuminosityOpacity(0.9);
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
        if (const auto acrylicInAppFillColorDefaultInverseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorDefaultInverseBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicInAppFillColorDefaultInverseBrush.TintLuminosityOpacity(0.85);
        }
        if (const auto acrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.92);
        }
        if (const auto acrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AcrylicInAppFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
        {
            acrylicInAppFillColorBaseBrush.TintLuminosityOpacity(0.92);
        }
        if (const auto accentAcrylicBackgroundFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
        {
            accentAcrylicBackgroundFillColorDefaultBrush.TintLuminosityOpacity(0.8);
        }
        if (const auto accentAcrylicInAppFillColorDefaultBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorDefaultBrush)).try_as<winrt::AcrylicBrush>())
        {
            accentAcrylicInAppFillColorDefaultBrush.TintLuminosityOpacity(0.8);
        }
        if (const auto accentAcrylicBackgroundFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicBackgroundFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
        {
            accentAcrylicBackgroundFillColorBaseBrush.TintLuminosityOpacity(0.8);
        }
        if (const auto accentAcrylicInAppFillColorBaseBrush = dictionary.Lookup(box_value(c_AccentAcrylicInAppFillColorBaseBrush)).try_as<winrt::AcrylicBrush>())
        {
            accentAcrylicInAppFillColorBaseBrush.TintLuminosityOpacity(0.8);
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
            const bool is21H1OrHigher = SharedHelpers::Is21H1OrHigher() && SharedHelpers::IsSelectionIndicatorModeAvailable();

            const bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            const bool isInCBSPackage = SharedHelpers::IsInCBSPackage();
            
            std::wstring postfix = s_tlsIsControlsResourcesVersion2 ? L"generic.xaml" : L"generic_v1.xaml";
            std::wstring releasePrefix = L"";
            
            if (isInFrameworkPackage)
            {
                if (is21H1OrHigher)
                {
                    releasePrefix = L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/21h1_";
                }
                else if (is19H1OrHigher)
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
                if (is21H1OrHigher)
                {
                    releasePrefix = L"ms-appx://" MUXCONTROLS_CBS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/21h1_";
                }
                else if (is19H1OrHigher)
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
                if (is21H1OrHigher)
                {
                    releasePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/21h1_";
                }
                else if (is19H1OrHigher)
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
