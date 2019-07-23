// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "RegUtil.h"
#include <pch.h>

bool RegUtil::s_hasMouseWheelScrollLinesCache{ false };
bool RegUtil::s_hasMouseWheelScrollCharsCache{ false };

int32_t RegUtil::s_mouseWheelScrollLines{ RegUtil::s_defaultMouseWheelScrollLines };
int32_t RegUtil::s_mouseWheelScrollChars{ RegUtil::s_defaultMouseWheelScrollChars };

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

// Used on RS4- to retrieve the reg key values for HKEY_CURRENT_USER\Control Panel\Desktop\WheelScrollChars and WheelScrollLines.
int32_t RegUtil::GetMouseWheelScrollLinesOrChars(bool useCache, bool isHorizontalMouseWheel) noexcept
{
    if (useCache)
    {
        if (isHorizontalMouseWheel)
        {
            if (s_hasMouseWheelScrollCharsCache)
            {
                return s_mouseWheelScrollChars;
            }
        }
        else
        {
            if (s_hasMouseWheelScrollLinesCache)
            {
                return s_mouseWheelScrollLines;
            }
        }
    }

    LPCWSTR subKey = L"Control Panel\\Desktop";
    LPCWSTR value = isHorizontalMouseWheel ? L"WheelScrollChars" : L"WheelScrollLines";
    WCHAR keyValue[16];
    DWORD keySize = sizeof(keyValue);
    int32_t mouseWheelScrollLinesOrChars = isHorizontalMouseWheel ? s_defaultMouseWheelScrollChars : s_defaultMouseWheelScrollLines;

    if (SUCCEEDED(HRESULT_FROM_WIN32(::RegGetValueW(HKEY_CURRENT_USER, subKey, value, RRF_RT_REG_SZ, nullptr, &keyValue, &keySize))))
    {
        try
        {
            mouseWheelScrollLinesOrChars = std::stoi(keyValue);
        }
        catch (const std::invalid_argument&)
        {
        }
    }

    if (isHorizontalMouseWheel)
    {
        s_hasMouseWheelScrollCharsCache = true;
        s_mouseWheelScrollChars = mouseWheelScrollLinesOrChars;
    }
    else
    {
        s_hasMouseWheelScrollLinesCache = true;
        s_mouseWheelScrollLines = mouseWheelScrollLinesOrChars;
    }

    return mouseWheelScrollLinesOrChars;
}
