// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the CPPEventArgumentsTest class.
//

#pragma once

#include "CPPEventArgumentsTest.g.h"

namespace winrt::Simple::implementation
{
    struct CPPEventArgumentsTest : CPPEventArgumentsTestT<CPPEventArgumentsTest>
    {
        CPPEventArgumentsTest();

        winrt::event_token TheFirstEvent(Simple::MyFirstEvent const& handler);
        void TheFirstEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheSecondEvent(Simple::MySecondEvent const& handler);
        void TheSecondEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheThirdEvent(Simple::MyThirdEvent const& handler);
        void TheThirdEvent(winrt::event_token const& token) noexcept;
        winrt::event_token TheFourthEvent(Simple::MyFourthEvent const& handler);
        void TheFourthEvent(winrt::event_token const& token) noexcept;

    private:
        winrt::event<Simple::MyFirstEvent> m_firstEvent;
        winrt::event<Simple::MySecondEvent> m_secondEvent;
        winrt::event<Simple::MyThirdEvent> m_thirdEvent;
        winrt::event<Simple::MyFourthEvent> m_fourthEvent;
    };
}

namespace winrt::Simple::factory_implementation
{
    struct CPPEventArgumentsTest : CPPEventArgumentsTestT<CPPEventArgumentsTest, implementation::CPPEventArgumentsTest>
    {
    };
}
