// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoadedImageSurface.h"
#include "LoadedImageSourceLoadCompletedEventArgs.h"
#include "eventmgr.h"
#include <DCompSurface.h>
#include <EncodedImageData.h>
#include <ImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageMetadata.h>
#include <ImageMetadataView.h>
#include <ImageProvider.h>
#include <ImageProviderInterfaces.h>
#include <PixelFormat.h>
#include <RawData.h>
#include <host.h>
#include "RootScale.h"
#include "DXamlServices.h"

// The main reason this exists is as an optimization so that an image can be allocated as virtual for resizing
// purposes if needed, otherwise it is not virtual meaning better performance but it will cause a crash if
// it is tried to Resize above the max texture limit.
// This is a "theoretical" maximum that could be violated in the future.
static const float c_maxPlateauScale = 7;


static DirectUI::LoadedImageSourceLoadStatus GetLoadStatusFromHRESULT(HRESULT hr)
{
    if (SUCCEEDED(hr))
    {
        return DirectUI::LoadedImageSourceLoadStatus::Success;
    }

    switch (HRESULT_FACILITY(hr))
    {
    case FACILITY_INTERNET:     // INET_E_*
        return DirectUI::LoadedImageSourceLoadStatus::NetworkError;
    case FACILITY_WINCODEC_ERR: // WINCODEC_ERR_*
        return DirectUI::LoadedImageSourceLoadStatus::InvalidFormat;
    default:
        return DirectUI::LoadedImageSourceLoadStatus::Other;
    }
}


class CLoadedImageSurfaceImageAvailableCallback : public CXcpObjectBase<IImageAvailableCallback>
{
public:
    CLoadedImageSurfaceImageAvailableCallback(_In_ CLoadedImageSurface* ownerLoadedImageSurface)
        : m_ownerLoadedImageSurface(ownerLoadedImageSurface)
    {}

    _Check_return_ HRESULT OnImageAvailable(_In_ IImageAvailableResponse* response) override
    {
        // Ignore device lost error and attempt to reload later when the device is recovered
        if (!GraphicsUtility::IsDeviceLostError(response->GetDecodingResult()))
        {
            IFC_RETURN(m_ownerLoadedImageSurface->FireLoadCompleted(response->GetDecodingResult()));
        }

        return S_OK;
    }

private:
    CLoadedImageSurface* m_ownerLoadedImageSurface;
};


CLoadedImageSurface::CLoadedImageSurface(_In_ CCoreServices* core)
    : CDependencyObject(core)
    , m_closed(false)
    , m_isDeviceListener(false)
    , m_isDisplayListener(false)
    , m_isPLMListener(false)
{
    SetIsCustomType();
}

CLoadedImageSurface::~CLoadedImageSurface()
{
    delete m_eventList;
    m_eventList = nullptr;

    if (!m_closed)
    {
        VERIFYHR(Close());
    }

    ASSERT(!m_isDeviceListener);
    ASSERT(!m_isDisplayListener);
}

_Check_return_ HRESULT CLoadedImageSurface::InitInstance()
{
    m_imageAvailableCallback = make_xref<CLoadedImageSurfaceImageAvailableCallback>(this);
    IFC_RETURN(ImageSurfaceWrapper::Create(GetContext(), FALSE /* mustKeepSoftwareSurface */, m_imageSurfaceWrapper.ReleaseAndGetAddressOf()));

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::AddEventListener(
    _In_ EventHandle event,
    _In_ CValue* value,
    _In_ XINT32 listenerType,
    _Out_opt_ CValue* result,
    _In_ bool handledEventsToo)
{
    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    return CEventManager::AddEventListener(this, &m_eventList, event, value, listenerType, result, handledEventsToo);
}

_Check_return_ HRESULT CLoadedImageSurface::RemoveEventListener(_In_ EventHandle event, _In_ CValue* value)
{
    return CEventManager::RemoveEventListener(this, m_eventList, event, value);
}

bool CLoadedImageSurface::AllowsHandlerWhenNotLive(XINT32 /* listenerType */, KnownEventIndex) const
{
    return true;
}

_Check_return_ HRESULT CLoadedImageSurface::GetValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value)
{
    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    const auto contentRootCoordinator = GetContext()->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    const auto zoomScale = RootScale::GetRasterizationScaleForContentRoot(root);

    switch (dp->GetIndex())
    {
    case KnownPropertyIndex::LoadedImageSurface_NaturalSize:
        if (m_imageMetadataView != nullptr)
        {
            if (auto imageMetadata = m_imageMetadataView->GetImageMetadata())
            {
                value->SetSize(new XSIZEF{ imageMetadata->width / zoomScale, imageMetadata->height / zoomScale });
            }
        }
        break;

    case KnownPropertyIndex::LoadedImageSurface_DecodedPhysicalSize:
        value->SetSize(new XSIZEF{
            static_cast<float>(m_imageSurfaceWrapper->GetWidth()),
            static_cast<float>(m_imageSurfaceWrapper->GetHeight())
        });
        break;

    case KnownPropertyIndex::LoadedImageSurface_DecodedSize:
        {
            value->SetSize(new XSIZEF{
                m_imageSurfaceWrapper->GetWidth() / zoomScale,
                m_imageSurfaceWrapper->GetHeight() / zoomScale
            });
        }
        break;

    default:
        IFC_RETURN(CDependencyObject::GetValue(dp, value));
    }

    return S_OK;
}


