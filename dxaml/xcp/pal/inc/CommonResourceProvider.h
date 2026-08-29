// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PalResourceManager.h"

class CommonResourceProvider final : public IPALResourceProvider, private CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ CommonResourceProvider **ppResourceProvider);
    ~CommonResourceProvider() override;

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    // IPALResourceProvider

    _Check_return_ HRESULT TryGetLocalResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) override;

    _Check_return_ HRESULT GetString(
        _In_ const xstring_ptr_view& key,
        _Out_ xstring_ptr* pstrString
        ) override;

    _Check_return_ HRESULT GetPropertyBag(
        _In_ const IPALUri *pUri,
        _Out_ PropertyBag& propertyBag
        ) noexcept override;

    _Check_return_ HRESULT SetScaleFactor(
        XUINT32 ulScaleFactor
        ) override;

    _Check_return_ HRESULT NotifyThemeChanged(
        ) override;

    _Check_return_ HRESULT SetProcessMUILanguages(
        ) override;

    void DetachEvents() override;

private:
    CommonResourceProvider();

    IPALUri *m_pBaseUri;
};
