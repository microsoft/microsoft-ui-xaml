// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MUXControlsTestHooksLoggingMessageEventArgs.h"

winrt::hstring MUXControlsTestHooksLoggingMessageEventArgs::Message()
{
    return m_message;
}

bool MUXControlsTestHooksLoggingMessageEventArgs::IsVerboseLevel()
{
    return m_isVerboseLevel;
}

void MUXControlsTestHooksLoggingMessageEventArgs::SetMessage(const wstring_view& message)
{
    m_message = message;
}

void MUXControlsTestHooksLoggingMessageEventArgs::SetIsVerboseLevel(bool isVerboseLevel)
{
    m_isVerboseLevel = isVerboseLevel;
}
