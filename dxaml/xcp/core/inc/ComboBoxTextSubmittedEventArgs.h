// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include <Activators.g.h>
#include "EventArgs.h"
#include "Microsoft.UI.Xaml.h"

class CComboBoxTextSubmittedEventArgs final : public CEventArgs
{
public:
    CComboBoxTextSubmittedEventArgs(_In_ HSTRING text)
    {
        xstring_ptr::CloneRuntimeStringHandle(text, &m_text);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override
    {
        IFC_RETURN(DirectUI::OnFrameworkCreateComboBoxTextSubmittedEventArgs(this, ppPeer));
        return S_OK;
    }

    bool GetHandled() const
    {
        return m_handled;
    }

    HRESULT get_Handled(_In_ BOOLEAN *pValue) const
    {
        *pValue = m_handled;
        return S_OK;
    }

    HRESULT put_Handled(_In_ BOOLEAN value)
    {
        m_handled = !!value;
        return S_OK;
    }

    HRESULT get_Text(_Out_ xstring_ptr *pValue) const
    {
        *pValue = m_text.Clone();
        return S_OK;
    }

private:
    bool m_handled = false;
    xstring_ptr m_text;
};

