// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MUXControlsTestHooksLoggingMessageEventArgs.g.h"

class MUXControlsTestHooksLoggingMessageEventArgs
    : public winrt::implementation::MUXControlsTestHooksLoggingMessageEventArgsT<MUXControlsTestHooksLoggingMessageEventArgs>
{
public:
    void SetMessage(const wstring_view& message);
    void SetIsVerboseLevel(bool isVerboseLevel);

    // IMUXControlsTestHooksLoggingMessageEventArgs overrides
    winrt::hstring Message();
    bool IsVerboseLevel();

private:
    winrt::hstring m_message{ };
    bool m_isVerboseLevel{ false };
};
