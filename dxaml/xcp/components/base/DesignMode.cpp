// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DesignMode.h"
#include <Windows.ApplicationModel.h>

bool DesignerInterop::s_allowMocks = false;
UINT32 DesignerInterop::s_hasCheckedDesignMode = false;

void DesignerInterop::SetAllowDesignModeMock(bool allowMocks)
{
    s_allowMocks = allowMocks;
}

// Returns the either None or V2. Used internally by GetDesignerMode
DesignerMode DesignerInterop::EvaluateDesignerMode()
{
    Microsoft::WRL::ComPtr<ABI::Windows::ApplicationModel::IDesignModeStatics> designModeStatics;
    if (SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DesignMode).Get(), &designModeStatics)))
    {
        Microsoft::WRL::ComPtr<ABI::Windows::ApplicationModel::IDesignModeStatics2> designModeStatics2;
        if (SUCCEEDED(designModeStatics.As(&designModeStatics2)))
        {
            boolean designMode2Enabled = FALSE;
            if (SUCCEEDED(designModeStatics2->get_DesignMode2Enabled(&designMode2Enabled)) && designMode2Enabled == TRUE)
            {
                return DesignerMode::V2Only;
            }
        }
    }

    return DesignerMode::None;
}

inline bool HasMode(DesignerMode actualMode, DesignerMode modeToTest)
{
    return (actualMode == modeToTest) || static_cast<int>(actualMode & modeToTest) != 0;
}

bool DesignerInterop::GetDesignerMode(DesignerMode mode)
{
    if (s_allowMocks)
    {
        // In tests, we allow people to mock the RoGetDesignMode* API, this will cause us
        // to re-evaluate it each time.
        return HasMode(EvaluateDesignerMode(), mode);
    }

    // Ensure we only evaluate once for performance reasons. This is driven via the app manifest so it can't change during the process.
    // This call isn't affinitized to a thread so protect against contention with interlocked compare.
    static DesignerMode currentMode = DesignerMode::None;
    if (!InterlockedCompareExchange(&s_hasCheckedDesignMode, TRUE, FALSE))
    {
        currentMode = EvaluateDesignerMode();
    }

    return HasMode(currentMode, mode);
}
