// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// imaging
#include "ImageMetadataViewImpl.h"
#include <AsyncDecodeResponse.h>
#include <AsyncImageDecoder.h>
#include <DecodedImageCache.h>
#include <EncodedImageData.h>
#include <ImageAsyncCallback.h>
#include <ImageCache.h>
#include <ImageCacheDecodeHandlerTask.h>
#include <ImageCacheDownloadCallbackMarshaller.h>
#include <ImageDecodeParams.h>
#include <ImageDecodeRequest.h>
#include <ImageDecoderFactory.h>
#include <ImageTaskDispatcher.h>
#include <OfferableSoftwareBitmap.h>
#include <PALMemoryProxy.h>
#include "GraphicsUtility.h"

// other
#include <corep.h>
#include <MUX-ETWEvents.h>
#include <palnetwork.h>
#include <PalResourceManager.h>

typedef ImageAsyncCallback<ImageCache> ImageCacheAsyncTask;

// For the BitmapImage.IgnoreImageCache scenario, it will use the ImageProvider::UncachedGetImage code path
// in image provider.  This creates a new cache object to do the download and decode.  This ignoreNetworkCache parameter
// is used to resynchronize and download a new update of the image if it has changed on the server.

ImageCache::ImageCache(
    _In_ const xstring_ptr& strCacheKey,
    _In_ const xstring_ptr& strUri,
    _In_opt_ IPALUri* absoluteUri,
    bool isSvg,
    _In_ CCoreServices* pCore,
    _In_ ImageTaskDispatcher* dispatcher,
    _In_ bool ignoreNetworkCache,
    _In_opt_ IInvalidateImageCacheCallback* invalidateCallback
    )
    : m_core(pCore)
    , m_dispatcher(dispatcher)
    , m_State(ImageCacheState::NotDownloaded)
    , m_strCacheKey(strCacheKey)
    , m_strUri(strUri)
    , m_downloadError(S_OK)
    , m_invalidatedCallback(invalidateCallback)
    , m_hasProcessDecodeRequestsTask(FALSE)
    , m_hasDownloadProgressTask(FALSE)
    , m_isProviderReleased(FALSE)
    , m_ignoreNetworkCache(ignoreNetworkCache)
    , m_isSvg(isSvg)
{
    XCP_WEAK(&m_core);
    XCP_WEAK(&m_invalidatedCallback);

    if (absoluteUri != nullptr)
    {
        IFCFAILFAST(absoluteUri->Clone(m_absoluteUri.ReleaseAndGetAddressOf()));
    }
}

