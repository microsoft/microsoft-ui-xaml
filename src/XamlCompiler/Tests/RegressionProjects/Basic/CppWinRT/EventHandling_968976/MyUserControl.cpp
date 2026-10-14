// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the MyUserControl class.
//

#include "pch.h"
#include "MyUserControl.h"
#include "MyUserControl.g.cpp"

namespace winrt::EventHandling_968976::implementation
{
    MyUserControl::MyUserControl()
    {
        InitializeComponent();
    }

    winrt::event_token MyUserControl::TheFirstEvent(EventHandling_968976::MyFirstEvent const& handler)
    {
        return m_firstEvent.add(handler);
    }

    void MyUserControl::TheFirstEvent(winrt::event_token const& token) noexcept
    {
        m_firstEvent.remove(token);
    }

    winrt::event_token MyUserControl::TheSecondEvent(EventHandling_968976::MySecondEvent const& handler)
    {
        return m_secondEvent.add(handler);
    }

    void MyUserControl::TheSecondEvent(winrt::event_token const& token) noexcept
    {
        m_secondEvent.remove(token);
    }

    winrt::event_token MyUserControl::TheThirdEvent(EventHandling_968976::MyThirdEvent const& handler)
    {
        return m_thirdEvent.add(handler);
    }

    void MyUserControl::TheThirdEvent(winrt::event_token const& token) noexcept
    {
        m_thirdEvent.remove(token);
    }

    winrt::event_token MyUserControl::TheFourthEvent(EventHandling_968976::MyFourthEvent const& handler)
    {
        return m_fourthEvent.add(handler);
    }

    void MyUserControl::TheFourthEvent(winrt::event_token const& token) noexcept
    {
        m_fourthEvent.remove(token);
    }

    winrt::event_token MyUserControl::GuidCharPass(EventHandling_968976::GuidCharPassEvent const& handler)
    {
        return m_guidCharPassEvent.add(handler);
    }

    void MyUserControl::GuidCharPass(winrt::event_token const& token) noexcept
    {
        m_guidCharPassEvent.remove(token);
    }

    winrt::event_token MyUserControl::GuidCharFill(EventHandling_968976::GuidCharFillEvent const& handler)
    {
        return m_guidCharFillEvent.add(handler);
    }

    void MyUserControl::GuidCharFill(winrt::event_token const& token) noexcept
    {
        m_guidCharFillEvent.remove(token);
    }

    winrt::event_token MyUserControl::GuidCharReceive(EventHandling_968976::GuidCharReceiveEvent const& handler)
    {
        return m_guidCharReceiveEvent.add(handler);
    }

    void MyUserControl::GuidCharReceive(winrt::event_token const& token) noexcept
    {
        m_guidCharReceiveEvent.remove(token);
    }
}
