// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationThemeTransitionTestPage.g.h"
#include <functional>
#include <memory>

namespace Private { namespace Foundation { namespace CustomTypes {


// Because Navigating to a page requries a type, the only way to insert custom code into such
// the process is to create a new type, which isn't simple, especially in our test environment.
// So to get around this, we will create this configuration class that will be used to  store
// information/callbacks on how the "next" NavigationThemeTransitionTestPage should be created.
// Typically, these would me static methods on the actual page, but there are restrictions on
// on what public members can be added to a winrt class.
class NavigationThemeTransitionTestPageConfiguration
{
public:
    typedef std::function<void(Microsoft::UI::Xaml::Controls::Page^ page, Microsoft::UI::Xaml::Navigation::NavigationEventArgs^ e)> NavigatedCallback;
    typedef std::function<void(Microsoft::UI::Xaml::Controls::Page^ page, Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e)> NavigatingCallback;
    typedef std::function<void(Microsoft::UI::Xaml::Controls::Page^ page)> PageInitializationCallback;

    static NavigationThemeTransitionTestPageConfiguration* s_current;

    NavigationThemeTransitionTestPageConfiguration()
    {
        if (s_current != nullptr)
        {
            throw;
        }
        s_current = this;
    }

    ~NavigationThemeTransitionTestPageConfiguration()
    {
        s_current = nullptr;
    }


    NavigatedCallback NavigatedTo;
    NavigatingCallback NavigatingFrom;
    PageInitializationCallback PageInitialization;

    void Reset()
    {
        NavigatedTo = nullptr;
        NavigatingFrom = nullptr;
        PageInitialization = nullptr;
    }
};




[::Windows::Foundation::Metadata::WebHostHidden]
public ref class NavigationThemeTransitionTestPage sealed
{
public:
    NavigationThemeTransitionTestPage();

    void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    void OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e) override;

private:
    NavigationThemeTransitionTestPageConfiguration::NavigatedCallback m_navigatedTo;
    NavigationThemeTransitionTestPageConfiguration::NavigatingCallback m_navigatingFrom;
};

} } }
