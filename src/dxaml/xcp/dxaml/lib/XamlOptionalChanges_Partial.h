// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    PARTIAL_CLASS(XamlOptionalChanges)
    {
    public:
        // Internal C++ API for platform code to query and lock state.
        // IsChangeEnabledInternal will FAIL_FAST if called before LockInternal().
        static bool IsChangeEnabledInternal(_In_ ABI::Microsoft::UI::Xaml::Settings::XamlChangeId changeId);
        static bool IsLockedInternal();

        // Called by DXamlCore::InitializeImpl() to auto-lock at XAML init time.
        // Returns true if this call performed the lock, false if already locked.
        // Safe to call multiple times; no-ops (and returns false) if already locked.
        static bool LockInternal();

        // Test-only: resets all state (bitmask + lock) for test isolation.
        static void ResetInternal();
    };
}
