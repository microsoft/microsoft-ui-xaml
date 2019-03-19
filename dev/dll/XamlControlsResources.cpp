// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlControlsResources.h"
#include "RevealBrush.h"

#ifndef BUILD_WINDOWS
#include "MUXControlsFactory.h"
#endif

XamlControlsResources::XamlControlsResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
#ifndef BUILD_WINDOWS
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
#endif
}

void XamlControlsResources::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_UseCompactResourcesProperty)
    {
        UpdateSource();
    }
}

void XamlControlsResources::UpdateSource()
{
    bool useCompactResources = UseCompactResources();
    // At runtime choose the URI to use. If we're in a framework package and/or running on a different OS, 
    // we need to choose a different version because the URIs they have internally are different and this 
    // is the best we can do without conditional markup.
    winrt::Uri uri{
        [useCompactResources]() -> hstring {
            // Our RS2 styles depend on XamlCompositionBrushBase so use that as the condition here.
            bool isRS2OrHigher = SharedHelpers::IsXamlCompositionBrushBaseAvailable();
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();

            bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();

            hstring compactPrefix = useCompactResources ? L"compact_" : L"";
            hstring packagePrefix = L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/";
            hstring postfix = L"themeresources.xaml";

            if (isInFrameworkPackage)
            {
                packagePrefix = L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR  "/Themes/";
            }

            hstring releasePrefix;

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
            else if (isRS2OrHigher)
            {
                releasePrefix = L"rs2_";
            }
            else
            {
                releasePrefix = L"rs1_";
            }

            return packagePrefix + releasePrefix + compactPrefix + postfix;
        }()
    };

    //  Prior to RS5, when ResourceDictionary.Source property is changed, we forgot to clear ThemeDictionaries.
    ThemeDictionaries().Clear();

    Source(uri);
}

void SetDefaultStyleKeyWorker(winrt::IControlProtected const& controlProtected, std::wstring_view const& className) 
{
    if (SharedHelpers::IsRS2OrHigher() || SharedHelpers::IsInDesignMode())
    {
        // On RS2 we set the DefaultStyleResourceUri so we don't need the complex DefaultStyleKey
        // logic to hint XAML about the different generic.xaml location. We are telling them directly, so
        // just have the DefaultStyleKey be the class name.

        controlProtected.DefaultStyleKey(box_value(className));
    }
    else
    {
        // On RS1 and earlier where we don't have DefaultStyleResourceUri we need to tell XAML where to look
        // to load generic.xaml. By default it takes the DefaultStyleKey string (like Microsoft.UI.Xaml.Controls.Blah) 
        // and does ms-appx:///Microsoft.UI.Xaml.Controls/Themes/generic.xaml. This doesn't work for us
        // because the PRI resource root is actually Microsoft.UI.Xaml. Luckily XAML has special logic if
        // the DefaultStyleKey is an IReference<TypeName>. In this case, XAML will look up generic.xaml in a path 
        // constructed from the Assembly Name, and then we can have that be what we want (Microsoft.UI.Xaml).
        // Also cache the style key that we make in a static because we don't want to build these two objects
        // (hstring and IReference) every time we instance a control.

        WCHAR szFullTypeName[256];
        // Use the full type name discovered from what C# produces with typeof(X).FullName for Microsoft.UI.Xaml types.
        StringCchPrintfW(szFullTypeName, _countof(szFullTypeName),
            L"%s, " MUXCONTROLSROOT_NAMESPACE_STR ", Version = 255.255.255.255, Culture = neutral, PublicKeyToken = null, ContentType = WindowsRuntime",
            className.data());

        winrt::TypeName assemblyTypeName{ winrt::hstring(szFullTypeName), winrt::TypeKind::Metadata };
        controlProtected.DefaultStyleKey(box_value(assemblyTypeName));
    }

#ifndef BUILD_WINDOWS
    if (auto control5 = controlProtected.try_as<winrt::IControl5>())
    {
        winrt::Uri uri{
            []() -> PCWSTR {
            // Our RS2 styles depend on XamlCompositionBrushBase so use that as the condition here.
            bool isRS2OrHigher = SharedHelpers::IsXamlCompositionBrushBaseAvailable();
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();

            bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            if (isInFrameworkPackage)
            {
                if (is19H1OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_generic.xaml";
                }
                else if (isRS5OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_generic.xaml";
                }
                else if (isRS4OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_generic.xaml";
                }
                else if (isRS3OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_generic.xaml";
                }
                else if (isRS2OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_generic.xaml";
                }
                else
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs1_generic.xaml";
                }
            }
            else
            {
                if (is19H1OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_generic.xaml";
                }
                else if (isRS5OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_generic.xaml";
                }
                else if (isRS4OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_generic.xaml";
                }
                else if (isRS3OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_generic.xaml";
                }
                else if (isRS2OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_generic.xaml";
                }
                else
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs1_generic.xaml";
                }
            }
        }()
        };
        // Choose a default resource URI based on whether we're running in a framework package scenario or not.
        control5.DefaultStyleResourceUri(uri);
    }
#endif
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
