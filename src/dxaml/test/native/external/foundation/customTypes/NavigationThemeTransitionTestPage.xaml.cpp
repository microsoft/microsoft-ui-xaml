// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationThemeTransitionTestPage.xaml.h"

using namespace Private::Foundation::CustomTypes;

NavigationThemeTransitionTestPageConfiguration* NavigationThemeTransitionTestPageConfiguration::s_current = nullptr;

NavigationThemeTransitionTestPage::NavigationThemeTransitionTestPage()
{
    ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///NavigationThemeTransitionTestPage.xaml");
    ::Microsoft::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
    if (NavigationThemeTransitionTestPageConfiguration::s_current)
    {
        m_navigatedTo = NavigationThemeTransitionTestPageConfiguration::s_current->NavigatedTo;
        m_navigatingFrom = NavigationThemeTransitionTestPageConfiguration::s_current->NavigatingFrom;
        if (NavigationThemeTransitionTestPageConfiguration::s_current->PageInitialization)
        {
            NavigationThemeTransitionTestPageConfiguration::s_current->PageInitialization(this);
        }
    }
}

void NavigationThemeTransitionTestPage::OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e)
{
    if (m_navigatingFrom) m_navigatingFrom(this, e);
}


void NavigationThemeTransitionTestPage::OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    if (m_navigatedTo) m_navigatedTo(this, e);
}
