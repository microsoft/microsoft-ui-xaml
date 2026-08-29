// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "EventArgs.h"

class CFocusManagerGotFocusEventArgs : public CEventArgs
{
public:
    CFocusManagerGotFocusEventArgs(
        _In_ CDependencyObject* newFocusedElement,
        _In_ GUID correlationId
    ) :
    m_newFocusedElement(newFocusedElement),
    m_correlationId(correlationId)
    {  }
    
    CFocusManagerGotFocusEventArgs() {  }
    
    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_NewFocusedElement(_Outptr_ CDependencyObject** newFocusedElement);
    _Check_return_ HRESULT get_CorrelationId(_Out_ GUID* correlationId);

private:
    xref_ptr<CDependencyObject> m_newFocusedElement;
    GUID m_correlationId = {};
};