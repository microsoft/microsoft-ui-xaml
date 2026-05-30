// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Process-wide state for XamlOptionalChanges.
//
// This header lives in the components layer so both core and DXaml code can
// query the bitmask.  The DXaml layer (XamlOptionalChanges_Partial.cpp) owns
// the write side; core code should only read via the query helpers below.

#pragma once

#include <cstdint>
#include <PerfOptIn.h>

namespace OptionalChangeState
{
    // Defined in OptionalChangeState.cpp (components layer).
    // No std::atomic — writes are serialized by g_srwLock and all writes
    // complete before XAML init spawns threads that read these.
    extern uint64_t g_enabledChanges;
    extern bool     g_locked;
    extern SRWLOCK  g_srwLock;

    // Bit-index constants — keep in sync with GetBitIndex() in
    // XamlOptionalChanges_Partial.cpp and the XamlChangeId enum in
    // Microsoft.UI.Xaml.Settings.cs.
    constexpr int BitIndex_IconNoGridOptimization = 0;

    inline bool IsOptionalChangeEnabled(int bitIndex)
    {
        ASSERT(bitIndex >= 0 && bitIndex < 64);
        return (g_enabledChanges & (1ULL << bitIndex)) != 0;
    }

    inline bool IsIconNoGridOptimizationEnabled()
    {
        return IsOptionalChangeEnabled(BitIndex_IconNoGridOptimization) || IsPerfOptInEnabled();
    }
}
