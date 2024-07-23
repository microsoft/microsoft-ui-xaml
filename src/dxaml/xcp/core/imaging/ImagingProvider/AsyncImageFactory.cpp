// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AsyncImageWorkData.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a new asynchronous image factory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
AsyncImageFactory::Create(
    _In_ IPALWorkItemFactory *pWorkItemFactory,
    _In_ ImageTaskDispatcher* pDispatcher,
    _Outptr_ AsyncImageFactory** ppAsyncImageFactory
    )
{

    XCP_FAULT_ON_FAILURE(pWorkItemFactory);
    XCP_FAULT_ON_FAILURE(pDispatcher);

    xref_ptr<AsyncImageFactory> pNewFactory;
    pNewFactory.attach(new AsyncImageFactory());

    IFC_RETURN(pNewFactory->Initialize(pWorkItemFactory, pDispatcher));

    *ppAsyncImageFactory = pNewFactory.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new asynchronous image factory.
//
//------------------------------------------------------------------------
AsyncImageFactory::AsyncImageFactory(
    )
    : m_continueDecode(TRUE)
    , m_pDispatcher(NULL)
    , m_pWorkItemFactory(NULL)
{
    XCP_WEAK(&m_pDispatcher);
    XCP_STRONG(&m_pWorkItemFactory);    
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up asynchronous image factory.
//
//------------------------------------------------------------------------
AsyncImageFactory::~AsyncImageFactory(
    )
{
    VERIFYHR(Shutdown());
    ReleaseInterface(m_pWorkItemFactory);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intialize the image factory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
AsyncImageFactory::Initialize(
    _In_ IPALWorkItemFactory *pWorkItemFactory,
    _In_ ImageTaskDispatcher* pDispatcher
    )
{
    m_pDispatcher = pDispatcher;
    m_pWorkItemFactory = pWorkItemFactory;
    AddRefInterface(m_pWorkItemFactory);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Shut down the async image factory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
AsyncImageFactory::Shutdown(
)
{
    HRESULT hr = S_OK;

    if (m_continueDecode)
    {
        m_continueDecode = FALSE;
    }
    if (m_pWorkItemFactory != NULL)
    {
        IFC_RETURN(m_pWorkItemFactory->CancelAndCleanupAllWork());
    }
    
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static entry point for decode thread.
//  Matches the signature of PALWORKITEMCALLBACK required by IPALWorkItems
//
//------------------------------------------------------------------------
HRESULT 
AsyncImageFactory::WorkCallback(
    _In_opt_ IObject *pData
    )
{
    AsyncImageWorkData *pAsyncImageWorkData = reinterpret_cast<AsyncImageWorkData*>(pData);
    ASSERT(pAsyncImageWorkData);
    IFC_RETURN(pAsyncImageWorkData->m_pTask->Execute());

    return S_OK;
}
