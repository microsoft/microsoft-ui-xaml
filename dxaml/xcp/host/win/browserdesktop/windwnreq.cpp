// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Windows specific code to download.
//      Does all platform specific downloading, and all interaction with Urlmon

#include "precomp.h"
#include <mutex>
#include <ThreadedJobQueue.h>
#include <wil\resource.h>
#include <MUX-ETWEvents.h>
#include <wininet.h>

#define CB_TEMP_BUFFER_SIZE 4096
#define HTTP_STATUS_CODE_NOT_SET 0

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Constructor
//
//-------------------------------------------------------------------------

CWindowsDownloadResponse::CWindowsDownloadResponse()
    : m_cRef( 1)
{
    m_pStream = NULL;
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Dtor
//
//-------------------------------------------------------------------------

CWindowsDownloadResponse::~CWindowsDownloadResponse()
{
    if (m_pStream)
        m_pStream->Release();
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Initialize
//
//  Synopsis:
//     Initialize an instance for use.
//
//-------------------------------------------------------------------------

HRESULT
CWindowsDownloadResponse::Initialize()
{
    HRESULT hr = S_OK ;

    RRETURN( hr ) ;
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::AddRef
//
//  Synopsis:
//    Increment ref-count
//
//-------------------------------------------------------------------------
XUINT32
CWindowsDownloadResponse::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Release
//
//  Synopsis:
//     Decrement ref-count
//
//-------------------------------------------------------------------------

XUINT32
CWindowsDownloadResponse::Release()
{
    ULONG ul = InterlockedDecrement(&m_cRef);

    if (ul == 0) delete this;

    return ul;
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Lock
//
//  Synopsis:
//     Return a memory buffer of the result.
//
//      Validates that lock hasn't been previously called ( m_pFile should be null)
//-------------------------------------------------------------------------
HRESULT
CWindowsDownloadResponse::Lock( _Outptr_ IPALMemory** ppMemory )
{
    HRESULT hr = S_OK;
    XUINT64 cSize;

    XUINT8 *pBuffer = NULL;

    IFCPTR( ppMemory );

    if (!m_pStream)
    {
        IFC(E_FAIL);
    }
    else
    {
        // If we were given a stream to use, then use that in place of a file.
        CBufferMemory *pMemory;

        XUINT32 cbRead;
        IFC(m_pStream->GetSize(&cSize));

        if (cSize > 0xffffffff)
        {
            cSize = 0xffffffff;
        }

        pBuffer = new(NO_FAIL_FAST) XUINT8[static_cast<XUINT32>(cSize)];
        IFCOOM(pBuffer);

        IFC(m_pStream->Read(pBuffer, static_cast<XUINT32>(cSize), &cbRead));
        if ( cbRead != cSize )
        {
            IFC(E_FAIL);
        }

        pMemory = new CBufferMemory(pBuffer, cbRead, TRUE /* OwnsBuffer */);

        // pMemory now has the buffer. Set pBuffer to NULL so that
        // buffer isn't deleted.
        pBuffer = NULL;

        *ppMemory = pMemory;
    }

Cleanup:
    if (pBuffer)
    {
        delete [] pBuffer;
        pBuffer = NULL;
    }
    RRETURN( hr );
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::Unlock
//
//  Synopsis:
//     Caller has done.
//
//-------------------------------------------------------------------------
HRESULT
CWindowsDownloadResponse::Unlock()
{
    HRESULT hr = S_OK;

    RRETURN( hr );
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::GetStream
//
//  Synopsis:
//     Return an IMFStream interface on the file.
//
//-------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)
HRESULT
CWindowsDownloadResponse::GetStream( _Outptr_ IPALStream** ppStream)
{
    HRESULT hr = S_OK;

    IFCPTR( ppStream );

    if (m_pStream)
    {
        m_pStream->AddRef();
        *ppStream = m_pStream;
    }
    else
    {
        IFC(E_FAIL);
    }

Cleanup :
    RRETURN( hr ) ;

}
#pragma warning(pop)

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadResponse::SetInputStream
//
//  Synopsis:
//     Set the response to take its data from a stream rather than a
//     file.  This is needed to workaround the small file download
//     bug in FF.  See bug 5171 for details.
//
//-------------------------------------------------------------------------
HRESULT
CWindowsDownloadResponse::SetInputStream(_In_opt_ IPALStream* pStream)
{
    HRESULT hr = S_OK;

    IFCPTR(pStream);

    if (m_pStream)
    {
        // This function should not be called more than once.
        IFC(E_UNEXPECTED);
    }

    m_pStream = pStream;
    m_pStream->AddRef();

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest constructor
//
//
//--------------------------------------------------------------------------
CWindowsDownloadRequest::CWindowsDownloadRequest(XUINT32  eUnsecureDownloadAction, bool bSendRefererHeader)
: m_cRef(1)
{
    XCP_STRONG(&m_pDownloader);
    Initialize(eUnsecureDownloadAction, bSendRefererHeader);
}
void CWindowsDownloadRequest::Initialize(XUINT32  eUnsecureDownloadAction, bool bSendRefererHeader)
{
    m_pBinding = NULL;
    m_pDownloader = NULL;
    m_pDownloadResponseCallback = NULL;
    m_xResult = S_OK ;
    m_szPath[0] = 0;
    m_pDataStream = NULL;
    m_length = 0;
    m_streamLength = 0;
    m_pDataBuffer = NULL;
    m_pStream = NULL;
    m_bDead = FALSE;
    m_bAborting = FALSE;
    m_cUrl = 0;
    m_pUrl = NULL;
#if DBG == 1
    m_eRequestState = Initialized;
#endif
    m_eUnsecureDownloadAction = eUnsecureDownloadAction;
    m_pHeaders = NULL;
    m_cHeaders = 0;
    m_pTempBuffer = NULL;
    m_ulHttpStatusCode = HTTP_STATUS_CODE_NOT_SET;
    m_fRequestCompleted = FALSE;
    m_bSendRefererHeader = bSendRefererHeader;
}
//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest destructor
//
//
//--------------------------------------------------------------------------
CWindowsDownloadRequest::~CWindowsDownloadRequest()
{
    // Abort in-progress downloads so there are no asynchronous callbacks after
    // the object is destroyed.
    Abort();

    ReleaseInterface(m_pBinding);
    ReleaseInterface(m_pDownloader);
    ReleaseInterface(m_pDownloadResponseCallback);
    ReleaseInterface(m_pDataStream);
    ReleaseInterface(m_pDataBuffer);
    if (m_pUrl)
    {
        delete [] m_pUrl;
        m_pUrl = NULL;
    }

    if (m_pHeaders)
    {
        delete [] m_pHeaders;
        m_pHeaders = NULL;
    }

    delete[] m_pTempBuffer;
}

#pragma warning(push)
#pragma warning(disable: 6387)
_Check_return_
STDMETHODIMP
CWindowsDownloadRequest::QueryInterface( _In_ REFIID riid, _Outptr_ void** ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppv = (IUnknown *) (IBindStatusCallback *) this;
    }
    else if (IsEqualIID(riid, IID_IBindStatusCallback)) {
        *ppv = (IBindStatusCallback *) this;
    }
    else if (IsEqualIID(riid, IID_IInternetBindInfo))
    {
        *ppv = static_cast<IInternetBindInfo*>(this);
    }
    else if (IsEqualIID(riid, IID_IAuthenticate)) {
        *ppv = (IAuthenticate *) this;
    }
    else if (IsEqualIID(riid, IID_IHttpNegotiate)) {
        *ppv = (IHttpNegotiate *) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
#pragma warning(pop)

STDMETHODIMP_(ULONG)
    CWindowsDownloadRequest::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
    CWindowsDownloadRequest::Release(void)
{
    ULONG ul = InterlockedDecrement(&m_cRef);

    if (ul == 0) delete this;

    return ul;
}


STDMETHODIMP
CWindowsDownloadRequest::GetPriority(_In_ LONG* pnPriority)
{
    if (pnPriority)
    {
        *pnPriority = THREAD_PRIORITY_NORMAL;
    }
    return S_OK;
}


STDMETHODIMP
CWindowsDownloadRequest::OnLowResource(DWORD dwReserved)
{
    return S_OK;
}


STDMETHODIMP
CWindowsDownloadRequest::OnProgress(ULONG ulProgress,  ULONG ulProgressMax, ULONG ulStatusCode, _In_reads_opt_(MAX_PATH) LPCWSTR szStatusText)
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);
    HRESULT hr = S_OK ;
    BOOL fContinue = FALSE;
    XUINT32 cString = 0;
    IPlatformUtilities* pUtilities = NULL;

    if (m_bDead || m_bAborting)
    {
        return E_FAIL;
    }

    IFCEXPECT_ASSERT(m_pDownloader);

    IFC( m_pDownloader->ContinueAllDownloads( & fContinue ) );

    if ( fContinue )
    {
        switch(ulStatusCode)
        {
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            IFC( SetCachePath(szStatusText, MAX_PATH)); // stash the filename
            break;

        case BINDSTATUS_BEGINDOWNLOADDATA:
            IFC(gps->GetPlatformUtilities(&pUtilities));
            cString = pUtilities->Xstrlen(szStatusText);
            IFC(xstring_ptr::CloneBuffer((WCHAR*)szStatusText, cString, &m_strFinalUrl));
            m_xResult = S_OK;
            m_length = ulProgressMax;
            break;
        case BINDSTATUS_DOWNLOADINGDATA:
            // Wait until OnDataAvailable to actually send callbacks...
            break;
        case BINDSTATUS_ENDDOWNLOADDATA:
            m_length = ulProgress;
            if (m_pDataBuffer)
            {
                //
                // Tell the buffer there's no more data coming.
                // Update final data size in stream data buffer.
                //
                IFCEXPECT(m_length == ~0ULL || m_length < XUINT32_MAX);
                m_pDataBuffer->SetFinalDownloadSize(static_cast<XUINT32>(m_length));
            }
            break;

        case BINDSTATUS_REDIRECTING:

            // We fail all redirects for all binds.
            //      Servicing redirects can easily lead to bugs,
            //      and make security restrictions more difficult to enforce.
            //
            //      It's more of a scenario for the top-level page.
            //

            if (m_eUnsecureDownloadAction & udaAllowRedirects)
            {
                m_xResult = S_OK;
            }
            else if (szStatusText != NULL)
            {
                m_bAborting = TRUE;
                m_pBinding->Abort();
                m_xResult = E_FAIL ; // let the caller know that this has failed.

            }
            break;

        case BINDSTATUS_MIMETYPEAVAILABLE:
        case BINDSTATUS_LOADINGMIMEHANDLER:
            break;
        }
    }
    else
    {
        m_bAborting = TRUE;
        m_pBinding->Abort();
        m_xResult = E_FAIL;
    }

Cleanup :
    RRETURN( hr ) ; // looks like urlmon handles this properly.
}

STDMETHODIMP
CWindowsDownloadRequest::OnStartBinding(DWORD dwReserved, _In_ IBinding* pbinding)
{
    TraceDownloadRequestBindingBegin(reinterpret_cast<uint64_t>(this));

    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);
    m_ulHttpStatusCode = HTTP_STATUS_CODE_NOT_SET;

    if (pbinding == NULL || m_bDead || m_bAborting)
    {
        return E_FAIL;
    }

    m_pBinding = pbinding;
    m_pBinding->AddRef();

    // Make sure the threaded job queue stays alive to pump messages until binding is complete.
    //
    // It is thread safe to acquire the job queue because it will always be created first in the InitiateRequest
    // method via the UI thread and if the downloader is destroyed, it will shut down the job queue thread and any
    // chance to call binding functions since they are called via the message pump.
    //
    // Also, OnStartBinding is called during the processing of the first job which guarantees a non-empty queue
    // and will not process the thread keep alive check on external references.  Therefore it is safe (and the
    // best location) to increment the external reference because from this point forward, the message pump provided
    // by the ThreadedJobQueue is required in order to handle binding callbacks.
    m_threadedJobQueueDeferral = m_pDownloader->GetJobQueue().GetDeferral();

#if DBG == 1
    m_eRequestState = GotResponse;
#endif

    return S_OK;
}

STDMETHODIMP
CWindowsDownloadRequest::OnStopBinding(HRESULT hrStatus, _In_reads_opt_(MAX_PATH) LPCWSTR szError)
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);

    if (SUCCEEDED(hrStatus) &&
        m_ulHttpStatusCode != HTTP_STATUS_CODE_NOT_SET &&
        !IsSuccessHttpStatusCode(m_ulHttpStatusCode))
    {
        hrStatus = E_FAIL;
    }

    // Release the binding so that synchronous operations on the url
    // will work
    ReleaseInterface(m_pBinding);

    if (m_bDead)
    {
        return E_FAIL;
    }

    m_threadedJobQueueDeferral.reset();

    IGNOREHR(FireGotResponse(hrStatus));

    //
    // Fix for 34603: OutOfMemory
    //
    // Once OnStopBinding is called,
    // No external code would want to call CWindowsDownloadRequest class
    // to get CWinDataStream and CWinDataStreamBuffer, it is safe to
    // release these objects to avoid potential big memory leak if
    // CWindowsDownloadRequest object is leaking by Urlmon somehow.
    //

    if (!(m_eUnsecureDownloadAction & udaShareDownloadStream))
    {
        ReleaseInterface(m_pDataStream);
        ReleaseInterface(m_pDataBuffer);

        delete[] m_pTempBuffer;
        m_pTempBuffer = NULL;
    }

    TraceDownloadRequestBindingEnd(reinterpret_cast<uint64_t>(this), hrStatus);
    return S_OK; // Note that urlmon eats this even if it's an error
}


STDMETHODIMP
CWindowsDownloadRequest::GetBindInfo(_Out_ DWORD * pgrfBINDF, _Inout_ BINDINFO * pbindInfo)
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);
    HRESULT hr = S_OK;
    IStream* pStream = NULL;

    if (m_bDead || m_bAborting)
    {
        IFC_NOTRACE(E_FAIL); // No download required, return error to tell urlmon to stop.
    }

    if (pbindInfo==NULL || pbindInfo->cbSize==0 || pgrfBINDF==NULL)
    {
        IFC(E_INVALIDARG);
    }

    *pgrfBINDF=m_nBindFlags;

// Leave this commented out but might be interesting to see why this
// was needed.  I guess the cache file issue causes it (KTG)
//    if (StrCmpNIA("res://", m_pIIS->GetPath(), 6) == 0)
//        *pgrfBINDF |= BINDF_PULLDATA;

    {
        ULONG cbSize = pbindInfo->cbSize;        // remember incoming cbSize
        memset(pbindInfo, 0, cbSize);            // zero out structure
        pbindInfo->cbSize = cbSize;                // restore cbSize
    }

    // set verb for the request
    pbindInfo->dwBindVerb = BINDVERB_GET;

Cleanup:
    ReleaseInterface(pStream);
    return hr;
}

STDMETHODIMP
CWindowsDownloadRequest::GetBindString(
    _In_ ULONG stringType,
    _Inout_ LPOLESTR* bindString,
    _In_ ULONG inCount,
    _Inout_ ULONG* outCount)
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);
    *outCount = 0;

    return INET_E_USE_DEFAULT_SETTING;
}

