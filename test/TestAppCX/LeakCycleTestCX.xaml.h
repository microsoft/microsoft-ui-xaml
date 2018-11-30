// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//
// LeakCycleTestCX.xaml.h
// Declaration of the LeakCycleTestCX class
//

#pragma once

using namespace Microsoft::UI::Xaml::Controls;

#include "LeakCycleTestCX.g.h"

namespace TestAppCX
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class LeakCycleTestCX sealed
    {
    public:
        LeakCycleTestCX();

        void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

        void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e) override;

    private:
        void OnSystemBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ e);

        Windows::Foundation::EventRegistrationToken m_backRequestedToken;

        Microsoft::UI::Xaml::Controls::ColorPicker^ m_colorPicker;
        Microsoft::UI::Xaml::Controls::NavigationViewItem^ m_navItem;
        Windows::Foundation::EventRegistrationToken m_navItemInvokedToken;
        void navMenuItem_Invoked(Microsoft::UI::Xaml::Controls::NavigationViewItem^ sender, Platform::Object^ args);
    };
}
