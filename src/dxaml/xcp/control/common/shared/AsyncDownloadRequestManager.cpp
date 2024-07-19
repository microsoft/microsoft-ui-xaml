// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Function: CAsyncDownloadRequestManager::CAsyncDownloadRequestManager
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------

CAsyncDownloadRequestManager::CAsyncDownloadRequestManager()
{
    m_cRef = 1;
    XCP_WEAK(&m_pSite);
    m_pSite = NULL;
    m_pDownloadRequestQueue = NULL;
    m_pAbortableAsyncDownloadList = NULL;
    m_bDownloadingRequests = FALSE;
    m_bIsEnabled = FALSE;
}

//------------------------------------------------------------------------
//
//  Function: CAsyncDownloadRequestManager::~CAsyncDownloadRequestManager
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------

CAsyncDownloadRequestManager::~CAsyncDownloadRequestManager()
{
    // notify abortables, because the manager only has a weak reference to the abortables, this
    // must be done before cleaning up the queue of download requests because it is possible that
    // a request has the final strong reference to one of the abortables in the list.
    if(m_pAbortableAsyncDownloadList)
    {
        CXcpList<CAbortableAsyncDownload>::XCPListNode* pNode = m_pAbortableAsyncDownloadList->GetHead();
        while (pNode)
        {
            if (pNode->m_pData)
            {
                //update the async downloads, as the manager is leaving
                pNode->m_pData->SetAsyncDownloadRequestManager(NULL);
                pNode->m_pData = NULL;
                //don't call Abort() on the downloads.  Let the download owners call it
                //don't delete it, just notify it
            }
            pNode = pNode->m_pNext;
        }

        delete m_pAbortableAsyncDownloadList;
    }
    m_pAbortableAsyncDownloadList = NULL;

    // Free download requests that have not yet been processed
    if (m_pDownloadRequestQueue)
    {
        CDownloadRequest *pDownloadRequest;
        while (m_pDownloadRequestQueue->Get((void**)&pDownloadRequest, 0) == S_OK)
        {
            //update the requests, as the manager is leaving
            pDownloadRequest->SetAsyncDownloadRequestManager(NULL);
            delete pDownloadRequest;
        }

        // Close deletes m_pDownloadRequestQueue
        VERIFYHR(m_pDownloadRequestQueue->Close());
    }
    m_pDownloadRequestQueue = NULL;
}

