// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <wininet.h>

CommonResourceProvider::CommonResourceProvider()
{
    m_pBaseUri = NULL;
}

CommonResourceProvider::~CommonResourceProvider()
{
    ReleaseInterface(m_pBaseUri);
}

_Check_return_
HRESULT CommonResourceProvider::Create(_Outptr_ CommonResourceProvider **ppResourceProvider)
{
    HRESULT hr = S_OK;
    CommonResourceProvider *pResourceProvider = NULL;
    WCHAR applicationDirectory[MAX_PATH] = {0};
    WCHAR baseUri[INTERNET_MAX_URL_LENGTH + 1] = {0};
    DWORD baseUriLength = INTERNET_MAX_URL_LENGTH;
    DWORD result = 0;

    pResourceProvider = new CommonResourceProvider();

    result = GetModuleFileName(NULL, applicationDirectory, MAX_PATH);
    if (result == 0)
    {
        IFC(HRESULT_FROM_WIN32(GetLastError()));
    }

    IFC(UrlCreateFromPath(applicationDirectory, baseUri, &baseUriLength, NULL));

    IFC(gps->UriCreate(baseUriLength, baseUri, &pResourceProvider->m_pBaseUri));

    *ppResourceProvider = pResourceProvider;
    pResourceProvider = NULL;

Cleanup:
    ReleaseInterface(pResourceProvider);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Retrieves a physical file path for a given file URI.
//
//  Notes:
//      ms-resource URIs are supported for files, and the resource map (if present)
//      is treated as a subdirectory.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT CommonResourceProvider::TryGetLocalResource(
    _In_ IPALUri* pResourceUri,
    _Outptr_result_maybenull_ IPALResource** ppResource
    )
{
    HRESULT hr = S_OK;
    xstring_ptr strResourceMap;
    xstring_ptr strFileRelativePath;
    bool hadFileRelativePath = false;
    IPALUri* pFileUri = NULL;
    WCHAR *pszPath = NULL;
    XUINT32 nPath = 0;
    xstring_ptr strCanonicalUri;
    bool isMsResourceUri = false;
    IPALResource* pResource = NULL;
    bool fResourceExists;

    *ppResource = NULL;

    IFC(MsUriHelpers::IsMsResourceUri(pResourceUri, &isMsResourceUri));
    if (!isMsResourceUri)
    {
        IFC(E_INVALIDARG);
    }

    IFC(pResourceUri->GetCanonical(&strCanonicalUri));

    IFC(MsUriHelpers::CrackMsResourceUri(pResourceUri, &strResourceMap, NULL, &strFileRelativePath, &hadFileRelativePath));
    if (!hadFileRelativePath)
    {
        IFC(E_INVALIDARG);
    }
    IFC(m_pBaseUri->Combine(strFileRelativePath.GetCount(), strFileRelativePath.GetBuffer(), &pFileUri));

    IFC(pFileUri->GetFilePath(&nPath, NULL));

    pszPath = new WCHAR[++nPath];
    IFC(pFileUri->GetFilePath(&nPath, pszPath));

    IFC(CFilePathResource::Create(pResourceUri, pszPath, nPath, &pResource));
    IFC(pResource->Exists(&fResourceExists));
    if (!fResourceExists)
    {
        if (gps->IsDebugTraceTypeActive(XCP_TRACE_RESOURCELOADING))
        {
            IGNOREHR(gps->DebugTrace(XCP_TRACE_RESOURCELOADING, L"CommonResourceProvider::TryGetLocalResource: file %s does not exist", pszPath));
        }
        goto Cleanup;
    }

    if (gps->IsDebugTraceTypeActive(XCP_TRACE_RESOURCELOADING))
    {
        IGNOREHR(gps->DebugTrace(XCP_TRACE_RESOURCELOADING, L"CommonResourceProvider::TryGetLocalResource: %s -> %s", strCanonicalUri.GetBuffer(), pResource->ToString()));
    }
    *ppResource = pResource;
    pResource = NULL;

Cleanup:
    delete[] pszPath;
    ReleaseInterface(pFileUri);
    ReleaseInterface(pResource);
    RRETURN(hr);
}

_Check_return_
HRESULT CommonResourceProvider::GetString(
    _In_ const xstring_ptr_view& key,
    _Out_ xstring_ptr* pstrString)
{
    RRETURN(E_NOTIMPL);
}

//---------------------------------------------------------------------------
//
//  Notes:
//      Returns an empty property bag.
//
//---------------------------------------------------------------------------
_Check_return_
HRESULT CommonResourceProvider::GetPropertyBag(
    _In_ const IPALUri *pUri,
    _Out_ PropertyBag& propertyBag
    ) noexcept
{
    propertyBag.clear();

    return S_OK;
}

_Check_return_
HRESULT CommonResourceProvider::SetScaleFactor(
    XUINT32 ulScaleFactor
    )
{
    // No-op: the CommonResourceProvider does not use scale factor.
    RRETURN(S_OK);
}

_Check_return_
HRESULT CommonResourceProvider::NotifyThemeChanged()
{
    // No-op: the CommonResourceProvider does not use theme.
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// See IPALResourceManager::SetProcessMUILanguages().
//
//-----------------------------------------------------------------------------
_Check_return_
HRESULT CommonResourceProvider::SetProcessMUILanguages(
    )
{
    return Mui_PropagateApplicationLanguages();
}

void CommonResourceProvider::DetachEvents()
{
    // Do nothing. This never attached any events.
}
