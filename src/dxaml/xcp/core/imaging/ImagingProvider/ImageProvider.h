// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageProviderInterfaces.h"
#include "ImagingInterfaces.h"
#include <flags_enum.h>
#include "ImagingTelemetry.h"

class CCoreServices;
class CValueStore;
class ImageTaskDispatcher;

enum class GetImageOptions
{
    None        = 0x00,
    IgnoreCache = 0x01,
    Synchronous = 0x02
};

template <>
struct is_flags_enum<GetImageOptions>
{
    static constexpr bool value = true;
};

class ImageProvider : public CXcpObjectBase< >,
                      private IInvalidateImageCacheCallback
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CCoreServices* pCore,
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ IAsyncImageFactory* pImageFactory,
        _Outptr_ ImageProvider** ppImageProvider
        );

    _Check_return_ HRESULT GetImage(
        _In_ const std::shared_ptr<EncodedImageData>& spEncodedImageData,
        _In_ xref_ptr<ImageDecodeParams>& spDecodeParams,
        GetImageOptions options,
        _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
        _In_ CCoreServices* pCore,
        _Out_ xref_ptr<IAbortableImageOperation>& spAbortableImageOperation
        );

    _Check_return_ HRESULT CopyImage(
        _In_ xref_ptr<ImageCopyParams>& spCopyParams,
        _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
        _In_ CCoreServices* pCore,
        _Out_ xref_ptr<IAbortableImageOperation>& spAbortableImageOperation
        );

    _Check_return_ HRESULT CleanupCaches();
    _Check_return_ HRESULT Clear();

    _Check_return_ HRESULT EnsureCacheEntry(
        _In_ const xstring_ptr& strUri,
        _In_opt_ IPALUri *pAbsoluteUri,
        bool isSvg,
        GetImageOptions options,
        uint64_t imageId,
        _Out_ bool *pCacheHit,
        _Outptr_ ImageCache **ppImageCache
        );

private:
    ImageProvider();
    ~ImageProvider() override;

    _Check_return_ HRESULT Initialize(
        _In_ CCoreServices* pCore,
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ IAsyncImageFactory* pImageFactory
        );

    _Check_return_ HRESULT GetImageCache(
        _In_ const xstring_ptr& strUri,
        _Outptr_result_maybenull_ ImageCache** ppCache
        );

    _Check_return_ HRESULT AddImageCache(_In_ ImageCache* pCache);

    _Check_return_ HRESULT RemoveImageCache(_In_ ImageCache* pCache);

    void OnImageCacheInvalidated(_In_ ImageCache* pCache) override;

    static void CleanupImageCacheStoreValue(
        _In_ const xstring_ptr& strKey,
        XHANDLE value,
        XHANDLE extraData
        );

    _Check_return_ HRESULT MakeCacheKey(_In_ IPALUri* pUri, _Out_ xstring_ptr* pstrCacheKey);

    _Check_return_ HRESULT ShouldUseCache(
        _In_opt_ IPALUri* absoluteUri,
        GetImageOptions options,
        _Out_ bool* useCache
        );

    CCoreServices* m_pCore;
    IAsyncImageFactory* m_pImageFactory;
    CValueStore* m_pStore;
    ImageTaskDispatcher* m_pDispatcher;
};
