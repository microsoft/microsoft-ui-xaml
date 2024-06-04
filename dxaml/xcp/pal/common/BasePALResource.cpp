// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BasePALResource.h"
#include "UriXStringGetters.h"

//-----------------------------------------------------------------------------
//
// The implementation of IPALResource::IsLocal() and
// IPALResourceManager::IsLocalResourceUri().
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::IsLocalResourceUri(_In_ IPALUri* pUri, _Out_ bool* pfLocal)
{
    HRESULT hr = S_OK;
    xstring_ptr strResourceScheme;

    *pfLocal = false;

    IFC(UriXStringGetters::GetScheme(pUri, &strResourceScheme));

    if (   strResourceScheme.Equals(STR_LEN_PAIR(L"ms-resource"))
        || strResourceScheme.Equals(STR_LEN_PAIR(L"ms-appx"))
        || strResourceScheme.Equals(STR_LEN_PAIR(L"ms-appdata")))
    {
        *pfLocal = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Create a CBasePALResource that has the given resource URI.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::Create(_In_ IPALUri* pResourceUri, _Outptr_ IPALResource** ppResource)
{
    CBasePALResource* pResource = NULL;
    HRESULT hr = Create(pResourceUri, &pResource);
    *ppResource = pResource;
    return hr;
}

//-----------------------------------------------------------------------------
//
// Create a CBasePALResource that has the given resource URI.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::Create(_In_ IPALUri* pResourceUri, _Outptr_ CBasePALResource** ppResource)
{
    HRESULT hr = S_OK;
    CBasePALResource* pResource = NULL;

    pResource = new CBasePALResource(pResourceUri);

    *ppResource = pResource;
    RRETURN(hr);//RRETURN_REMOVAL
}

//-----------------------------------------------------------------------------
//
// See IPALResource::Load().
//
// CBasePALResource doesn't implement Load(). Subclasses may override and
// provide an implementation.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::Load(_Outptr_ IPALMemory** ppMemory)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::ToString().
//
// CBasePALResource implements ToString() by returning the resource URI.
// Subclasses may override and provide a more informative implementation.
//
//-----------------------------------------------------------------------------
const WCHAR* CBasePALResource::ToString()
{
    if (m_strToString.IsNull())
    {
        if (FAILED(m_pResourceUri->GetCanonical(&m_strToString)))
        {
            return L"[CBasePALResource - NOTIMPL]";
        }
    }

    return m_strToString.GetBuffer();
}

//-----------------------------------------------------------------------------
//
// See IPALResource::GetResourceUriNoRef().
//
// CBasePALResource is responsible for storing the resource URI and
// implementing GetResourceUriNoRef(). Subclasses will not generally override
// this implementation.
//
//-----------------------------------------------------------------------------
IPALUri* CBasePALResource::GetResourceUriNoRef()
{
    return m_pResourceUri;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::GetPhysicalResourceUriNoRef().
//
// CBasePALResource is responsible for storing the resource URI and
// implementing GetPhysicalResourceUriNoRef(). Subclasses will not generally override
// this implementation.
//
//-----------------------------------------------------------------------------
IPALUri* CBasePALResource::GetPhysicalResourceUriNoRef()
{
    return m_pResourceUri;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::IsLocal().
//
// CBasePALResource is responsible for implementing IsLocal(), since it stores
// the resource URI. Subclasses will not generally override this implementation.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::IsLocal(_Out_ bool* pfLocal)
{
    return IsLocalResourceUri(m_pResourceUri, pfLocal);
}

//-----------------------------------------------------------------------------
//
// See IPALResource::Exists().
//
// CBasePALResource doesn't implement Exists(). Subclasses may override and
// provide an implementation.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::Exists(_Out_ bool* pfExists)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::TryGetFilePath().
//
// CBasePALResource always returns NULL from TryGetFilePath(). Subclasses may
// override and provide an implementation if they represent a resource that
// has an available file path.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::TryGetFilePath(_Out_ xstring_ptr* pstrFilePath)
{
    pstrFilePath->Reset();
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::TryGetRawStream().
//
// CBasePALResource always returns NULL from TryGetRawStream(). Subclasses may
// override and provide an implementation if they represent a resource that
// has an available stream.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CBasePALResource::TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream)
{
    *ppStream = NULL;
    return S_OK;
}

//-----------------------------------------------------------------------------
//
// See IPALResource::GetScalePercentage().
//
// CBasePALResource always returns 0 from GetScalePercentage() - meaning the
// resource has no associated scale. Subclasses may override and provide a
// scale value if they represent a resource that has an associated scale.
//
//-----------------------------------------------------------------------------
XUINT32 CBasePALResource::GetScalePercentage()
{
    return 0;
}


