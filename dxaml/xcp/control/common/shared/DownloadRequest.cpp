// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::~CDownloadRequest
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------

CDownloadRequest::CDownloadRequest()
{
    XCP_STRONG(&m_pDownloader);
    m_pDownloader = NULL;
    m_pUriAbsolute = NULL;
    m_pCallback = NULL;
    m_pAbortableDownload = NULL;
    m_fRemoveHttpsLock = FALSE;
}

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::~CDownloadRequest
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------

CDownloadRequest::~CDownloadRequest()
{
    ReleaseInterface(m_pDownloader);
    ReleaseInterface(m_pUriAbsolute);
    ReleaseInterface(m_pCallback);
    ReleaseInterface(m_pAbortableDownload);
}

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::Create
//
//  Synopsis:
//      Creator. Most params are the same as those of
// IDownloaderSite::CreateDownloadRequest.
//
//------------------------------------------------------------------------

_Check_return_ 
HRESULT CDownloadRequest::Create(
    _In_ IDownloader*                           pDownloader,
    _In_ IPALUri*                               pUriAbsolute,
    _In_ IPALDownloadResponseCallback*          pCallback,
    _In_ XINT32                                 fCrossDomain,
    _In_ XINT32                                 fRemoveHttpsLock,
    _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
    _In_ XUINT32                                eUnsecureDownloadAction,
    _In_ IAsyncDownloadRequestManager*          pAsyncDownloadRequestManager,
    _In_ IAsyncDownloadRequestManagerSite *     pAsyncDownloadRequestManagerSite,
    _Out_  CDownloadRequest**                   ppDownloadRequest)
{
    HRESULT hr = S_OK;
    CDownloadRequest *pDownloadRequest = NULL;

    *ppDownloadRequest = NULL;

    pDownloadRequest = new CDownloadRequest();

    IFC(pDownloadRequest->Init(pDownloader, pUriAbsolute, pCallback,
        fCrossDomain, fRemoveHttpsLock, ppIAbortableDownload, eUnsecureDownloadAction,
        pAsyncDownloadRequestManager, pAsyncDownloadRequestManagerSite));
    
    *ppDownloadRequest = pDownloadRequest;
    pDownloadRequest = NULL;

Cleanup:
    delete pDownloadRequest;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::Init
//
//  Synopsis:
//      Initializer. Most params are the same as those of
// IDownloaderSite::CreateDownloadRequest.
//
//------------------------------------------------------------------------

_Check_return_ 
HRESULT CDownloadRequest::Init(
    _In_ IDownloader*                           pDownloader,
    _In_ IPALUri*                               pUriAbsolute,
    _In_ IPALDownloadResponseCallback*          pCallback,
    _In_ XINT32                                 fCrossDomain,
    _In_ XINT32                                 fRemoveHttpsLock,
    _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
    _In_ XUINT32                                eUnsecureDownloadAction,
    _In_ IAsyncDownloadRequestManager*          pAsyncDownloadRequestManager,
    _In_ IAsyncDownloadRequestManagerSite *     pAsyncDownloadRequestManagerSite)
{
    HRESULT hr = S_OK;
    IPALAbortableOperation * pIAbortableDownload = NULL;

    if (ppIAbortableDownload)
    {
        *ppIAbortableDownload = NULL;
    }

    m_pDownloader = pDownloader;
    AddRefInterface(pDownloader);
    
    m_pUriAbsolute = pUriAbsolute;
    AddRefInterface(pUriAbsolute);

    m_pCallback = pCallback;
    AddRefInterface(pCallback);
    
    m_fCrossDomain = fCrossDomain;
    m_fRemoveHttpsLock = fRemoveHttpsLock;

    // Return a IPALAbortableOperation wrapper which will later point to the 
    // real IPALAbortableOperation when the download request is processed.
    if (ppIAbortableDownload)
    {    
        IFC(CAbortableAsyncDownload::Create(this, pAsyncDownloadRequestManager, 
                        &m_pAbortableDownload));

        pIAbortableDownload = m_pAbortableDownload;
        pIAbortableDownload->AddRef();
    }

    m_eUnsecureDownloadAction = eUnsecureDownloadAction;

    // To prevent a cycle, don't AddRef m_pAsyncDownloadRequestManager,
    // because it is holding a ref on this
    m_pAsyncDownloadRequestManager = pAsyncDownloadRequestManager;
    m_pAsyncDownloadRequestManagerSite = pAsyncDownloadRequestManagerSite;

    if (ppIAbortableDownload)
    {
        *ppIAbortableDownload = pIAbortableDownload;
        pIAbortableDownload = NULL;
    }
    
Cleanup:
    ReleaseInterface(pIAbortableDownload);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::Download
//
//  Synopsis:
//      Download request
//
//------------------------------------------------------------------------

_Check_return_ 
HRESULT CDownloadRequest::Download()
{
    HRESULT hr = S_OK;
    
    IPALAbortableOperation* pAbortable = NULL;

    // Ask site to download
    IFC(m_pAsyncDownloadRequestManagerSite->ProcessAsyncDownloadRequest(
                            m_pDownloader, m_pUriAbsolute,
                            m_pCallback,m_fCrossDomain,m_fRemoveHttpsLock,
                            m_pAbortableDownload ? &pAbortable : NULL,
                            m_eUnsecureDownloadAction));

    // Set real IPALAbortableOperation into the wrapper
    if (m_pAbortableDownload && pAbortable)
    {
        IFC(m_pAbortableDownload->SetAbortable(pAbortable));
        m_pAbortableDownload->SetAsyncDownloadRequestManager(m_pAsyncDownloadRequestManager);
    }

Cleanup:
    ReleaseInterface(pAbortable);

    if (FAILED(hr))
    {
        if (m_pCallback)
        {
            m_pCallback->GotResponse(NULL, E_INVALIDARG);
        }
    }
    
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Function: CDownloadRequest::SetAsyncDownloadRequestManager
//
//  Synopsis:
//      update the AsyncDownloadRequestManager if it changes
//
//------------------------------------------------------------------------

  void CDownloadRequest::SetAsyncDownloadRequestManager(_In_opt_ IAsyncDownloadRequestManager* pManager)
  {
        m_pAsyncDownloadRequestManager = pManager;
        if(m_pAbortableDownload)
        {
            m_pAbortableDownload->SetAsyncDownloadRequestManager(pManager);
        }
  }

