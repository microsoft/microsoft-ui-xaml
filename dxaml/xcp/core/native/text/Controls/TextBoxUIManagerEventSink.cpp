// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxUIManagerEventSink.h"

using Microsoft::WRL::ComPtr;

TextBoxUIManagerEventSink::TextBoxUIManagerEventSink(_In_ CTextBoxBase *controlSource)
    : m_refCount(1)
    , m_controlSource(controlSource)
{
}

void TextBoxUIManagerEventSink::UnadviseControl()
{
    m_controlSource = nullptr;
}

#pragma region IUIManagerEventSink methods

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowOpening(_In_ RECT *prcBounds)
{
    IFC_RETURN(RaiseCandidateWindowBoundsChangedEventForScreenRect(prcBounds));
    return S_OK;
}

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowOpened(_In_ RECT *prcBounds)
{
    return S_OK;
}

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowUpdating(_In_ RECT *prcUpdatedBounds)
{
    IFC_RETURN(RaiseCandidateWindowBoundsChangedEventForScreenRect(prcUpdatedBounds));
    return S_OK;
}

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowUpdated(_In_ RECT *prcUpdatedBounds)
{
    return S_OK;
}

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowClosing()
{
    return S_OK;
}

IFACEMETHODIMP TextBoxUIManagerEventSink::OnWindowClosed()
{
    XRECTF empty {};
    IFC_RETURN(m_controlSource->RaiseCandidateWindowBoundsChangedEvent(empty));
    return S_OK;
}

#pragma endregion

#pragma region IUnknown methods

HRESULT TextBoxUIManagerEventSink::QueryInterface(
    _In_ REFIID riid,
    _COM_Outptr_ void **ppvObject)
{
    IFCPTR_RETURN(ppvObject);

    if (riid != IID_IUnknown &&
        riid != IID_IUIManagerEventSink)
    {
        *ppvObject = nullptr;
        IFC_RETURN(E_NOINTERFACE);
    }

    AddRef();
    *ppvObject = static_cast<IUIManagerEventSink*>(this);
    return S_OK;
}

ULONG TextBoxUIManagerEventSink::AddRef(void)
{
    return static_cast<ULONG>(InterlockedIncrement(&m_refCount));
}

ULONG TextBoxUIManagerEventSink::Release(void)
{
    unsigned int refCount = InterlockedDecrement(&m_refCount);
    if (0 == refCount)
        delete this;
    return static_cast<ULONG>(refCount);
}

#pragma endregion

_Check_return_ HRESULT TextBoxUIManagerEventSink::RaiseCandidateWindowBoundsChangedEventForScreenRect(_In_ RECT *prcBounds)
{
    HRESULT hr = S_OK;
    XPOINT ltView = { prcBounds->left, prcBounds->top };
    XPOINT rbView = { prcBounds->right, prcBounds->bottom };

    IFC_RETURN(m_controlSource->ScreenToTextBox(&ltView));
    IFC_RETURN(m_controlSource->ScreenToTextBox(&rbView));

    prcBounds->left = ltView.x;
    prcBounds->top = ltView.y;
    prcBounds->right = rbView.x;
    prcBounds->bottom = rbView.y;

    IFC_RETURN(m_controlSource->RaiseCandidateWindowBoundsChangedEventForRoot(prcBounds));

    RRETURN(hr);
}