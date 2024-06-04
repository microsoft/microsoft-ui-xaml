// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextContextMenuCommandHandler.h"

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
TextContextMenuCommandHandler::TextContextMenuCommandHandler() :
    m_refCount(1)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
TextContextMenuCommandHandler::~TextContextMenuCommandHandler()
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE TextContextMenuCommandHandler::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void **ppObject
    )
{
    HRESULT hr = E_NOINTERFACE;
    *ppObject = NULL;

    if (riid == IID_IUnknown || riid == wup::IID_IUICommandInvokedHandler)
    {
        *ppObject = static_cast<wup::IUICommandInvokedHandler*>(this);
    }
    else if (riid == __uuidof(wf::IAsyncOperationCompletedHandler<wup::IUICommand*>))
    {
        *ppObject = static_cast<wf::IAsyncOperationCompletedHandler<wup::IUICommand*>*>(this);
    }

    if (*ppObject != NULL)
    {
        AddRef();
        hr = S_OK;
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE TextContextMenuCommandHandler::AddRef()
{
    return ++m_refCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IUnknown override.
//
//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE TextContextMenuCommandHandler::Release()
{
    ASSERT(m_refCount != 0);
    ULONG refCount = --m_refCount;

    if (refCount == 0)
    {
        delete this;
    }

    return refCount;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the integer id assigned to an wup::IUICommand.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextContextMenuCommandHandler::GetCommandId(
    _In_ wup::IUICommand *pCommand,
    _Out_ XINT32 *pId
    )
{

    wrl::ComPtr<IInspectable> pObject;
    IFC_RETURN(pCommand->get_Id(&pObject));
    wrl::ComPtr<wf::IPropertyValue> pValue;
    IFC_RETURN(pObject.As(&pValue));

    IFC_RETURN(pValue->GetInt32(pId));

    return S_OK;
}
