// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  CAbortableAsyncDownload::Create
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CAbortableAsyncDownload::Create(
        _In_ CDownloadRequest* pDownloadRequest,
        _In_ IAsyncDownloadRequestManager* pAsyncDownloadRequestManager,
        _Outptr_ CAbortableAsyncDownload** ppAbortableDownload)
{
    HRESULT hr = S_OK;
    CAbortableAsyncDownload* pAbortableDownload = NULL;

    *ppAbortableDownload = NULL;

    pAbortableDownload = new CAbortableAsyncDownload;

    // To prevent a cycle, don't AddRef m_pAsyncDownloadRequestManager,
    // because it is holding a ref on this through CDownloadRequest
    pAbortableDownload->m_pAsyncDownloadRequestManager = pAsyncDownloadRequestManager;

    pAbortableDownload->m_pRequest = pDownloadRequest;

    *ppAbortableDownload = pAbortableDownload;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  CAbortableAsyncDownload::Abort
//
//  Synopsis:
//      Tell the Downloader to remove the download request from its queue
//
//------------------------------------------------------------------------
void
CAbortableAsyncDownload::Abort()
{
    //if there is an abortable download, we act on that
    //if there isn't, we're still in the manager's request queue, so
    //we need to ask it to remove us (if the manager still exists)
    if (m_pAbortable)
    {
        //if the manager still exists, then the BH exists
        //the abortable operations are reliant on BH still existing
        if(m_pAsyncDownloadRequestManager)
        {
            if (FAILED((m_pAsyncDownloadRequestManager->RemoveAbortableAsyncDownload(this))))
            {
                // Assert here to help find a stress/Watson bug (see Silverlight bug 26749).
                ASSERT(FALSE);
            }

            // RS5 Bug #14265396
            // On xbox, Abort can cause EM_WATCHDOG hangs due to the background thread getting blocked for long periods
            // of time trying to read a file from the optical drive.  The delay is likely due to the
            // user ejecting the optical drive just after the file read operation has started.
            // The fix is to move the Abort to a worker thread, freeing UI thread from the blockage.
            if (m_pAbortable->SupportsAbortAsync())
            {
                m_pAbortable->AbortAsync(m_pAsyncDownloadRequestManager);
            }
            else
            {
                m_pAbortable->Abort();
            }
        }
    }
    else if (m_pAsyncDownloadRequestManager)
    {
        // we still want to NULL our m_pAsyncDownloadRequestManager and the request in the
        // failure case.  This will only fail if the request is not
        // not found.
        IGNOREHR(m_pAsyncDownloadRequestManager->RemoveRequest(m_pRequest));
    }

    m_pAsyncDownloadRequestManager = NULL;
    m_pRequest = NULL;
    ReleaseInterface(m_pAbortable);
}

//------------------------------------------------------------------------
//
//  CAbortableAsyncDownload::AddRef
//
//------------------------------------------------------------------------
XUINT32
CAbortableAsyncDownload::AddRef()
{
    return ++m_cRef;
}

//------------------------------------------------------------------------
//
//  CAbortableAsyncDownload::Release
//
//------------------------------------------------------------------------
XUINT32
CAbortableAsyncDownload::Release()
{
    XUINT32 cRef = --m_cRef;
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//------------------------------------------------------------------------
//
//  CAbortableAsyncDownload::SetAbortable
//
//  Synopsis:
//      Provide abortable wrapper with an abortable and switch it to
//      use that when aborting
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CAbortableAsyncDownload::SetAbortable(_In_ IPALAbortableOperation* pAbortable)
{
    HRESULT hr = E_FAIL;

    IFCPTR(pAbortable);

    if (m_pAsyncDownloadRequestManager)
    {
        // No longer needed
        m_pRequest = NULL;
    }

    if (m_pAbortable == NULL)
    {
        m_pAbortable = pAbortable;
        m_pAbortable->AddRef();
        hr = S_OK;
    }
Cleanup:
    RRETURN(hr);
}


