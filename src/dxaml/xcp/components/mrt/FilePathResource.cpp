// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PalResourceManager.h"
#include "BasePALResource.h"
#include "FilePathResource.h"
#include "FileMappedMemory.h"

// Creates a CFilePathResource for the given resource URI and file path.
_Check_return_ HRESULT CFilePathResource::Create(_In_ IPALUri* pResourceUri, _In_ const xstring_ptr& strFilePath, _Outptr_ IPALResource** ppResource)
{
    HRESULT hr = S_OK;
    xref_ptr<CFilePathResource> pResource;

    pResource = make_xref<CFilePathResource>(pResourceUri);

    pResource->m_strFilePath = strFilePath;

    *ppResource = pResource.detach();

    RRETURN(hr);//RRETURN_REMOVAL
}

// Creates a CFilePathResource for the given resource URI and file path.
_Check_return_ HRESULT CFilePathResource::Create(_In_ IPALUri* pResourceUri, _In_reads_(cchFilePath) WCHAR* wszFilePath, unsigned int cchFilePath, _Outptr_ IPALResource** ppResource)
{
    xref_ptr<CFilePathResource> pResource;

    pResource = make_xref<CFilePathResource>(pResourceUri);

    IFC_RETURN(xstring_ptr::CloneBuffer(
        wszFilePath,
        cchFilePath,
        &(pResource->m_strFilePath)));

    *ppResource = pResource.detach();

    return S_OK;
}

CFilePathResource::CFilePathResource(_In_ IPALUri* pResourceUri)
    : CBasePALResource(pResourceUri)
    , m_strFilePath()
{
}

CFilePathResource::~CFilePathResource()
{
}

// See IPALResource::Load().
// CFilePathResource implements Load() by opening the file and reading it into
// memory, using PAL functionality.
_Check_return_ HRESULT CFilePathResource::Load(_Outptr_ IPALMemory** ppMemory)
{
    if (m_memory == nullptr)
    {
        IFC_RETURN(FileMappedMemory::Create(m_strFilePath, m_memory.ReleaseAndGetAddressOf()));
    }

    m_memory.CopyTo(ppMemory);

    return S_OK;
}

// See IPALResource::ToString().
// CFilePathResource implements ToString() by returning the file path this
// resource represents.
const WCHAR* CFilePathResource::ToString()
{
    return m_strFilePath.GetBuffer();
}

// See IPALResource::Exists().
// CFilePathResource implements Exists() by calling GetFileAttributesW() to
// check whether the file path this resource represents is an existing file.
_Check_return_ HRESULT CFilePathResource::Exists(_Out_ bool* pfExists)
{
    *pfExists = GetFileAttributesW(m_strFilePath.GetBuffer()) != INVALID_FILE_ATTRIBUTES;

    return S_OK;
}

// See IPALResource::TryGetFilePath().
_Check_return_ HRESULT CFilePathResource::TryGetFilePath(_Out_ xstring_ptr* pstrFilePath)
{
    *pstrFilePath = m_strFilePath;

    RRETURN(S_OK);
}