void CLoadedImageSurface::SetDesiredSize(float width, float height)
{
    // can only set once
    ASSERT(m_desiredMaxWidth == 0 && m_desiredMaxHeight == 0);
    m_desiredMaxWidth = width;
    m_desiredMaxHeight = height;
}

static HRESULT GetImageCache(
    _In_ CCoreServices &core,
    const xstring_ptr &uri,
    const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& decodeActivity,
    uint64_t imageId,
    _Outptr_ ImageCache **imageCache)
{
    *imageCache = nullptr;

    xref_ptr<IPALUri> absoluteUri;
    if (!uri.IsNullOrEmpty())
    {
        xref_ptr<IPALResourceManager> resourceManager;
        IFC_RETURN(core.GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
        IFC_RETURN(resourceManager->CombineResourceUri(core.GetBaseUriNoRef(), uri, absoluteUri.ReleaseAndGetAddressOf()));
    }

    bool cacheHit = false;

    // EnsureCacheEntry creates the ImageCache object and doesn't cache it unless there is a valid URI.
    // The returned ImageCache is being used here as a download/decode engine.
    IFC_RETURN(core.GetImageProvider()->EnsureCacheEntry(uri, absoluteUri.get(), false /* isSvg */, GetImageOptions::None, decodeActivity, imageId, &cacheHit, imageCache));

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::GetImageDescription(ImageCache &imageCache)
{
    if (m_imageMetadataView != nullptr)
    {
        m_imageMetadataView->RemoveImageViewListener(*this);
        m_imageMetadataView.reset();
    }

    m_imageMetadataView = imageCache.GetMetadataView(GetContext()->GetImageProvider()->GetDecodeActivity(), reinterpret_cast<uint64_t>(this));
    m_imageMetadataView->AddImageViewListener(*this);
    if (m_imageMetadataView->GetImageMetadata())
    {
        IFC_RETURN(OnImageViewUpdated(*m_imageMetadataView));
    }
    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::InitFromUri(xstring_ptr uri)
{
    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    const auto& decodeActivity = GetContext()->GetImageProvider()->GetDecodeActivity();

    if (decodeActivity)
    {
        decodeActivity->SetLoadedImageSurfaceUri(reinterpret_cast<uint64_t>(this), uri.GetBuffer());
    }

    auto core = GetContext();
    IFC_RETURN(GetImageCache(*core, uri, decodeActivity, reinterpret_cast<uint64_t>(this), m_imageCache.ReleaseAndGetAddressOf()));

    // Start asynchronous download/decode operation
    IFC_RETURN(GetImageDescription(*m_imageCache));

    core->AddDeviceListener(this);
    m_isDeviceListener = true;

    core->AddDisplayListener(this);
    m_isDisplayListener = true;

    IFC_RETURN(core->RegisterPLMListener(this));
    m_isPLMListener = true;

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::InitFromMemory(_In_ wistd::unique_ptr<IRawData> rawData)
{
    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    const auto& decodeActivity = GetContext()->GetImageProvider()->GetDecodeActivity();

    if (decodeActivity)
    {
        decodeActivity->SetLoadedImageSurfaceMemory(reinterpret_cast<uint64_t>(this));
    }

    auto core = GetContext();
    IFC_RETURN(GetImageCache(*core, xstring_ptr::NullString() /* uri */, decodeActivity, reinterpret_cast<uint64_t>(this), m_imageCache.ReleaseAndGetAddressOf()));

    // Set encoded data to skip async downloading
    m_imageCache->SetEncodedImageData(std::make_shared<EncodedImageData>(std::move(rawData)));

    // Start asynchronous decode operation.
    IFC_RETURN(GetImageDescription(*m_imageCache));

    core->AddDeviceListener(this);
    m_isDeviceListener = true;

    core->AddDisplayListener(this);
    m_isDisplayListener = true;

    IFC_RETURN(core->RegisterPLMListener(this));
    m_isPLMListener = true;

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::Close()
{
    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    m_closed = true;

    if (m_isPLMListener)
    {
        IFCFAILFAST(GetContext()->UnregisterPLMListener(this));
        m_isPLMListener = false;
    }

    if (m_isDisplayListener)
    {
        GetContext()->RemoveDisplayListener(this);
        m_isDisplayListener = false;
    }

    if (m_isDeviceListener)
    {
        GetContext()->RemoveDeviceListener(this);
        m_isDeviceListener = false;
    }

    if (m_imageSurfaceWrapper != nullptr)
    {
        m_imageSurfaceWrapper->ResetSurfaces(false /* mustKeepSoftwareSurface */, false /* mustKeepHardwareSurfaces */);
        m_imageSurfaceWrapper.reset();
    }

    if (m_abortableImageOperation != nullptr)
    {
        m_abortableImageOperation->DisconnectImageOperation();
        m_abortableImageOperation.reset();
    }

    if (m_imageMetadataView != nullptr)
    {
        m_imageMetadataView->RemoveImageViewListener(*this);
        m_imageMetadataView.reset();
    }

    return S_OK;
}

bool CLoadedImageSurface::IsVirtualPossible(XSIZE maxKnownPixelSize) const
{
    bool assumeVirtual = true;

    if (maxKnownPixelSize.Width > 0 && maxKnownPixelSize.Height > 0)
    {
        const INT maxTextureSize = static_cast<INT>(GetContext()->GetMaxTextureSize());
        assumeVirtual = maxKnownPixelSize.Width > maxTextureSize ||
            maxKnownPixelSize.Height > maxTextureSize;
    }

    return assumeVirtual;
}

_Check_return_ HRESULT CLoadedImageSurface::GetCompositionSurface(_Outptr_ DCompSurface** dcompSurface)
{
    *dcompSurface = nullptr;

    IFCEXPECTRC_RETURN(!m_closed, RO_E_CLOSED);

    auto hwTexture = m_imageSurfaceWrapper->GetHardwareSurface();
    if (hwTexture == nullptr)
    {
        // It is critical that this code path not attempt to use the D3D device at this time,
        // as we are being called internally by DComp which expects XAML to return a surface wrapper.
        // If we don't have a D3D device, or encounter device lost here, these would be fatal errors.
        // To avoid this altogether, we allocate a HWTexture and WinRT surface wrapper but no actual hardware surface,
        // which have no dependency on the D3D device or SurfaceFactory.
        m_imageSurfaceWrapper->AllocateHWTextureWithNoHardware(
            GetContext()->GetHWTextureManagerNoRef(),
            IsVirtualPossible(GetDesiredMaxPixelSize(c_maxPlateauScale))
            );
        hwTexture = m_imageSurfaceWrapper->GetHardwareSurface();
    }

    SetInterface(*dcompSurface, hwTexture->GetCompositionSurface());

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::FireLoadCompleted(HRESULT result)
{
    CEventManager* eventManager = GetContext()->GetEventManager();
    IFCEXPECT_RETURN(eventManager);

    auto eventArgs = make_xref<CLoadedImageSourceLoadCompletedEventArgs>();
    IFC_RETURN(eventArgs->put_Status(GetLoadStatusFromHRESULT(result)));

    eventManager->Raise(
        EventHandle(KnownEventIndex::LoadedImageSurface_LoadCompleted),
        TRUE /* refire */,
        this,
        eventArgs,
        false /* raiseSync */);

    if (SUCCEEDED(result))
    {
        // Extra tick to submit texture updates
        if (ITickableFrameScheduler *scheduler = GetContext()->GetBrowserHost()->GetFrameScheduler())
        {
            IFC_RETURN(scheduler->RequestAdditionalFrame(0U, RequestFrameReason::LoadedImageSurface));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::StartDecodingHelper()
{
    const ImageMetadata* metadata = m_imageMetadataView->GetImageMetadata();
    ASSERT(metadata != nullptr);

    const auto core = GetContext();
    const auto contentRootCoordinator = core->GetContentRootCoordinator();
    const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    auto decodedScale = RootScale::GetRasterizationScaleForContentRoot(root);

    const bool isVirtualPossible = IsVirtualPossible(GetDesiredMaxPixelSize(c_maxPlateauScale));
    if (!isVirtualPossible)
    {
        // Downscale the image if the plateau scale ever exceeds the max. If we don't do this we may
        // exceed the max texture size if the surface originally qualified as non-virtual.
        decodedScale = std::min(c_maxPlateauScale, decodedScale);
    }

    const XSIZE decodePixelSize = GetDecodePixelSize(*metadata, decodedScale);

    IFC_NOTRACE_RETURN(m_imageSurfaceWrapper->AllocateHardwareResources(
        core->GetHWTextureManagerNoRef(),
        decodePixelSize.Width,
        decodePixelSize.Height,
        pixelColor32bpp_A8R8G8B8,
        false /* opaque */,
        isVirtualPossible /* forceVirtual */,
        true /* persistent */));

    // Get the DComp surfaces that were created and the rects associated with them
    // and populate the decodeParams for the background thread to decode to
    SurfaceUpdateList surfaceUpdateList;
    IFC_NOTRACE_RETURN(m_imageSurfaceWrapper->GetHWSurfaceUpdateList(surfaceUpdateList));

    auto decodeParams = make_xref<ImageDecodeParams>(
        pixelColor32bpp_A8R8G8B8,
        decodePixelSize.Width,
        decodePixelSize.Height,
        false /* isAutoPlay */,
        surfaceUpdateList,
        true /* isLoadedImageSurface */,
        GetContext()->GetImageProvider()->GetDecodeActivity(),
        reinterpret_cast<XUINT64>(this),
        xstring_ptr::EmptyString());

    // TODO: find a way to reuse the existing operation
    if (m_abortableImageOperation != nullptr)
    {
        m_abortableImageOperation->DisconnectImageOperation();
        m_abortableImageOperation.reset();
    }

    IFC_NOTRACE_RETURN(m_imageCache->GetImage(
        decodeParams,
        m_imageAvailableCallback,
        m_abortableImageOperation));

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::StartFinalSizeDecoding()
{
    // It is not safe to assume we can attempt to allocate a hardware surface at this time as it's possible the D3D
    // device and main SurfaceFactory were released.  This can happen if the app is using the ReleaseGraphicsDeviceOnSuspend
    // feature, or if the app lost its device and was ticking while the window was hidden.
    // First check to see if we lost our device, if this has happened just wait for the next UI thread tick and we'll try
    // again in OnDeviceCreated() after re-creating our D3D device.
    bool deviceLost = false;
    IFC_RETURN(GetContext()->DetermineDeviceLost(&deviceLost));
    if (!deviceLost)
    {
        HRESULT decodeStartedHR = StartDecodingHelper();

        // If we lost our device, attempt to reload later when the device is recovered
        if (FAILED(decodeStartedHR) && !GraphicsUtility::IsDeviceLostError(decodeStartedHR))
        {
            IFC_RETURN(FireLoadCompleted(decodeStartedHR));
        }

        // Give the Core a chance to schedule another tick if we lost our device in this method.
        // It's very important that we not allow any device lost error to escape this method,
        // as this method is called via multiple public API entry points and a device-lost would crash the app.
        GetContext()->HandleDeviceLost(&decodeStartedHR);
    }

    return S_OK;
}

XSIZE CLoadedImageSurface::GetDesiredMaxPixelSize(float plateauScale) const
{
    return { XcpCeiling(m_desiredMaxWidth * plateauScale), XcpCeiling(m_desiredMaxHeight * plateauScale) };
}

XSIZE CLoadedImageSurface::GetDecodePixelSize(_In_ const ImageMetadata& metadata, float plateauScale) const
{
    const XSIZE desiredMaxPixelSize = GetDesiredMaxPixelSize(plateauScale);

    float imageScale = 1;
    if (desiredMaxPixelSize.Width > 0)
    {
        imageScale = std::min(imageScale, static_cast<float>(desiredMaxPixelSize.Width) / metadata.width);
    }
    if (desiredMaxPixelSize.Height > 0)
    {
        imageScale = std::min(imageScale, static_cast<float>(desiredMaxPixelSize.Height) / metadata.height);
    }

    return { XcpRound(metadata.width * imageScale), XcpRound(metadata.height * imageScale) };
}

bool CLoadedImageSurface::IsDownloadComplete() const
{
    return m_imageMetadataView->GetImageMetadata() != nullptr;
}

_Check_return_ HRESULT CLoadedImageSurface::OnDeviceRemoved(bool cleanupDComp)
{
    ReleaseSurfaceMemory();

    if (cleanupDComp)
    {
        // In the case of cleaning up the entire DComp device (currently limited to our tests)
        // we need to also cleanup the wrapper surface.
        // It's critical that we never release this surface wrapper in the context of real product code
        // as the app may have a permanent reference to this wrapper.
        m_imageSurfaceWrapper->CleanupDeviceRelatedResources();
    }

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::OnDeviceCreated()
{
    // The image may have not finished downloading yet. In this case it will reload when download completes.
    if (IsDownloadComplete())
    {
        IFC_RETURN(StartFinalSizeDecoding());
    }
    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::OnScaleChanged()
{
    if (m_imageCache != nullptr && m_imageMetadataView != nullptr)
    {
        xstring_ptr uri = m_imageCache->GetUri();

        if (!uri.IsNullOrEmpty())
        {
            // Underlying resource MRT may have changed.
            // Drop the existing asynchronous download/decode operation and start a new one.
            if (m_abortableImageOperation != nullptr)
            {
                m_abortableImageOperation->DisconnectImageOperation();
                m_abortableImageOperation.reset();
            }

            m_imageMetadataView->RemoveImageViewListener(*this);
            m_imageMetadataView.reset();

            // TODO: GetImageCache should operate based on fully resolved file path.
            //       Then we could skip downloading for non-MRT resources.
            IFC_RETURN(GetImageCache(*GetContext(), uri, GetContext()->GetImageProvider()->GetDecodeActivity(), reinterpret_cast<uint64_t>(this), m_imageCache.ReleaseAndGetAddressOf()));
            IFC_RETURN(GetImageDescription(*m_imageCache));
        }
        else
        {
            // In the stream case determine new size and redecode from existing encoded data
            ASSERT(IsDownloadComplete());

            // If there's already a decode in progress, we have a problem. The first step of redecoding is to resize the hardware
            // surface to the new dimensions (after the scale change). The decode thread is potentially accessing this hardware
            // surface, so it's not safe to resize it. There's no way to cancel the ongoing decode operation, either. At best we
            // can disconnect from it, which means we don't get called back when it completes, but there's no way to stop it if
            // it's in the middle of a decode already.
            //
            // We work around this by discarding the old hardware surface entirely and creating a new one before decoding. This way
            // the existing operation won't get its surface resized before it gets to complete.
            //
            // Note that in practice, this will call AsyncImageDecoder::CleanupDeviceRelatedResources, which takes a mutex that
            // the decode thread is holding during decode (the lambda in AsyncImageDecoder::SetupDecodeCurrentFrame), so we're
            // effectively blocking on the decode to complete anyway.
            ReleaseSurfaceMemory();

            IFC_RETURN(StartFinalSizeDecoding());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::OnSuspend(_In_ bool isTriggeredByResourceTimer)
{
    return S_OK;
}

_Check_return_ HRESULT CLoadedImageSurface::OnResume()
{
    // The image may have not finished downloading yet. In this case it will reload when download completes.
    if (IsDownloadComplete() && m_imageSurfaceWrapper->HasLostHardwareResources())
    {
        ReleaseSurfaceMemory();
        IFC_RETURN(StartFinalSizeDecoding());
    }

    return S_OK;
}

void CLoadedImageSurface::ReleaseSurfaceMemory()
{
    // If there is an active decode operation we need to make sure it does not access the freed surface
    if (m_abortableImageOperation != nullptr)
    {
        m_abortableImageOperation->CleanupDeviceRelatedResources();
        m_abortableImageOperation->DisconnectImageOperation();
        m_abortableImageOperation.reset();
    }

    if (auto hwTexture = m_imageSurfaceWrapper->GetHardwareSurface())
    {
        hwTexture->GetCompositionSurface()->ReleaseLegacyDCompResources(true /* resetWucSurface */);
    }
}

void CLoadedImageSurface::OnLowMemory()
{
    ReleaseSurfaceMemory();
}

_Check_return_ HRESULT CLoadedImageSurface::OnImageViewUpdated(ImageViewBase& sender)
{
    HRESULT decodeOrDownloadHR = sender.GetHR(GetContext()->GetImageProvider()->GetDecodeActivity(), reinterpret_cast<uint64_t>(this));
    if (SUCCEEDED(decodeOrDownloadHR))
    {
        // If we're in the middle of shutting down then don't bother decoding. The device that we'll need is released.
        if (IsDownloadComplete() && !DirectUI::DXamlServices::IsDXamlCoreShuttingDown())
        {
            IFC_RETURN(StartFinalSizeDecoding());
        }
    }
    // Ignore device lost error and attempt to reload later when the device is recovered
    else if (!GraphicsUtility::IsDeviceLostError(decodeOrDownloadHR))
    {
        IFC_RETURN(FireLoadCompleted(decodeOrDownloadHR));
    }

    return S_OK;
}
