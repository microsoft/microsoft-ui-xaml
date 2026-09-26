// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    PARTIAL_CLASS(WinUIProcessShutdownCompletedEventArgs)
    {
    public:
        _Check_return_ HRESULT get_RequestDllUnloadImpl(_Out_ BOOLEAN* value)
        {
            *value = m_requestDllUnload;
            return S_OK;
        }

        _Check_return_ HRESULT put_RequestDllUnloadImpl(BOOLEAN value)
        {
            m_requestDllUnload = value;
            return S_OK;
        }

    private:
        BOOLEAN m_requestDllUnload{ false };
    };
}
