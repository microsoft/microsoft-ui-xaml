// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MUXControlsTestHooks.h"
#include "common.h"

MUXControlsTestHooks* MUXControlsTestHooks::s_testHooks = nullptr;

void MUXControlsTestHooks::EnsureHooks()
{
    if (s_testHooks == nullptr)
    {
        s_testHooks = winrt::make_self<MUXControlsTestHooks>().detach();
    }
}

void MUXControlsTestHooks::SetOutputDebugStringLevelForType(winrt::hstring const& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    EnsureHooks();
    s_testHooks->SetOutputDebugStringLevelForTypeImpl(type, isLoggingInfoLevel, isLoggingVerboseLevel);
}

void MUXControlsTestHooks::SetLoggingLevelForType(winrt::hstring const& type, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    EnsureHooks();
    s_testHooks->SetLoggingLevelForTypeImpl(type, isLoggingInfoLevel, isLoggingVerboseLevel);
}

void MUXControlsTestHooks::SetLoggingLevelForInstance(winrt::IInspectable const& sender, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
{
    EnsureHooks();
    s_testHooks->SetLoggingLevelForInstanceImpl(sender, isLoggingInfoLevel, isLoggingVerboseLevel);
}

winrt::event_token MUXControlsTestHooks::LoggingMessage(winrt::TypedEventHandler<winrt::IInspectable, winrt::MUXControlsTestHooksLoggingMessageEventArgs>  /*unused*/const& value)
{
    EnsureHooks();
    return s_testHooks->LoggingMessageImpl(value);
}

void MUXControlsTestHooks::LoggingMessage(winrt::event_token const& token)
{
    if (s_testHooks != nullptr)
    {
        s_testHooks->LoggingMessageImpl(token);
    }
}
