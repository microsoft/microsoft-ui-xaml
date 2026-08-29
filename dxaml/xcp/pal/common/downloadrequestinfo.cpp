// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//-----------------------------------------------------------------------------
// ctor
//-----------------------------------------------------------------------------
CDownloadRequestInfo::CDownloadRequestInfo()
{
    m_cRef = 1;

    m_pAbsoluteUri = NULL;
    m_pCallback = NULL;
    m_flags = udaNone;
    m_bUseDefaultCreds = TRUE;
}

//-----------------------------------------------------------------------------
// dtor
//-----------------------------------------------------------------------------
CDownloadRequestInfo::~CDownloadRequestInfo()
{
    ReleaseInterface(m_pAbsoluteUri);
    ReleaseInterface(m_pCallback);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::Create
//
// Static factory method to create a request object for the provided url and
// using the provided IPALDownloadResponseCallback to deliver progress events
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::Create(_In_ const xstring_ptr& strUrl,
                             _In_ IPALDownloadResponseCallback* pCallback,
                             _Outptr_ CDownloadRequestInfo** ppDownloadReq)
{
    RRETURN(CDownloadRequestInfo::Create(strUrl, NULL, pCallback, ppDownloadReq));
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::Create
//
// Static factory method to create a request object for the provided url and
// using the provided IPALDownloadResponseCallback to deliver progress events
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::Create(
    _In_ const xstring_ptr& strUrl,
    _In_opt_ IPALUri* pAbsoluteUri,
    _In_ IPALDownloadResponseCallback* pCallback,
    _Outptr_ CDownloadRequestInfo** ppDownloadReq
    )
{
    HRESULT hr = S_OK;
    CDownloadRequestInfo* pDownloadReq = NULL;

    IFCPTR(pCallback);
    IFCPTR(ppDownloadReq);

    pDownloadReq = new CDownloadRequestInfo();
    IFC(pDownloadReq->Initialize(strUrl, pAbsoluteUri, pCallback));
    
    *ppDownloadReq = pDownloadReq;
    pDownloadReq = NULL;

Cleanup:
    ReleaseInterface(pDownloadReq);
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::Initialize
//
// Initializer.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::Initialize(
    _In_ const xstring_ptr& strUrl,
    _In_opt_ IPALUri* pAbsoluteUri,
    _In_ IPALDownloadResponseCallback* pCallback
    )
{
    HRESULT hr = S_OK;

    m_strUrl = strUrl;

    if (pAbsoluteUri != NULL)
    {
        IFC(pAbsoluteUri->Clone(&m_pAbsoluteUri));
    }

    m_pCallback = pCallback;
    AddRefInterface(m_pCallback);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetUrl
//
// Get the url string for this request.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetUrl(_Out_ xstring_ptr* pstrUrl)
{
    HRESULT hr = S_OK;
    auto lock = m_Lock.lock();

    IFCEXPECT_ASSERT(!m_strUrl.IsNull());

    *pstrUrl = m_strUrl;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetAbsoluteUri
//
// Get the absolute URI for this request, if present.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetAbsoluteUri(_Outptr_result_maybenull_ IPALUri** ppAbsoluteUri)
{
    HRESULT hr = S_OK;
    auto lock = m_Lock.lock();

    IFCPTR(ppAbsoluteUri);

    if (m_pAbsoluteUri != NULL)
    {
        IFC(m_pAbsoluteUri->Clone(ppAbsoluteUri));
    }
    else
    {
        *ppAbsoluteUri = NULL;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetCallback
//
// Get the response callback for this request.  The caller is responsible for
// releasing the callback.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetCallback(_Outptr_ IPALDownloadResponseCallback** ppCallback)
{
    HRESULT hr = S_OK;
    auto lock = m_Lock.lock();

    IFCPTR(ppCallback);

    IFCEXPECT_ASSERT(m_pCallback);

    AddRefInterface(m_pCallback);
    *ppCallback = m_pCallback;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetOptionFlags
//
// Get the download option flags for this request
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetOptionFlags(_Out_ XUINT32* pDownloadOptions)
{
    HRESULT hr = S_OK;

    IFCPTR(pDownloadOptions);

    *pDownloadOptions = m_flags;

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::SetOptionFlags
//
// Set the download option flags for this request
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::SetOptionFlags(_In_ XUINT32 downloadOptions)
{
    auto lock = m_Lock.lock();

    m_flags = downloadOptions;

    RRETURN(S_OK);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetUseDefaultCredentials
//
// Get a boolean indicating whether or not this request should automatically
// prompt for http authentication credentials if needed.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetUseDefaultCredentials(_Out_ bool* pbUseDefaultCredentials)
{
    HRESULT hr = S_OK;
    auto lock = m_Lock.lock();

    IFCPTR(pbUseDefaultCredentials);

    *pbUseDefaultCredentials = m_bUseDefaultCreds;
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::SetCredentials
//
// Set the http authentication credentials for this request.  If this method is
// called, then the useDefaultCredentials boolean will automatically be set to
// FALSE.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::SetCredentials(_In_ const xstring_ptr& strUsername, _In_ const xstring_ptr& strPassword, _In_ const xstring_ptr& strAuthDomain)
{
    auto lock = m_Lock.lock();

    // This function should only be called once
    ASSERT(m_strUsername.IsNull() &&
           m_strPassword.IsNull() &&
           m_strAuthDomain.IsNull());

    m_bUseDefaultCreds = FALSE;

    m_strUsername = strUsername;
    m_strPassword = strPassword;
    m_strAuthDomain = strAuthDomain;

    RRETURN(S_OK);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetUsername
//
// Get the http authentication user name for this request.
//-------- ---------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetUsername(_Out_ xstring_ptr* pstrUsername)
{
    auto lock = m_Lock.lock();

    // Okay to return NULL for headers
    *pstrUsername = m_strUsername;

    RRETURN(S_OK);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetPassword
//
// Get the http authentication password for this request.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetPassword(_Out_ xstring_ptr* pstrPassword)
{
    auto lock = m_Lock.lock();

    // Okay to return NULL for headers
    *pstrPassword = m_strPassword;

    RRETURN(S_OK);
}

//-----------------------------------------------------------------------------
// CDownloadRequestInfo::GetAuthDomain
//
// Get the http authentication domain for this request.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT 
CDownloadRequestInfo::GetAuthDomain(_Out_ xstring_ptr* pstrAuthDomain)
{
    auto lock = m_Lock.lock();

    // Okay to return NULL for headers
    *pstrAuthDomain = m_strAuthDomain;

    RRETURN(S_OK);
}
