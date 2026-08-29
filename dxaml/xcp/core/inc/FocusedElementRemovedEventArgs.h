// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RoutedEventArgs.h"

class CFocusedElementRemovedEventArgs : public CEventArgs
{
public:
    CFocusedElementRemovedEventArgs(_In_ CDependencyObject* focusedElement, _In_ CDependencyObject* currentNextFocusableElement) :
        m_oldFocusedElement(focusedElement),
        m_newFocusedElement(currentNextFocusableElement) {}

    ~CFocusedElementRemovedEventArgs() override {}

    _Check_return_ HRESULT get_OldFocusedElement(_Outptr_ CDependencyObject** oldFocusedElement);
    _Check_return_ HRESULT get_NewFocusedElement(_Outptr_ CDependencyObject** newFocusedElement);
    _Check_return_ HRESULT put_NewFocusedElement(_In_ CDependencyObject* newFocusedElement);

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) final;

    CDependencyObject* GetNewFocusedElementNoRef() const
    {
        return m_newFocusedElement;
    }

    xref_ptr<CDependencyObject> m_oldFocusedElement;
    xref_ptr<CDependencyObject> m_newFocusedElement;
};