// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDownloadRequest;
class CAbortableAsyncDownload;

//------------------------------------------------------------------------
//
//  interface:  IAsyncDownloadsRequestManager
//
//  Synopsis: Interface to manage async download requests
//
//------------------------------------------------------------------------

class IAsyncDownloadRequestManager
{
public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;

    virtual _Check_return_ HRESULT QueueRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction) = 0;

    virtual _Check_return_ HRESULT RemoveRequest(
        _In_ CDownloadRequest *pDownloadRequest) = 0;

    virtual _Check_return_ HRESULT DownloadRequests() = 0;

    virtual _Check_return_ HRESULT RemoveAbortableAsyncDownload(
        _In_ CAbortableAsyncDownload *pAbortableAsyncDownload) = 0;

    virtual void AbortAsync(
        _In_ IPALAbortableOperation *abortable) = 0;

    virtual bool IsEnabled() = 0;
    virtual void SetIsEnabled(_In_ bool bIsEnabled) = 0;

};

//------------------------------------------------------------------------
//
//  interface:  IAsyncDownloadsRequestManagerSite
//
//  Synopsis: Site for IAsyncDownloadsRequestManager
//
//------------------------------------------------------------------------

class IAsyncDownloadRequestManagerSite
{
public:
    virtual _Check_return_ HRESULT ProcessAsyncDownloadRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction) = 0;

    virtual _Check_return_ HRESULT StartAsyncDownloadTrigger() = 0;

    virtual void AbortAsync(
        _In_ IPALAbortableOperation *abortable) = 0;
};

//------------------------------------------------------------------------
//
//  class:  CAsyncDownloadRequestManager
//
//  Synopsis:
//      Urlmon will sometimes bind synchronously, even if asked to bind
//  asynchronously. This typically occurs when content to be downloaded
//  is small or already in the cache. Such a synchronous download can cause
//  unexpected reentrancy in the client that requested the async download.
//  CAsyncDownloadRequestManager forces async by queuing download requests
//  and processing the request when an async trigger is fired.
//
//------------------------------------------------------------------------

class CAsyncDownloadRequestManager final :
            public IAsyncDownloadRequestManager
{
public:
    static _Check_return_ HRESULT Create(
        _In_ IAsyncDownloadRequestManagerSite*  pSite,
        _Out_ IAsyncDownloadRequestManager** ppManager);

    // IAsyncDownloadsRequestManager
    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT QueueRequest(
        _In_ IDownloader*                           pDownloader,
        _In_ IPALUri*                               pUriAbsolute,
        _In_ IPALDownloadResponseCallback*          pCallback,
        _In_ XINT32                                 fCrossDomain,
        _In_ XINT32                                 fRemoveHttpsLock,
        _Outptr_result_maybenull_ IPALAbortableOperation**    ppIAbortableDownload,
        _In_ XUINT32                                eUnsecureDownloadAction) override;

    _Check_return_ HRESULT RemoveRequest(
        _In_ CDownloadRequest *pDownloadRequest) override;

    _Check_return_ HRESULT DownloadRequests() override;

    _Check_return_ HRESULT RemoveAbortableAsyncDownload(
        _In_ CAbortableAsyncDownload *pAbortableAsyncDownload) override;

    void AbortAsync(
        _In_ IPALAbortableOperation *abortable) override;

    bool IsEnabled() override
    {
        return m_bIsEnabled;
    }
    void SetIsEnabled(_In_ bool bIsEnabled) override
    {
        m_bIsEnabled = bIsEnabled;
    }

private:
    CAsyncDownloadRequestManager();
    ~CAsyncDownloadRequestManager();

    _Check_return_ HRESULT Init(_In_ IAsyncDownloadRequestManagerSite*  pSite);

private:
    // Reference count
    XUINT32                 m_cRef;

    // Site
    IAsyncDownloadRequestManagerSite * m_pSite;

    // Queue which holds async download requests
    IPALQueue * m_pDownloadRequestQueue;

    // Are queued requests being downloaded?
    XINT32  m_bDownloadingRequests;

    // List which holds current abortable async downloads
    CXcpList<CAbortableAsyncDownload>* m_pAbortableAsyncDownloadList;

    bool m_bIsEnabled;
};

template<>
void CXcpList<CAbortableAsyncDownload>::DeleteItem(CAbortableAsyncDownload* pData);

