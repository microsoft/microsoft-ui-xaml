// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ThemeResources.h"

#ifndef BUILD_WINDOWS
#include "MUXControlsFactory.h"
#endif

ThemeResources::ThemeResources()
{
    // On Windows, we need to add theme resources manually.  We'll still add an instance of this element to get the rest of
    // what it does, though.
#ifndef BUILD_WINDOWS
    MUXControlsFactory::EnsureInitialized();

    // At runtime choose the URI to use. If we're in a framework package and/or running on a different OS, 
    // we need to choose a different version because the URIs they have internally are different and this 
    // is the best we can do without conditional markup.
    winrt::Uri uri{
        []() -> PCWSTR {
            // Our RS2 styles depend on XamlCompositionBrushBase so use that as the condition here.
            bool isRS2OrHigher = SharedHelpers::IsXamlCompositionBrushBaseAvailable();
            // RS3 styles should be used on builds where ListViewItemPresenter's VSM integration works.
            bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher();
            bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();

            bool isInFrameworkPackage = SharedHelpers::IsInFrameworkPackage();
            if (isInFrameworkPackage)
            {
                if (is19H1OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_themeresources.xaml";
                }
                else if (isRS5OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_themeresources.xaml";
                }
                else if (isRS4OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_themeresources.xaml";
                }
                else if (isRS3OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_themeresources.xaml";
                }
                else if (isRS2OrHigher)
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_themeresources.xaml";
                }
                else
                {
                    return L"ms-appx://" MUXCONTROLS_PACKAGE_NAME "/" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs1_themeresources.xaml";
                }
            }
            else
            {
                if (is19H1OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/19h1_themeresources.xaml";
                }
                if (isRS5OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs5_themeresources.xaml";
                }
                else if (isRS4OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs4_themeresources.xaml";
                }
                else if (isRS3OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs3_themeresources.xaml";
                }
                else if (isRS2OrHigher)
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs2_themeresources.xaml";
                }
                else
                {
                    return L"ms-appx:///" MUXCONTROLSROOT_NAMESPACE_STR "/Themes/rs1_themeresources.xaml";
                }
            }
        }()
    };

    Source(uri);
#endif
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
            bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher();
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