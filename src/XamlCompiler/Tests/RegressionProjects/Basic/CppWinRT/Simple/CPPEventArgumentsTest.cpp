// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Implementation of the CPPEventArgumentsTest class.
//

#include "pch.h"
#include "CPPEventArgumentsTest.h"
#include "CPPEventArgumentsTest.g.cpp"

namespace winrt::Simple::implementation
{
    CPPEventArgumentsTest::CPPEventArgumentsTest()
    {
        InitializeComponent();
    }

    winrt::event_token CPPEventArgumentsTest::TheFirstEvent(Simple::MyFirstEvent const& handler)
    {
        return m_firstEvent.add(handler);
    }

    void CPPEventArgumentsTest::TheFirstEvent(winrt::event_token const& token) noexcept
    {
        m_firstEvent.remove(token);
    }

    winrt::event_token CPPEventArgumentsTest::TheSecondEvent(Simple::MySecondEvent const& handler)
    {
        return m_secondEvent.add(handler);
    }

    void CPPEventArgumentsTest::TheSecondEvent(winrt::event_token const& token) noexcept
    {
        m_secondEvent.remove(token);
    }

    winrt::event_token CPPEventArgumentsTest::TheThirdEvent(Simple::MyThirdEvent const& handler)
    {
        return m_thirdEvent.add(handler);
    }

    void CPPEventArgumentsTest::TheThirdEvent(winrt::event_token const& token) noexcept
    {
        m_thirdEvent.remove(token);
    }

    winrt::event_token CPPEventArgumentsTest::TheFourthEvent(Simple::MyFourthEvent const& handler)
    {
        return m_fourthEvent.add(handler);
    }

    void CPPEventArgumentsTest::TheFourthEvent(winrt::event_token const& token) noexcept
    {
        m_fourthEvent.remove(token);
    }
}
