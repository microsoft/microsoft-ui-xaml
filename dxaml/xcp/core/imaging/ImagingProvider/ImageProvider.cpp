// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageProvider.h"
#include <wincodec.h>
#include <AsyncDecodeResponse.h>
#include <AsyncImageDecoder.h>
#include <ImageCache.h>
#include <ImagingInterfaces.h>
#include <EncodedImageData.h>
#include <OfferableSoftwareBitmap.h>
#include <ImagingUtility.h>
#include <ImageDecoderFactory.h>
#include "ImageDecodeParams.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a new image provider.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::Create(
    _In_ CCoreServices* pCore,
    _In_ ImageTaskDispatcher* pDispatcher,
    _In_ IAsyncImageFactory* pImageFactory,
    _Outptr_ ImageProvider** ppImageProvider
    )
{
    xref_ptr<ImageProvider> pNewProvider;

    pNewProvider.attach(new ImageProvider());

    IFC_RETURN(pNewProvider->Initialize(pCore, pDispatcher, pImageFactory));

    *ppImageProvider = pNewProvider.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new image provider.
//
//------------------------------------------------------------------------
ImageProvider::ImageProvider(
    )
    : m_pImageFactory(NULL)
    , m_pCore(NULL)
    , m_pStore(NULL)
    , m_pDispatcher(NULL)
{
    XCP_WEAK(&m_pCore);
    XCP_WEAK(&m_pDispatcher);
    XCP_STRONG(&m_pImageFactory);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up image provider.
//
//------------------------------------------------------------------------
ImageProvider::~ImageProvider(
    )
{
    if (m_pStore != NULL)
    {
        IGNOREHR(m_pStore->Traverse(&CleanupImageCacheStoreValue, this));
    }

    //TODO: Response to aborted callbacks rather than just dropping them.

    ReleaseInterface(m_pImageFactory);
    ReleaseInterface(m_pStore);

    // Note: The dtor will log an error if Stop isn't called first.
    if (m_decodeActivity && m_decodeActivity->IsRunning())
    {
        m_decodeActivity->Stop();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intialize the image provider.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::Initialize(
    _In_ CCoreServices* pCore,
    _In_ ImageTaskDispatcher* pDispatcher,
    _In_ IAsyncImageFactory* pFactory
    )
{
    m_pCore = pCore;
    m_pDispatcher = pDispatcher;

    SetInterface(m_pImageFactory, pFactory);

    m_pStore = new CValueStore(false);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get an image from an encoded image.
//
//  NOTE:
//      This will always trigger a decode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::GetImage(
    _In_ const std::shared_ptr<EncodedImageData>& spEncodedImageData,
    _In_ xref_ptr<ImageDecodeParams>& spDecodeParams,
    GetImageOptions options,
    _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
    _In_ CCoreServices* pCore,
    _Outref_ xref_ptr<IAbortableImageOperation>& spAbortableImageOperation
    )
{
    const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity = spDecodeParams->GetDecodeActivity();

    auto parseHR = spEncodedImageData->Parse(pCore->GetGraphicsDevice(), pCore->GetContentRootMaxSize());

    bool isAnimatedImage = SUCCEEDED(parseHR) && spEncodedImageData->IsAnimatedImage();
    bool isSynchronousDecode = flags_enum::is_set(options, GetImageOptions::Synchronous);

    if (isSynchronousDecode)
    {
        HRESULT decodeResult = parseHR;
        wrl::ComPtr<IWICBitmapSource> bitmapSource;
        if (SUCCEEDED(decodeResult))
        {
            //
            // Bug 32451627: [Watson Failure] caused by STOWED_EXCEPTION_887a0005_Windows.UI.Xaml.dll!CD3D11Device::EnsureD2DResources
            //
            // The CreateDecoder call below used to be a failfast, which caused crashes when it got device lost errors. The common code
            // path for CreateDecoder is a call from ImageCache::BeginDecode, where we now tolerate a device lost error. Here is a call
            // to CreateDecoder from a synchronous decode (i.e. the app called the synchronous SetSource overload), which doesn't exist
            // for SVG (SvgImageSource has only SetSourceAsync). So we maintain the old failfast behavior for this CreateDecoder call.
            //
            std::unique_ptr<IImageDecoder> decoder;
            IFCFAILFAST(ImageDecoderFactory::CreateDecoder(m_pCore, *spEncodedImageData, decoder));

            std::chrono::milliseconds frameDelay;
            decodeResult = decoder->DecodeFrame(
                *spEncodedImageData,
                *spDecodeParams,
                0 /* frameIndex */,
                bitmapSource,
                frameDelay);
        }

        xref_ptr<OfferableSoftwareBitmap> spSoftwareBitmap;
        if (SUCCEEDED(decodeResult))
        {
            decodeResult = ImagingUtility::RealizeBitmapSource(
                spEncodedImageData->GetMetadata(),
                bitmapSource.Get(),
                *spDecodeParams,
                spSoftwareBitmap);
        }

        auto spResponse = make_xref<AsyncDecodeResponse>(decodeResult, std::move(spSoftwareBitmap));

        IFC_RETURN(spImageAvailableCallback->OnImageAvailable(spResponse));
    }

    // Also start the asynchronous decode operation in case of an animated image.
    // TODO: Find a way to reuse the first frame from the synchronous path above.
    if (isAnimatedImage || !isSynchronousDecode)
    {
        if (FAILED(parseHR))
        {
            auto spResponse = make_xref<AsyncDecodeResponse>(parseHR);

            IFC_RETURN(spImageAvailableCallback->OnImageAvailable(spResponse.get()));
        }
        else
        {
            // Issue the asynchronous decoding operation

            auto imageCache = make_xref<ImageCache>(
                xstring_ptr::NullString() /* strCacheKey */,
                xstring_ptr::NullString() /* strUri */,
                nullptr /* pAbsoluteUri */,
                spEncodedImageData->IsSvg(),
                m_pCore,
                m_pDispatcher,
                true /* ignoreNetworkCache */,
                nullptr /* pInvalidateCallback */);

            imageCache->SetEncodedImageData(spEncodedImageData);

            decodeActivity->CreateImageCacheFromExistingEncodedData(spDecodeParams->GetImageId(), spDecodeParams->GetStrSource().GetBuffer());

            IFC_RETURN(imageCache->GetImage(spDecodeParams, spImageAvailableCallback, spAbortableImageOperation));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the image cache for a Uri.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::GetImageCache(
    _In_ const xstring_ptr& strUri,
    _Outptr_result_maybenull_ ImageCache** ppCache
    )
{
    HRESULT hr = S_OK;
    XHANDLE storeValue = NULL;

    if (SUCCEEDED(m_pStore->GetValue(strUri, &storeValue)))
    {
        ImageCache* pCache = static_cast<ImageCache*>(storeValue);

        *ppCache = pCache;
        AddRefInterface(pCache);
    }
    else
    {
        *ppCache = NULL;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Store an image cache by Uri.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::AddImageCache(
    _In_ ImageCache* pCache
    )
{
    const xstring_ptr& cacheKey = pCache->GetCacheKey();

    IFCEXPECT_RETURN(!cacheKey.IsNull());

#if DBG
    {
        XHANDLE storeValue = NULL;

        if (SUCCEEDED(m_pStore->GetValue(cacheKey, &storeValue)))
        {
            ImageCache* pStoredCache = static_cast<ImageCache*>(storeValue);
            ASSERT(pCache == pStoredCache);
        }
    }
#endif

    //
    // Insert new cache value.
    //
    IFC_RETURN(m_pStore->PutValue(cacheKey, static_cast<XHANDLE>(pCache)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Remove an image cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::RemoveImageCache(
    _In_ ImageCache* pCache
    )
{
    XHANDLE storeValue = NULL;

    if (pCache->GetCacheKey().IsNull())
    {
        // if this ImageCache wasn't created to be cached, ignore it
        return S_OK;
    }

    //
    // Check if the cache is still being stored for its Uri.
    //
    if (SUCCEEDED(m_pStore->GetValue(pCache->GetCacheKey(), &storeValue)))
    {
        ImageCache* pStoredCache = static_cast<ImageCache*>(storeValue);

        if (pCache == pStoredCache)
        {
            IFC_RETURN(m_pStore->PutValue(pCache->GetCacheKey(), NULL));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for an image cache's contents becoming invalid or unused.
//
//------------------------------------------------------------------------
void ImageProvider::OnImageCacheInvalidated(
    _In_ ImageCache* pCache
    )
{
    VERIFYHR(RemoveImageCache(pCache));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Callback method for cleaning up image caches from Uri store.
//
//------------------------------------------------------------------------
void
ImageProvider::CleanupImageCacheStoreValue(
    _In_ const xstring_ptr& strKey,
    XHANDLE value,
    XHANDLE extraData
    )
{
    ImageCache* pCache = static_cast<ImageCache*>(value);

    if (pCache != NULL)
    {
        pCache->OnProviderReleased();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clear the cache completely
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::Clear()
{
    if (m_pStore != nullptr)
    {
        IFC_RETURN(m_pStore->Traverse(&CleanupImageCacheStoreValue, this));
    }

    // All elements have been removed, now recreate the store completely
    ReleaseInterface(m_pStore);
    m_pStore = new CValueStore(false);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a cache key for use with the given resource URI and
//      resource invalidation ID.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ImageProvider::MakeCacheKey(_In_ IPALUri* pUri, _Out_ xstring_ptr* pstrCacheKey)
{
    xstring_ptr strUri;
    bool canBeInvalidated;
    xstring_ptr strResourceInvalidationId;
    XStringBuilder cacheKeyBuilder;

    // The cache key will at a minimum contain the canonical resource URI.
    IFC_RETURN(pUri->GetCanonical(&strUri));

    // If this resource URI can't be invalidated, then the cache key need only contain the canonical resource URI.
    wrl::ComPtr<IPALResourceManager> pResourceManager;
    IFC_RETURN(m_pCore->GetResourceManager(&pResourceManager));
    IFC_RETURN(pResourceManager->CanResourceBeInvalidated(pUri, &canBeInvalidated));
    if (!canBeInvalidated)
    {
        *pstrCacheKey = std::move(strUri);
        return S_OK;
    }

    // This resource URI can be invalidated, so the cache key must also contain the current invalidation ID.

    IFC_RETURN(xstring_ptr::CreateFromUInt32(pResourceManager->GetResourceInvalidationId(), &strResourceInvalidationId));

    IFC_RETURN(cacheKeyBuilder.Initialize(strUri.GetCount() + strResourceInvalidationId.GetCount() + 3));

    IFC_RETURN(cacheKeyBuilder.Append(strUri));
    IFC_RETURN(cacheKeyBuilder.Append(L":[", 2));
    IFC_RETURN(cacheKeyBuilder.Append(strResourceInvalidationId));
    IFC_RETURN(cacheKeyBuilder.Append(L"]", 1));

    IFC_RETURN(cacheKeyBuilder.DetachString(pstrCacheKey));

    return S_OK;
}

// If caching is allowed, get a pre-existing image cache if it already exists otherwise create a new one.
_Check_return_ HRESULT
ImageProvider::EnsureCacheEntry(
    _In_ const xstring_ptr& strUri,
    _In_opt_ IPALUri *pAbsoluteUri,
    bool isSvg,
    GetImageOptions options,
    _In_ const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity,
    uint64_t imageId,
    _Out_ bool *pCacheHit,
    _Outptr_ ImageCache **ppImageCache
    )
{
    xstring_ptr strCacheKey = xstring_ptr::NullString();
    xref_ptr<ImageCache> imageCache;

    ASSERT(pCacheHit != nullptr);

    bool useCache = false;
    IFC_RETURN(ShouldUseCache(pAbsoluteUri, options, &useCache));

    if (useCache)
    {
        //
        // If we found a cached decoded hardware surface, then we're not interested in the software
        // decoded surface anymore. This flag tells the ImageCache to not kick off a decode even if
        // the decoded software surface is not found.
        //
        // This flag is temporary until the hardware surface cache is merged into the ImageProvider.
        //

        IFC_RETURN(MakeCacheKey(pAbsoluteUri, &strCacheKey));
        IFC_RETURN(GetImageCache(strCacheKey, imageCache.ReleaseAndGetAddressOf()));
    }

    if (imageCache == nullptr)
    {
        decodeActivity->CreateImageCache(imageId, strUri.GetBuffer());

        imageCache = make_xref<ImageCache>(
            strCacheKey,
            strUri,
            pAbsoluteUri,
            isSvg,
            m_pCore,
            m_pDispatcher,
            !useCache /* ignoreNetworkCache */,
            useCache ? static_cast<IInvalidateImageCacheCallback*>(this) : nullptr);

        if (useCache)
        {
            IFC_RETURN(AddImageCache(imageCache));
        }

        *pCacheHit = false;
    }
    else
    {
        decodeActivity->FoundImageCache(imageId, strUri.GetBuffer());
        *pCacheHit = true;
    }

    *ppImageCache = imageCache.detach();

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not cache management should be used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProvider::ShouldUseCache(
    _In_opt_ IPALUri* absoluteUri,
    GetImageOptions options,
    _Out_ bool* useCache
    )
{
    bool canCacheImage = false;

    if (absoluteUri != nullptr && !flags_enum::is_set(options, GetImageOptions::IgnoreCache))
    {
        xref_ptr<IPALResourceManager> resourceManager;
        IFC_RETURN(m_pCore->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
        IFC_RETURN(resourceManager->CanCacheResource(absoluteUri, &canCacheImage));
    }

    *useCache = canCacheImage;

    return S_OK;
}
