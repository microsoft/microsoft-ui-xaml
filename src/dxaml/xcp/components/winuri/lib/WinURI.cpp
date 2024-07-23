// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Below there is the implemenation of how to deal with Uris
// CWinUriImpl
//  Uses ierutil API's and interfaces to crack uri strings and store state

#include "precomp.h"
#include "winuri.h"

#include <palcore.h>
#include <shlwapi.h>
#include <urlmon.h>
#include <wininet.h>
#include <tchar.h>
#include <xstrutil.h>
#include <XStringBuilder.h>

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates a URI object from a string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriFactory::Create(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ IPALUri **ppUri
    )
{
    wrl::ComPtr<IPALUri> spUri;

    IFC_NOTRACE_RETURN(CWinUriImpl::Create(cString, pString, spUri.GetAddressOf()));

   *ppUri = spUri.Detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CWinUri object
//
//------------------------------------------------------------------------

CWinUriImpl::~CWinUriImpl()
{
    ReleaseInterface(m_pUri);

}

//------------------------------------------------------------------------
//
//  Method:   Create
//
//  Synopsis:
//      Creates a URI object from a string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::Create(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR *pString,
    _Out_ IPALUri **ppUri
    )
{
    wrl::ComPtr<IUri> spRawUri;
    LPCWSTR pszUri = reinterpret_cast<LPCWSTR>(pString);

    // Legacy implementation did not generate an error in all scenarios and
    // ResourceManager used it in a "try create" pattern.
    // This is the reason why CWinUriFactory::Create uses IFC_NOTRACE and therefore we should not
    // raise the error here in cases which may not be real errors.
    // Note that the HRESULT will still be correct, we are just avoiding the call to OnFailure
    // in IFC from here.
    IFC_NOTRACE_RETURN(CreateUri(pszUri, Uri_CREATE_CANONICALIZE | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME, 0, spRawUri.GetAddressOf()));
    if (spRawUri.Get() == nullptr)
    {
        // For http://task.ms/40558176, we're seeing cases where the system is low on memeory and we're getting back
        // a NULL for spRawUri, even though CreateUri has returned a success code.  Let's convert this to a proper OOM
        // error code and failfast so that the bucket will be more accurate in Watson.
        IFCFAILFAST(E_OUTOFMEMORY);
    }

    auto spUri = make_xref<CWinUriImpl>();

    spUri->m_pUri = spRawUri.Detach();

    // Return the URI object and its ownership to the caller.
   *ppUri = spUri.detach();

   return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Clone
//
//  Synopsis:
//      Makes a copy of the URI object.  This allows you to make a copy then
//  merge two URI objects together with Combine but still retain the original
//  URI object.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::Clone(_Out_ IPALUri **ppUri) const
{
    auto spUri = make_xref<CWinUriImpl>();

    // Underlying m_pUri is an imutable read-only object which is
    // safe to share between CWinUriImpl instances.
    SetInterface(spUri->m_pUri, m_pUri);

    spUri->m_componentResourceLocation = m_componentResourceLocation;

    // Return the new URI object and its ownership to the caller.
    *ppUri = spUri.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: UriSchemeEquals
//
//  Synopsis:
//      A non-member helper method to test whether a URI string has a
//      specified scheme. Internally uses InternetCrackUrl to parse the
//      URI string. Returns TRUE if the schemes match.
//
//------------------------------------------------------------------------
_Check_return_ static HRESULT UriSchemeEquals(
    _In_reads_(cUri) const WCHAR* pstrUri,
    const XUINT32 cUri,
    _In_reads_(cScheme) const WCHAR* pstrScheme,
    const XUINT32 cScheme,
    _Out_ bool* pResult)
{
    wil::unique_bstr bstrScheme;
    wrl::ComPtr<IUri> spUri;
    DWORD dwUriSchemeLen = 0;

    IFCPTR_RETURN(pResult);
    *pResult = false;

    IFC_RETURN(CreateUri(pstrUri, Uri_CREATE_CANONICALIZE, 0, spUri.GetAddressOf()));
    IFC_RETURN(spUri->GetSchemeName(&bstrScheme));

    // set the dwSchemeLength member to a non-zero value - this indicates that
    // we want InternetCrackUrl to set scheme information in the structure
    dwUriSchemeLen = SysStringLen(bstrScheme.get());


    if (dwUriSchemeLen == cScheme && xstrncmpi(pstrScheme, bstrScheme.get(), cScheme) == 0)
    {
        *pResult = true;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   Combine
//
//  Synopsis:
//      Combines two URI objects.  For example you could combine:
//
//  file://c:\windows\system32 with file://fonts\cour.ttf to get
//  file://c:\windows\system32\fonts\cour.ttf
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::Combine(
    _In_ XUINT32 relativeUriLength,
    _In_reads_(relativeUriLength) const WCHAR *relativeUri,
    _Outptr_ IPALUri** ppUriCombine)
{
    xstring_ptr baseUri;

    IFC_RETURN(GetCanonical(&baseUri));

    DWORD combinedUriLength = INTERNET_MAX_URL_LENGTH;
    WCHAR combinedUri[INTERNET_MAX_URL_LENGTH];

    IFC_RETURN(::UrlCombine(
        baseUri.GetBuffer(),
        relativeUri,
        combinedUri,
        &combinedUriLength,
        0));

    wrl::ComPtr<IUri> spRawUri;

    IFC_RETURN(CreateUri(
       combinedUri,
       Uri_CREATE_CANONICALIZE,
       0,
       &spRawUri));

    auto spUri = make_xref<CWinUriImpl>();

    spUri->m_pUri = spRawUri.Detach();
    *ppUriCombine = spUri.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateBaseURI
//
//  Synopsis:
//      Returns the base URI for the current URI, by removing the last path.
//      NOTE: CWinUri's implementation currently simply returns a clone, because
//            Combine will deal with the last part of the URI already.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::CreateBaseURI(_Out_ IPALUri **ppBaseUri)
{
    IFC_RETURN(Combine(1, L".", ppBaseUri));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetCanonical
//
//  Synopsis:
//      Returns the canonicalized URI
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetCanonical(_Inout_ XUINT32 * pBufferLength,
                      _Out_writes_to_opt_(*pBufferLength, *pBufferLength) WCHAR * pszBuffer) const
{
    DWORD dwCount = *pBufferLength;
    wil::unique_bstr bstrAbsoluteUri;

    IFC_RETURN(m_pUri->GetPropertyBSTR(Uri_PROPERTY_ABSOLUTE_URI, &bstrAbsoluteUri, Uri_PUNYCODE_IDN_HOST));

    DWORD dwUriLen = SysStringLen(bstrAbsoluteUri.get()) + 1; // add one for null terminator

    *pBufferLength = dwUriLen;

    if (pszBuffer)
    {
        if (dwCount < dwUriLen)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            memcpy(pszBuffer, bstrAbsoluteUri.get(), sizeof(WCHAR) * dwUriLen);
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetCanonical
//
//  Synopsis:
//      Returns the canonicalized URI as an xstring_ptr.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CWinUriImpl::GetCanonical(_Out_ xstring_ptr* pstrCanonical) const
{
    XUINT32 cchBuffer = 0;
    WCHAR* wszBufferNoRef = nullptr;
    XStringBuilder bufferBuilder;

    // determine buffer size needed
    HRESULT hr = GetCanonical(&cchBuffer, nullptr);
    if (hr == E_OUTOFMEMORY)
    {
        hr = S_OK;
    }
    IFC_RETURN(hr);

    // Note: cchBuffer includes space for NULL terminator. InitializeAndGetFixedBuffer adds one extra for the NULL terminator.
    // RS5 Bug #16634037:  Visual Studio expects an HSTRING without any embedded null terminator.
    // Since GetCanonical now returns a null terminated string, we subtract 1 character from the string builder
    // so it does not include the embedded null terminator in the detached string it creates.
    IFC_RETURN(bufferBuilder.InitializeAndGetFixedBuffer(cchBuffer - 1, &wszBufferNoRef));

    IFC_RETURN(GetCanonical(&cchBuffer, wszBufferNoRef));

    bufferBuilder.SetFixedBufferCount(cchBuffer - 1);
    IFC_RETURN(bufferBuilder.DetachString(pstrCanonical));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetExtension
//
//  Synopsis:
//      Returns the file extension from the URI.  Without the leading '.'
//  character.  So file://c:foo.bar would return 'bar'
//
//------------------------------------------------------------------------

static const WCHAR CHAR_DOT         = L'.';
static const WCHAR CHAR_SLASH       = L'/';

_Check_return_ HRESULT
CWinUriImpl::GetExtension(_Inout_ XUINT32 * pBufferLength,
                      _Out_writes_(*pBufferLength) WCHAR * pszBuffer)
{
    XUINT32 buflen = *pBufferLength;
    XUINT32 fileExtensionLength = 0;
    wil::unique_bstr bstrPath;

    IFC_RETURN(m_pUri->GetPath(&bstrPath));
    WCHAR *pFileExtention = _tcsrchr(bstrPath.get(), CHAR_DOT);

    // If we found an extention name
    if ( pFileExtention != nullptr)
    {
        // skip the dot
        pFileExtention = &pFileExtention[1];

        fileExtensionLength = (XUINT32)_tcslen(pFileExtention);
    }

    *pBufferLength = fileExtensionLength;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < fileExtensionLength + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (pFileExtention != nullptr)
    {
        memcpy(pszBuffer, pFileExtention, fileExtensionLength * sizeof(WCHAR));
    }

    pszBuffer[fileExtensionLength] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetFileName
//
//  Synopsis:
//      Returns the file name from the URI.
//  So file://c:foo.bar would return 'foo.bar'
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetFileName(_Inout_ XUINT32 * pBufferLength,
                      _Out_writes_(*pBufferLength) WCHAR * pszBuffer)
{
    wil::unique_bstr bstrPath;
    XUINT32 buflen = *pBufferLength;
    XUINT32 fileNameLength = 0;

    IFC_RETURN(m_pUri->GetPath(&bstrPath));

    // look back for the first slash
    WCHAR  *pFileName = _tcsrchr(bstrPath.get(), CHAR_SLASH);

    if ( pFileName != nullptr)
    {
        // we found a slash, we want to exclude the slash from the file name
        pFileName = &pFileName[1];
    }
    else
    {
        pFileName = bstrPath.get();
    }


    if ( pFileName != nullptr)
    {
        fileNameLength = (XUINT32)_tcslen(pFileName);
    }


    *pBufferLength = fileNameLength;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < fileNameLength + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (pFileName != nullptr)
    {
        memcpy(pszBuffer, pFileName, fileNameLength * sizeof(WCHAR));
    }

    pszBuffer[fileNameLength] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetHost
//
//  Synopsis:
//      Returns the host string from the URI.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::GetHost(_Inout_ XUINT32 * pBufferLength,
                 _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const
{
    wil::unique_bstr bstrHost;
    XUINT32 buflen = *pBufferLength;

    IFC_RETURN(m_pUri->GetPropertyBSTR(Uri_PROPERTY_HOST, &bstrHost, Uri_DISPLAY_IDN_HOST));
    DWORD dwHostLen = SysStringLen(bstrHost.get());

    *pBufferLength = dwHostLen;
    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwHostLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrHost)
    {
        memcpy(pszBuffer, bstrHost.get(), dwHostLen * sizeof(WCHAR));
    }

    pszBuffer[dwHostLen] = 0;

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   GetPassword
//
//  Synopsis:
//      Returns the password string from the URI
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetPassword(_Inout_ XUINT32 * pBufferLength,
                     _Out_writes_(*pBufferLength) WCHAR * pszBuffer)
{
    wil::unique_bstr bstrPassword;
    XUINT32 buflen = *pBufferLength;

    IFC_RETURN(m_pUri->GetPassword(&bstrPassword));
    DWORD dwPasswordLen = SysStringLen(bstrPassword.get());
    *pBufferLength = dwPasswordLen;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwPasswordLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrPassword)
    {
        memcpy(pszBuffer, bstrPassword.get(), dwPasswordLen * sizeof(WCHAR));
    }
    pszBuffer[dwPasswordLen] = 0;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetPath
//
//  Synopsis:
//      Returns the path string from the URI
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetPath(_Inout_ XUINT32 * pBufferLength,
                 _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const
{
    XUINT32 buflen = *pBufferLength;
    wil::unique_bstr bstrPath;

    IFC_RETURN(m_pUri->GetPath(&bstrPath));
    DWORD dwPathLen = SysStringLen(bstrPath.get());
    *pBufferLength = dwPathLen;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwPathLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrPath)
    {
        memcpy(pszBuffer, bstrPath.get(), dwPathLen * sizeof(WCHAR));
    }

    pszBuffer[dwPathLen] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetScheme
//
//  Synopsis:
//      Returns the scheme string from the URI.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetScheme(_Inout_ XUINT32 * pBufferLength,
                   _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const
{
    XUINT32 buflen = *pBufferLength;
    wil::unique_bstr bstrScheme;

    IFC_RETURN(m_pUri->GetSchemeName(&bstrScheme));
    DWORD dwSchemeLen = SysStringLen(bstrScheme.get());
    *pBufferLength = dwSchemeLen;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwSchemeLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrScheme)
    {
        memcpy(pszBuffer, bstrScheme.get(), dwSchemeLen * sizeof(WCHAR));
    }

    pszBuffer[dwSchemeLen] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetUsername
//
//  Synopsis:
//      Returns the username string from the URI.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CWinUriImpl::GetUsername(_Inout_ XUINT32 * pBufferLength,
                     _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer)
{
    XUINT32 buflen = *pBufferLength;
    wil::unique_bstr bstrUser;

    IFC_RETURN(m_pUri->GetUserName(&bstrUser));
    DWORD dwUserLen = SysStringLen(bstrUser.get());
    *pBufferLength = dwUserLen;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwUserLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrUser)
    {
        memcpy(pszBuffer, bstrUser.get(), dwUserLen * sizeof(WCHAR));
    }

    pszBuffer[dwUserLen] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetQueryString
//
//  Synopsis:
//      Returns the query string from the URI.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::GetQueryString(_Inout_ XUINT32 * pBufferLength,
                        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer)
{
    XUINT32 buflen = *pBufferLength;
    wil::unique_bstr bstrQuery;

    IFC_RETURN(m_pUri->GetQuery(&bstrQuery));
    DWORD dwQueryLen = SysStringLen(bstrQuery.get());

    *pBufferLength = dwQueryLen;

    if (pszBuffer == nullptr)
    {
        return S_OK;
    }

    if (buflen < dwQueryLen + 1)
    {
        return E_OUTOFMEMORY;
    }

    if (bstrQuery)
    {
        memcpy(pszBuffer, bstrQuery.get(), dwQueryLen * sizeof(WCHAR));
    }

    pszBuffer[dwQueryLen] = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetPortNumber
//
//  Synopsis:
//      Returns the port number.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::GetPortNumber(_Out_ XUINT32 *pPortNumber)
{
    IFC_RETURN(m_pUri->GetPort((DWORD*)pPortNumber));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transforms this URI to an ms-resource URI by replacing the scheme and adding a
//      "/Files/" prefix to the path.
//
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CWinUriImpl::TransformToMsResourceUri(_Outptr_ IPALUri **ppUri) const
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strFilesPrefix, L"/Files");
    XStringBuilder adjustedPathBuilder;
    wrl::ComPtr<IUriBuilder> spUriBuilder;
    LPCWSTR wszPath = nullptr;
    DWORD dwPathLen = 0;

    auto spUri = make_xref<CWinUriImpl>();

    IFC_RETURN(CreateIUriBuilder(m_pUri, 0, 0, spUriBuilder.GetAddressOf()));

    IFC_RETURN(spUriBuilder->SetSchemeName(L"ms-resource"));

    IFC_RETURN(spUriBuilder->GetPath(&dwPathLen, &wszPath));

    // Prefix path with '/Files' and a '/' if the path doesn't already start with one
    IFC_RETURN(adjustedPathBuilder.Initialize(dwPathLen + c_strFilesPrefix.GetCount() + 1));
    IFC_RETURN(adjustedPathBuilder.Append(c_strFilesPrefix));

    if (dwPathLen && wszPath[0] != L'/')
    {
        IFC_RETURN(adjustedPathBuilder.AppendChar(L'/'));
    }

    IFC_RETURN(adjustedPathBuilder.Append(xephemeral_string_ptr(wszPath, dwPathLen)));

    IFC_RETURN(spUriBuilder->SetPath(adjustedPathBuilder.GetBuffer()));

    IFC_RETURN(spUriBuilder->CreateUri(0, 0, 0, &spUri->m_pUri));

    spUri->m_componentResourceLocation = m_componentResourceLocation;

    *ppUri = spUri.detach();

    return S_OK;
}

void CWinUriImpl::SetComponentResourceLocation(_In_ ComponentResourceLocation resourceLocation)
{
    m_componentResourceLocation = resourceLocation;
}

//------------------------------------------------------------------------
//
//  Method:   GetFilePath
//
//  Synopsis: Returns path to file given by uri if it's a file uri
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWinUriImpl::GetFilePath(
    _Inout_ XUINT32 * pBufferLength,
    _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const
{
    WCHAR strPath[MAX_PATH];
    DWORD cPath = ARRAY_SIZE(strPath);

    wil::unique_bstr bstrScheme;
    wil::unique_bstr bstrUri ;

    // This function only works for file uris
    IFC_RETURN(m_pUri->GetSchemeName(&bstrScheme));
    IFCEXPECT_ASSERT_RETURN(xstrncmpi(L"file", bstrScheme.get(), SysStringLen(bstrScheme.get())) == 0);

    IFC_RETURN(m_pUri->GetAbsoluteUri(&bstrUri));
    IFC_RETURN(PathCreateFromUrl(bstrUri.get(), strPath, &cPath, 0));

     if (pszBuffer == nullptr)
     {
         *pBufferLength = cPath;
         return S_OK;
     }

     if (*pBufferLength < cPath + 1)
     {
         return E_OUTOFMEMORY;
     }

     *pBufferLength = cPath;
     memcpy(pszBuffer, strPath, cPath * sizeof(WCHAR));

     pszBuffer[cPath] = 0;

     return S_OK;
}

ComponentResourceLocation CWinUriImpl::GetComponentResourceLocation() const
{
    return m_componentResourceLocation;
}
