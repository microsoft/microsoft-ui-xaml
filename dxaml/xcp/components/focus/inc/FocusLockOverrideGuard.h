// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CFocusManager;

// Normally, we lock focus once we've started raising focus events to prevent reentrancy. In select situation, such 
// as Window Activation/Deactivation, we want focus to be able to change, even when locked. Therefore, we allow
// the focus change to occur by ignoring the focus lock. This should be used with care... 
// if used irresponsibly, we can enable unsupported reentrancy scenarios.

class UnsafeFocusLockOverrideGuard
{
public:
    void* operator new(size_t) = delete;

    UnsafeFocusLockOverrideGuard(
        _In_ CFocusManager *focusManager) : m_focusManager(focusManager)
    {
        m_focusManager->SetIgnoreFocusLock(true);
    }

    ~UnsafeFocusLockOverrideGuard()
    {
        m_focusManager->SetIgnoreFocusLock(false);
    }

private:
    CFocusManager* m_focusManager;
};