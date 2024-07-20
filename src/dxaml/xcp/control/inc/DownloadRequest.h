// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CAbortableAsyncDownload;

//------------------------------------------------------------------------
//
//  class:  CDownloadRequest
//
//  Synopsis:
//      Queued download request, which is used to always enforce
//  async download. 
//      Urlmon sometimes binds synchronously, even when asked to bind 
//  asynchronously, so the Windows Browser Host queues these requests
//  in IAsyncDownloadRequestManager as CDownloadRequests.
//
//------------------------------------------------------------------------
class CDownloadRequest
{
private:
    // IDownloaderSite::CreateDownloadRequest params
    IDownloader* m_pDownloader;
    IPALUri* m_pUriAbsolute;
    IPALDownloadResponseCallback* m_pCallback;
    XINT32 m_fCrossDomain;
    XINT32 m_fRemoveHttpsLock;
    CAbortableAsyncDownload* m_pAbortableDownload;
    XUINT32 m_eUnsecureDownloadAction;

    // Async Download Request Manager
    IAsyncDownloadRequestManager * m_pAsyncDownloadRequestManager;

    // Site of Async Download Request Manager
    IAsyncDownloadRequestManagerSite * m_pAsyncDownloadRequestManagerSite;

public:
    static _Check_return_ HRESULT Create(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction,
        _In_ IAsyncDownloadRequestManager*          pAsyncDownloadRequestManager,
        _In_ IAsyncDownloadRequestManagerSite *     pAsyncDownloadRequestManagerSite,
        _Out_ CDownloadRequest**                    pDownloadRequest);

    _Check_return_ HRESULT Download();

    void SetAsyncDownloadRequestManager(_In_opt_ IAsyncDownloadRequestManager* pManager);

    CAbortableAsyncDownload* GetAbortableAsyncDownload()
    {
        return m_pAbortableDownload;
    }

    ~CDownloadRequest();

private:
    CDownloadRequest();

    _Check_return_ HRESULT Init(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction,
        _In_ IAsyncDownloadRequestManager*          pAsyncDownloadRequestManager,
        _In_ IAsyncDownloadRequestManagerSite *     pAsyncDownloadRequestManagerSite);
    
};

