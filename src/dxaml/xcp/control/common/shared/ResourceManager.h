// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "refcounting.h"
#include "paltypes.h"

struct IPALResourceProvider;
struct IPALUri;
class CCoreServices;

class ResourceManager final : public IPALResourceManager, private CReferenceCount
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices *pCore,
        _Outptr_ IPALResourceManager **ppResourceManager
        );

    ~ResourceManager() override;

    FORWARD_ADDREF_RELEASE(CReferenceCount);

    // IPALResourceManager

    _Check_return_ HRESULT IsLocalResourceUri(
        _In_ IPALUri* pUri,
        _Out_ bool* pfIsLocal
        ) override;

    _Check_return_ HRESULT TryGetLocalResource(
        _In_ IPALUri* pResourceUri,
        _Outptr_result_maybenull_ IPALResource** ppResource
        ) override;

    _Check_return_ HRESULT TryResolveUri(
        _In_ const xstring_ptr_view& strUri,
        _In_opt_ IPALUri* pBaseUri,
        _Outptr_result_maybenull_ IPALResource** ppResource) override;

    _Check_return_ HRESULT CanResourceBeInvalidated(
        _In_ IPALUri* resourceUri,
        _Out_ bool* canBeInvalidated
        ) override;

    XUINT32 GetResourceInvalidationId(
        ) override;

    _Check_return_
    HRESULT GetPropertyBag(
           _In_ const xstring_ptr_view& xUid,
           _In_ const IPALUri *pBaseUri,
           _Out_ PropertyBag& propertyBag
        ) override;

    _Check_return_ HRESULT CombineResourceUri(
        _In_ IPALUri *pBaseUri,
        _In_ const xstring_ptr_view& strFragment,
        _Outptr_ IPALUri **ppCombinedUri
        ) override;

    _Check_return_ HRESULT IsAmbiguousUriFragment(
        _In_ const xstring_ptr_view& strUriFragment,
        _Out_ bool *pIsAmbiguous
        ) override;

    _Check_return_ HRESULT CanCacheResource(
        _In_ const IPALUri *pUri,
        _Out_ bool *pCanCache
        ) override;

    _Check_return_ HRESULT SetScaleFactor(
        XUINT32 ulScaleFactor
        ) override;

    _Check_return_ HRESULT NotifyThemeChanged(
        ) override;

    _Check_return_ HRESULT SetProcessMUILanguages(
        ) override;

    void DetachEvents() override;

    _Check_return_ HRESULT GetUriForPropertyBagLookup(
        _In_ const xstring_ptr_view& strXUid,
        _In_ const xref_ptr<IPALUri>& spBaseUri,
        _Out_ xref_ptr<IPALUri>& spPropertyBagUri
        ) override;

protected:
    ResourceManager(
        _In_ CCoreServices *pCore,
        _In_ IPALResourceProvider *pResourceProvider
        );

    CCoreServices *m_pCore;
    IPALResourceProvider *m_pResourceProvider;
    IPALApplicationDataProvider *m_pAppDataProvider;
    XUINT32 m_resourceInvalidationId;

private:
    _Check_return_ HRESULT GetAppDataProviderNoRef(_Outptr_ IPALApplicationDataProvider** provider);

};
