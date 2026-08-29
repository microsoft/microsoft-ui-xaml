// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include <string>
#include "EventArgs.h"

class CAccessKeyDisplayRequestedEventArgs final : public CEventArgs
{
public:
    CAccessKeyDisplayRequestedEventArgs(const wchar_t* strPressedKeys)
        : m_strPressedKeys(strPressedKeys)
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_PressedKeys(_Out_ xstring_ptr* pstrPressedKeys) const
    {
        return xstring_ptr::CloneBuffer(m_strPressedKeys.c_str(),pstrPressedKeys);
    }

    _Check_return_ HRESULT put_PressedKeys(_In_ const xephemeral_string_ptr& strPressedKeys)
    {
        m_strPressedKeys = strPressedKeys.GetBuffer();
        return S_OK;
    }

private:
    std::wstring m_strPressedKeys;
};

