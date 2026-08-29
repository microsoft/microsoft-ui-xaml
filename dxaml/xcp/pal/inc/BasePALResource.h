// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PalResourceManager.h"

//-----------------------------------------------------------------------------
//
// CBasePALResource represents the most abstract kind of resource. The only
// information it stores is a resource URI.
//
// All other implementations of IPALResource derive from CBasePALResource and
// provide additional information about the resource they represent.
//
//-----------------------------------------------------------------------------
class CBasePALResource : public IPALResource, private CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(_In_ IPALUri* pResourceUri, _Outptr_ CBasePALResource** ppResource);
    static _Check_return_ HRESULT Create(_In_ IPALUri* pResourceUri, _Outptr_ IPALResource** ppResource);

    static _Check_return_ HRESULT IsLocalResourceUri(_In_ IPALUri* pUri, _Out_ bool* pfLocal);

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    // IPALResource
    _Check_return_ HRESULT Load(_Outptr_ IPALMemory** ppMemory) override;
    const WCHAR* ToString() override;
    IPALUri* GetResourceUriNoRef() override;
    IPALUri* GetPhysicalResourceUriNoRef() override;
    _Check_return_ HRESULT IsLocal(_Out_ bool* pfLocal) override;
    _Check_return_ HRESULT Exists(_Out_ bool* pfExists) override;
    _Check_return_ HRESULT TryGetFilePath(_Out_ xstring_ptr* pstrFilePath) override;
    _Check_return_ HRESULT TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream) override;
    XUINT32 GetScalePercentage() override;

protected:
    CBasePALResource(_In_ IPALUri* pResourceUri) :
        m_pResourceUri(pResourceUri),
        m_strToString()
    {
        AddRefInterface(m_pResourceUri);
    }

    ~CBasePALResource() override
    {
        ReleaseInterface(m_pResourceUri);
    }

private:
    IPALUri* m_pResourceUri;
    xstring_ptr m_strToString;
};
