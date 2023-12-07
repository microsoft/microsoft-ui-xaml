// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "EventArgs.h"

class CFocusManagerLostFocusEventArgs : public CEventArgs
{
public:
    CFocusManagerLostFocusEventArgs(
        _In_ CDependencyObject* oldFocusedElement,
        _In_ GUID correlationId
    ) :
    m_oldFocusedElement(oldFocusedElement),
    m_correlationId(correlationId)
    {  }

    CFocusManagerLostFocusEventArgs() {  }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_OldFocusedElement(_Outptr_ CDependencyObject** oldFocusedElement);

    _Check_return_ HRESULT get_CorrelationId(_Out_ GUID* correlationId);

private:
    xref_ptr<CDependencyObject> m_oldFocusedElement;
    GUID m_correlationId = {};
};