// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "SplitView.h"

class CSplitViewPaneClosingEventArgs final : public CEventArgs
{
public:
    CSplitViewPaneClosingEventArgs()
        : m_cancel(false)
    {
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // Cancel property.
    _Check_return_ HRESULT get_Cancel(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = static_cast<BOOLEAN>(m_cancel);
        return S_OK;
    }

    _Check_return_ HRESULT put_Cancel(BOOLEAN value)
    {
        m_cancel = !!value;
        return S_OK;
    }

public:
    // Public fields
    bool m_cancel;
};
