// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// LeakCycleTestCX.xaml.cpp
// Implementation of the LeakCycleTestCX class
//

#include "pch.h"
#include "LeakCycleTestCX.xaml.h"

using namespace TestAppCX;

using namespace Platform;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::UI::Core;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

LeakCycleTestCX::LeakCycleTestCX()
{
    InitializeComponent();
}

void TestAppCX::LeakCycleTestCX::OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    __super::OnNavigatedTo(e);

    m_colorPicker = ref new Microsoft::UI::Xaml::Controls::ColorPicker();
    // Create a cycle with the event which will be severed in the navigate away.
    m_colorPicker->ColorChanged += ref new TypedEventHandler<Microsoft::UI::Xaml::Controls::ColorPicker^, Microsoft::UI::Xaml::Controls::ColorChangedEventArgs^>([this](auto, auto) {
        
    });

    auto navManager = SystemNavigationManager::GetForCurrentView();
    navManager->AppViewBackButtonVisibility = AppViewBackButtonVisibility::Visible;
    m_backRequestedToken = (navManager->BackRequested += ref new ::Windows::Foundation::EventHandler<::Windows::UI::Core::BackRequestedEventArgs ^>(this, &LeakCycleTestCX::OnSystemBackRequested));
}

void TestAppCX::LeakCycleTestCX::OnNavigatingFrom(Microsoft::UI::Xaml::Navigation::NavigatingCancelEventArgs ^ e)
{
    __super::OnNavigatingFrom(e);

    m_colorPicker = nullptr;

    auto navManager = SystemNavigationManager::GetForCurrentView();
    navManager->AppViewBackButtonVisibility = AppViewBackButtonVisibility::Collapsed;
    navManager->BackRequested -= m_backRequestedToken;
}

void TestAppCX::LeakCycleTestCX::OnSystemBackRequested(Platform::Object ^ sender, ::Windows::UI::Core::BackRequestedEventArgs^ e)
{
    if (Frame->CanGoBack)
    {
        Frame->GoBack();
    }
}


void TestAppCX::LeakCycleTestCX::navMenuItem_Invoked(Microsoft::UI::Xaml::Controls::NavigationViewItem^ sender, Platform::Object^ args)
{

}
