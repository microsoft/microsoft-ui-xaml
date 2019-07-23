// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MUXControlsTestHooksLoggingMessageEventArgs.h"
#include "WinEventLogLevels.h"

// Logging level that results in no logging at all.
#define WINEVENT_LEVEL_NONE 0x0

#include "MUXControlsTestHooks.g.h"

class MUXControlsTestHooks :
    public winrt::implementation::MUXControlsTestHooksT<MUXControlsTestHooks>
{
public:
    UCHAR GetLoggingLevelForType(const wstring_view& type);
    UCHAR GetLoggingLevelForInstance(const winrt::IInspectable& sender);
    void SetOutputDebugStringLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);
    void SetLoggingLevelForTypeImpl(const wstring_view& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);
    void SetLoggingLevelForInstanceImpl(const winrt::IInspectable& sender, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);
    void LogMessage(const winrt::IInspectable& sender, const wstring_view& message, bool isVerboseLevel);
    winrt::event_token LoggingMessageImpl(winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs> const& value);
    void LoggingMessageImpl(winrt::event_token const& token);
    winrt::event_token BuildTreeCompletedImpl(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value); // subscribe
    void BuildTreeCompletedImpl(winrt::event_token const& token); // unsubscribe
    void NotifyBuildTreeCompletedImpl();

    static com_ptr<MUXControlsTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks->get_strong();
    }

    static void SetOutputDebugStringLevelForType(winrt::hstring const& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);
    static void SetLoggingLevelForType(winrt::hstring const& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);
    static void SetLoggingLevelForInstance(winrt::IInspectable const& sender, bool isLoggingInfoLevel, bool isLoggingVerboseLevel);

    static winrt::event_token LoggingMessage(winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs> const& value);
    static void LoggingMessage(winrt::event_token const& token);

    static winrt::event_token BuildTreeCompleted(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value); // subscribe
    static void BuildTreeCompleted(winrt::event_token const& token); // unsubscribe
    static void NotifyBuildTreeCompleted();

    static winrt::IInspectable CreateRepeaterElementFactoryGetArgs();
    static winrt::IInspectable CreateRepeaterElementFactoryRecycleArgs();
    static int GetElementFactoryElementIndex(winrt::IInspectable const& getArgs);

private:
    static MUXControlsTestHooks* s_testHooks;

    static void EnsureHooks();

private:
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs>> m_loggingMessageEventSource;
    std::map<std::wstring /*key:Type*/, UCHAR /*value:LoggingProviderLevel*/, std::less<>> m_typeLoggingLevels{};
    std::map<winrt::IInspectable /*key:Instance*/, UCHAR /*value:LoggingProviderLevel*/> m_instanceLoggingLevels{};
    UCHAR m_globalLoggingLevel{ WINEVENT_LEVEL_NONE };
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable>> m_buildTreeCompleted;
};