//------------------------------------------------------------------------
//
//  Function: CAsyncDownloadRequestManager::Create
//
//  Synopsis:
//      Creator
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CAsyncDownloadRequestManager::Create(
    _In_ IAsyncDownloadRequestManagerSite*  pSite,
    _Out_ IAsyncDownloadRequestManager** ppManager)
{
    HRESULT hr = S_OK;
    CAsyncDownloadRequestManager *pManager = NULL;

    pManager = new CAsyncDownloadRequestManager();

    IFC(pManager->Init(pSite));

    *ppManager = static_cast<IAsyncDownloadRequestManager *>(pManager);
    pManager = NULL;

Cleanup:
    delete pManager;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::Init
//
//  Synopsis:
//      Initializer
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CAsyncDownloadRequestManager::Init(
    _In_ IAsyncDownloadRequestManagerSite*  pSite)
{
    HRESULT hr = S_OK;

    m_pSite = pSite;
    ASSERT(m_pSite);

    // Create queue for async download requests
    IFC(gps->QueueCreate(&m_pDownloadRequestQueue));

    m_pAbortableAsyncDownloadList = new CXcpList<CAbortableAsyncDownload>();

    m_bIsEnabled = TRUE;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CAsyncDownloadRequestManager::AddRef
//
//  Synopsis:
//     Increments reference count
//
//-------------------------------------------------------------------------
XUINT32  CAsyncDownloadRequestManager::AddRef()
{
    return ++m_cRef;
}

//-------------------------------------------------------------------------
//
//  Function:   CAsyncDownloadRequestManager::Release
//
//  Synopsis:
//     Decrements reference count
//
//-------------------------------------------------------------------------
XUINT32 CAsyncDownloadRequestManager::Release()
{
    XUINT32 cRef = --m_cRef;

    if (!cRef)
    {
        delete this;
    }

    return cRef;
}

//-------------------------------------------------------------------------
//
//  Function:   CAsyncDownloadRequestManager::QueueRequest
//
//  Synopsis:
//     Queue Request. Params correspond to IDownloaderSite::CreateDownloadRequest
//
//-------------------------------------------------------------------------

_Check_return_
HRESULT CAsyncDownloadRequestManager::QueueRequest(
    _In_ IDownloader*                           pDownloader,
    _In_ IPALUri*                               pUriAbsolute,
    _In_ IPALDownloadResponseCallback*          pCallback,
    _In_ XINT32                                 fCrossDomain,
    _In_ XINT32                                 fRemoveHttpsLock,
    _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
    _In_ XUINT32                                eUnsecureDownloadAction)
{
    HRESULT hr = S_OK;
    CDownloadRequest *pDownloadRequest = NULL;

    // Create download request
    IFC(CDownloadRequest::Create(pDownloader, pUriAbsolute,
            pCallback, fCrossDomain, fRemoveHttpsLock, ppIAbortableDownload, eUnsecureDownloadAction,
            this, m_pSite, &pDownloadRequest));

    // Queue request
    m_pDownloadRequestQueue->Post(pDownloadRequest);
    pDownloadRequest = NULL;

    // Start async trigger. When the trigger fires, DownloadRequests()
    // will be called, which will download the queued request
    IFC(m_pSite->StartAsyncDownloadTrigger());

Cleanup:
    delete pDownloadRequest;

    if (FAILED(hr))
    {
        if (ppIAbortableDownload)
        {
            ReleaseInterface(*ppIAbortableDownload);
        }
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CAsyncDownloadRequestManager::RemoveRequest
//
//  Parameters:
//      pDownloadRequest - Request to be removed
//
//  Synopsis:
//     Remove Request from queue
//
//-------------------------------------------------------------------------

_Check_return_
HRESULT CAsyncDownloadRequestManager::RemoveRequest(
    _In_ CDownloadRequest *pDownloadRequest)
{
    if (m_pDownloadRequestQueue != NULL)
    {
        IFC_RETURN(m_pDownloadRequestQueue->Remove(pDownloadRequest));
        delete pDownloadRequest;
    }
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CAsyncDownloadRequestManager::DownloadRequests
//
//  Synopsis:
//     Download all queued requests
//
//-------------------------------------------------------------------------

_Check_return_
HRESULT CAsyncDownloadRequestManager::DownloadRequests()
{
    HRESULT hr = S_OK;
    CDownloadRequest* pDownloadRequest = NULL;
    CAbortableAsyncDownload *pAbortableDownload = NULL;

    // Skip processing on reentrancy
    if (m_bDownloadingRequests)
    {
        RRETURN(hr);
    }

    m_bDownloadingRequests = TRUE;

    // Download each queued request
    while (m_pDownloadRequestQueue->Get((void**)&pDownloadRequest, 0) == S_OK)
    {
        IFC(pDownloadRequest->Download());
        IFCEXPECT_ASSERT(IsEnabled());
        pAbortableDownload = pDownloadRequest->GetAbortableAsyncDownload();
        if(pAbortableDownload)
        {
            if (pAbortableDownload->GetAsyncDownloadRequestManager() == this)
            {
                m_pAbortableAsyncDownloadList->Add(pAbortableDownload);
            }
            else
            {
                // The AAD does not have a pointer to us, so do not add to our list. If we did,
                // we could end up holding a dangling pointer since the AAD can't remove itself
                // from our list when it is destroyed.

                // Make sure the AAD is not holding a soft ref to any ADRM, since there won't
                // be any way for the soft ref to be removed (we are not adding it to our list).
                pAbortableDownload->SetAsyncDownloadRequestManager(NULL);

                // Assert here to help find a stress/Watson bug (see Silverlight bug 26749).
                ASSERT(FALSE);
            }
        }
        delete pDownloadRequest;
        pDownloadRequest = NULL;
    }

Cleanup:
    delete pDownloadRequest;
    m_bDownloadingRequests = FALSE;
    RRETURN(hr);
}

_Check_return_
HRESULT CAsyncDownloadRequestManager::RemoveAbortableAsyncDownload(_In_ CAbortableAsyncDownload* pAbortableAsyncDownload)
{
    HRESULT hr = S_OK;

    if (m_pAbortableAsyncDownloadList != NULL)
    {
        IFC(m_pAbortableAsyncDownloadList->Remove(pAbortableAsyncDownload, FALSE /*bDoDelete*/));
        if (S_FALSE == hr)
        {
            // Fail if we did not find the passed-in AAD in our list.
            IFC(E_FAIL);
        }
    }

Cleanup:
    RRETURN(hr);
}

void CAsyncDownloadRequestManager::AbortAsync(
    _In_ IPALAbortableOperation *abortable)
{
    m_pSite->AbortAsync(abortable);
}

template<>
void CXcpList<CAbortableAsyncDownload>::DeleteItem(CAbortableAsyncDownload* pData)
{
    // Specialize this template method so that delete is a no-op.
    //
    // The default template uses the delete operator, but CAbortableAsyncDownload
    // does not have a public dtor, so the default template does not compile.
    //
    // It's OK for this to be a no-op, since we never delete CAbortableAsyncDownload
    // through a CXcpList<CAbortableAsyncDownload>.
    ASSERT(pData == NULL);
}


