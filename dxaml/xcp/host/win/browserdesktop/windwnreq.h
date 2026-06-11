// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Windows specific code to download.
//      Does all platform specific downloading, and all interaction with Urlmon

#pragma once

#include <urlmon.h>
#include <mutex>
#include <wil\resource.h>

class ThreadedJobQueueDeferral;

#if DBG == 1
enum RequestState
{
    Initialized,
    AsyncRequestBegun,
    SyncRequestBegun,
    GotResponse,
    ResponseComplete,
    TransmittedResponse,
    Done
};
#endif

#define ACCEPT_LANGUAGE_HEADER_PREFIX L"Accept-Language:"
#define ACCEPT_LANGUAGE_HEADER_PREFIX_LENGTH 16
#define MAX_LOCALE_COMPONENT_LENGTH 9
#define MAX_LOCALE_LENGTH (2 * MAX_LOCALE_COMPONENT_LENGTH)

class CWindowsDownloadResponse final : public IPALDownloadResponse
{
public:
    CWindowsDownloadResponse();
    ~CWindowsDownloadResponse();

    HRESULT Initialize ();

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    // IPALDownloadResponse
    HRESULT Lock( _Outptr_ IPALMemory** ppIMemory ) override;
    HRESULT Unlock() override;
    HRESULT GetStream( _Outptr_ IPALStream** ppStream ) override;

    HRESULT SetInputStream(_In_opt_ IPALStream* pStream);

    HRESULT GetResource(_Outptr_result_maybenull_ IPALResource** ppResource) override
    {
        *ppResource = NULL;
        return S_OK;
    }


private :
    DWORD m_cRef;
    IPALStream* m_pStream;
};

class CWinDataStream;
class CWinDataStreamBuffer;

class CWindowsDownloadRequest final
    : public IBindStatusCallback
    , public IInternetBindInfo
    , public IAuthenticate
    , public IHttpNegotiate
{
public :
    ~CWindowsDownloadRequest();
    CWindowsDownloadRequest(XUINT32  eUnsecureDownloadAction, bool bSendRefererHeader);
    void Initialize(XUINT32  eUnsecureDownloadAction, bool bSendRefererHeader);
    // Actual call from implementor of IHalServices.
    HRESULT InitiateRequest (  _In_ IDownloader* pDownload,
                                 _In_ IUnknown* pUnkContainer,
                                 _In_ IPALUri* pUriAbsolute,
                                 _In_ XINT32 fCrossDomain,
                                 _In_ IPALDownloadResponseCallback* pICallback,
                                 bool suppressCredentialsDialog);


   // IUnknown methods
    _Check_return_ STDMETHOD(QueryInterface)( _In_ REFIID riid, _Outptr_ void ** ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IBindStatusCallback methods
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, _In_ IBinding* pbinding);
    STDMETHOD(GetPriority)(_In_ LONG* pnPriority);
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnProgress)(
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        _In_reads_opt_(MAX_PATH) LPCWSTR pwzStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hrResult, _In_reads_opt_(MAX_PATH) LPCWSTR szError);
    STDMETHOD(GetBindInfo)(_Out_ DWORD* pgrfBINDF, _Inout_ BINDINFO* pbindinfo);
    STDMETHOD(OnDataAvailable)(
        DWORD grfBSCF,
        DWORD dwSize,
        _In_opt_ FORMATETC *pfmtetc,
        _In_     STGMEDIUM* pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown* punk);

    // IInternetBindInfo methods
    STDMETHOD(GetBindString)(
        _In_ ULONG stringType,
        _Inout_ LPOLESTR* bindString,
        _In_ ULONG inCount,
        _Inout_ ULONG* outCount);

    // IAuthenticate methods
    STDMETHOD(Authenticate)(_Out_ HWND * phwnd,
                            _Out_writes_(1) LPWSTR * pwszUser,
                            _Out_writes_(1) LPWSTR * pwszPassword);

    // IHttpNegotiate methods
    STDMETHOD(BeginningTransaction)(
        _In_z_ LPCWSTR szURL,
        _In_z_ LPCWSTR szHeaders,
        DWORD dwReserved,
        _Outptr_result_z_ LPWSTR *pszAdditionalHeaders
    );

    STDMETHOD(OnResponse)(
        DWORD dwResponseCode,
        _In_z_ LPCWSTR szResponseHeaders,
        _In_z_ LPCWSTR szRequestHeaders,
        _Outptr_result_z_ LPWSTR *pszAdditionalRequestHeaders
    );

    void Abort();

    // Functions to aid in download optimizations
    XINT32 IsActive();
    void AbortRequest();

    XUINT32 GetUnsecureDownloadAction()
    {
        return m_eUnsecureDownloadAction;
    }

    void SetRequestCompleted(bool fRequestCompeleted)
    {
        m_fRequestCompleted = fRequestCompeleted;
    }

    bool GetRequestCompleted()
    {
        return m_fRequestCompleted;
    }
