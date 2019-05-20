// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>

// RegGetValueW, implemented in linked AdvApi32.lib, is declared here because winreg.h's declaration is
// skipped for Windows Store code. Its declaration condition would need to change to
// #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_APP)
// from the current #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM).
extern "C"
WINADVAPI
LSTATUS
APIENTRY
RegGetValueW(
    _In_ HKEY hkey,
    _In_opt_ LPCWSTR lpSubKey,
    _In_opt_ LPCWSTR lpValue,
    _In_ DWORD dwFlags,
    _Out_opt_ LPDWORD pdwType,
    _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ ||
    (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
        (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) ||
        *pdwType == REG_SZ ||
        *pdwType == REG_EXPAND_SZ, _Post_z_)
    _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ ||
        *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
    _Out_writes_bytes_to_opt_(*pcbData, *pcbData) PVOID pvData,
    _Inout_opt_ LPDWORD pcbData
);


class RegUtil
{
public:
    // Used on RS4 and RS5 to indicate whether ScrollBars must auto-hide or not.
    static bool UseDynamicScrollbars() noexcept
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
};
