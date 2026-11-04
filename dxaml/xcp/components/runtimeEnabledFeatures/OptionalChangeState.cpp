// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <OptionalChangeState.h>

namespace OptionalChangeState
{
    uint64_t g_enabledChanges = 0;
    bool     g_locked = false;
    SRWLOCK  g_srwLock = SRWLOCK_INIT;

#if DBG
    NotifySetterAppliedMode GetNotifySetterAppliedMode()
    {
        static const auto mode = []
        {
            DWORD value = 0;
            DWORD size = sizeof(value);
            const auto result = RegGetValueW(
                HKEY_CURRENT_USER,
                XAML_ROOT_KEY,
                L"NotifySetterAppliedMode",
                RRF_RT_REG_DWORD,
                nullptr,
                &value,
                &size);

            if (result == ERROR_FILE_NOT_FOUND || result == ERROR_PATH_NOT_FOUND)
            {
                return NotifySetterAppliedMode::Default;
            }

            IFCFAILFAST(HRESULT_FROM_WIN32(result));
            IFCFAILFAST(value >= 1 && value <= 3 ? S_OK : E_INVALIDARG);
            return static_cast<NotifySetterAppliedMode>(value);
        }();

        return mode;
    }
#endif
}