STDMETHODIMP
CWindowsDownloadRequest::OnDataAvailable(DWORD grfBSCF,
                                         DWORD dwSize,
                                         _In_opt_ FORMATETC * pfmtetc,
                                         _In_ STGMEDIUM * pstgmed)
{
    TraceDownloadRequestDataAvailableInfo(reinterpret_cast<uint64_t>(this), dwSize);

    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);
    HRESULT hr = S_OK;
    bool bFirstDataNotification = false;
    bool bSendDataCallbacks = true;
    xstring_ptr strAcceptRanges;

    if (m_bDead || m_bAborting)
    {
        IFC(E_FAIL);
    }

    if (m_ulHttpStatusCode != HTTP_STATUS_CODE_NOT_SET && !IsSuccessHttpStatusCode(m_ulHttpStatusCode))
    {
        // This is a failure status code, so we do not want to provide data callbacks
        // to the IPALDownloadResponseCallback in order to maintain the same behavior
        // as previous versions of Silverlight.  We cannot fail here because we need to
        // read data from the IStream in order to have UrlMon continue downloading the
        // file and eventually trigger the OnStopBinding call where we will send
        // a GotResponse callback with a failure code.
        bSendDataCallbacks = FALSE;
    }

    bFirstDataNotification = (BSCF_FIRSTDATANOTIFICATION & grfBSCF) != 0;

    if (bFirstDataNotification)
    {
        // Save off the IStream from the bind operation into m_pStream.
        if (TYMED_ISTREAM != pstgmed->tymed)
        {
            IFC(E_UNEXPECTED);
        }
        m_pStream = pstgmed->pstm;

        // We need to create a buffer to hold the download data.
        IFC(InitializeDataBuffer(dwSize));

        if (!m_strHeaders.IsNull())
        {
            if(m_strHeaders.StartsWith(XSTRING_PTR_EPHEMERAL(L"HTTP/1.0"), xstrCompareCaseInsensitive))
            {
                // This server is returning a HTTP/1.0 response, so it will not support
                // byte range requests
                m_pDataBuffer->SetFastSeekSupport(FALSE);
            }
            else if (SUCCEEDED(ExtractFirstHeaderValueFromPackedString(m_strHeaders, XSTRING_PTR_EPHEMERAL(L"Accept-Ranges"), &strAcceptRanges)))
            {
                if (strAcceptRanges.Equals(XSTRING_PTR_EPHEMERAL(L"none"), xstrCompareCaseInsensitive))
                {
                    // This server contains an Accept-Ranges header that explicitly does not
                    // support byte-range requests.  A server that doesn't return an Accept-Ranges
                    // header may or may not support range requests, so in that case leave the
                    // stream willing to attempt them.
                    m_pDataBuffer->SetFastSeekSupport(FALSE);
                }
            }
        }
    }

    IFC(ReadStreamPull());

    if (bFirstDataNotification && bSendDataCallbacks)
    {
        IFC(SetupDataStreams());
    }

    if (m_pDataBuffer && bSendDataCallbacks)
    {
        IFC(FireGotData());
    }