ImageCache::~ImageCache()
{
    if (!m_isProviderReleased && m_invalidatedCallback != nullptr)
    {
        m_invalidatedCallback->OnImageCacheInvalidated(this);
    }

    if (m_downloadOperation != nullptr)
    {
        m_downloadOperation->Abort();
        m_downloadOperation.reset();
    }

    m_decodedImages.clear();

    ASSERT(m_decodeRequests.empty());
    ASSERT(m_decodingRequests.empty());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the cache key used with this ImageCache. Returns nullptr if this
//      ImageCache was not created with a cache key, indicating this
//      ImageCache was not intended to be cached.
//
//------------------------------------------------------------------------
const xstring_ptr&
ImageCache::GetCacheKey()
{
    return m_strCacheKey;
}

const xstring_ptr& ImageCache::GetUri()
{
    return m_strUri;
}

_Check_return_ HRESULT ImageCache::Download()
{
    XUINT32 options =
        udaAllowCrossDomain |
        udaAllowRedirects |
        udaShareDownloadStream |
        udaAllowMediaPermissions;

    ASSERT(m_State == ImageCacheState::NotDownloaded);

    IFCEXPECT_RETURN(m_State == ImageCacheState::NotDownloaded);

    // TODO: This can be updated to go in ImageDecodeActivity, but we need a trail of breadcrumbs back to the
    // ImageSource/LoadedImageSurface that triggered the download. There may be others that are waiting for this
    // download as well. Do they get events too?
    TraceImageCacheDownloadBegin(m_strUri.GetBuffer());

    m_State = ImageCacheState::Downloading;

    ASSERT(m_downloadCallback == nullptr);

    if (m_ignoreNetworkCache)
    {
        options |= udaResynchronizeStream;
    }

    m_downloadCallback = make_xref<ImageCacheDownloadCallbackMarshaller>(m_dispatcher, this);

    HRESULT hr = m_core->UnsecureDownloadFromSite(
        m_strUri,
        m_absoluteUri,
        m_downloadCallback,
        options,
        m_downloadOperation.ReleaseAndGetAddressOf(),
        nullptr);

    if (FAILED(hr))
    {
        IFC_RETURN(GotDownloadResponse(nullptr /* response */, hr));
    }

    return S_OK;
}

std::shared_ptr<ImageMetadataView> ImageCache::GetMetadataView(_In_opt_ const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity, uint64_t imageId)
{
    if (!m_metadataViewImpl)
    {
        if (m_State == ImageCacheState::NotDownloaded &&
            !m_hasProcessDecodeRequestsTask)
        {
            if (decodeActivity) // Will be null in unit tests
            {
                decodeActivity->QueueProcessDownload(imageId, m_strUri.GetBuffer());
            }
            m_downloadError = TriggerProcessDecodeRequests();
        }

        m_metadataViewImpl = std::make_shared<ImageMetadataViewImpl>();
        m_metadataViewImpl->SetEncodedImageData(m_encodedImageData, m_downloadError);
    }

    return m_metadataViewImpl;
}

// Asynchronously get an image with the specified decode parameters.
_Check_return_ HRESULT ImageCache::GetImage(
    _In_ xref_ptr<ImageDecodeParams>& decodeParams,
    _In_ const xref_ptr<IImageAvailableCallback>& imageAvailableCallback,
    _Outref_ xref_ptr<IAbortableImageOperation>& abortableImageOperation
    )
{
    //
    // Check if an already decoded image surface is available.
    //

    bool alreadyDecoded = false;

    for (auto& decodedImage: m_decodedImages)
    {
        ASSERT(m_encodedImageData != nullptr);
        if (!m_encodedImageData->IsAnimatedImage() &&
            AreDecodeParamsEqual(decodeParams, decodedImage->GetDecodeParams()))
        {
            // The image is already cached so return it
            IFC_RETURN(imageAvailableCallback->OnImageAvailable(make_xref<AsyncDecodeResponse>(
                S_OK,
                xref_ptr<OfferableSoftwareBitmap>(decodedImage->GetSurface())
                )));

            alreadyDecoded = true;

            break;
        }
    }

    //
    // If a surface wasn't already available then add a new decode request.
    //
    if (!alreadyDecoded)
    {
        auto decodeRequest = make_xref<ImageDecodeRequest>(m_core, xref_ptr<ImageCache>(this));
        IFC_RETURN(decodeRequest->SetDecodeParams(imageAvailableCallback, decodeParams));
        IFC_RETURN(m_decodeRequests.push_back(decodeRequest));

        IFC_RETURN(TriggerProcessDecodeRequests());

        abortableImageOperation = std::move(decodeRequest);
    }

    return S_OK;
}

std::shared_ptr<EncodedImageData> ImageCache::GetEncodedImageData() const
{
    return m_encodedImageData;
}

void ImageCache::SetEncodedImageData(std::shared_ptr<EncodedImageData> encodedImageData)
{
    ASSERT((m_encodedImageData == nullptr) || (m_encodedImageData == encodedImageData));
    ASSERT(encodedImageData != nullptr);
    m_encodedImageData = std::move(encodedImageData);
    m_State = ImageCacheState::Downloaded;
    m_downloadProgress = 1.0f;

    if (m_metadataViewImpl)
    {
        m_metadataViewImpl->SetEncodedImageData(m_encodedImageData, S_OK);
    }
}

static HRESULT MakeEncodedImageData(_In_ IPALDownloadResponse* response, bool isSvg, _Out_ std::shared_ptr<EncodedImageData> *result)
{
    xref_ptr<IPALResource> resource;
    IFC_RETURN(response->GetResource(resource.ReleaseAndGetAddressOf()));

    xref_ptr<IPALMemory> imageMemory;
    IFC_RETURN(response->Lock(imageMemory.ReleaseAndGetAddressOf()));

    auto encodedImageData = std::make_shared<EncodedImageData>(
        wil::make_unique_failfast<PALMemoryProxy>(imageMemory),
        resource ? resource->GetScalePercentage() : 0u,
        isSvg);

    IFC_RETURN(response->Unlock());

    *result = std::move(encodedImageData);

    return S_OK;
}

// Handler for the image source download completing.
_Check_return_ HRESULT ImageCache::GotDownloadResponse(
    _In_ IPALDownloadResponse* response,
    HRESULT status
    )
{
    ASSERT(m_State == ImageCacheState::Downloading);

    m_downloadOperation.reset();
    m_downloadCallback.reset();

    std::shared_ptr<EncodedImageData> encodedImageData;
    if (SUCCEEDED(status))
    {
        status = MakeEncodedImageData(response, IsSvg(), &encodedImageData);
    }

    if (SUCCEEDED(status))
    {
        SetEncodedImageData(std::move(encodedImageData));
    }
    else
    {
        m_State = ImageCacheState::DownloadFailed;
        m_downloadError = status;

        if (m_metadataViewImpl)
        {
            m_metadataViewImpl->SetEncodedImageData(nullptr, m_downloadError);
        }
    }

    IFC_RETURN(TriggerProcessDecodeRequests());

    TraceImageCacheDownloadEnd(m_strUri.GetBuffer());

    return S_OK;
}

_Check_return_ HRESULT ImageCache::GotDownloadProgressUpdate(
    XUINT64 size,
    XUINT64 totalSize
    )
{
    if (m_State == ImageCacheState::Downloading)
    {
        XDOUBLE downloadProgress = static_cast< XDOUBLE >(size) / static_cast< XDOUBLE >(totalSize);

        m_downloadProgress = static_cast< XFLOAT >(downloadProgress);

        IFC_RETURN(TriggerDownloadProgressUpdate());
    }

    return S_OK;
}

// Trigger a download progress update to callbacks.
_Check_return_ HRESULT ImageCache::TriggerDownloadProgressUpdate()
{
    if (!m_hasDownloadProgressTask)
    {
        auto task = make_xref<ImageCacheAsyncTask>(this, &ImageCache::RaiseDownloadProgress);
        IFC_RETURN(m_dispatcher->QueueTask(std::move(task)));
    }

    return S_OK;
}

// Raise download progress events to callbacks.
_Check_return_ HRESULT ImageCache::RaiseDownloadProgress()
{
    m_hasDownloadProgressTask = FALSE;

    if (m_State == ImageCacheState::Downloading && (m_metadataViewImpl != nullptr))
    {
        m_metadataViewImpl->SetDownloadProgress(m_downloadProgress);
    }

    return S_OK;
}

// Start asynchronously process decode requests.
_Check_return_ HRESULT ImageCache::TriggerProcessDecodeRequests()
{
    if (!m_hasProcessDecodeRequestsTask)
    {
        // This ImageCache may be associated with multiple ImageSources (i.e. the same image decoding to multiple
        // different sizes). Telemetry is associated with the decoded image, so report telemetry for all the
        // decodes that will be triggered by this ImageCache.
        for (ImageDecodeRequest* decodeRequest : m_decodeRequests)
        {
            const auto& decodeParams = decodeRequest->GetDecodeParams();

            // The call below won't actually AV if there's nothing logging an ETW trace. Assert it explicitly so we
            // crash consistently if it's null.
            ASSERT(decodeParams->GetDecodeActivity());
            decodeParams->GetDecodeActivity()->QueueProcessDecodeRequests(decodeParams->GetImageId(), decodeParams->GetStrSource().GetBuffer());
        }

        auto task = make_xref<ImageCacheAsyncTask>(this, &ImageCache::ProcessDecodeRequests);

        IFC_RETURN(m_dispatcher->QueueTask(std::move(task)));
        m_hasProcessDecodeRequestsTask = TRUE;
    }

    return S_OK;
}

// Process decode requests.
//
// a) New decode requests are checked against the surface cache.
// b) If the source is downloaded, a decode request is queued.
// c) If download failed an error is returned to the callback.
_Check_return_ HRESULT ImageCache::ProcessDecodeRequests()
{
    m_hasProcessDecodeRequestsTask = FALSE;

    for (ImageDecodeRequest* decodeRequest : m_decodeRequests)
    {
        const auto& decodeParams = decodeRequest->GetDecodeParams();
        decodeParams->GetDecodeActivity()->ProcessDecodeRequests(decodeParams->GetImageId(), decodeParams->GetStrSource().GetBuffer(), m_State);
    }

    //
    // If there are requests that were not available from the cache, begin downloading the image source.
    //
    if (m_State == ImageCacheState::NotDownloaded)
    {
        IFC_RETURN(Download());
    }
    //
    // If the image source is available then begin decoding the image.
    //
    else if (m_State == ImageCacheState::Downloaded)
    {
        ASSERT(m_encodedImageData != nullptr);

        while (!m_decodeRequests.empty())
        {
            ImageDecodeRequest *request = m_decodeRequests.back();
            IFC_RETURN(m_decodeRequests.pop_back());
            IFC_RETURN(BeginDecode(xref_ptr<ImageDecodeRequest>(request)));
        }
    }
    //
    // If the image source was not available raise an error to the callback.
    //
    else if (m_State == ImageCacheState::DownloadFailed)
    {
        ASSERT(m_encodedImageData == nullptr);

        while (!m_decodeRequests.empty())
        {
            ImageDecodeRequest *request = m_decodeRequests.back();
            IFC_RETURN(m_decodeRequests.pop_back());
            IFC_RETURN(request->NotifyCallback(make_xref<AsyncDecodeResponse>(m_downloadError)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImageCache::BeginDecode(
    _In_ xref_ptr<ImageDecodeRequest> decodeRequest
    )
{
    bool gotCachedImage = false;

    ASSERT(m_State == ImageCacheState::Downloaded);
    ASSERT(m_encodedImageData != nullptr);

    //
    // Check if an already decoded image surface is available.
    //
    for (auto& decodedImage: m_decodedImages)
    {
        if (!m_encodedImageData->IsAnimatedImage() &&
            AreDecodeParamsEqual(decodedImage->GetDecodeParams(), decodeRequest->GetDecodeParams()))
        {
            auto spImageResponse = make_xref<AsyncDecodeResponse>(
                S_OK,
                xref_ptr<OfferableSoftwareBitmap>(decodedImage->GetSurface())
                );
            IFC_RETURN(decodeRequest->NotifyCallback(spImageResponse));

            gotCachedImage = TRUE;

            break;
        }
    }

    //
    // If a decoded surface is not available, start a new decode or wait for an already existing decode to
    // complete if it matches the decode parameters.
    //
    if (!gotCachedImage)
    {
        bool decodeInProgress = false;

        //
        // Check if a decode of the image with the desired parameters is already in progress.
        //
        for (auto &request: m_decodingRequests)
        {
            if (!m_encodedImageData->IsAnimatedImage() &&
                request->IsDecodeInProgress() &&
                AreDecodeParamsEqual(request->GetDecodeParams(), decodeRequest->GetDecodeParams()))
            {
                decodeInProgress = TRUE;

                break;
            }
        }

        //
        // Store the request for when the decode completes.
        //
        m_decodingRequests.push_back(decodeRequest);

        //
        // Start a decode if one was not already in progress.
        //
        // For most Xaml imaging API's, a HW Cache is used to store the realized surface, so normally
        // if a request with the same decode params is already in progress we don't need to spin up a new decoder.
        // An exception to this is LoadedImageSurface, which does not use the HW cache and where
        // each request expects a surface to be filled in by the AsyncImageDecoder.
        //
        if (!decodeInProgress || decodeRequest->GetDecodeParams()->IsLoadedImageSurface())
        {
            const auto& decodeParams = decodeRequest->GetDecodeParams();
            const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity = decodeParams->GetDecodeActivity();

            decodeActivity->QueueDecodeFromImageCache(decodeParams->GetImageId(), decodeParams->GetStrSource().GetBuffer());
            auto parseHR = m_encodedImageData->Parse(m_core->GetGraphicsDevice(), m_core->GetContentRootMaxSize());

            if (FAILED(parseHR))
            {
                auto spImageDecodeResponse = make_xref<AsyncDecodeResponse>(parseHR);
                IFC_RETURN(HandleDecodeResponse(std::move(spImageDecodeResponse), decodeRequest->GetRequestId()));
            }
            else
            {
                // Issue the asynchronous decoding operation
                std::unique_ptr<IImageDecoder> decoder;
                const HRESULT createDecoderHR = ImageDecoderFactory::CreateDecoder(m_core, *m_encodedImageData, decoder);

                //
                // SVG decoders can return device lost - they require D2D to do the decode. If that's the case, we've
                // queued a tick to recover from device lost. Also notify the corresponding ImageSource to handle the
                // error. CSvgImageSource::OnDownloadImageAvailableImpl will respond to the device lost error by queueing
                // another decode attempt.
                //
                // Note that there's some subtlety here. We're running as a task in ImageTaskDispatcher::Execute, and
                // CSvgImageSource will respond to the failure by queueing /another/ task in the same ImageTaskDispatcher.
                // This would lead to an infinite cycle if ImageTaskDispatcher just ran until its queue was empty. Instead,
                // it knows to ignore tasks queued while it was already processing tasks and come back to them later. This
                // gives the UI thread a chance to recover from the device loss before we try to decode again.
                //
                if (GraphicsUtility::IsDeviceLostError(createDecoderHR))
                {
                    auto spImageDecodeResponse = make_xref<AsyncDecodeResponse>(createDecoderHR);
                    IFC_RETURN(HandleDecodeResponse(std::move(spImageDecodeResponse), decodeRequest->GetRequestId()));
                }
                else
                {
                    IFC_RETURN(createDecoderHR);

                    auto asyncImageDecoder = std::make_unique<AsyncImageDecoder>(
                        std::move(decoder),
                        m_encodedImageData,
                        decodeParams->IsAutoPlay(),
                        xref::get_weakref(this));

                    IFC_RETURN(decodeRequest->SetImageDecoder(std::move(asyncImageDecoder)));
                }
            }
        }
    }

    return S_OK;
}

// Handler for when decoding has completed of the source.
//  NOTE: Called from the decode thread and dispatches to the UI thread.
_Check_return_ HRESULT ImageCache::OnDecode(
    _In_ xref_ptr<IImageAvailableResponse> response,
    _In_ uint64_t requestId
    )
{
    // calls ImageCache::HandleDecodeResponse on the UI thread
    auto spTask = make_xref<ImageCacheDecodeHandlerTask>(
        xref_ptr<ImageCache>(this),
        std::move(response),
        requestId);

    IFC_RETURN(m_dispatcher->QueueTask(spTask));

    return S_OK;
}

// Handle a decode response and dispatch to callbacks.
_Check_return_ HRESULT ImageCache::HandleDecodeResponse(
    _In_ xref_ptr<IImageAvailableResponse> response,
    _In_ uint64_t requestId
    )
{
    TraceImageCacheDecodeEnd(m_strUri.GetBuffer());

    ASSERT(m_encodedImageData != nullptr);

    auto found = std::find_if(m_decodingRequests.begin(), m_decodingRequests.end(),
        [&](ImageDecodeRequest *request)
        {
            return request->GetRequestId() == requestId;
        });

    if (found == m_decodingRequests.end())
    {
        // The request may have been discarded during decode
        return S_OK;
    }

    // Keep the copy on stack in case the completed request gets released in the callback
    xref_ptr<ImageDecodeRequest> completedDecodeRequest(*found);

    //
    // Find decode requests that match the newly decoded image.
    //
    for (auto it = m_decodingRequests.begin(); it != m_decodingRequests.end();)
    {
        // make sure the request is not destroyed while we work with it
        xref_ptr<ImageDecodeRequest> currentRequest(*it);

        // Animated images do not share decoder to allow independent playback control
        // so do not notify other callbacks even their decode params match.
        bool isOwnRequest = (completedDecodeRequest == currentRequest);
        bool isAnimatedImage = SUCCEEDED(response->GetDecodingResult()) && m_encodedImageData->IsAnimatedImage();

        // LoadedImageSurface's design includes a seprate HW surface to be filled with each request,
        // forcing separate decoders. We need to only notify the matching request here.
        // To enforce this, skip the loop below if either the request triggering this response
        // or the request we are currently looking at is for LoadedImageSurface. That prevents
        // potential non-LIS responses from notifying similar LIS requests and vice versa.
        bool isLoadedImageSurface = currentRequest->GetDecodeParams()->IsLoadedImageSurface() ||
            completedDecodeRequest->GetDecodeParams()->IsLoadedImageSurface();

        if (isOwnRequest || (!isAnimatedImage && !isLoadedImageSurface && AreDecodeParamsEqual(completedDecodeRequest->GetDecodeParams(), currentRequest->GetDecodeParams())))
        {
            IFC_RETURN(currentRequest->NotifyCallback(response));

            if (SUCCEEDED(response->GetDecodingResult()))
            {
                ++it;
            }
            else
            {
                it = m_decodingRequests.erase(it);
            }
        }
        else
        {
            ++it;
        }
    }

    //
    // If decode succeeded then store the surface.
    //
    // It is possible for the decode to succeed and the surface to be nullptr if decoding was deferred
    // and only the size of the encoded image was requested.  In this case, no surface can be added
    // to the decoded images.
    //
    auto &surface = response->GetSurface();
    if (SUCCEEDED(response->GetDecodingResult()) && (surface != nullptr))
    {
        auto decodedImageCache = make_xref<DecodedImageCache>(static_cast<IInvalidateDecodedImageCacheCallback*>(this),
            completedDecodeRequest->GetDecodeParams(),
            surface);
        IFC_RETURN(m_decodedImages.push_back(std::move(decodedImageCache)));
    }

    return S_OK;
}

// Handler for when a decoded image cache becomes invalid.
_Check_return_ HRESULT ImageCache::OnCacheInvalidated(_In_ DecodedImageCache* cache)
{
    //
    // Surfaces should only be released from the UI thread.
    //
    ASSERT((m_core == nullptr) || (GetCurrentThreadId() == m_core->GetThreadID()));

    //
    // Remove the decode image from the cache as the surface is no longer available.
    //
    for (auto it = m_decodedImages.begin(); it != m_decodedImages.end(); ++it)
    {
        if (*it == cache)
        {
            IFC_RETURN(m_decodedImages.erase(it));
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark the provider as released.
//
//      BitmapImages hold references to this ImageCache, so it could
//      outlive its ImageProvider, which gets cleared when the visual
//      tree resets. If the provider has been released, there's no need
//      to remove this ImageCache once it becomes unused. The provider
//      would have already released its reference on this ImageCache.
//
//------------------------------------------------------------------------
void ImageCache::OnProviderReleased()
{
    m_isProviderReleased = TRUE;
}

_Check_return_ HRESULT ImageCache::OnRequestReleasing(_In_ const ImageDecodeRequest* const request)
{
    {
        auto it = std::find(m_decodeRequests.begin(), m_decodeRequests.end(), request);
        if (m_decodeRequests.end() != it)
        {
            IFC_RETURN(m_decodeRequests.erase(it));
        }
    }

    {
        auto it = std::find(m_decodingRequests.begin(), m_decodingRequests.end(), request);
        if (m_decodingRequests.end() != it)
        {
            m_decodingRequests.erase(it);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ImageCache::OnDecoderDetached(_In_ const ImageDecodeRequest* const request, std::unique_ptr<AsyncImageDecoder> imageDecoder)
{
    // See if we can hand the decoder to another request
    if ((request->GetDecodeParams() != nullptr) &&
        !request->GetDecodeParams()->IsLoadedImageSurface())
    {
        for (auto& existingRequest: m_decodingRequests)
        {
            if (existingRequest != request &&
                !existingRequest->HasDecoder() &&
                AreDecodeParamsEqual(existingRequest->GetDecodeParams(), request->GetDecodeParams()))
            {
                IFC_RETURN(existingRequest->SetImageDecoder(std::move(imageDecoder)));
                break;
            }
        }
    }

    return S_OK;
}

// Compares the object contents of two ImageDecodeParams pointers to test for eqality
bool ImageCache::AreDecodeParamsEqual(
    _In_ const xref_ptr<ImageDecodeParams>& spLHS,
    _In_ const xref_ptr<ImageDecodeParams>& spRHS
    )
{
    // Don't need to compare the image surface list since that is temporary for background image loading
    // and will be released after decoding completes.
    // Don't need to compare AutoPlay since animated playback caches based on different criteria.

    return
        spLHS &&
        spRHS &&
        (spLHS->GetFormat() == spRHS->GetFormat()) &&
        (spLHS->GetDecodeWidth() == spRHS->GetDecodeWidth()) &&
        (spLHS->GetDecodeHeight() == spRHS->GetDecodeHeight());
}

bool ImageCache::HasSoftwareImageDecodeInProgressWithParams(
    unsigned int width,
    unsigned int height,
    PixelFormat pixelFormat
    ) const
{
    for (auto &request: m_decodingRequests)
    {
        if (
            request->IsDecodeInProgress() &&
            !request->GetDecodeParams()->IsHardwareOutput() &&
            request->GetDecodeParams()->GetDecodeWidth() == width &&
            request->GetDecodeParams()->GetDecodeHeight() == height)
        {
            return true;
        }
    }

    for (auto &request: m_decodeRequests)
    {
        if (
            !request->GetDecodeParams()->IsHardwareOutput() &&
            request->GetDecodeParams()->GetDecodeWidth() == width &&
            request->GetDecodeParams()->GetDecodeHeight() == height)
        {
            return true;
        }
    }

    return false;
}
