// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include "RegUtil.h"

// Used on RS4 and RS5 to indicate whether ScrollBars must auto-hide or not.
bool RegUtil::UseDynamicScrollbars() noexcept
{
    LPCWSTR subKey = L"Control Panel\\Accessibility";
    LPCWSTR value = L"DynamicScrollbars";
    DWORD keyValue = 0;
    DWORD keySize = sizeof(DWORD);

    if (SUCCEEDED(HRESULT_FROM_WIN32(::RegGetValueW(HKEY_CURRENT_USER, subKey, value, RRF_RT_REG_DWORD, nullptr, &keyValue, &keySize))))
    {
        return keyValue != 0;
    }

    return true;
}