Cleanup:
    if (FAILED(hr) && !m_bAborting)
    {
        // If we failed, abort the binding so urlmon won't call OnDataAvailable again.
        Abort();
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CWindowsDownloadRequest::InitializeDataBuffer(DWORD cbCurrentlyAvailable)
{
    HRESULT hr = S_OK;

    ReleaseInterface(m_pDataBuffer);
    IFCEXPECT(m_length == ~0ULL || m_length < XUINT32_MAX);
    IFC(CWinDataStreamBuffer::Create(false /*fCreateMemoryStream*/, static_cast<XUINT32>(m_length), &m_pDataBuffer));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CWindowsDownloadRequest::FireGotData()
{
    if (m_pDownloadResponseCallback)
    {
        m_pDownloadResponseCallback->GotData(m_streamLength, m_length);
    }

    return S_OK;
}

_Check_return_ HRESULT CWindowsDownloadRequest::SetupDataStreams()
{
    HRESULT hr = S_OK;

    // Create the stream, if we still have a callback (and therefore haven't aborted)
    if (m_pDownloadResponseCallback)
    {
        IFC(m_pDataBuffer->CreateStream(&m_pDataStream));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CWindowsDownloadRequest::FireGotResponse(HRESULT xrStatus)
{
    HRESULT hr = S_OK;
    XINT32 fContinueDownload = FALSE;

#if DBG == 1
    ASSERT( (  m_eRequestState == GotResponse  )
            || m_eRequestState == AsyncRequestBegun) ;
    m_eRequestState = ResponseComplete ;
#endif

    if (m_pDownloadResponseCallback != NULL)
    {
        CWindowsDownloadResponse * pResponse = NULL;

        IFCEXPECT_ASSERT(m_pDownloader);
        IFC(m_pDownloader->ContinueAllDownloads(&fContinueDownload));
        if (fContinueDownload)
        {
            if (m_pDownloadResponseCallback)
            {
                IFC( CreateResponse( xrStatus, & pResponse ));
                if (SUCCEEDED(m_xResult))
                {
                    ASSERT(m_pDataStream != NULL);

                    m_xResult = pResponse->SetInputStream(m_pDataStream);
                }

                m_pDownloadResponseCallback->GotResponse(xref_ptr<IPALDownloadResponse>(pResponse), m_xResult);
                SetRequestCompleted(TRUE);

                ReleaseInterface(pResponse);
            }
        }

// Release all our state.
        ReleaseInterface(pResponse);
        ReleaseInterface(m_pDownloader);
        ReleaseInterface(m_pDownloadResponseCallback);

#if DBG == 1
        m_eRequestState = Done ;
#endif

    }


Cleanup :
    m_bDead = TRUE;

    RRETURN( hr );
}

_Check_return_ HRESULT CWindowsDownloadRequest::ReadStreamPull()
{
    HRESULT hr = S_OK;
    XUINT32 cbRead = 0;
    bool bKeepReading = true;

    // sanity check that m_pDataBuffer is valid
    IFCEXPECT_ASSERT(m_pDataBuffer);

    if (!m_pTempBuffer)
    {
        m_pTempBuffer = new XUINT8[CB_TEMP_BUFFER_SIZE];
    }

    while (bKeepReading)
    {
        cbRead = 0;
        hr = m_pStream->Read(m_pTempBuffer, CB_TEMP_BUFFER_SIZE, (ULONG*)&cbRead);

        if (E_PENDING == hr || S_FALSE == hr)
        {
            bKeepReading = FALSE;
            hr = S_OK;
        }

        IFC(hr);

        if (cbRead != 0)
        {
            IFCEXPECT(m_streamLength == ~0ULL || m_streamLength < XUINT32_MAX);
            IFC(m_pDataBuffer->Write(m_pTempBuffer, cbRead, static_cast<XUINT32>(m_streamLength), NULL /* m_pStream */));
            m_streamLength += cbRead;
        }
    }

    if (m_pDataBuffer->Size() > m_length)
    {
        m_length = m_pDataBuffer->Size();
    }

Cleanup:
     RRETURN(hr);
}

bool CWindowsDownloadRequest::IsSuccessHttpStatusCode(XUINT32 ulStatusCode)
{
    if (ulStatusCode >= 200 && ulStatusCode <= 299)
    {
        return true;
    }

    // This function should only be called with a valid status code.
    ASSERT(ulStatusCode != HTTP_STATUS_CODE_NOT_SET);

    // We shouldn't get 1xx or 3xx as our final status code from urlmon.
    // These are not failures, but represent status or intermediate state.
    ASSERT(!(ulStatusCode >= 100 && ulStatusCode <= 199));
    ASSERT(!(ulStatusCode >= 300 && ulStatusCode <= 399));

    return false;
}

STDMETHODIMP
CWindowsDownloadRequest::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return S_OK;
}


STDMETHODIMP
CWindowsDownloadRequest::Authenticate(_Out_ HWND * phwnd,
                                      _Out_writes_(1) LPWSTR * pwszUser,
                                      _Out_writes_(1) LPWSTR * pwszPassword)
{
    if (m_bDead || m_bAborting)
    {
        return E_FAIL;
    }
    if ((phwnd == NULL) || (pwszUser == NULL) || (pwszPassword == NULL))
    {
        return E_INVALIDARG;
    }

    // Return the HWND for the calling thread's message queue which is currently bound to urlmon.
    *phwnd = m_hwnd;

    // We don't provide any special username/password.
    *pwszUser = NULL;
    *pwszPassword = NULL;

    return S_OK;
}

// IHttpNegotiate methods
STDMETHODIMP
CWindowsDownloadRequest::BeginningTransaction( _In_z_ LPCWSTR szURL,
                                               _In_z_ LPCWSTR szHeaders,
                                               DWORD dwReserved,
                                               _Outptr_result_z_ LPWSTR *pszAdditionalHeaders)
{
   // Here's our opportunity to add headers
   if (!pszAdditionalHeaders)
   {
      return E_POINTER;
   }

   *pszAdditionalHeaders = NULL;

   if (m_bDead || m_bAborting)
   {
       return E_ABORT;
   }

   // Attach any headers specified in the request
   if (m_cHeaders)
   {
      LPWSTR wszAdditionalHeaders =
             (LPWSTR)CoTaskMemAlloc(
                (m_cHeaders+1) *sizeof(WCHAR));
      IFCOOMFAILFAST(wszAdditionalHeaders);

      wcscpy_s(wszAdditionalHeaders, m_cHeaders + 1, m_pHeaders);
      *pszAdditionalHeaders = wszAdditionalHeaders;
   }
   return S_OK;
}

STDMETHODIMP
CWindowsDownloadRequest::OnResponse( DWORD dwResponseCode,
                                     _In_z_ LPCWSTR szResponseHeaders,
                                     _In_z_ LPCWSTR szRequestHeaders,
                                     _Outptr_result_z_ LPWSTR *pszAdditionalRequestHeaders)
{
    HRESULT hr = S_OK;
    IPlatformUtilities* pUtilities = NULL;

    if (!pszAdditionalRequestHeaders)
    {
        return E_POINTER;
    }

    m_ulHttpStatusCode = dwResponseCode;

    IFC(gps->GetPlatformUtilities(&pUtilities));

    IFC(xstring_ptr::CloneBuffer(
        const_cast<WCHAR*>(szResponseHeaders),
        xstrlen(szResponseHeaders),
        &m_strHeaders));

    *pszAdditionalRequestHeaders = NULL;

Cleanup:
    return S_OK;
}

typedef HRESULT (*FnGetAcceptLanguages)(LPTSTR, LPDWORD);

//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest GetInternetExplorerAcceptLanguageString
//
//  Synopsis : Create an Accept-Language header value using the IE setting
//
//--------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsDownloadRequest::GetInternetExplorerAcceptLanguageString(_Out_ xstring_ptr* pstrAcceptLanguageString)
{
    HRESULT hr = S_OK;
    WCHAR szAcceptLanguage[2048];
    DWORD cchAcceptLanguage = ARRAY_SIZE(szAcceptLanguage);
    HMODULE hShlwapi = NULL;
    FnGetAcceptLanguages pfnGetAcceptLanguages = NULL;

    // Call GetAcceptLanguagesW, publicly documented at:
    // https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-getacceptlanguagesw
    IFCW32(hShlwapi = LoadLibraryEx(L"api-ms-win-core-url-l1-1-0.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));

    pfnGetAcceptLanguages = (FnGetAcceptLanguages)GetProcAddress(hShlwapi, "GetAcceptLanguagesW");
    IFCW32(pfnGetAcceptLanguages);

    IFC(pfnGetAcceptLanguages(szAcceptLanguage, &cchAcceptLanguage));

    // GetAcceptLanguages can return a count that includes a terminating null character,
    // so ensure that we remove those here in order to avoid placing a null in the
    // middle of the combined headers string.
    while (cchAcceptLanguage > 0 && szAcceptLanguage[cchAcceptLanguage - 1] == L'\0')
    {

        cchAcceptLanguage--;
    }

    IFC(xstring_ptr::CloneBuffer(szAcceptLanguage, cchAcceptLanguage, pstrAcceptLanguageString));

Cleanup:
    if (hShlwapi)
    {
        FreeLibrary(hShlwapi);
    }
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest GetWindowsAcceptLanguageString
//
//  Synopsis : Create an Accept-Language header value using the Windows
//    OS setting
//
//--------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsDownloadRequest::GetWindowsAcceptLanguageString(_Out_ xstring_ptr* pstrAcceptLanguageString)
{
    HRESULT hr = S_OK;
    WCHAR szLocaleName[MAX_LOCALE_LENGTH];
    XUINT32 cchLocaleName = 0;
    XUINT32 cchCountryName = MAX_LOCALE_COMPONENT_LENGTH;
    XUINT32 cchLanguageName = MAX_LOCALE_COMPONENT_LENGTH;
    LCID lcid = GetUserDefaultLCID();

    cchLanguageName = GetLocaleInfoW(
        lcid,
        LOCALE_SISO639LANGNAME,
        szLocaleName,
        cchLanguageName);

#pragma warning(push)
#pragma warning(disable: 26014)
    IFCEXPECT_ASSERT(cchLanguageName > 0 && cchLanguageName < MAX_LOCALE_LENGTH);
    szLocaleName[cchLanguageName - 1] = L'-';
    cchCountryName = GetLocaleInfoW(
        lcid,
        LOCALE_SISO3166CTRYNAME,
        szLocaleName + cchLanguageName,
        cchCountryName);
    IFCEXPECT_ASSERT(cchCountryName > 0);
    cchLocaleName = cchLanguageName + cchCountryName + 1 /*L"-"*/ - 2 /* 2 included nulls */;

    IFCEXPECT_ASSERT(cchLocaleName <= MAX_LOCALE_LENGTH);
    IFC(xstring_ptr::CloneBuffer(szLocaleName, cchLocaleName, pstrAcceptLanguageString));
#pragma warning(pop)

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest InitiateRequest
//
//  Synopsis : Begin a request to download asynchronously.
//
//--------------------------------------------------------------------------
HRESULT
CWindowsDownloadRequest::InitiateRequest (
                                            _In_ IDownloader* pIDownloader,
                                            _In_ IUnknown* pUnkContainer,
                                            _In_ IPALUri* pUriAbsolute,
                                            _In_ XINT32 fCrossDomain,
                                            _In_ IPALDownloadResponseCallback* pICallback,
                                            bool suppressCredentialsDialog)
{
    HRESULT hr = S_OK ;
    WCHAR szHostName[XINTERNET_MAX_HOST_NAME_LENGTH + 1];
    XUINT32 cHostName = XINTERNET_MAX_HOST_NAME_LENGTH;
    XUINT32 offset = 0;
    bool bAddAcceptLanguageHeader = false;
    xstring_ptr strAcceptLanguageHeader;

    IFCPTR( pICallback );
    IFCPTR( pIDownloader );

    m_pDownloader = pIDownloader;
    m_pDownloader->AddRef();

    ASSERT(m_pDownloadResponseCallback == nullptr);

    m_pDownloadResponseCallback = pICallback;
    m_pDownloadResponseCallback->AddRef();
    m_pDataStream = nullptr;

    m_nBindFlags = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

    if (suppressCredentialsDialog)
    {
        m_nBindFlags |= BINDF_NO_UI | BINDF_SILENTOPERATION;
    }

    if (m_eUnsecureDownloadAction & udaResynchronizeStream)
    {
        m_nBindFlags |= BINDF_RESYNCHRONIZE;
    }
    else
    {
        // If the request is for localhost, we want to make sure that we get the most
        // recent version of the resources by sending an if-modified-since request.  This
        // is done to address the scenario of a developer working in visual studio and
        // making changes then refreshing the page.  Without the flag, UrlMon was likely
        // to use the cached version rather than sending a new request.
        if (pUriAbsolute)
        {
#pragma prefast( push )
#pragma prefast( disable: 26015 "during the call to GetHost, cHostName can only decrease, cHostName is constrained by XINTERNET_MAX_HOST_NAME_LENGTH")
            IFC(pUriAbsolute->GetHost(&cHostName, szHostName));
            xephemeral_string_ptr strHostName(szHostName, cHostName);
#pragma prefast( pop )
            if (strHostName.Equals(XSTRING_PTR_EPHEMERAL(L"localhost"), xstrCompareCaseInsensitive))
            {
                m_nBindFlags |= BINDF_RESYNCHRONIZE;
            }
        }
    }

    //set the headers
    m_cHeaders = 0;

    // Add the Accept-Language header for IE.  As with the Referer
    // header, this is quirks moded for only SL4+
    if (m_bSendRefererHeader)
    {
        // Attempt to get the IE override of the Accept-Language header value
        if (SUCCEEDED(GetInternetExplorerAcceptLanguageString(&strAcceptLanguageHeader)))
        {
            // If the registry key for IE exists but is empty, don't add any Accept-Language header
            if (strAcceptLanguageHeader.GetCount() > 0)
            {
                m_cHeaders += strAcceptLanguageHeader.GetCount() + ACCEPT_LANGUAGE_HEADER_PREFIX_LENGTH + 2;
                bAddAcceptLanguageHeader = TRUE;
            }
        }
        else
        {
            // Fallback to the Windows setting
            IFC(GetWindowsAcceptLanguageString(&strAcceptLanguageHeader));
            m_cHeaders += strAcceptLanguageHeader.GetCount() + ACCEPT_LANGUAGE_HEADER_PREFIX_LENGTH + 2;
            bAddAcceptLanguageHeader = TRUE;
        }
    }

    if (m_cHeaders)
    {
        m_pHeaders = new WCHAR[m_cHeaders + 1];
        if (bAddAcceptLanguageHeader)
        {
            //we need to add the accept-language header
            memcpy(m_pHeaders+offset, ACCEPT_LANGUAGE_HEADER_PREFIX, ACCEPT_LANGUAGE_HEADER_PREFIX_LENGTH * sizeof(WCHAR));
            offset += ACCEPT_LANGUAGE_HEADER_PREFIX_LENGTH;
            memcpy(m_pHeaders+offset, strAcceptLanguageHeader.GetBuffer(), strAcceptLanguageHeader.GetCount() * sizeof(WCHAR));
            offset += strAcceptLanguageHeader.GetCount();
            memcpy(m_pHeaders+offset, L"\r\n", 2 * sizeof(WCHAR));
            offset += 2;
        }
        m_pHeaders[m_cHeaders] = L'\0';
    }

// Initiate the bind.
    IFC( _Download( pUriAbsolute, pUnkContainer ));

#if DBG == 1
    m_eRequestState = AsyncRequestBegun ;
#endif

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(m_pDownloadResponseCallback);
        ReleaseInterface(m_pDataStream);
    }
    RRETURN( hr );
}


//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest SetCachePath
//
//  Synopsis : Store the cache path
//
//--------------------------------------------------------------------------
HRESULT
CWindowsDownloadRequest::SetCachePath(__in_ecount_z(cszPath) LPCWSTR szPath, XUINT32 cszPath)
{
    if (cszPath > MAX_PATH || FAILED(StringCchCopy(m_szPath, cszPath, szPath)))
    {
        m_szPath[0] = 0;
        return E_FAIL ;
    }
    return S_OK ;
}

//+-------------------------------------------------------------------------
//
//  Function: CWindowsDownloadRequest CreateResponse
//
//  Synopsis : Create a Response object to return to the caller.
//
//--------------------------------------------------------------------------

HRESULT
CWindowsDownloadRequest::CreateResponse(HRESULT hrStatus, _Outptr_result_maybenull_ CWindowsDownloadResponse** ppResponse )
{
    HRESULT hr = S_OK ;
    IFCPTR( ppResponse );
    ASSERT( m_szPath != NULL );

// store the last status - unless we already had failure.
    if ( SUCCEEDED( m_xResult ))
        m_xResult = hrStatus;

// Create a response object - pass it back to the caller.
    *ppResponse = new CWindowsDownloadResponse();
    IFC( (*ppResponse)->Initialize());

Cleanup:
    RRETURN( hr );
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadRequest::_Download
//
//  Synopsis:
//     Initiate the actual download.
//
//-------------------------------------------------------------------------
HRESULT
CWindowsDownloadRequest::_Download( _In_ IPALUri * pUriAbsolute,
                                    _Inout_opt_ IUnknown* pUnkContainer)
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);

    // Note: make_shared<wchar_t[]> will not work since wchar_t[] is incompatible for the GetCanonical and GetScheme methods.
    //       It compensates by using a shared_ptr to the type and specifies a default delete for the array type so delete[] will
    //       be called on the char arrays.
    //       Also, they are currently shared_ptr since lambda move semantics for unique_ptr aren't available until C++ 14.
    uint32_t canonicalUrlLength = INTERNET_MAX_URL_LENGTH + 1;
    std::shared_ptr<wchar_t> canonicalUrl(new wchar_t[canonicalUrlLength], std::default_delete<wchar_t[]>());
    IFC_RETURN(pUriAbsolute->GetCanonical(&canonicalUrlLength, canonicalUrl.get()));

    uint32_t schemeLength = INTERNET_MAX_SCHEME_LENGTH + 1;
    std::shared_ptr<wchar_t> scheme(new wchar_t[schemeLength], std::default_delete<wchar_t[]>());
    IFC_RETURN(pUriAbsolute->GetScheme(&schemeLength, scheme.get()));

    wchar_t* urlRaw = nullptr;
    uint32_t urlLength = 0;
    IFC_RETURN(PercentEncodeString(canonicalUrlLength, canonicalUrl.get(), &urlLength, &urlRaw));
    std::shared_ptr<wchar_t> url(urlRaw, std::default_delete<wchar_t[]>());

    m_cUrl = urlLength;
    IFCOOM_RETURN(m_pUrl = xstralloc(url.get(), m_cUrl));

    TraceDownloadRequestQueueInfo(
        reinterpret_cast<uint64_t>(this),
        urlRaw);

    // The asynchronous thread will cleanup the bstrURL.
    // Take a strong reference on this object so it is guaranteed to be alive until the lambda runs.
    // The lambda will release the reference after it completes.
    m_pDownloader->GetJobQueue().QueueJob([=, thisStrongRef = xref_ptr<CWindowsDownloadRequest>(this)] (HWND hwnd)
    {
        // Transform the Uri to a string for legacy Urlmon functions.
        // Note that the bstrURL must be cleaned up on the same thread it was created so it must be allocated here.
        BSTR bstrURL = nullptr;
        if (xstrncmpi(scheme.get(), L"file", 4) == 0)
        {
            // UrlMon doesn't work properly with percent-encoded file
            // urls.  It requires non US-ASCII characters to be left
            // in the string without percent encoding them...
            //
            // Note: This means that pre-encoded file:// urls will not
            // work in IE since we're not doing any decoding.
            //
            // http://blogs.msdn.com/ie/archive/2006/12/06/file-uris-in-windows.aspx
            bstrURL = SysAllocString(canonicalUrl.get());
        }
        else
        {
            bstrURL = SysAllocString(url.get());
        }

        IFCOOMFAILFAST(bstrURL);

        m_hwnd = hwnd;

        auto urlCleanup = wil::scope_exit([&] { SysFreeString(bstrURL); });

        // Create a bindctx - and register this object as the callback.
        xref_ptr<IBindCtx> bindContext;
        IFCFAILFAST(CreateBindCtx(0, bindContext.ReleaseAndGetAddressOf()));
        IFCFAILFAST(RegisterBindStatusCallback(bindContext.get(), static_cast<IBindStatusCallback*>(this), 0, 0L));

        xref_ptr<IMoniker> moniker;
        IFCFAILFAST(CreateURLMonikerEx(NULL, bstrURL, moniker.ReleaseAndGetAddressOf(), URL_MK_UNIFORM));

        xref_ptr<IStream> stream;
        auto bindToStorageHR = moniker->BindToStorage(bindContext.get(), 0, __uuidof(IStream), reinterpret_cast<void**>(stream.ReleaseAndGetAddressOf()));
        if (MK_S_ASYNCHRONOUS == bindToStorageHR)
        {
            // Added this to mask a return telling us the download is asynchronous, we return S_OK on success.
            bindToStorageHR = S_OK;
        }

        // Stow the exception but OnStopBinding() should propagate any errors that arise.
        IGNOREHR(bindToStorageHR);
    });

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadRequest::Abort
//
//  Synopsis:
//     Abort the current download.
//
//-------------------------------------------------------------------------
void
CWindowsDownloadRequest::Abort()
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);

    m_bAborting = TRUE;

    {
        // If we don't have a binding, then we can't abort the download
        if (m_pBinding)
        {
            IGNOREHR(m_pBinding->Abort());
            m_xResult = E_FAIL;
        }
        else
        {
            // Only use dead marker if we can't abort properly
            m_bDead = TRUE;
        }
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadRequest::IsActive
//
//  Synopsis:
//     Returns true if the download is active
//
//-------------------------------------------------------------------------
XINT32
CWindowsDownloadRequest::IsActive()
{
    return (m_pDownloadResponseCallback && !m_bDead && !m_bAborting);
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsDownloadRequest::AbortRequest
//
//  Synopsis:
//     This abort is used by AbortableDownload to cancel the download.
//
//-------------------------------------------------------------------------
void
CWindowsDownloadRequest::AbortRequest()
{
    std::lock_guard<std::recursive_mutex> lock(m_asyncMutex);

    if (m_pDownloadResponseCallback)
    {
        // Remove the callback from the list
        ReleaseInterface(m_pDownloadResponseCallback);

        // Remove associated data stream
        ReleaseInterface(m_pDataStream);

        Abort();
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsAbortableDownload::Create
//
//  Synopsis:
//     Create an Abortable Download from a Windows Download
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsAbortableDownload::Create(
    _In_opt_ CWindowsDownloadRequest* pWindowsDownloadRequest,
    _Out_opt_ CWindowsAbortableDownload** ppAbortableDownload
    )
{
    HRESULT hr = S_OK;
    CWindowsAbortableDownload *pAbortableDownload = new CWindowsAbortableDownload();
    IFCPTR(pWindowsDownloadRequest);
    IFCPTR(ppAbortableDownload);

    IFC(pAbortableDownload->SetDownloadRequest(pWindowsDownloadRequest));

    *ppAbortableDownload = pAbortableDownload;
    pAbortableDownload = NULL;

Cleanup:
    ReleaseInterface(pAbortableDownload);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsAbortableDownload::Abort
//
//  Synopsis:
//     Abort this download
//
//-------------------------------------------------------------------------
void
CWindowsAbortableDownload::Abort()
{
    ASSERT(m_pDownloadRequest != NULL);
    m_pDownloadRequest->AbortRequest();
}

// Async version of Abort, used to avoid blocking UI thread (see notes in CAbortableAsyncDownload::Abort())
void CWindowsAbortableDownload::AbortAsync(_In_ IAsyncDownloadRequestManager* manager)
{
    manager->AbortAsync(this);
}

XUINT32
CWindowsAbortableDownload::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

XUINT32
CWindowsAbortableDownload::Release()
{
    XUINT32 cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
    {
        delete this;
    }

    return cRef;
}

//-------------------------------------------------------------------------
//
//  Function:   CWindowsAbortableDownload::SetDownloadRequest
//
//  Synopsis:
//     Associate the Dowload request with this AbortableOperation
//     Can only be called once.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsAbortableDownload::SetDownloadRequest(
    _In_opt_ CWindowsDownloadRequest *pDownloadRequest
    )
{
    HRESULT hr = S_OK;

    IFCPTR(pDownloadRequest);
    if (m_pDownloadRequest != NULL)
    {
        IFC(E_FAIL);
    }

    m_pDownloadRequest = pDownloadRequest;
    m_pDownloadRequest->AddRef();

Cleanup:
    RRETURN(hr);
}