private :

    HRESULT SetCachePath(__in_ecount_z(cszPath) LPCWSTR szPath, XUINT32 cszPath );
    HRESULT _Download( _In_ IPALUri * pUriAbsolute,
                       _Inout_opt_ IUnknown* pUnkContainer );

    HRESULT CreateResponse(HRESULT hrStatus, _Outptr_result_maybenull_ CWindowsDownloadResponse** ppResponse );

    void SetStream(IStream * pStream)
    {
        ReleaseInterface(m_pStream);
        m_pStream = pStream;
        AddRefInterface(m_pStream);
    }

    _Check_return_ HRESULT InitializeDataBuffer(DWORD cbCurrentlyAvailable);
    _Check_return_ HRESULT SetupDataStreams();
    _Check_return_ HRESULT FireGotData();
    _Check_return_ HRESULT FireGotResponse(HRESULT xrStatus);
    _Check_return_ HRESULT ReadStreamPull();
    bool IsSuccessHttpStatusCode(XUINT32 ulStatusCode);

    _Check_return_ HRESULT GetInternetExplorerAcceptLanguageString(_Out_ xstring_ptr* pstrAcceptLanguageString);
    _Check_return_ HRESULT GetWindowsAcceptLanguageString(_Out_ xstring_ptr* pstrAcceptLanguageString);

    //
    // State/Lifetime management & callback to owner.
    //
    DWORD     m_cRef;
    IDownloader* m_pDownloader;
    IPALDownloadResponseCallback* m_pDownloadResponseCallback;

    //
    // Required for managing and storing the results of a bind.
    //
    IBinding * m_pBinding;
    IStream* m_pStream;
    HRESULT  m_xResult ;
    wchar_t m_szPath[MAX_PATH + 1];
    DWORD m_nBindFlags;
    XUINT32  m_eUnsecureDownloadAction;
    XUINT64 m_length;
    XUINT64 m_streamLength;
    IPALStream* m_pDataStream;
    CWinDataStreamBuffer* m_pDataBuffer;
    XINT32 m_bDead;
    XINT32 m_bAborting;
    XUINT32 m_cUrl;
    _Field_size_(m_cUrl) XCHAR* m_pUrl;

    XUINT32 m_cHeaders;
    XCHAR* m_pHeaders;

    xstring_ptr m_strFinalUrl;
    xstring_ptr m_strHeaders;

    XUINT8* m_pTempBuffer;
    XUINT32 m_ulHttpStatusCode;

#if DBG == 1
    RequestState m_eRequestState;
#endif

    bool m_fRequestCompleted;

    bool m_bSendRefererHeader;

    // Generally this object is thread-safe since it is initialized and then handed off to work asynchronously.
    // However, it is abortable by the UI thread so all binding callback methods and the Abort methods need to
    // be mutex protected.
    std::recursive_mutex m_asyncMutex;

    // Keep-alive deferral object to ensure the message pump keeps messages flowing for urlmon
    wistd::unique_ptr<ThreadedJobQueueDeferral> m_threadedJobQueueDeferral;

    HWND m_hwnd;
};

class CWindowsAbortableDownload final : public IPALAbortableOperation
{
public:
    _Check_return_ static HRESULT Create(
        _In_opt_ CWindowsDownloadRequest* pWindowsDownloadRequest,
        _Out_opt_ CWindowsAbortableDownload** ppAbortableDownload);

    void Abort() override;
    bool SupportsAbortAsync() const override { return true; }
    void AbortAsync(_In_ IAsyncDownloadRequestManager* manager) override;

    XUINT32 AddRef() override;
    XUINT32 Release() override;

protected:
    CWindowsAbortableDownload()
    : m_cRef(1)
    , m_pDownloadRequest(NULL)
    {
    }

    _Check_return_ HRESULT SetDownloadRequest(
        _In_opt_ CWindowsDownloadRequest *pDownloadRequest);

private:
    ~CWindowsAbortableDownload() override
    {
        ReleaseInterface(m_pDownloadRequest);
    }

    XUINT32                  m_cRef;
    CWindowsDownloadRequest *m_pDownloadRequest;
};

