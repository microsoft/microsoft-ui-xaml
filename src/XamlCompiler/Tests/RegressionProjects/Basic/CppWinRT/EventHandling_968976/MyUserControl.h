// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the MyUserControl class.
//

#pragma once

#include "MyUserControl.g.h"

namespace winrt::EventHandling_968976::implementation
{
    struct MyUserControl : MyUserControlT<MyUserControl>
    {
        MyUserControl();

        winrt::event_token TheFirstEvent(EventHandling_968976::MyFirstEvent const& handler);
        void TheFirstEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheSecondEvent(EventHandling_968976::MySecondEvent const& handler);
        void TheSecondEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheThirdEvent(EventHandling_968976::MyThirdEvent const& handler);
        void TheThirdEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheFourthEvent(EventHandling_968976::MyFourthEvent const& handler);
        void TheFourthEvent(winrt::event_token const& token) noexcept;

    private:
        winrt::event<EventHandling_968976::MyFirstEvent> m_firstEvent;
        winrt::event<EventHandling_968976::MySecondEvent> m_secondEvent;
        winrt::event<EventHandling_968976::MyThirdEvent> m_thirdEvent;
        winrt::event<EventHandling_968976::MyFourthEvent> m_fourthEvent;
    };
}

namespace winrt::EventHandling_968976::factory_implementation
{
    struct MyUserControl : MyUserControlT<MyUserControl, implementation::MyUserControl>
    {
    };
}
