// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ExperimentalXamlControlsResources.h"
#include "MUXControlsFactory.h"

#define MUX_EXPERIMENTAL_NAMESPACE_STR L"Microsoft.Experimental.UI.Xaml"

ExperimentalXamlControlsResources::ExperimentalXamlControlsResources()
{
    MUXControlsFactory::EnsureInitialized();
    UpdateSource();
}


void ExperimentalXamlControlsResources::UpdateSource()
{
    // At runtime choose the URI to use. 
    winrt::Uri uri {
        []() -> hstring {
            const bool isRS3OrHigher = SharedHelpers::DoesListViewItemPresenterVSMWork();
            const bool isRS4OrHigher = SharedHelpers::IsRS4OrHigher();
            const bool isRS5OrHigher = SharedHelpers::IsRS5OrHigher() && SharedHelpers::IsControlCornerRadiusAvailable();
            const bool is19H1OrHigher = SharedHelpers::Is19H1OrHigher();
            const bool is21H1OrHigher = SharedHelpers::Is21H1OrHigher() && SharedHelpers::IsSelectionIndicatorModeAvailable();

            hstring packagePrefix = L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/";
            hstring postfix = L"themeresources.xaml";

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

            return packagePrefix + releasePrefix + postfix;
        }()
    };

    ThemeDictionaries().Clear();

    Source(uri);
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
            
            std::wstring postfix = L"generic.xaml";
            std::wstring releasePrefix = L"";

            if (is21H1OrHigher)
            {
                releasePrefix = L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/21h1_";
            }
            else if (is19H1OrHigher)
            {
                releasePrefix =  L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/19h1_";
            }
            else if (isRS5OrHigher)
            {
                releasePrefix =  L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/rs5_";
            }
            else if (isRS4OrHigher)
            {
                releasePrefix =  L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/rs4_";
            }
            else if (isRS3OrHigher)
            {
                releasePrefix =  L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/rs3_";
            }
            else
            {
                releasePrefix =  L"ms-appx:///" MUX_EXPERIMENTAL_NAMESPACE_STR "/Themes/rs2_";
            }

            return releasePrefix + postfix;
        }()
        };
        control5.DefaultStyleResourceUri(uri);
    }
}
