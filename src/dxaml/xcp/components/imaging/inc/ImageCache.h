// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImageProviderInterfaces.h"
#include "ImagingTelemetry.h"
#include "PixelFormat.h"

class AsyncImageDecoder;
class ImageCacheDownloadCallbackMarshaller;
class ImageDecodeParams;
class ImageDecodeRequest;
class ImageMetadataView;
class ImageMetadataViewImpl;
class ImageTaskDispatcher;
class EncodedImageData;
struct IAbortableImageOperation;
struct IPALAbortableOperation;
struct IPALDownloadResponse;

class CCoreServices;

enum class ImageCacheState
{
    NotDownloaded,
    Downloading,
    DownloadFailed,
    Downloaded
};

//------------------------------------------------------------------------
//
//  Images have 3 things that can be cached:
//  1. The encoded bits
//  2. The decoded bits, in a software surface
//  3. The hardware surface, either a HWTexture or a bunch of tiles
//
//  The ImageCache holds items 1 and 2. Each ImageCache holds software bits for a single URI.
//  Images created from streams are not cached. There is a single EncodedImageCache for the
//  encoded bits, and one DecodedImageCache for each pixel format/decode width/decode height
//  combination for the image.
//
//  The ImageProvider holds a list of ImageCaches, keyed by the URI. There is one ImageProvider
//  for the entire app.
//
//  Item 3 is held in the SurfaceCache. The SurfaceCache doesn't behave like the ImageCache,
//  but instead like the ImageProvider. There is only one SurfaceCache for the entire app, and
//  it holds a collection of hardware resources, keyed by the URI/decode width/decode height
//  combination. Pixel formats don't matter, since we only use pixelColor32bpp_A8R8G8B8.
//
//  This inconsistency can be removed by moving the hardware surface caches into the ImageCache
//  and having the ImageProvider handle everything. We would also be able to unify the code path
//  for decoded software surface lookup and hardware surface lookup.
//
//------------------------------------------------------------------------

class ImageCache
    : public IImageDecodeCallback
    , private IInvalidateDecodedImageCacheCallback
{
public:
    ImageCache(
        _In_ const xstring_ptr& strCacheKey,
        _In_ const xstring_ptr& strUri,
        _In_ IPALUri* pAbsoluteUri,
        bool isSvg,
        _In_ CCoreServices* pCore,
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ bool ignoreNetworkCache,
        _In_opt_ IInvalidateImageCacheCallback* pInvalidateCallback
        );

    std::shared_ptr<ImageMetadataView> GetMetadataView(uint64_t imageId);

    _Check_return_ HRESULT GetImage(
        _In_ xref_ptr<ImageDecodeParams>& spDecodeParams,
        _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
        _Out_ xref_ptr<IAbortableImageOperation>& spAbortableImageOperation
        );

    std::shared_ptr<EncodedImageData> GetEncodedImageData() const;
    void SetEncodedImageData(std::shared_ptr<EncodedImageData> spEncodedImageData);

    const xstring_ptr& GetCacheKey();

    const xstring_ptr& GetUri();

    _Check_return_ HRESULT GotDownloadResponse(
        _In_ IPALDownloadResponse* pResponse,
        HRESULT status
        );

    _Check_return_ HRESULT GotDownloadProgressUpdate(
        XUINT64 size,
        XUINT64 totalSize
        );

    // IImageDecodeCallback
    _Check_return_ HRESULT OnDecode(
        _In_ xref_ptr<IImageAvailableResponse> spResponse,
        _In_ uint64_t requestId
        ) override;

    _Check_return_ HRESULT HandleDecodeResponse(
        _In_ xref_ptr<IImageAvailableResponse> spResponse,
        _In_ uint64_t requestId
        );

    _Check_return_ HRESULT ProcessDecodeRequests(
        );

    void OnProviderReleased();

    _Check_return_ HRESULT OnRequestReleasing(
        _In_ const ImageDecodeRequest* const request
        );
    _Check_return_ HRESULT OnDecoderDetached(
        _In_ const ImageDecodeRequest* const request,
        std::unique_ptr<AsyncImageDecoder> imageDecoder
        );

    bool HasSoftwareImageDecodeInProgressWithParams(
        unsigned int width,
        unsigned int height,
        PixelFormat pixelFormat
    ) const;

private:
    ~ImageCache() override;

    _Check_return_ HRESULT Download();

    _Check_return_ HRESULT TriggerProcessDecodeRequests();

    _Check_return_ HRESULT BeginDecode(_In_ xref_ptr<ImageDecodeRequest> decodeRequest);

    // IInvalidateDecodedImageCacheCallback
    _Check_return_ HRESULT OnCacheInvalidated(_In_ DecodedImageCache* cache) override;

    _Check_return_ HRESULT TriggerDownloadProgressUpdate();

    _Check_return_ HRESULT RaiseDownloadProgress();

    bool AreDecodeParamsEqual(
        _In_ const xref_ptr<ImageDecodeParams>& pLHS,
        _In_ const xref_ptr<ImageDecodeParams>& pRHS
        );

    bool IsSvg() const { return m_isSvg; }

    CCoreServices* m_core{};
    xref_ptr<ImageTaskDispatcher> m_dispatcher;
    xstring_ptr m_strCacheKey;
    xstring_ptr m_strUri;
    xref_ptr<IPALUri> m_absoluteUri;
    ImageCacheState m_State;
    xref_ptr<ImageCacheDownloadCallbackMarshaller> m_downloadCallback;
    xref_ptr<IPALAbortableOperation> m_downloadOperation;
    HRESULT m_downloadError;

    std::shared_ptr<EncodedImageData> m_encodedImageData;

    std::shared_ptr<ImageMetadataViewImpl> m_metadataViewImpl;

    xvector<ImageDecodeRequest*> m_decodeRequests;
    std::list<ImageDecodeRequest*> m_decodingRequests;
    xvector<xref_ptr<DecodedImageCache>> m_decodedImages;
    IInvalidateImageCacheCallback* m_invalidatedCallback;
    XFLOAT m_downloadProgress = 0;

    bool m_hasProcessDecodeRequestsTask : 1;
    bool m_hasDownloadProgressTask : 1;
    bool m_isProviderReleased : 1;
    bool m_ignoreNetworkCache : 1;
    bool m_isSvg : 1;
};
