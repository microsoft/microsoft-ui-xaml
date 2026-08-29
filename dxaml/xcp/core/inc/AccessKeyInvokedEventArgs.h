// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "EventArgs.h"

class CAccessKeyInvokedEventArgs final : public CEventArgs
{
public:
    CAccessKeyInvokedEventArgs() : m_bHandled(false)
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    bool GetHandled() const
    {
        return m_bHandled;
    }

    HRESULT get_Handled(_In_ BOOLEAN *pValue) const
    {
        (*pValue) = m_bHandled;
        return S_OK;
    }

    HRESULT put_Handled(_In_ BOOLEAN value)
    {
        m_bHandled = !!value;
        return S_OK;
    }

private:
    bool m_bHandled;
};

