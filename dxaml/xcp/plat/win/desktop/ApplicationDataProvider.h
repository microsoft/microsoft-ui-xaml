// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PalResourceManager.h"

// Implements the ms-appdata protocol
class ApplicationDataProvider final : public IPALApplicationDataProvider, private CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ ApplicationDataProvider **ppAppDataProvider);

    ~ApplicationDataProvider();

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    // IPALApplicationDataProvider
    virtual _Check_return_ HRESULT GetAppDataResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_ IPALResource** ppResource
        );

private:
    ApplicationDataProvider();
    _Check_return_ HRESULT Initialize();
    static _Check_return_ HRESULT GetStateFolderUris(
        _Outptr_ IPALUri **ppLocalFolder,
        _Outptr_ IPALUri **ppRoamingFolder,
        _Outptr_ IPALUri **ppTempFolder
        );
    static _Check_return_ HRESULT GetPackageIdentityName(_Out_ xstring_ptr* pstrPackageIdentityName);

    static _Check_return_ HRESULT PathAddBackslashHString(_Inout_ wrl_wrappers::HString& path);

    static _Check_return_
        HRESULT GetStateFolderUrisUsingApplicationData(
            const wrl::ComPtr<wst::IStorageFolder> &spStorageFolder,
            _Outptr_ IPALUri **folder
        );

    IPALUri *m_pLocalFolder;
    IPALUri *m_pTempFolder;
    IPALUri *m_pRoamingFolder;
    xstring_ptr m_strPackageIdentityName;
};


