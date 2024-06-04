// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Handle.h"

class CPowerModeRequestor
{
public:
    CPowerModeRequestor()
        : m_isPowerSavingEnabled(true),
        m_hPowerRequest(INVALID_HANDLE_VALUE)
    {}

    void DisablePowerSaving();
    void RestorePowerSaving();
private:
    void EnsurePowerRequestHandle();

    Handle m_hPowerRequest;
    bool m_isPowerSavingEnabled;
};

