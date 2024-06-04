// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MUXControlsTestHooksFactory.h"

#ifdef SELECTORBAR_INCLUDED
#include "SelectorBarTrace.h"
#endif

#ifdef ANNOTATEDSCROLLBAR_INCLUDED
#include "AnnotatedScrollBarTrace.h"
#endif

#ifdef ITEMCONTAINER_INCLUDED
#include "ItemContainerTrace.h"
#endif

#ifdef ITEMSVIEW_INCLUDED
#include "ItemsViewTrace.h"
#endif

#ifdef NAVIGATIONVIEW_INCLUDED
#include "NavigationViewTrace.h"
#endif

#ifdef SCROLLPRESENTER_INCLUDED
#include "ScrollPresenterTrace.h"
#endif

#ifdef SCROLLVIEW_INCLUDED
#include "ScrollViewTrace.h"
#endif

#ifdef SWIPECONTROL_INCLUDED
#include "SwipeControlTrace.h"
#endif

#ifdef COMMANDBARFLYOUT_INCLUDED
#include "CommandBarFlyoutTrace.h"
#endif

#ifdef REPEATER_INCLUDED
#include "ItemsRepeaterTrace.h"
#include "LinedFlowLayoutTrace.h"
#endif

#include "TabViewTrace.h"

/*static*/
UCHAR MUXControlsTestHooks::GetLoggingLevelForType(const wstring_view& type)
{
    if (type.empty())
    {
        return m_globalLoggingLevel;
    }
    else
    {
        const auto iterator = m_typeLoggingLevels.find(type);

        if (iterator != m_typeLoggingLevels.end())
        {
            return iterator->second;
        }
        else
        {
            return WINEVENT_LEVEL_NONE;
        }
    }
}

UCHAR MUXControlsTestHooks::GetLoggingLevelForInstance(const winrt::IInspectable& sender)
{
    if (sender)
    {
        const auto iterator = m_instanceLoggingLevels.find(sender);

        if (iterator != m_instanceLoggingLevels.end())
        {
            return iterator->second;
        }
        else
        {
            return WINEVENT_LEVEL_NONE;
        }
    }
    else
    {
        return m_globalLoggingLevel;
    }
}

void MUXControlsTestHooks::SetOutputDebugStringLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
#ifdef SELECTORBAR_INCLUDED
    if (type == L"SelectorBar" || type.empty())
    {
        SelectorBarTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        SelectorBarTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef ANNOTATEDSCROLLBAR_INCLUDED
if (type == L"AnnotatedScrollBar" || type.empty())
{
    AnnotatedScrollBarTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
    AnnotatedScrollBarTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
}
#endif
#ifdef ITEMCONTAINER_INCLUDED
    if (type == L"ItemContainer" || type.empty())
    {
        ItemContainerTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ItemContainerTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef ITEMSVIEW_INCLUDED
    if (type == L"ItemsView" || type.empty())
    {
        ItemsViewTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ItemsViewTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef NAVIGATIONVIEW_INCLUDED
    if (type == L"NavigationView" || type.empty())
    {
        NavigationViewTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        NavigationViewTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef SCROLLPRESENTER_INCLUDED
    if (type == L"ScrollPresenter" || type.empty())
    {
        ScrollPresenterTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ScrollPresenterTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef SCROLLVIEW_INCLUDED
    if (type == L"ScrollView" || type.empty())
    {
        ScrollViewTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        ScrollViewTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef SWIPECONTROL_INCLUDED
    if (type == L"SwipeControl" || type.empty())
    {
        SwipeControlTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        SwipeControlTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef COMMANDBARFLYOUT_INCLUDED
    if (type == L"CommandBarFlyout" || type.empty())
    {
        CommandBarFlyoutTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        CommandBarFlyoutTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
#ifdef REPEATER_INCLUDED
    if (type == L"ItemsRepeater" || type.empty())
    {
        ItemsRepeaterTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
    }
    if (type == L"LinedFlowLayout" || type.empty())
    {
        LinedFlowLayoutTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
        LinedFlowLayoutTrace::s_IsVerboseDebugOutputEnabled = isLoggingVerboseLevel;
    }
#endif
    if (type == L"TabView" || type.empty())
    {
        TabViewTrace::s_IsDebugOutputEnabled = isLoggingInfoLevel || isLoggingVerboseLevel;
    }
}

void MUXControlsTestHooks::SetLoggingLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    UCHAR loggingProviderLevel = WINEVENT_LEVEL_NONE;

    if (isLoggingVerboseLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_VERBOSE;
    }
    else if (isLoggingInfoLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_INFO;
    }

    if (type.empty())
    {
        m_globalLoggingLevel = loggingProviderLevel;
    }
    else
    {
        const auto iterator = m_typeLoggingLevels.find(type);

        if (iterator != m_typeLoggingLevels.end())
        {
            if (loggingProviderLevel == WINEVENT_LEVEL_NONE)
            {
                m_typeLoggingLevels.erase(iterator);
            }
            else
            {
                iterator->second = loggingProviderLevel;
            }
        }
        else if (loggingProviderLevel != WINEVENT_LEVEL_NONE)
        {
            m_typeLoggingLevels.emplace(type, loggingProviderLevel);
        }
    }
}

void MUXControlsTestHooks::SetLoggingLevelForInstanceImpl(const winrt::IInspectable& sender, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    UCHAR loggingProviderLevel = WINEVENT_LEVEL_NONE;

    if (isLoggingVerboseLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_VERBOSE;
    }
    else if (isLoggingInfoLevel)
    {
        loggingProviderLevel = WINEVENT_LEVEL_INFO;
    }

    if (sender)
    {
        const auto iterator = m_instanceLoggingLevels.find(sender);

        if (iterator != m_instanceLoggingLevels.end())
        {
            if (loggingProviderLevel == WINEVENT_LEVEL_NONE)
            {
                m_instanceLoggingLevels.erase(iterator);
            }
            else
            {
                iterator->second = loggingProviderLevel;
            }
        }
        else if (loggingProviderLevel != WINEVENT_LEVEL_NONE)
        {
            m_instanceLoggingLevels.emplace(sender, loggingProviderLevel);
        }
    }
    else
    {
        m_globalLoggingLevel = loggingProviderLevel;
    }
}

void MUXControlsTestHooks::LogMessage(const winrt::IInspectable& sender, const wstring_view& message, bool isVerboseLevel)
{
    com_ptr<MUXControlsTestHooksLoggingMessageEventArgs> loggingMessageEventArgs = winrt::make_self<MUXControlsTestHooksLoggingMessageEventArgs>();

    loggingMessageEventArgs->SetMessage(message);
    loggingMessageEventArgs->SetIsVerboseLevel(isVerboseLevel);
    m_loggingMessageEventSource(sender, *loggingMessageEventArgs);
}

winrt::event_token MUXControlsTestHooks::LoggingMessageImpl(
    winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs> const& value)
{
    return m_loggingMessageEventSource.add(value);
}

void MUXControlsTestHooks::LoggingMessageImpl(winrt::event_token const& token)
{
    m_loggingMessageEventSource.remove(token);
}
