// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define USE_SHARED_PTR_POOLED_ALLOCATOR 1

#include <minxcptypes.h>
#include <minerror.h>
#include <palcore.h>
#include <IParserCoreServices.h>

struct IErrorService;
class XamlNodeStreamCacheManager;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class XamlNodeStreamCacheManagerUnitTests;

    class MockResourceManager : public IPALResourceManager
    {
        friend class Microsoft::UI::Xaml::Tests::Parser::XamlNodeStreamCacheManagerUnitTests;

    public:
        MockResourceManager()
            : m_cRef(1)
        {}

        // IObject implementation
        uint32_t AddRef() override
        {
            return ++m_cRef;
        }

        uint32_t Release() override
        {
            uint32_t newRefCount = --m_cRef;
            if (m_cRef == 0)
            {
                delete this;
            }
            return newRefCount;
        }

        // IPALResourceManager implementation
        _Check_return_ HRESULT IsLocalResourceUri(_In_ IPALUri* pUri, _Out_ bool* pfIsLocal) override
        { return S_OK; }
        _Check_return_ HRESULT TryGetLocalResource(_In_ IPALUri* pResourceUri, _Outptr_result_maybenull_ IPALResource** ppResource) override;
        _Check_return_ HRESULT TryResolveUri(
            _In_ const xstring_ptr_view& strUri,
            _In_opt_ IPALUri* pBaseUri,
            _Outptr_result_maybenull_ IPALResource** ppResource) override
        { return S_OK; }
        _Check_return_ HRESULT CanResourceBeInvalidated(
            _In_ IPALUri* resourceUri,
            _Out_ bool* canBeInvalidated) override
        { return S_OK; }
        UINT32 GetResourceInvalidationId() override
        { return S_OK; }
        _Check_return_ HRESULT GetPropertyBag(
            _In_ const xstring_ptr_view& strUid,
            _In_ const IPALUri *pBaseUri,
            _Out_ PropertyBag& propertyBag) override
        { return S_OK; }
        _Check_return_ HRESULT CombineResourceUri(
            _In_ IPALUri *pBaseUri,
            _In_ const xstring_ptr_view& strFragment,
            _Outptr_ IPALUri **ppCombinedUri) override
        { return S_OK; }
        _Check_return_ HRESULT IsAmbiguousUriFragment(_In_ const xstring_ptr_view& strUriFragment, _Out_ bool* pIsAmbiguous) override
        { return S_OK; }
        _Check_return_ HRESULT CanCacheResource(_In_ const IPALUri *pUri, _Out_ bool* pCanCache) override
        { return S_OK; }
        _Check_return_ HRESULT SetScaleFactor(UINT32 ulScaleFactor) override
        { return S_OK; }
        _Check_return_ HRESULT NotifyThemeChanged() override
        { return S_OK; }
        _Check_return_ HRESULT SetProcessMUILanguages() override
        { return S_OK; }
        _Check_return_ HRESULT GetUriForPropertyBagLookup(_In_ const xstring_ptr_view& strXUid, _In_ const xref_ptr<IPALUri>& spBaseUri, _Out_ xref_ptr<IPALUri>& spPropertyBagUri) override
        { return S_OK; }

        void DetachEvents() override {}

        std::function<HRESULT(IPALUri*, IPALResource**)> TryGetLocalResourceCallback;

    private:
        mutable uint32_t m_cRef;
    };

    class MockResource : public IPALResource
    {
        friend class Microsoft::UI::Xaml::Tests::Parser::XamlNodeStreamCacheManagerUnitTests;

    public:
        MockResource()
            : m_cRef(1)
        {}

        // IObject implementation
        uint32_t AddRef() override
        {
            return ++m_cRef;
        }

        uint32_t Release() override
        {
            uint32_t newRefCount = --m_cRef;
            if (m_cRef == 0)
            {
                delete this;
            }
            return newRefCount;
        }

        // IPALResource
        _Check_return_ HRESULT Load(_Outptr_ IPALMemory** ppMemory) override
        { return S_OK; }
        const WCHAR* ToString() override
        { return nullptr; }
        IPALUri* GetResourceUriNoRef() override
        { return nullptr; }
        IPALUri* GetPhysicalResourceUriNoRef() override;
        _Check_return_ HRESULT IsLocal(_Out_ bool* pfLocal) override
        { return S_OK; }
        _Check_return_ HRESULT Exists(_Out_ bool* pfExists) override
        { return S_OK; }
        _Check_return_ HRESULT TryGetFilePath(_Out_ xstring_ptr* pstrFilePath) override
        { return S_OK; }
        _Check_return_ HRESULT TryGetRawStream(const PALResources::RawStreamType streamType, _Outptr_result_maybenull_ void** ppStream) override
        { return S_OK; }
        UINT32 GetScalePercentage() override
        { return 0U; }

        std::function<IPALUri*()> GetPhysicalResourceUriNoRefCallback;

    private:
        mutable uint32_t m_cRef;
    };

} } } } }

