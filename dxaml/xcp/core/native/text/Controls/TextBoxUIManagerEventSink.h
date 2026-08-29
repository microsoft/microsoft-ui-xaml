// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ctffunc.h>

class CTextBoxBase;

// Forwards IUIManagerEventSink events to the event manager on behalf of the CTextBoxBase control.
class TextBoxUIManagerEventSink
    : public IUIManagerEventSink
{
public:
    TextBoxUIManagerEventSink(_In_ CTextBoxBase *controlSource);

    void UnadviseControl();

    // IUIManagerEventSink
    IFACEMETHODIMP OnWindowOpening(_In_ RECT *prcBounds) override;
    IFACEMETHODIMP OnWindowOpened(_In_ RECT *prcBounds) override;
    IFACEMETHODIMP OnWindowUpdating(_In_ RECT *prcUpdatedBounds) override;
    IFACEMETHODIMP OnWindowUpdated(_In_ RECT *prcUpdatedBounds) override;
    IFACEMETHODIMP OnWindowClosing() override;
    IFACEMETHODIMP OnWindowClosed() override;

    // IUnknown
    IFACEMETHOD(QueryInterface)(_In_ REFIID riid, _COM_Outptr_ void **ppv) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

private:
    _Check_return_ HRESULT RaiseCandidateWindowBoundsChangedEventForScreenRect(_In_ RECT *prcBounds);

    unsigned int m_refCount;
    CTextBoxBase *m_controlSource;
};

