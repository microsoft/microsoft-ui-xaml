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

    class MockUri : public IPALUri
    {
        friend class Microsoft::UI::Xaml::Tests::Parser::XamlNodeStreamCacheManagerUnitTests;

    public:
        MockUri()
            : m_cRef(1)
        {}

        // IPALUri implementation
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

        _Check_return_ HRESULT Clone(_Out_ IPALUri **ppUri) const override
        { return S_OK; }
        _Check_return_ HRESULT Combine(_In_ UINT32 cUri, _In_reads_(cUri) const WCHAR* pUri, _Outptr_ IPALUri** ppUriCombine) override
        { return S_OK; }
        _Check_return_ HRESULT CreateBaseURI(_Out_ IPALUri **ppBaseUri) override
        { return S_OK; }
        _Check_return_ HRESULT GetCanonical(_Inout_ UINT32 * pBufferLength, _Out_writes_to_opt_(*pBufferLength, *pBufferLength) WCHAR * pszBuffer) const override;
        _Check_return_ HRESULT GetCanonical(_Out_ xstring_ptr* pstrCanonical) const override;
        _Check_return_ HRESULT GetExtension(_Inout_ UINT32 * pBufferLength, _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override
        { return S_OK; }
        _Check_return_ HRESULT GetFileName(_Inout_ UINT32 * pBufferLength, _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override
        { return S_OK; }
        _Check_return_ HRESULT GetHost(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override
        { return S_OK; }
        _Check_return_ HRESULT GetPassword(_Inout_ UINT32 * pBufferLength, _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override
        { return S_OK; }
        _Check_return_ HRESULT GetPath(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override
        { return S_OK; }
        _Check_return_ HRESULT GetScheme(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override
        { return S_OK; }
        _Check_return_ HRESULT GetUsername(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) override
        { return S_OK; }
        _Check_return_ HRESULT GetQueryString(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) override
        { return S_OK; }
        _Check_return_ HRESULT GetPortNumber(_Out_ UINT32 *pPortNumber) override
        { return S_OK; }
        _Check_return_ HRESULT GetFilePath(_Inout_ UINT32 * pBufferLength, _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override
        { return S_OK; }
        _Check_return_ HRESULT TransformToMsResourceUri(_Outptr_ IPALUri **ppUri) const override
        { return S_OK; }
        void SetComponentResourceLocation(_In_ ComponentResourceLocation resourceLocation) override
        { }
        ComponentResourceLocation GetComponentResourceLocation() const override
        { return ComponentResourceLocation::Application; }

        std::function<HRESULT(xstring_ptr*)> GetCanonicalCallback;
        std::function<HRESULT(UINT32*, WCHAR*)> GetCanonicalCallback2;

    private:
        mutable uint32_t m_cRef;
    };

} } } } }

