// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wil\resource.h>
#include <atomic>

class ThreadedJobQueue;

class CNULLDownloadResponse final : public IPALDownloadResponse
{
public:
    CNULLDownloadResponse(_In_opt_ IPALResource* pResource, _In_ IPALMemory *pPALMemory)
    {
        m_cRef = 1;
        m_pPalMemory = pPALMemory;
        m_pStream = NULL;
        m_pResource = pResource;
        AddRefInterface(m_pPalMemory);
        AddRefInterface(pResource);
    }

    ~CNULLDownloadResponse()
    {
        ReleaseInterface(m_pStream);
        ReleaseInterface(m_pPalMemory);
        ReleaseInterface(m_pResource);
    }

    XUINT32 AddRef() override
    {
        return ++m_cRef;
    }
    XUINT32 Release() override
    {
        XDWORD ul = --m_cRef;
        if (ul == 0) delete this;
        return ul;
    }

    // IPALDownloadResponse
    _Check_return_ HRESULT Lock( _Outptr_ IPALMemory** ppIMemory ) override
    {
        *ppIMemory = m_pPalMemory;
        AddRefInterface(m_pPalMemory);

        return S_OK;
    }

    _Check_return_ HRESULT Unlock() override
    {
        return S_OK;
    }

    _Check_return_ HRESULT GetStream( _Outptr_ IPALStream** ppStream ) override
    {
        HRESULT hr = S_OK;
        *ppStream = NULL;

        if (NULL == m_pStream)
        {
            IFC(gps->CreateIPALStreamFromIPALMemory(m_pPalMemory, &m_pStream));
        }

        AddRefInterface(m_pStream);
        *ppStream = m_pStream;

Cleanup:
        return hr;
    }

    _Check_return_ HRESULT GetResource(_Outptr_result_maybenull_ IPALResource** ppResource) override
    {
        *ppResource = m_pResource;
        AddRefInterface(*ppResource);
        return S_OK;
    }

private :
    XDWORD m_cRef;
    IPALStream *m_pStream;
    IPALMemory *m_pPalMemory;
    IPALResource *m_pResource;
};
class CDownloader : public CXcpObjectBase<IDownloader, CXcpObjectThreadSafeAddRefPolicy>
{
protected:
    CDownloader();
    ~CDownloader() override;

public:

   _Check_return_ HRESULT static Create(_Outptr_ CDownloader **ppDownloader,
                         _In_ IDownloaderSite* pDownloaderSite,
                         _In_ CCoreServices* pCore );

   void EnableCrossDomainDownloads() { m_fAllowCrossDomain = TRUE; }

// IDownloader

    // This member is used for cases where there are legitimate exceptions
    // to site-of-origin download restriction.  If you make a call to this
    // member, comment the call as to why the exception is valid and secure.
    HRESULT _Check_return_ CreateUnsecureDownloadRequest(
                            _In_ IPALDownloadRequest* pRequest,
                            _Outptr_opt_ IPALAbortableOperation **ppIAbortableDownload,
                            _In_opt_ IPALUri *pPreferredBaseUri = NULL) override;


    HRESULT _Check_return_ ContinueAllDownloads( _Out_ XINT32 * pfContinue ) override;

    void ResetDownloader() override;

    HRESULT _Check_return_ StopAllDownloads() override;

    HRESULT _Check_return_ CheckUri(
                           _In_ const xstring_ptr& strRelativeUri,
                           _In_ XUINT32 eUnsecureDownloadAction,
                           _Out_opt_ XINT32 *pfCrossDomain = NULL,
                           _Out_opt_ XINT32 *pfRemoveSecurityLock = NULL) override;

    HRESULT _Check_return_ CheckUri(
                           _In_ IPALUri* pUriRelative,
                           _In_ XUINT32 eUnsecureDownloadAction,
                           _Out_opt_ XINT32* pfCrossDomain = NULL,
                           _Out_opt_ XINT32* pfRemoveSecurityLock = NULL) override;

    _Check_return_ HRESULT CreateUnsecureDownloadRequest(
                                _In_ IPALUri* pAbsoluteUri,
                                _In_ IPALDownloadRequest* pRequestInfo,
                                _Outptr_opt_ IPALAbortableOperation ** ppIAbortableDownload,
                                bool bCheckForPolicyExpiration = true) override;

    ThreadedJobQueue& GetJobQueue() override;

protected :

    IDownloaderSite* m_pDownloaderSite = nullptr;
    XUINT32 m_cRef = 1;
    XINT32 m_fAllowCrossDomain = FALSE;
    CCoreServices* m_pCore = nullptr;

    // This variable may be written/read on different threads since CWindowsDownloadRequest
    // has it's own threading model now.  Therefore it is protected by a simple atomic for
    // well defined interlocked modifications.  It's also fairly cheap and deadlock safe.
    std::atomic<XINT32> m_fStopDownloads = FALSE;

    wil::critical_section m_Lock;
    DWORD m_mainThreadId = 0;

    wistd::unique_ptr<ThreadedJobQueue> m_threadedJobQueue;

    enum DownloadScheme
    {
        dsHTTP, dsHTTPS, dsFILE, dsXGADGET, dsOFFLINE, dsUNKNOWN
    };
};

