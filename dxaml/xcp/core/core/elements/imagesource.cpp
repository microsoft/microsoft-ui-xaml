// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImageCacheIdentifier.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <ImageAvailableCallback.h>
#include <ImageMetadataView.h>
#include <SynchronousImageAvailableCallback.h>
#include <GraphicsUtility.h>
#include <AsyncDecodeResponse.h>
#include <ImageDecodeBoundsFinder.h>
#include <ImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageProvider.h>
#include <ImagingInterfaces.h>
#include <EncodedImageData.h>
#include <OfferableSoftwareBitmap.h>
#include <PixelFormat.h>
#include <RawData.h>
#include "DOPointerCast.h"
#include <UIThreadScheduler.h>
#include <WindowsGraphicsDeviceManager.h>
#include <XamlServiceProviderContext.h>
#include <XamlQualifiedObject.h>
#include <SvgImageSource.h>
#include <SvgImageSourceOpenedEventArgs.h>
#include <SvgImageSourceFailedEventArgs.h>
#include <DXamlServices.h>
#include <RootScale.h>
#include <shlwapi.h>
#include "ImagingTelemetry.h"

using namespace DirectUI;

//  IMAGE PIPELINE PRIMER
//
//  == Overview ==
//
//  The image pipeline has gotten more and more complex over the years so this
//  comment will go over some of the parts and major features and should be
//  added to and revised over time.
//
//  The central logic for handling images is done through the classes in this
//  file and the objects that these objects hold.  There are MSDN articles
//  on ImageSource/BitmapSource/BitmapImage/WriteableBitmap, so this comment
//  will not describe what each of those do.
//
//  == Major classes ==
//
//  ImageSurfaceWrapper is an object the backing uncompressed surfaces and attempts
//  to abstract a lot of the logic around managing those surfaces.  It manages
//  the software and hardware surfaces and can transfer the data from software to hardware.
//  It also has logic to managed the CTiledSurface which splits larger images into
//  segments that can better fit into atlases and video memory.
//
//  In the event that the hardware surface is lost, the image will be re-decoded in the
//  case of BitmapImage and re-uploaded to hardware after that completes.  Additionally,
//  the software surface will be kept if encoded bits aren't available such as with
//  WriteableBitmap.  This is controlled with the ImageSurfaceWrapper::MustKeepSystemMemory
//  method.
//
//  ImageCache is an object that handles downloading and decode requests for images
//  and generally is managed by the ImageProvider class which provides an interface
//  to request downloads/decodes and does the appropriate caching of surfaces.  Currently
//  software and hardware surfaces are cached separately.  Refer to ImageCache.cpp and
//  hwsurfacecache.cpp.
//
//  AsyncDecodeToSurfaceTask works with WICImageDecoder to provide asynchronous (and
//  synchronous) decoding functionality.
//
//  CImageBrush provides a lot of the main functionality that is used by the software
//  render code path and has special interaction with the objects in this file around
//  realizing software surfaces.
//
//  == Major Features ==
//
//  Decode-To-Render-Size (DTRS) or Right-Sized-Decoding (RSD)
//
//  This is a feature built to take advantage of the Jupiter layout engine to decode
//  an image to the size that it will be displayed at.  There are several requirements
//  in order for this feature to work properly that can be found by sifting through the
//  comments in this file and other files.  These can generally be tracked by
//  looking at the TraceDecodeToRenderSizeDisabledInfo ETW event.  This feature can also
//  be enabled/disabled with the DecodeToRenderSize registry key.
//
//  This feature works by first requesting a metadata decode which asynchronously
//  determines the width and height of the image so that layout can proceed to determine
//  the rendered size of the image.  When the rendering walks the image element in HWWalk
//  it will call to QueueRequestDecode to request a decode to the desired size.  At the
//  end of the render walk, these requests are processed in ProcessTrackedBitmapImages
//  located in CCoreServices and any decodes that need to be issued will be.  This includes
//  decoding an image to a larger size if necessary.
//
//  Background Thread Image Loading (BTIL)
//
//  This feature was built to decode an image as directly as possible to the hardware surface
//  it will ultimately be rendered from.  It is currently enabled/disabled by the
//  RuntimeEnabledFeature::BackgroundThreadImageLoading control switch which also has a
//  corresponding registry key.  It provides benefits in terms of memory savings by not
//  requiring a full system memory buffer to store the uncompressed bits for the UI thread to
//  copy.  It also improves UI responsiveness, since the image uncompressed bits aren't being
//  copied on the UI thread during the render walk.
//
//  This feature works by modifying a few of the fundamental classes used for image decoding.
//  ImageSurfaceWrapper abstracts a lot of the work used for managing the hardware surfaces
//  and provides some new functions to AllocateHardwareResources, UpdateHardwareResources and
//  GetHWSurfaceUpdateList.  These methods are somewhat self explanatory but there are some
//  nuances.  AllocateHardwareResources provides a new autoUpdate field which allows control
//  of when the textures are queued for update.  For synchronization reasons, the background
//  thread is prevented from accessing the HWTextureManager by separating the HWRgbTexture::Unlock
//  from queueing the surface for update.  This surface update is actually queued in
//  EnsureAndUpdateHardwareResources located in this file through the new UpdateHardwareResources
//  method.  UpdateHardwareResources will check if the surface has been updated already, and
//  if it hasn't, it will queue the texture for update.  Future support can be added to update
//  the image directly in the background thread, as long as the interaction is not done through
//  the HWTextureManager update queue and only interacts with local objects and DComp/D3D
//  interfaces.
//
//  GetHWSurfaceUpdateList retrieves a list of SurfaceDecodeParams objects that have a rectangle
//  indicating their position and size in a larger image and the IPALSurface backing them.
//  This list is meant to have the minimal amount of information necessary to provide to
//  a background thread so that the image can be decoded.  This list is then processed
//  for decoding in WICImageDecoder::DecodeToMultipleTiles.  This decoding method
//  also has the benefit of decoding several lines at a time to a smaller temporary system
//  memory buffer, and then locking the hardware surface to copy these lines.  This reduces
//  contention with the UI thread for the DX device and reduces peak memory usage by
//  not requiring a full software buffer to store the image before copying it to the
//  hardware staging surface.  Instead it is copied several lines at a time to the
//  hardware staging surface.  This can be further optimized as suggested above to also
//  update the hardware DComp resource after decoding is complete as long as it isn't
//  queued.
//
//  Thread synchronization is done by allocating the surfaces ahead of time through
//  AllocateHardwareResources and LockHardwareMutex/UnlockHardwareMutex in
//  ImageSurfaceWrapper.  This Lock/Unlock prevents ImageSurfaceWrapper from exposing the
//  hardware surface or operating on the hardware surface.  The terminology is like a mutex
//  lock, not a surface lock (which returns a pointer to the data).  The has the effect
//  of rendering the hardware surfaces "invisible", so that no one else can get the surfaces
//  to operate on them.  This also requires that the background thread only operates on the
//  surfaces that are given from GetHWSurfaceUpdateList and the operations called on those
//  surfaces (specifically Lock/Unlock), and do not operate an a globally accessible object.
//  The ImageSurfaceWrapper does allow the hardware surfaces to be released, however since
//  that is controlled by interlocked reference counting which is thread safe.  The background
//  decode thread also checks this reference count so if the reference count drops to 1,
//  meaning that the background thread is the last object holding onto the reference, it will
//  stop decoding early.
//
//  (This part will be updated in 1501 when XAML Drag and Drop custom visual will be updated
//  following the update of Core Drag and Drop API)
//  As of TP2, Drag and Drop uses the software bits of ImageBitmap to create custom Drag&Drop.
//  An ImageBrush is created in this case, see dxaml\lib\DragVisual_Partial.cpp
//  Note that Drag and Drop Custom Visual uses IsImageDecoded to detect if the bits are available
//  because when the BitmapImage is passed to the CustomVisual, it may be already too late
//  to be able to handle ImageOpened or ImageFailed events
//

// TODO: Anytime in this file we do OfTypeByIndex<KnownTypeIndex::SvgImageSource>() it is poorly using the polymorphism built into this class.
//                 since generally the derived class should be able to specialize the functionality of the base class in those circumstances.  Try
//                 to refactor this in a future change.

CImageSource::CImageSource(_In_ CCoreServices *pCore)
    : CMultiParentShareableDependencyObject(pCore)
{
    m_pImageSurfaceWrapper  = NULL;
    m_fSourceUpdated        = FALSE;
    m_fSourceNeedsMeasure   = FALSE;
    m_pAsyncAction          = nullptr;
    m_fDecodeToRenderSize   = FALSE;
    m_fDecodeToRenderSizeForcedOff = FALSE;
    m_pendingDecodeForLostSoftwareSurface = FALSE;
    m_pEventList = nullptr;
    m_downloadProgress = 0;
    m_nCreateOptions = DirectUI::BitmapCreateOptions::None;
    m_decodePixelWidth = 0;
    m_decodePixelHeight= 0;
    m_decodePixelType = DirectUI::DecodePixelType::Physical;
    m_bitmapState = BitmapImageState::Initial;
    m_fReloadNeeded = false;
    m_fRegisteredWithReloadManager = false;
    m_forceDecodeRequest = false;
    m_publicBitmapEventFired = false;
    m_keepSoftwareSurfaceOnReload = false;
}

HRESULT CImageSource::SetupImageSource(
    bool mustKeepSoftwareSurface,
    _In_opt_ CREATEPARAMETERS* createParameters
)
{
    IFC_RETURN(ImageSurfaceWrapper::Create(GetContext(), mustKeepSoftwareSurface, &m_pImageSurfaceWrapper));

    if (createParameters != nullptr)
    {
        if (createParameters->m_value.GetType() == valueString)
        {
            uint32_t stringLen = 0;
            const WCHAR* pString = createParameters->m_value.AsEncodedString().GetBufferAndCount(&stringLen);
            if (stringLen == 0)
            {
                m_strSource.Reset();
            }
            else
            {
                IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(pString, stringLen, &m_strSource));
                m_fReloadNeeded = true;

                // Propagate the base URI from the parse context
                if (createParameters->m_spServiceProviderContext)
                {
                    auto serviceProviderContext = createParameters->m_spServiceProviderContext;

                    auto baseUri = serviceProviderContext->GetBaseUri();
                    if (baseUri)
                    {
                        SetBaseUri(baseUri);
                    }
                }
            }

            ImagingTelemetry::SetUriSource(reinterpret_cast<uint64_t>(this), m_strSource.GetBuffer());
        }
    }

    return S_OK;
}

// Provides a factory-like mechanism to parse extension and create a CSvgImageSource or a CBitmapImage
_Check_return_ HRESULT
CImageSource::Create(
    _Outptr_ CDependencyObject** retObject,
    _In_ CREATEPARAMETERS* createParameters
    )
{
    // Create an SVGImageSource when parsing <Image Source="blah.svg"/>.
    if (createParameters->m_value.GetType() == valueString)
    {
        uint32_t stringLen = 0;
        auto string = createParameters->m_value.AsEncodedString().GetBufferAndCount(&stringLen);
        auto ext = PathFindExtension(string);
        //::setlocale(LC_ALL,"");
        if (!_wcsicmp(ext, L".svg") || !_wcsicmp(ext, L".svgz")) //local insensitive svgz.
        {
            return CSvgImageSource::Create(retObject, createParameters);
        }
    }

    //NOTE: ImageSource is meant to be an abstract class for image sources and as such
    //      shouldn't be creatable. A bitmap image is returned instead to provide
    //      default behavior.

    return CBitmapImage::Create(retObject, createParameters);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up the image source.
//
//------------------------------------------------------------------------
CImageSource::~CImageSource()
{
    auto core = GetContext();

    if (m_imageMetadataView != nullptr)
    {
        m_imageMetadataView->RemoveImageViewListener(*this);
        m_imageMetadataView.reset();
    }

    DisconnectImageOperation();

    core->RemoveImageUpdateRequest(m_pImageSurfaceWrapper);

    VERIFYHR(UnregisterWithReloadManager());

    if (m_pEventList)
    {
        delete m_pEventList;
        m_pEventList = NULL;
    }

    VERIFYHR(core->RemoveImageDecodeRequest(this));
    VERIFYHR(core->StopTrackingImageForRenderWalk(this));
    core->StopTrackingAnimatedImage(this);

    ReleaseInterface(m_pImageSurfaceWrapper);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for any asynchronous decode/download completing for the
//      image source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::OnImageAvailableCommon(
    _In_ IImageAvailableResponse* pResponse
    )
{
    IFC_RETURN(SetDirty());

    HRESULT hrImageResult = pResponse->GetDecodingResult();

    ImagingTelemetry::DecodeResultAvailable(reinterpret_cast<uint64_t>(this), hrImageResult);
    // TODO: Trace the surface upload as well.

    if (SUCCEEDED(hrImageResult))
    {
        auto &spSurface = pResponse->GetSurface();

        // pSurface may be null if the image was decoded directly to a hardware surface.  Only update it if
        // it has a valid value.
        if (spSurface != nullptr)
        {
            m_pImageSurfaceWrapper->SetSoftwareSurface(spSurface);
        }
    }

    // If the hardware resources were locked for BackgroundThreadImageLoading, then unlock it here
    if (m_pImageSurfaceWrapper->IsHardwareLocked())
    {
        m_pImageSurfaceWrapper->UnlockHardwareMutex();
    }

    // Make sure there is a surface available before marking it for update.  A surface may not be available in the callback
    // if this was a metadata decode to get the size of the image for DTRS.
    if (m_pImageSurfaceWrapper->HasAnySurface())
    {
        // Indicate that an update of the hardware surface is required during the next EnsureAndUpdateHardwareResources
        m_pImageSurfaceWrapper->SetUpdateRequired(true);

        // Enqueue an image update request in the case that the image does not get processed in the render walk.
        GetContext()->EnqueueImageUpdateRequest(m_pImageSurfaceWrapper);
    }

    // It is possible that we could have created images totally outside of the XAML
    // tree (e.g. SoftwareBitmapImage) and these items could have hardware surfaces
    // attached to them.  Even though they aren't in the tree, we need to ensure that
    // the surfaces they are holding onto get cleaned during device lost so they don't hold
    // onto the DComp Surface Factory and/or the D3D device.  So, if once have have completed
    // the decoding, if they haven't been added to the tree we will register them as being
    // available for cleanup (as if they had been in the tree and then left).
    //
    // Note: There is an assert when registering for cleanup if the device is already in the
    //       list.  That can now happen if an item is in the tree and then gets removed
    //       prior to this method being called.  So, if the cleanup timestamp is non zero
    //       we assume that it has come out of the tree and is already in the list (or has
    //       already been cleaned up).

    if (!IsActive() && m_pImageSurfaceWrapper && m_pImageSurfaceWrapper->GetDeviceCleanupTimestamp() == 0LL)
    {
        RegisterForCleanupOnLeave();
    }

    return S_OK;
}

void CImageSource::DisconnectImageOperation()
{
    m_forceDecodeRequest = true;

    if (m_spAbortableImageOperation != nullptr)
    {
        m_spAbortableImageOperation->DisconnectImageOperation();
        m_spAbortableImageOperation.reset();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the image source as dirty for rendering.
//
//------------------------------------------------------------------------
HRESULT
CImageSource::SetDirty()
{
    // Mark this object as dirty for rendering.
    CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);

    // If we get here via CCoreServices::FlushImageDecodeRequests(), the
    // above setting of the dirty flag will not schedule another tick because
    // we're currently in the middle of a tick, CRootVisual::NWPropagateDirtyFlag detects
    // this and avoids requesting another tick.
    // So, we manually request a tick ourselves for this case.
    IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::ImageDirty));
        }
    }

    return S_OK;
}

HRESULT
CImageSource::RequestMeasure()
{
    m_fSourceNeedsMeasure = TRUE;

    IFC_RETURN(SetDirty());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the surfaces in the surface wrapper.
//
//------------------------------------------------------------------------
void
CImageSource::ResetSurfaces(
    bool mustKeepSoftwareSurface,
    bool mustKeepHardwareSurfaces
    )
{
    m_pImageSurfaceWrapper->ResetSurfaces(mustKeepSoftwareSurface, mustKeepHardwareSurfaces);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether there is a hardware surface or tiles that are lost.
//
//------------------------------------------------------------------------
bool
CImageSource::CheckForLostHardwareResources()
{
    return !!m_pImageSurfaceWrapper->CheckForLostHardwareResources();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Image source registers its surface wrapper for cleanup on leave.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CMultiParentShareableDependencyObject::LeaveImpl(pNamescopeOwner, params));
    if (params.fIsLive)
    {
        RegisterForCleanupOnLeave();
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CImageSource::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CMultiParentShareableDependencyObject::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    if (m_pImageSurfaceWrapper != nullptr)
    {
        m_pImageSurfaceWrapper->CleanupDeviceRelatedResources();
    }

    if (m_spAbortableImageOperation != nullptr)
    {
        m_spAbortableImageOperation->CleanupDeviceRelatedResources();
    }

    m_spPendingDecodeParams = nullptr;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      This method is used to find what elements in the tree this imagesource
//  is parented to and fire the TraceBitmapImageRelationInfo event
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::TraceImageSourceRelationEtw(
    _In_ CDependencyObject *pDO
    )
{
    // Use a queue to walk the parent element tree to find the first UIElement element
    // that is a parent of the element
    xqueue<CDependencyObject*> traversalQueue;

    IFC_RETURN(traversalQueue.push(pDO));

    while (!traversalQueue.empty())
    {
        CDependencyObject* pCurrentDO;
        IFC_RETURN(traversalQueue.front(pCurrentDO));
        IFC_RETURN(traversalQueue.pop());

        // Check if this is the DO that matches the criteria
        if (pCurrentDO->OfTypeByIndex<KnownTypeIndex::UIElement>() && pCurrentDO != pDO)
        {
            TraceImageSourceRelationInfo(reinterpret_cast<UINT64>(pDO), reinterpret_cast<UINT64>(pCurrentDO), pCurrentDO->m_strName.GetBuffer());
        }
        else if (pCurrentDO->OfTypeByIndex<KnownTypeIndex::ImageBrush>() ||
            pCurrentDO->OfTypeByIndex<KnownTypeIndex::ImageSource>())
        {
            // Push all the DO parents into the queue to search next
            auto parentCount = pCurrentDO->GetParentCount();

            for (size_t parentIndex = 0; parentIndex < parentCount; parentIndex++)
            {
                CDependencyObject *pParentNoRef = pCurrentDO->GetParentItem(parentIndex);
                IFC_RETURN(traversalQueue.push(pParentNoRef));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if this image source has a source URI set and if it is
//      the same as another image source URI.
//
//------------------------------------------------------------------------

bool
CImageSource::IsSourceSetAndEqualTo( _In_opt_ CImageSource * pOther )
{
    return (!m_strSource.IsNull() &&
            pOther != NULL &&
            m_strSource.Equals( pOther->m_strSource ));
}

bool
CImageSource::ShouldContinueAsyncAction()
{
    if (m_pAsyncAction != nullptr)
    {
        return !!m_pAsyncAction->CoreContinueAsyncAction();
    }
    else
    {
        return true;
    }
}

void
CImageSource::CompleteAsyncAction(
    HRESULT hr
    )
{
    if (m_pAsyncAction != nullptr)
    {
        if (FAILED(hr))
        {
            m_pAsyncAction->CoreSetError(hr);
        }
        m_pAsyncAction->CoreFireCompletion();
        m_pAsyncAction->CoreReleaseRef();
        m_pAsyncAction = nullptr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new bitmap source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapSource::Create(
    _Outptr_ CDependencyObject** retObject,
    _In_ CREATEPARAMETERS* createParameters
    )
{
    xref_ptr<CBitmapSource> bitmapSource;
    bitmapSource.attach(new CBitmapSource(createParameters->m_pCore));

    // Intentionally doesn't set CREATEPARAMETERS for CBitmapSource
    IFC_RETURN(bitmapSource->SetupImageSource(true /*mustKeepSoftwareSurface*/));

    *retObject = bitmapSource.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Synchronously decodes an image from memory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::DecodeStream(
    _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
    bool synchronousDecode,
    bool retainPlaybackState
    )
{
    XUINT32 decodeWidthInPhysicalPixels = 0;
    XUINT32 decodeHeightInPhysicalPixels = 0;

    GetDecodePixelWidthAndHeightInPhysicalPixels(
        GetDecodeType(),
        GetDecodeWidth(),
        GetDecodeHeight(),
        &decodeWidthInPhysicalPixels,
        &decodeHeightInPhysicalPixels);

    IFC_RETURN(DecodeStreamWithSize(
        spImageAvailableCallback,
        decodeWidthInPhysicalPixels,
        decodeHeightInPhysicalPixels,
        synchronousDecode,
        TRUE /* forceDecode */,
        retainPlaybackState,
        false /* decodeToRenderSize */));

    return S_OK;
}

_Check_return_ HRESULT CImageSource::SetImageCache(xref_ptr<ImageCache> imageCache)
{
    if (m_imageCache != imageCache)
    {
        DisconnectImageOperation();

        if (m_imageMetadataView != nullptr)
        {
            m_imageMetadataView->RemoveImageViewListener(*this);
            m_imageMetadataView.reset();
        }

        m_imageCache = std::move(imageCache);
        m_downloadProgress = 0;

        if (m_imageCache != nullptr)
        {
            const bool isExplicitDecodeSizeSet = GetDecodeWidth() || GetDecodeHeight();
            if (isExplicitDecodeSizeSet)
            {
                TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::DecodeSizeSpecified);
            }

            SetBitmapState(BitmapImageState::Downloading);

            m_imageMetadataView = m_imageCache->GetMetadataView(reinterpret_cast<uint64_t>(this));
            m_imageMetadataView->SetGraphicsDevice(GetContext()->GetGraphicsDevice());
            m_imageMetadataView->SetMaxRootSize(GetContext()->GetContentRootMaxSize());
            m_imageMetadataView->AddImageViewListener(*this);
            if (m_imageMetadataView->GetImageMetadata() || FAILED(m_imageMetadataView->GetHR(reinterpret_cast<uint64_t>(this))))
            {
                // We already have metadata available, so this doesn't need to be downloaded. Go straight to
                // OnImageViewUpdated (which gets called when the download completes).
                ImagingTelemetry::FoundCompletedDownload(reinterpret_cast<uint64_t>(this));
                IFC_RETURN(OnImageViewUpdated(*m_imageMetadataView));
            }
            else
            {
                // The ImageCache doesn't have metadata yet, so the download is still in progress. We'll wait for the
                // existing download to complete and be notified via OnImageViewUpdated afterwards (the
                // AddImageViewListener call above puts us in the list of things to notify once download completes).
                ImagingTelemetry::WaitForDownloadInProgress(reinterpret_cast<uint64_t>(this));
            }
        }
        else
        {
            SetBitmapState(BitmapImageState::Initial);
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------------------------
//
//  Allows an image to be loaded from a stream, as opposed to a URI.
//  This used for both the old synchronous SetSource API and the new SetSourceAsync API.
//
//-------------------------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::SetSource(
    XUINT32 cInputSize,
    _In_reads_(cInputSize) XUINT8 *pInputBuffer,
    _In_ ICoreAsyncAction *pCoreAsyncAction
    )
{
    bool bSyncDecode = (pCoreAsyncAction == NULL);

    TraceImageSetSourceBegin(reinterpret_cast<XUINT64>(this));

    auto traceGuard = wil::scope_exit([this]
    {
        TraceImageSetSourceEnd(reinterpret_cast<XUINT64>(this));
    });

    IFCPTR_RETURN(pInputBuffer);

    // Clear the existing URI source.
    m_strSource.Reset();

    ImagingTelemetry::SetStreamSource(reinterpret_cast<uint64_t>(this));

    // OnSourceSet() is called when we have a stream source. We don't do any automatic
    // reloading for a stream source, so unregister from the reload manager.
    IFC_RETURN(UnregisterWithReloadManager());

    // If we're being asked to do a synchronous decode, we cannot use DecodeToRenderSize
    // because this currently requires async decoding, which is a breaking behavior change.
    // In the case of this particular bug, we're seeing the app query for the PixelWidth/PixelHeight
    // just after calling SetSource and failing if these are not set to expected values.
    // The fix is to just turn off DecodeToRenderSize for all synchronous decodes.
    if (bSyncDecode)
    {
        TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::SynchronousDecode);
        m_fDecodeToRenderSizeForcedOff = TRUE;
    }

    // Compatibility issue with DecodeToRenderSize:
    // When using SetSource, the existing contract the API makes is that the decode will happen
    // regardless of whether or not the element is in the live tree.  We need to honor this contract.
    // If this element is not in the live tree, there is no guarantee it will ever enter the tree
    // in the future and thus there's no guarantee it will ever get rendered.  This means we
    // cannot use DecodeToRenderSize since a render walk to the element is required.
    // The fix is to just turn off DecodeToRenderSize if the element is not in the live tree.
    // Note that for URI-based BitmapImage's, the element must be in the live tree for decoding to occur.
    if (!IsActive())
    {
        TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::NotInLiveTree);
        m_fDecodeToRenderSizeForcedOff = TRUE;
    }

    auto rawData = wil::make_unique_failfast<RawData>();
    rawData->Allocate(cInputSize);

    memcpy(rawData->GetData(), pInputBuffer, rawData->GetSize());

    // Svg is known ahead of time based on the type of object.  This will save time parsing to determine
    // file type later.
    bool isSvg = OfTypeByIndex<KnownTypeIndex::SvgImageSource>();

    auto encodedImageData = std::make_shared<EncodedImageData>(
        std::move(rawData),
        0u,
        isSvg);

    // Cancel any current image download or decode operation.
    DisconnectImageOperation();

    ResetSurfaces(!!m_pImageSurfaceWrapper->MustKeepSystemMemory(), true /* mustKeepHardwareSurfaces */);

    if (m_pAsyncAction != NULL)
    {
        m_pAsyncAction->CoreReleaseRef();
    }
    m_pAsyncAction = pCoreAsyncAction;

    IFC_RETURN(PrepareDecode(false /*retainPlaybackState*/));

    // Flag all callbacks that they should get events.
    IFC_RETURN(ResetCallbackEvents());

    {
        xref_ptr<ImageCache> imageCache;

        bool cacheHit = false;
        IFC_RETURN(GetContext()->GetImageProvider()->EnsureCacheEntry(
            xstring_ptr::EmptyString() /* strUri */,
            nullptr /* absoluteUri */,
            false /* isSvg */,
            GetImageOptions::None,
            reinterpret_cast<uint64_t>(this),
            &cacheHit,
            imageCache.ReleaseAndGetAddressOf()));

        imageCache->SetEncodedImageData(std::move(encodedImageData));

        IFC_RETURN(SetImageCache(std::move(imageCache)));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for when a download/decode has completed from a stream.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::OnStreamImageAvailable(
    _In_ IImageAvailableResponse* pResponse
    )
{
    HRESULT hr = S_OK;

    if (FAILED(pResponse->GetDecodingResult()))
    {
        DisconnectImageOperation();
    }

    if (ShouldContinueAsyncAction())
    {
        // Only keep hardware surfaces under these conditions:
        // - If there is no software surface provided and there is an active decode to hardware operation.
        // - If the software surface matches the hardware surface dimensions.
        auto &spSoftwareBitmap = pResponse->GetSurface();

        bool mustKeepHardwareSurfaces = false;

        if (spSoftwareBitmap == nullptr)
        {
            mustKeepHardwareSurfaces = m_pImageSurfaceWrapper->IsHardwareLocked();
        }
        else
        {
            mustKeepHardwareSurfaces =
                (m_pImageSurfaceWrapper->GetHardwareSurface() != nullptr) &&
                (spSoftwareBitmap->GetWidth() == m_pImageSurfaceWrapper->GetHardwareSurface()->GetWidth()) &&
                (spSoftwareBitmap->GetHeight() == m_pImageSurfaceWrapper->GetHardwareSurface()->GetHeight());
        }

        // Reset the image source properties, we do this here only
        // after a non-Cancelled SetSource/SetSourceAsync
        //
        // When the image is already decoded, fired events,
        // but the software surface was released to free up memory.
        // Later, the software surface was requested again for rendering by CacheMode == BitmapCache
        // It needs to be redecoded under the covers without clearing the image source.
        if (!m_pendingDecodeForLostSoftwareSurface)
        {
            // Reset the object state and keep surfaces if necessary.
            IFC(ResetForSourceChange(
                ShouldKeepSoftwareSurfaces(),
                mustKeepHardwareSurfaces,
                true /*keepEncodedData*/));
        }
        else if (spSoftwareBitmap != nullptr)
        {
            // Must do a reset on the surfaces but not the object state based on the same conditions.
            ResetSurfaces(
                !!m_pImageSurfaceWrapper->MustKeepSystemMemory(),
                mustKeepHardwareSurfaces);
        }

        IFC(OnDownloadImageAvailableImpl(pResponse));
        m_pendingDecodeForLostSoftwareSurface = FALSE;
    }
Cleanup:
    CompleteAsyncAction(pResponse->GetDecodingResult());

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static method for getter property for the pixel width.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::PixelWidth(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    CBitmapSource *pBitmapSource = NULL;

    IFCPTR_RETURN(pObject);

    IFC_RETURN(DoPointerCast(pBitmapSource, pObject));

    pResult->SetSigned(pBitmapSource->GetPixelWidth());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static method for getter property for the pixel height.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::PixelHeight(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    CBitmapSource *pBitmapSource = NULL;

    IFCPTR_RETURN(pObject);

    IFC_RETURN(DoPointerCast(pBitmapSource, pObject));

    pResult->SetSigned(pBitmapSource->GetPixelHeight());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetDecodePixelWidthAndHeightInPhysicalPixels
//
//  Synopsis:
//      A helper function to convert decode pixel type/width/height into
//      the correct physical pixel values - these physical pixel values are
//      needed, for instance, when interacting with the ImageProvider.
//
//------------------------------------------------------------------------
void
CImageSource::GetDecodePixelWidthAndHeightInPhysicalPixels(
    _In_  const DirectUI::DecodePixelType decodePixelType,
    _In_  const XUINT32         decodePixelWidth,
    _In_  const XUINT32         decodePixelHeight,
    _Out_       XUINT32 * const pDecodePixelWidthInPhysicalPixels,
    _Out_       XUINT32 * const pDecodePixelHeightInPhysicalPixels)
{
    if (DirectUI::DecodePixelType::Logical == decodePixelType)
    {
        // Warning: This may not work correctly with AppWindows
        //     Task 19490172: CImageSource::RegisterWithReloadManager must be AppWindow aware
        const auto root = VisualTree::GetContentRootForElement(this);
        const auto scaleFactor = RootScale::GetRasterizationScaleForContentRoot(root);

        *pDecodePixelWidthInPhysicalPixels  = XcpRound(decodePixelWidth  * scaleFactor);
        *pDecodePixelHeightInPhysicalPixels = XcpRound(decodePixelHeight * scaleFactor);
    }
    else
    {
        ASSERT(DirectUI::DecodePixelType::Physical == decodePixelType);
        *pDecodePixelWidthInPhysicalPixels  = decodePixelWidth;
        *pDecodePixelHeightInPhysicalPixels = decodePixelHeight;
    }
}

bool CImageSource::IsMetadataAvailable() const
{
    return (m_imageMetadataView != nullptr) && (m_imageMetadataView->GetImageMetadata() != nullptr);
}

bool CImageSource::IsAnimatedBitmap()
{
    if (OfTypeByIndex<KnownTypeIndex::BitmapImage>())
    {
        CValue value;
        IFCFAILFAST(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::BitmapImage_IsAnimatedBitmap), &value));
        return value.AsBool();
    }
    return false;
}

_Check_return_ HRESULT CImageSource::SuspendAnimation()
{
    if (m_spAbortableImageOperation != nullptr && IsAnimatedBitmap())
    {
        m_spAbortableImageOperation->SuspendAnimation();
    }
    return S_OK;
}

_Check_return_ HRESULT CImageSource::ResumeAnimation()
{
    if (m_spAbortableImageOperation != nullptr && IsAnimatedBitmap())
    {
        IFC_RETURN(m_spAbortableImageOperation->ResumeAnimation());
    }
    return S_OK;
}

_Check_return_ HRESULT CImageSource::PlayAnimation()
{
    if (m_spAbortableImageOperation != nullptr && IsAnimatedBitmap())
    {
        IFC_RETURN(m_spAbortableImageOperation->PlayAnimation());
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsPlaying, true));
    }
    return S_OK;
}

_Check_return_ HRESULT CImageSource::StopAnimation()
{
    if (m_spAbortableImageOperation != nullptr && IsAnimatedBitmap())
    {
        IFC_RETURN(m_spAbortableImageOperation->StopAnimation());
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsPlaying, false));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base AddEventListener.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}

static _Check_return_ HRESULT GetAbsoluteUri(CDependencyObject &base, const xstring_ptr& sourceUri, _Outptr_ IPALUri** absoluteUri)
{
    *absoluteUri = nullptr;

    xref_ptr<IPALUri> preferredBaseUri;
    preferredBaseUri.attach(base.GetBaseUri()); // GetBaseUri increments the ref count of the returned pointer

    auto core = base.GetContext();

    if (!preferredBaseUri)
    {
        preferredBaseUri = core->GetBaseUriNoRef();
        IFCCATASTROPHIC_RETURN(preferredBaseUri);
    }

    xref_ptr<IPALResourceManager> resourceManager;
    IFC_RETURN(core->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
    IFC_RETURN(resourceManager->CombineResourceUri(preferredBaseUri.get(), sourceUri, absoluteUri));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the absolute URI from the source string and base URI.
//      This gets cached because URI create/combine operations are
//      expensive, and the same value is needed in several places.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::EnsureAbsoluteUri()
{
    if (m_absoluteUri == nullptr && !m_strSource.IsNull())
    {
        IFC_RETURN(GetAbsoluteUri(*this, m_strSource, m_absoluteUri.ReleaseAndGetAddressOf()));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to handle setting the source property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::BitmapImage_UriSource
        || args.m_pDP->GetIndex() == KnownPropertyIndex::SvgImageSource_UriSource)
    {
        xstring_ptr strSource;

        if (args.m_value.GetType() == valueString)
        {
            XUINT32 cString = 0;
            const WCHAR* pString = args.m_value.AsEncodedString().GetBufferAndCount(&cString);
            if (cString)
            {
                IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(
                    pString,
                    cString,
                    &strSource));
            }
        }
        else if (args.m_value.GetType() == valueObject)
        {
            const CString *pSource = do_pointer_cast<CString>(args.m_value);
            if (pSource && !pSource->m_strString.IsNull())
            {
                IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(
                    pSource->m_strString.GetBuffer(),
                    pSource->m_strString.GetCount(),
                    &strSource));
            }
        }
        else if (args.m_value.GetType() == valueNull)
        {
            // do nothing.  The strSource is empty.
        }
        else
        {
            IFC_RETURN(E_FAIL); // Unsupported Type.
        }

        if (!strSource.IsNull() && !m_strSource.IsNull() && strSource.Equals(m_strSource))
        {
            return S_OK;
        }

        // During parse, avoid URL validation because we'll have to validate again once we have the
        // final base URI when we enter the tree. Redundant validation is expensive.
        xref_ptr<IPALUri> absoluteUriValidated;
        if (!IsParsing() && !strSource.IsNull())
        {
            // Note: The following call will also implicitly validate the source URI.
            IFC_RETURN(GetAbsoluteUri(*this, strSource, absoluteUriValidated.ReleaseAndGetAddressOf()));
        }

        m_strSource = std::move(strSource);
        m_absoluteUri = std::move(absoluteUriValidated); // it's ok to be nullptr - we'll refresh on demand

        // Flag all callbacks that they should get events.
        IFC_RETURN(ResetCallbackEvents());

        IFC_RETURN(OnUriSourceChanged(false /*retainPlaybackState*/));

        m_fSourceUpdated = TRUE;  // Update the Image.Source string property.

        // TODO: MERGE: EXISTING: why not setvalue? is this an "optimization?"
        IFC_RETURN(SetPropertyIsLocal(args.m_pDP));
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::BitmapImage_CreateOptions)
    {
        IFC_RETURN(__super::SetValue(args));

        // Only reload if there is no surface available and a download/decode is not
        // already in progress, and there is a URI source to reload
        if (!m_pImageSurfaceWrapper->HasAnySurface() && m_spAbortableImageOperation == nullptr && !m_strSource.IsNullOrEmpty())
        {
            // If there's no surface, then we're in the initial, failed, downloading, decoding, or encoded image only states.
            // We'll have an image get operation during downloading, decoding, and encoded image only.
            ASSERT(m_bitmapState == BitmapImageState::Initial
                || m_bitmapState == BitmapImageState::Failed);
            m_fReloadNeeded = true;
            IFC_RETURN(TriggerReloadImage(false /*uriChanged*/, true /*retainPlaybackState*/, true /*checkForParsing*/));
        }
    }
    else if ((args.m_pDP->GetIndex() == KnownPropertyIndex::BitmapImage_DecodePixelWidth))
    {
        XUINT32 originalDPW = m_decodePixelWidth;

        IFC_RETURN(__super::SetValue(args));

        // Only update the decode size if the width was changed, and if it doesn't match the current image width and
        // if it wasn't already inferred based on the aspect ratio
        // Note: Logic breakdown for readability, compiler will optimize
        bool dpwChanged = (originalDPW != m_decodePixelWidth);

        bool dpwAuto = (m_decodePixelWidth == 0);

        bool dpwMatch =
            (m_decodePixelWidth == GetWidth()) &&
            (m_decodePixelHeight == GetHeight());

        XUINT32 inferredWidth = 0;

        if (IsMetadataAvailable())
        {
            auto imageMetadata = m_imageMetadataView->GetImageMetadata();

            inferredWidth = (m_decodePixelHeight * imageMetadata->width) / imageMetadata->height;
        }

        bool inferredMatch =
            ((originalDPW == 0) &&
            IsMetadataAvailable() &&
            (m_decodePixelWidth == inferredWidth)) ||
            (dpwAuto &&
            (originalDPW != inferredWidth));

        if (dpwChanged &&
            (!dpwMatch ||
            dpwAuto) &&
            !inferredMatch)
        {
            IFC_RETURN(UpdateDecodeSize());
        }
    }
    else if ((args.m_pDP->GetIndex() == KnownPropertyIndex::BitmapImage_DecodePixelHeight))
    {
        XUINT32 originalDPH = m_decodePixelHeight;

        IFC_RETURN(__super::SetValue(args));

        // Only update the decode size if the height was changed, and if it doesn't match the current image height and
        // if it wasn't already inferred based on the aspect ratio
        // Note: Logic breakdown for readability, compiler will optimize
        bool dpwChanged = (originalDPH != m_decodePixelHeight);

        bool dpwAuto = (m_decodePixelHeight == 0);

        bool dpwMatch =
            (m_decodePixelWidth == GetWidth()) &&
            (m_decodePixelHeight == GetHeight());

        XUINT32 inferredHeight = 0;

        if (IsMetadataAvailable())
        {
            auto imageMetadata = m_imageMetadataView->GetImageMetadata();

            inferredHeight = (m_decodePixelWidth * imageMetadata->height) / imageMetadata->width;
        }

        bool inferredMatch =
            ((originalDPH == 0) &&
            IsMetadataAvailable() &&
            (m_decodePixelHeight == inferredHeight)) ||
            (dpwAuto &&
            (originalDPH != inferredHeight));

        if (dpwChanged &&
            (!dpwMatch ||
            dpwAuto) &&
            !inferredMatch)
        {
            IFC_RETURN(UpdateDecodeSize());
        }
    }
    else
    {
        IFC_RETURN(__super::SetValue(args));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This function exists so we can have a common code path between
//      SetValue of UriSource and ReloadOnResourceInvalidation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageSource::OnUriSourceChanged(bool retainPlaybackState)
{
    ImagingTelemetry::SetUriSource(reinterpret_cast<uint64_t>(this), m_strSource.GetBuffer());

    // We may keep the HW surface to avoid flickering when switching URIs.
    IFC_RETURN(ResetForSourceChange(
        false /* mustKeepSoftwareSurface */,
        !m_strSource.IsNull() /* mustKeepHardwareSurfaces */,
        false /* keepEncodedData */));

    // Always mark ReloadedNeeded whenever m_strSource changes, except when
    // m_strSource is NULL.
    m_fReloadNeeded = !m_strSource.IsNull();

    IFC_RETURN(TriggerReloadImage(true /*uriChanged*/, retainPlaybackState, true /*checkForParsing*/));

    if (m_strSource.IsNull())
    {
        // Reset the retained size since there is no longer any previous source to retain.
        m_retainedNaturalWidth = 0;
        m_retainedNaturalHeight = 0;

        // If setting the URI source to NULL, we don't need to support reloading.
        IFC_RETURN(UnregisterWithReloadManager());
    }

    return S_OK;
}

_Check_return_
HRESULT CImageSource::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue
    )
{
    IFC_RETURN(__super::GetValue(pdp, pValue));

    if (pdp->GetIndex() == KnownPropertyIndex::BitmapImage_UriSource && !m_strSource.IsNull())
    {
        // If the URI source is set in XAML, we only store a relative fragment such as
        // "images/logo.png" in m_strSource. When returning it, however, we have to
        // return an absolute URI that has been combined with the base URI.

        IFC_RETURN(EnsureAbsoluteUri());
        IFCCATASTROPHIC_RETURN(m_absoluteUri);

        xstring_ptr strCanonicalUri;
        IFC_RETURN(m_absoluteUri->GetCanonical(&strCanonicalUri));
        pValue->SetString(std::move(strCanonicalUri));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle parsing of the element from XAML completing.
//
//  NOTE: IsParsing is still true at this point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::CreationComplete(
    )
{
    IFC_RETURN(__super::CreationComplete());

    IFC_RETURN(TriggerReloadImage(true /*uriChanged*/, false /*retainPlaybackState*/, false /*checkForParsing*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Attempt to reload the image for property changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::TriggerReloadImage(
    bool uriChanged,
    bool retainPlaybackState,
    bool checkForParsing
    )
{
    //
    // Check that the object is not currently being parsed from XAML as properties
    // may not have all been set yet.
    //
    if (!checkForParsing || !IsParsing())
    {
        if (!m_strSource.IsNull())
        {
            // Reload now if we're currently alive in the tree
            // If we're not Active, Image/ImageBrush entering tree will kickstart the download.
            if (IsActive())
            {
                IFC_RETURN(ReloadImage(uriChanged, retainPlaybackState));
            }
        }
        else
        {
            ASSERT(GetSoftwareSurface() == NULL);
            ASSERT(!m_pImageSurfaceWrapper->HasHardwareSurfaces());
            ASSERT(!m_imageMetadataView);
            ASSERT(m_bitmapState == BitmapImageState::Initial);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a software surface for printing. If the surface wrapper
//      has a software surface, then return that. If it doesn't, then
//      synchronously decode the encoded image into a software surface
//      and return that.
//
//      Printing is considered a special case and will not put the bitmap
//      image into hardware and software mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::GetSoftwareSurfaceForPrinting(
    _Out_ xref_ptr<IPALSurface>& spSoftwareSurface
    )
{
    spSoftwareSurface = GetSoftwareSurface();

    //
    // If there is no software surface but there is an encoded image, decode it to get a software
    // surface. Don't save the surface on the bitmap image though.
    //
    // The app is supposed to wait for all downloads to complete.
    // In the event they aren't, we don't decode to avoid stomping over the
    // existing operation. This bitmap image will just print nothing by design.
    //

    bool recoverImage = false;

    if (IsMetadataAvailable())
    {
        if (spSoftwareSurface == nullptr &&
            (m_pImageSurfaceWrapper->HasHardwareSurfaces()
                || m_bitmapState == BitmapImageState::HasEncodedImageOnly
                || m_bitmapState == BitmapImageState::DecodePending))
        {
             recoverImage = true;
        }
        else if (spSoftwareSurface != nullptr)
        {
            // The printing code path opens PrintDialog.exe.  On Phone, this moves the app to the background
            // and causes it to become suspended.  When the app is suspended, it's software memory for the
            // uncompressed images is offered up.  The OS could reclaim this memory which is what is happening
            // in this bug which makes the memory region invalid.  The PrintDialog will then try to print the
            // XAML content and crash when trying to access the software buffer.
            // This attempts to reclaim the memory and in the case it has been discarded, it will re-decode the
            // image synchronously for the printing code path.
            bool wasDiscarded = false;
            IFC_RETURN(spSoftwareSurface->Reclaim(&wasDiscarded));
            recoverImage = !!wasDiscarded;
        }
    }

    if (recoverImage)
    {
        auto spSyncDecodeCallback = make_xref<SynchronousImageAvailableCallback>();

        IFC_RETURN(DecodeStream(spSyncDecodeCallback, true /* synchronousDecode */, false /* retainPlaybackState */));

        spSoftwareSurface = spSyncDecodeCallback->GetResponse()->GetSurface();

        // In case something is caching the callback, release the surface reference on the callback object to
        // ensure the memory isn't held onto elsewhere.  This wouldn't be needed if DecodeStream were guaranteed
        // to never cache anything.  This can be removed if the caching system is reworked to not need it.
        spSyncDecodeCallback->Reset();
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the ImageBrush callback to our list of callbacks. This way,
//      when BitmapImage gets a DownloadProgress or an ImageFailed event,
//      it can notify all ImageBrushes that are pointing to this
//      BitmapImage. A BitmapImage either adds the callback when an
//      ImageBrush calls into CImageSource::Reload and passes the callback
//      or the ImageBrush adds a callback to itself when it is known
//      that CImageSource::ReloadImage will be called elsewhere with no
//      callback passed in.
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::AddCallback(
    _In_ CBitmapImageReportCallback* pImageErrorCallback
    )
{
    ASSERT(pImageErrorCallback != NULL);

    IFC_RETURN(m_imageReportCallbacks.push_back(xref_ptr< CBitmapImageReportCallback >(pImageErrorCallback)));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the width of the image in pixels.
//
//------------------------------------------------------------------------
XINT32
CImageSource::GetPixelWidth() const
{
    if (IsMetadataAvailable())
    {
        return m_imageMetadataView->GetImageMetadata()->width;
    }
    else
    {
        auto width = GetWidth();

        if (width != 0)
        {
            return width;
        }
        else
        {
            return m_retainedNaturalWidth;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the height of the image in pixels.
//
//------------------------------------------------------------------------
XINT32
CImageSource::GetPixelHeight() const
{
    if (IsMetadataAvailable())
    {
        return m_imageMetadataView->GetImageMetadata()->height;
    }
    else
    {
        auto height = GetHeight();

        if (height != 0)
        {
            return height;
        }
        else
        {
            return m_retainedNaturalHeight;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reloads the Image, this is called by ImageBrush when ImageBrush's
//      image is reloaded.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::ReloadImage(bool uriChanged, bool retainPlaybackState)
{
    HRESULT hr = S_OK;
    IPALSurface* pImageSurfaceSystemMemory = NULL;
    ImageProvider *pImageProvider = NULL;
    bool fPegged = false;

    xref_ptr<ImageCache> imageCache;

    // We might have changed the ImageSource on the ImageBrush, but if that ImageSource
    // is already being used somewhere else and it has already been downloaded, there is no
    // need to decode it again.  m_fReloadNeeded lets us know if the what the source is
    // pointing to has actually changed.  m_fReloadNeeded will be cleared below,
    // so the next Brush to use this ImageSource will not result in Reload happening again
    if (m_strSource.IsNull() ||
        !m_fReloadNeeded)
    {
        goto Cleanup;
    }

    // We clear the ReloadNeeded flag now before we begin the decode
    // so that if further brushes use the same source they don't try to
    // reload the image and start more downloads
    m_fReloadNeeded = false;

    if (m_absoluteUri == nullptr)
    {
        hr = EnsureAbsoluteUri();

        if (FAILED(hr))
        {
            HRESULT errorToRaise = AG_E_NETWORK_ERROR;

            if (hr == E_INVALID_CHARS_IN_URI)
            {
                errorToRaise = AG_E_INVALID_SOURCE_URI;
            }

            IFC(FireImageFailed(errorToRaise));
            goto Cleanup;
        }

        IFCCATASTROPHIC(m_absoluteUri);
    }

    // Peg to keep the Peer alive during the Reset, and until we can
    // transfer ownership to the ImageAvailableCallback.
    IFC(PegManagedPeer(FALSE /* isShutdownException */, &fPegged));

    if (uriChanged)
    {
        // We may keep the HW surface to avoid flickering when switching URIs.
        IFC(ResetForSourceChange(
            m_keepSoftwareSurfaceOnReload /* mustKeepSoftwareSurface */,
            !m_strSource.IsNull() /* mustKeepHardwareSurfaces */,
            false /* keepEncodedData */));

        // If we keep the surface with the previous URI content it must not be reused
        // to upload new image bits as it may be shared with other image sources.
        m_pImageSurfaceWrapper->SetForceRecreateHardwareResources();

        DisconnectImageOperation();
    }

    IFC(PrepareDecode(retainPlaybackState));

    {
        GetImageOptions imageGetOptions = GetImageOptions::None;
        if ((m_nCreateOptions & DirectUI::BitmapCreateOptions::IgnoreImageCache) != DirectUI::BitmapCreateOptions::None)
        {
            imageGetOptions = imageGetOptions | GetImageOptions::IgnoreCache;
        }

        IFC(EnsureAbsoluteUri());

        bool cacheHit = false;
        IFC(GetContext()->GetImageProvider()->EnsureCacheEntry(
            m_strSource,
            m_absoluteUri,
            OfTypeByIndex<KnownTypeIndex::SvgImageSource>(),
            imageGetOptions,
            reinterpret_cast<uint64_t>(this),
            &cacheHit,
            imageCache.ReleaseAndGetAddressOf()));
    }

    IFC(SetImageCache(std::move(imageCache)));

    IFC(RegisterWithReloadManager());

Cleanup:
    if (fPegged)
    {
        UnpegManagedPeer();
    }

    ReleaseInterface(pImageSurfaceSystemMemory);
    ReleaseInterface(pImageProvider);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//     The decoded bits have been invalidated, so redecode the compressed
//     stream to a new set of decoded bits.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::RedecodeEncodedImage(
    bool lostSurface
    )
{
    //
    // A new download would clear out the existing encoded image. But since there's no
    // download, we should always have an encoded image.
    //
    FAIL_FAST_ASSERT(IsMetadataAvailable());

    //TRACE(TraceAlways, L"Reloading released software surface.");

    m_pendingDecodeForLostSoftwareSurface = lostSurface;

    if (!m_fDecodeToRenderSize)
    {
        auto spDecodeCallback = make_xref<ImageAvailableCallback<CImageSource>>(this, &CImageSource::OnStreamImageAvailable);

        IFC_RETURN(DecodeStream(spDecodeCallback, false /* synchronousDecode */, true /* retainPlaybackState */));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Issues an asynchronous WIC query of the encoded images dimensions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::UpdateDecodeSize()
{
    // Refresh the DecodeToRenderSize state
    m_fDecodeToRenderSize = ShouldDecodeToRenderSize();

    if (!m_fDecodeToRenderSize && IsMetadataAvailable())
    {
        if (m_bitmapState == BitmapImageState::DecodePending)
        {
            SetBitmapState(BitmapImageState::Decoding);
            IFC_RETURN(RedecodeEncodedImage(FALSE));
        }
        else
        {
            IFC_RETURN(RedecodeEncodedImage(TRUE));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This image was previously only used by the primitive compositor
//      so we released the software surface after copying it to hardware.
//      Now that something needs the software surface, so reload it by
//      decoding the encoded image again. Also mark the surface as always
//      needing to keep the system memory copy.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::ReloadReleasedSoftwareImage()
{
    // Must set this before decoding the image so it doesn't use the
    // background thread image loading code path which will decode
    // directly to a hardware surface.
    m_pImageSurfaceWrapper->SetKeepSystemMemory();

    if (!IsMetadataAvailable())
    {
        // It is possible to get into a state with a missing encoded image data if a previous
        // decode operation was aborted (which could happen if a URI was set multiple times quickly).
        // This should only happen in the case that a URI was set because m_pEncodedImage is set
        // immediately in the case of a stream.  This means that it is possible to do a full image
        // reload which will fetch the encoded image.  ReloadImage will raise a catastrophic error
        // if there is URI.
        IFC_RETURN(ReloadImage(true /*uriChanged*/, false /*retainPlaybackState*/));
    }
    else if ((m_bitmapState == BitmapImageState::Decoded)||
             (m_bitmapState == BitmapImageState::HasEncodedImageOnly))
    {
        // If the surface must keep the system memory surface then it should
        // have never been released. The only exception is when the memory
        // surface was released before it was made mandatory and we are
        // still in the process of reloading it asynchronously.
        // TODO: Disabled this assertion for 1506 to unblock legacy test execution.  This is
        //       tracked by TFS bug 1438910 and should be fixed for 1510 release.
        //ASSERT(!m_pImageSurfaceWrapper->MustKeepSystemMemory() || (m_spAbortableImageOperation != NULL));

        IFC_RETURN(RedecodeEncodedImage(TRUE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Call the event manager to fire ImageFailed event.
//      If no event handler was added, the error is then reported to
//      the global OnError handler in plugin control level.
//
//------------------------------------------------------------------------
HRESULT
CImageSource::FireImageFailed(_In_ XUINT32 iErrorCode)
{
    auto core = GetContext();
    ResetSurfaces(!!m_pImageSurfaceWrapper->MustKeepSystemMemory(), false);

    CEventManager *pEventManager = core->GetEventManager();

    //
    // Check if events should be raised through the event system to managed/script.
    //
    if (ShouldFirePublicBitmapImageEvents())
    {
        // If the event handler is not registered, it will fall back to OnError handler
        // so there is no need to do the m_pEventList check here.
        if (pEventManager)
        {
            if (OfTypeByIndex<KnownTypeIndex::BitmapImage>())
            {
                auto errArgs = make_xref<CErrorEventArgs>(core);

                errArgs->m_eType = ImageError;
                errArgs->m_iErrorCode = iErrorCode;

                IGNOREHR(errArgs->UpdateErrorMessage( TRUE ));

                // Raise the event but prevent error fallback. If this is a critical failure, an image in the tree failed
                // to load the image will raise the event to the generic error handler.
                pEventManager->Raise(
                    EventHandle(KnownEventIndex::BitmapImage_ImageFailed),
                    true,
                    this,
                    errArgs
                    );
            }
            else
            {
                ASSERT(OfTypeByIndex<KnownTypeIndex::SvgImageSource>());
                auto svgImageSourceFailedEventArgs = make_xref<CSvgImageSourceFailedEventArgs>();
                DirectUI::SvgImageSourceLoadStatus status;
                switch(iErrorCode)
                {
                    case AG_E_NETWORK_ERROR:
                    case INET_E_RESOURCE_NOT_FOUND:
                    {
                        status = DirectUI::SvgImageSourceLoadStatus::NetworkError;
                        break;
                    }
                    case E_INVALIDARG:
                    {
                        status = DirectUI::SvgImageSourceLoadStatus::InvalidFormat;
                        break;
                    }
                    default:
                    {
                        status = DirectUI::SvgImageSourceLoadStatus::Other;
                        break;
                    }
                }

                IFC_RETURN(svgImageSourceFailedEventArgs->put_Status(status));
                pEventManager->Raise(
                    EventHandle(KnownEventIndex::SvgImageSource_OpenFailed),
                    true,
                    this,
                    svgImageSourceFailedEventArgs
                    );
            }
        }
    }

    SetBitmapState(BitmapImageState::Failed);

    // If we have callbacks, this means that the ImageBrushes hosting us want to know when something has failed.
    // When something has failed, BitmapImage needs to notify all ImageBrushes that are pointing to us (multiple
    // ImageBrushes can share the same BitmapImage. Now, when we call back into ImageBrush, ImageBrush can decide
    // whether or not is being hosted in an Image. If it is, it will notify the Image it is being hosted in,
    // that something has failed, otherwise, it will fire its own failed event. So, here, we need to walk our list
    // of callbacks and fire the callbacks to notify of a failure.
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportCallbacks.begin(); it != m_imageReportCallbacks.end(); ++it)
    {
        IFC_RETURN((*it)->FireImageFailed(iErrorCode));
    }

    IFC_RETURN(MarkHandledCallbackEvents());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raise the image download event to listeners.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageSource::FirePublicDownloadEvent()
{
    HRESULT hr = S_OK;
    // Get an event handle ...
    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hEvent(KnownEventIndex::BitmapImage_DownloadProgress);
    CDownloadProgressEventArgs *pDownloadArgs = NULL;

    //
    // Check if events should be raised through the event system to managed/script.
    //
    if (ShouldFirePublicBitmapImageEvents() && !m_publicBitmapEventFired)
    {
        if (pEventManager)
        {
            //
            // Generate the ErrorEventArgs based on the error code.
            //
            pDownloadArgs = new CDownloadProgressEventArgs();

            pDownloadArgs->m_downloadProgress = m_downloadProgress;

            // ... And fire it!
            pEventManager->Raise(hEvent, TRUE, this, pDownloadArgs);
        }
    }

    IFC(FireInternalDownloadEvent());

Cleanup:
    ReleaseInterface(pDownloadArgs);

    RRETURN(hr);
}

_Check_return_ HRESULT CImageSource::FireInternalDownloadEvent()
{
    // If we have callbacks, this means that the ImageBrushes hosting us want to know when DownloadProgress has changed.
    // When DownloadProgress has changed, BitmapImage needs to notify all ImageBrushes that are pointing to it (multiple
    // ImageBrushes can share the same BitmapImage. Now, when we call back into ImageBrush, ImageBrush can decide
    // whether or not is being hosted in an Image. If it is, it will notify the Image it is being hosted in,
    // that DownloadProgress has changed, otherwise, it will fire its own DownloadProgressChanged event. So, here,
    // we need to walk our list of callbacks and fire the callbacks to notify of a DownloadProgress change.
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportCallbacks.begin(); it != m_imageReportCallbacks.end(); ++it)
    {
        IFC_RETURN((*it)->FireDownloadProgress(m_downloadProgress));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raise the image opened event to listeners.
//
//------------------------------------------------------------------------
HRESULT
CImageSource::FireImageOpened()
{
    CEventManager *pEventManager = GetContext()->GetEventManager();

    //
    // Check if events should be raised through the event system to managed/script.
    //
    if (ShouldFirePublicBitmapImageEvents() && !m_publicBitmapEventFired)
    {
        if (!OfTypeByIndex<KnownTypeIndex::SvgImageSource>())
        {
            auto eventArgs = make_xref<CRoutedEventArgs>();

            // Raise the event
            pEventManager->Raise(
                EventHandle(KnownEventIndex::BitmapImage_ImageOpened),
                true,
                this,
                eventArgs
                );
        }
        else
        {
            auto openedEventArgs = make_xref<CSvgImageSourceOpenedEventArgs>();
            pEventManager->Raise(
                EventHandle(KnownEventIndex::SvgImageSource_Opened),
                true,
                this,
                openedEventArgs
                );
        }
    }

    SetBitmapState(BitmapImageState::Decoded);

    //
    // Notify all listening callbacks that the image is opened so they can bubble the event notification.
    //
    for (xvector< xref_ptr<CBitmapImageReportCallback> >::iterator it = m_imageReportCallbacks.begin(); it != m_imageReportCallbacks.end(); ++it)
    {
        IFC_RETURN((*it)->FireImageOpened());
    }

    IFC_RETURN(MarkHandledCallbackEvents());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IPALUri* preferredBaseURI = GetPreferredBaseUri();

    // If we don't have a preferred URI, use the namescope owner's
    if (!preferredBaseURI && pNamescopeOwner)
    {
        xref_ptr<IPALUri> baseURI;
        baseURI.attach(pNamescopeOwner->GetBaseUri());
        // GetBaseUri add-refs, but it gets dropped in this scope since CDOSharedState manages lifetime of URI object
        // and in turn this CDO participates in that CDOSharedState's lifetime.
        SetBaseUri(baseURI);
        m_absoluteUri.reset();
    }

    IFC_RETURN(CMultiParentShareableDependencyObject::EnterImpl(pNamescopeOwner, params));
    if (params.fIsLive)
    {
        UnregisterForCleanupOnEnter();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Remove callback from list.
//
//------------------------------------------------------------------------
void CImageSource::RemoveCallback(
    _In_ CBitmapImageReportCallback* pImageErrorCallback
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    ASSERT(pImageErrorCallback != NULL);

    //
    // Remove the callback from the image report list.
    //
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportCallbacks.begin(); it != m_imageReportCallbacks.end(); ++it)
    {
        if ((*it) == pImageErrorCallback)
        {
            IFC(m_imageReportCallbacks.erase(it));

            break;
        }
    }

    //
    // Remove the callback from the already reported callback list.
    //
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportAlreadyReportedCallbacks.begin(); it != m_imageReportAlreadyReportedCallbacks.end(); ++it)
    {
        if ((*it) == pImageErrorCallback)
        {
            IFC(m_imageReportAlreadyReportedCallbacks.erase(it));

            break;
        }
    }

Cleanup:
    ;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark all callbacks as requiring image report events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::ResetCallbackEvents(
    )
{
    //
    // Mark all callbacks as not yet reported.
    //
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportAlreadyReportedCallbacks.begin(); it != m_imageReportAlreadyReportedCallbacks.end(); ++it)
    {
        IFC_RETURN(m_imageReportCallbacks.push_back((*it)));
    }

    m_imageReportAlreadyReportedCallbacks.clear();

    m_publicBitmapEventFired = false;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark all callbacks as no longer requiring image report events.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::MarkHandledCallbackEvents(
    )
{
    //
    // Mark all callbacks as reported.
    //
    for (xvector< xref_ptr< CBitmapImageReportCallback > >::iterator it = m_imageReportCallbacks.begin(); it != m_imageReportCallbacks.end(); ++it)
    {
        IFC_RETURN(m_imageReportAlreadyReportedCallbacks.push_back((*it)));
    }

    m_imageReportCallbacks.clear();

    m_publicBitmapEventFired = true;

    return S_OK;
}

_Check_return_ HRESULT
CImageSource::OnDownloadImageAvailable(
    _In_ IImageAvailableResponse* pResponse
    )
{
    IFC_RETURN(OnDownloadImageAvailableImpl(pResponse));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for when a image download/decode has finished
//      for a uri source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::OnDownloadImageAvailableImpl(
    _In_ IImageAvailableResponse* pResponse
    )
{
    return OnImageAvailableCommon(pResponse);
}

// Special version of GetCacheIdentifier used to get a cache identifier
// given a size.  This is to avoid a chicken and the egg issue with
// Background Thread Image Loading which requires the cache identifier to
// check for a hardware cached resources before generating a m_spPendingDecodeParams.
// However the default version of GetCacheIdentifier uses the m_spPendingDecodeParams to build
// the cache identifier.  Luckily we know the explicit size that will be used to
// build the m_spPendingDecodeParams.
_Check_return_ HRESULT
CImageSource::GetCacheIdentifier(
    unsigned long width,
    unsigned long height,
    PixelFormat pixelFormat,
    _Out_ xstring_ptr* pCacheIdentifier
    )
{
    pCacheIdentifier->Reset();

    if (!m_strSource.IsNull() && IsMetadataAvailable() && !m_imageMetadataView->GetImageMetadata()->IsAnimatedImage())
    {
        IFC_RETURN(EnsureAbsoluteUri());
        FAIL_FAST_ASSERT(m_absoluteUri != nullptr);

        bool shouldIncludeInvalidationId;
        xref_ptr<IPALResourceManager> spResourceManager;
        IFC_RETURN(GetContext()->GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
        IFC_RETURN(spResourceManager->CanResourceBeInvalidated(m_absoluteUri, &shouldIncludeInvalidationId));

        xstring_ptr strCanonicalUri;
        IFC_RETURN(m_absoluteUri->GetCanonical(&strCanonicalUri));

        IFC_RETURN(GenerateCacheIdentifier(
            strCanonicalUri,
            width,
            height,
            pixelFormat,
            shouldIncludeInvalidationId,
            shouldIncludeInvalidationId ? spResourceManager->GetResourceInvalidationId() : 0,
            pCacheIdentifier));
    }

    return S_OK;
}

_Check_return_ HRESULT
CImageSource::GetCacheIdentifier(
    _In_ const ImageDecodeParams& imageDecodeParams,
    _Out_ xstring_ptr* cacheIdentifier
    )
{
    if (IsMetadataAvailable() && m_imageMetadataView->GetImageMetadata()->IsAnimatedImage())
    {
        cacheIdentifier->Reset();
    }
    else
    {
        if (m_strCacheIdentifier.IsNull() && !m_strSource.IsNull())
        {
            XUINT32 decodeWidthInPhysicalPixels = 0;
            XUINT32 decodeHeightInPhysicalPixels = 0;

            GetDecodePixelWidthAndHeightInPhysicalPixels(
                GetDecodeType(),
                GetDecodeWidth(),
                GetDecodeHeight(),
                &decodeWidthInPhysicalPixels,
                &decodeHeightInPhysicalPixels);

            if ((decodeWidthInPhysicalPixels == 0) &&
                (decodeHeightInPhysicalPixels == 0))
            {
                if ((imageDecodeParams.GetDecodeWidth() != 0) ||
                    (imageDecodeParams.GetDecodeHeight() != 0))
                {
                    decodeWidthInPhysicalPixels = imageDecodeParams.GetDecodeWidth();
                    decodeHeightInPhysicalPixels = imageDecodeParams.GetDecodeHeight();
                }
                else if (HasDimensions())
                {
                    decodeWidthInPhysicalPixels = GetWidth();
                    decodeHeightInPhysicalPixels = GetHeight();
                }
            }

            IFC_RETURN(GetCacheIdentifier(
                decodeWidthInPhysicalPixels,
                decodeHeightInPhysicalPixels,
                imageDecodeParams.GetFormat(),
                &m_strCacheIdentifier));
        }

        *cacheIdentifier = m_strCacheIdentifier;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reset the cache identifier associated with this image.
//
//------------------------------------------------------------------------
void
CImageSource::ResetCacheIdentifier(
    )
{
    m_strCacheIdentifier.Reset();
}

// Returns the 'physical' size of the underlying image surface, for use when rendering.
XUINT32 CImageSource::GetWidth() const
{
    if (m_pImageSurfaceWrapper->HasAnySurface() || m_pImageSurfaceWrapper->HasRetainedSize())
    {
        return m_pImageSurfaceWrapper->GetWidth();
    }
    else if (m_spAbortableImageOperation != nullptr)
    {
        return m_spAbortableImageOperation->GetDecodedSize().Width;
    }
    else
    {
        return 0;
    }
}

// Returns the 'physical' size of the underlying image surface, for use when rendering.
XUINT32 CImageSource::GetHeight() const
{
    if (m_pImageSurfaceWrapper->HasAnySurface() || m_pImageSurfaceWrapper->HasRetainedSize())
    {
        return m_pImageSurfaceWrapper->GetHeight();
    }
    else if (m_spAbortableImageOperation != nullptr)
    {
        return m_spAbortableImageOperation->GetDecodedSize().Height;
    }
    else
    {
        return 0;
    }
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the width to be used for image decoding operations.
//
//------------------------------------------------------------------------
XUINT32
CImageSource::GetDecodeWidth(
    )
{
    return MAX(m_decodePixelWidth, 0);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the height to be used for image decoding operations.
//
//------------------------------------------------------------------------
XUINT32
CImageSource::GetDecodeHeight(
    )
{
    return MAX(m_decodePixelHeight, 0);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the decode type to be used for image decoding operations.
//
//------------------------------------------------------------------------
DecodePixelType
CImageSource::GetDecodeType(
    ) const
{
    return m_decodePixelType;
}

uint32_t CImageSource::GetPhysicalWidth() const
{
    if (m_fDecodeToRenderSize)
    {
        // If decode to render size is enabled, it means that m_decodePixelWidth and
        // m_decodePixelHeight are 0, so the size can be calculated from GetPixelWidth()
        return GetPixelWidth();
    }
    else
    {
        // if it is not enabled, it means that m_decodePixelWidth could be set or
        // the feature is disabled in which case the image decoded size can be returned
        // with GetWidth()
        return GetWidth();
    }
}

uint32_t CImageSource::GetPhysicalHeight() const
{
    if (m_fDecodeToRenderSize)
    {
        // If decode to render size is enabled, it means that m_decodePixelWidth and
        // m_decodePixelHeight are 0, so the size can be calculated from GetPixelHeight()
        return GetPixelHeight();
    }
    else
    {
        // if it is not enabled, it means that m_decodePixelWidth could be set or
        // the feature is disabled in which case the image decoded size can be returned
        // with GetHeight()
        return GetHeight();
    }
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to prevent double-scaling from logical decode sizing.
//
//------------------------------------------------------------------------
XUINT32
CImageSource::GetLayoutWidth()
{
    return GetLogicalSize(GetPhysicalWidth());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to prevent double-scaling from logical decode sizing
//
//------------------------------------------------------------------------
XUINT32
CImageSource::GetLayoutHeight()
{
    return GetLogicalSize(GetPhysicalHeight());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to calculate the logical size for images with a logical decode size.
//
//------------------------------------------------------------------------
XUINT32
CImageSource::GetLogicalSize(XUINT32 physicalSize) const
{
    XUINT32 logicalSize;
    if (HasLogicalDecodeSize())
    {
        // Warning: This may not work correctly with AppWindows
        //     Task 19490172: CImageSource::RegisterWithReloadManager must be AppWindow aware
        const auto contentRootCoordinator = GetContext()->GetContentRootCoordinator();
        const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        const auto scaleFactor = RootScale::GetRasterizationScaleForContentRoot(root);

        // Return the logical size at plateau 1.0 if the image has a logical decode size.
        logicalSize = XcpRound(static_cast<XFLOAT>(physicalSize) / scaleFactor);
    }
    else if (IsMetadataAvailable() && m_imageMetadataView->GetImageMetadata()->scalePercentage != 0)
    {
        // If the image has an associated scale, its logical size is the physical size
        // normalized to 100% scale.

        const XFLOAT scaleFactor = static_cast<XFLOAT>(m_imageMetadataView->GetImageMetadata()->scalePercentage) / 100.0f;

        logicalSize = XcpRound(static_cast<XFLOAT>(physicalSize) / scaleFactor);
    }
    else
    {
        // Otherwise, return the physical size of the underlying surface.

        logicalSize = physicalSize;
    }

    return logicalSize;
}

bool
CImageSource::HasLogicalDecodeSize() const
{
    // If the image has a logical decode size, it will be decoded at a scaled-up size to ensure it appears at higher
    // quality at a higher plateau. If the image is being layed out to its natural size (e.g. Stretch="None"), it's
    // important that it doesn't use this scaled up size again. Logical bounds need to remain the same at different
    // plateaus (the scaling is applied optically via a RenderTransform). If the logical bounds were also scaled, then
    // the optical scale would enlarge the image a second time and cause it to appear too large, relative to other
    // UIElements.
    return DirectUI::DecodePixelType::Logical == m_decodePixelType
        && (m_decodePixelHeight || m_decodePixelWidth);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reset state when a new source is set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::ResetForSourceChange(
    bool mustKeepSoftwareSurface,
    bool mustKeepHardwareSurfaces,
    bool keepEncodedData
    )
{
    HRESULT hr = S_OK;

    TraceImageResetForSourceChangeBegin(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    ResetCacheIdentifier();

    ResetSurfaces(mustKeepSoftwareSurface, mustKeepHardwareSurfaces);

    IFC(SetDirty());

    if (!keepEncodedData)
    {
        IFCFAILFAST(SetImageCache(nullptr));
        SetBitmapState(BitmapImageState::Initial);
    }

Cleanup:

    TraceImageResetForSourceChangeEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determine whether or not to make an image decode request.
//
//------------------------------------------------------------------------
bool
CImageSource::ShouldRequestDecode()
{
    if (!IsMetadataAvailable())
    {
        return false;
    }

    // Notes:  Prior to RS3, we would not allow decode requests while a transform animation is running, in
    // order to guard against a performance issue that would result if an element is animating its scale larger
    // (this would result in decoding the image over and over again at larger and larger size).
    // This check is no longer necessary, due to the logic in HWWalk::EnsureCompositionPeer(), which guarantees
    // that the TransformToRoot is "locked" to a stable, unchanging transform while the element is being animated.
    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Enqueue the request to decode at a specified size
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::QueueRequestDecode(
    XUINT32 width,
    XUINT32 height,
    bool retainPlaybackState
    )
{
    IFC_RETURN(GetContext()->EnqueueImageDecodeRequest(this, width, height, retainPlaybackState));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Request decoding of an image with a different width/height for
//      storage.  This is used in the deferred decoding case to decode
//      to render size during the render walk.  This method guarantees
//      the source will be decoded to a size greater than or equal to
//      the requested size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::RequestDecode(
    XUINT32 requestWidth,
    XUINT32 requestHeight,
    bool retainPlaybackState
    )
{
    if (IsMetadataAvailable() && m_fDecodeToRenderSize)
    {
        const char *imageState = "";
        switch (m_bitmapState)
        {
        case BitmapImageState::Initial:
            imageState = "Initial";
            break;
        case BitmapImageState::DecodePending:
            imageState = "DecodePending";
            break;
        case BitmapImageState::Decoding:
            imageState = "Decoding";
            break;
        case BitmapImageState::Decoded:
            imageState = "Decoded";
            break;
        case BitmapImageState::Downloading:
            imageState = "Downloading";
            break;
        case BitmapImageState::HasEncodedImageOnly:
            imageState = "HasEncodedImageOnly";
            break;
        case BitmapImageState::Failed:
            imageState = "Failed";
            break;
        default:
            ASSERT(false);
        }

        ImagingTelemetry::RequestDecodeToRenderSize(
            reinterpret_cast<uint64_t>(this),
            imageState,
            requestWidth,
            requestHeight,
            m_fDecodeToRenderSize);

        IFC_RETURN(DecodeToRenderSize(requestWidth, requestHeight, retainPlaybackState));
    }

    return S_OK;
}

_Check_return_ HRESULT CImageSource::OnImageViewUpdated(ImageViewBase& sender)
{
    ASSERT(m_imageMetadataView.get() == &sender);

    ImagingTelemetry::ImageDownloadCompleteNotification(reinterpret_cast<uint64_t>(this), m_strSource.GetBuffer(), m_fDecodeToRenderSize);

    if (ShouldContinueAsyncAction())
    {
        xref_ptr<CImageSource> strongThis(this);

        // Unintuitive behavior: this GetHR call kicks off the initial parse of the encoded image to read the metadata.
        // Move this out to an explicit "parse for metadata" call?
        HRESULT parseHR = m_imageMetadataView->GetHR(reinterpret_cast<uint64_t>(this));

        if (FAILED(parseHR))
        {
            auto asyncCompleteExit = wil::scope_exit([=]
            {
                CompleteAsyncAction(parseHR);
            });
            bool isSvg = OfTypeByIndex<KnownTypeIndex::SvgImageSource>();
            IFC_RETURN(FireImageFailed(isSvg ? parseHR : AG_E_NETWORK_ERROR));

            //  If we fail we still want to dirty ourselves so that we will
            //  re-render the empty image.
            IFC_RETURN(RequestMeasure());
        }
        else
        {
            auto imageMetadata = m_imageMetadataView->GetImageMetadata();

            float downloadProgress = m_imageMetadataView->GetDownloadProgress();
            if (downloadProgress > m_downloadProgress)
            {
                m_downloadProgress = downloadProgress;
                IFC_RETURN(FirePublicDownloadEvent());
            }

            // Note: We check that imageMetadata exists even though we don't use it anywhere inside this if statement.
            // This is because we'll be requesting a layout lass (RequestMeasure), which can require the native size of
            // the image, which requires image metadata. In that case we'll read the metadata off of m_imageMetadataView
            // again.
            if (imageMetadata != nullptr)
            {
                IFC_RETURN(RequestMeasure());

                if (m_fDecodeToRenderSize)
                {
                    IFC_RETURN(GetContext()->StartTrackingImageForRenderWalk(this));
                    SetBitmapState(BitmapImageState::DecodePending);
                }
                else
                {
                    m_pendingDecodeForLostSoftwareSurface = FALSE;

                    auto imageLoadCallback = make_xref<ImageAvailableCallback<CImageSource>>(this, &CImageSource::OnStreamImageAvailable);
                    bool syncDecode = (m_pAsyncAction == nullptr) && m_strSource.IsNull();
                    IFC_RETURN(DecodeStream(imageLoadCallback, syncDecode, false /* retainPlaybackState */));
                }

                IFC_RETURN(RegisterWithReloadManager());
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This method does all the logic to determine if we should actually
//      perform a decode, and adjusts the size to ensure the source
//      will be decoded to a size greater than or equal to the requested size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::DecodeToRenderSize(
    XUINT32 requestWidth,
    XUINT32 requestHeight,
    bool retainPlaybackState
    )
{
    const XUINT32 imageSourceWidth = GetWidth();
    const XUINT32 imageSourceHeight = GetHeight();
    XUINT32 width = requestWidth;
    XUINT32 height = requestHeight;

    ASSERT(IsMetadataAvailable() && m_fDecodeToRenderSize);
    auto imageMetadata = m_imageMetadataView->GetImageMetadata();

    // Clamp to the layout size which is the original size of the image.
    // Width/Height == 0 indicates full width/height from the render walk, so adjust it to the natural width/height
    if ((width == 0) ||
        ((width > imageMetadata->width) &&
        (imageMetadata->width != 0)))
    {
        width = imageMetadata->width;
        height = imageMetadata->height;
    }

    if ((height == 0) ||
        ((height > imageMetadata->height) &&
        (imageMetadata->height != 0)))
    {
        width = imageMetadata->width;
        height = imageMetadata->height;
    }

    // Only decode in the following circumstances:
    // - If a surface doesn't exist and has no decode operation to produce it.
    // - If the image was previously decoded to a hardware surface, the image must be re-decoded to the software surface for
    //   the software rendering code path;
    // - If the surface needs to be updated due to a new size.
    // Also, make sure we don't already have a decode request in progress that
    // is big enough to service this request, to avoid cancelling/restarting unnecessarily.
    const bool missingImage = m_forceDecodeRequest ||
        (!m_pImageSurfaceWrapper->HasAnySurface() &&
        (!m_spAbortableImageOperation || !m_spAbortableImageOperation->IsDecodeInProgress()));

    const bool softwareDecode =
        !m_pImageSurfaceWrapper->HasSoftwareSurface() &&
        m_pImageSurfaceWrapper->MustKeepSystemMemory();

    const uint32_t pendingWidth = m_spPendingDecodeParams ? m_spPendingDecodeParams->GetDecodeWidth() : imageSourceWidth;
    const uint32_t pendingHeight = m_spPendingDecodeParams ? m_spPendingDecodeParams->GetDecodeHeight() : imageSourceHeight;

    // Svg images should always be resized to have crisp quality whenever a new
    // size is requested. Non-Svg images decode to a larger size to have
    // maximum quality while also limiting the number of times the image is decoded.
    const bool isSvg = OfTypeByIndex<KnownTypeIndex::SvgImageSource>();
    bool sizeChange = false;
    if (isSvg)
    {
        sizeChange =
            (width != pendingWidth) ||
            (height != pendingHeight);
    }
    else
    {
        sizeChange =
            (width > std::max(imageSourceWidth, pendingWidth)) ||
            (height > std::max(imageSourceHeight, pendingHeight));
    }

    if (missingImage ||
        softwareDecode ||
        (m_pImageSurfaceWrapper->HasAnySurface() && sizeChange))
    {
        uint32_t newWidth = width;
        uint32_t newHeight = height;
        if (!isSvg)
        {
            newWidth = std::max(width, imageSourceWidth);
            newHeight = std::max(height, imageSourceHeight);
        }

        if (m_fDecodeToRenderSize &&
            (m_bitmapState == BitmapImageState::DecodePending))
        {
            SetBitmapState(BitmapImageState::Decoding);
        }

        m_pendingDecodeForLostSoftwareSurface = FALSE;

        ImagingTelemetry::DecodeToRenderSizeStart(
            reinterpret_cast<uint64_t>(this),
            imageSourceWidth,
            imageSourceHeight,
            newWidth,
            newHeight,
            imageMetadata->width,
            imageMetadata->height);

        // Keep this old event for now. There are tests listening for it.
        TraceDecodeToRenderSizeBegin1(
            reinterpret_cast<XUINT64>(this),
            width,
            height,
            imageSourceWidth,
            imageSourceHeight,
            newWidth,
            newHeight,
            imageMetadata->width,
            imageMetadata->height,
            GetLayoutWidth(),
            GetLayoutHeight());

        // Windows Blue Bug #571750
        // When using SetSourceAsync + DecodeToRenderSize, we were using the OnDownloadImageAvailable callback
        // which was designed only for completing URI-based image decodes, and does not perform important
        // actions needed for stream based decodes, most notably, signaling completion of the IAsyncAction.
        // The lack of completion of the IAsyncAction caused the app to wait forever to complete.
        // The fix is to use the OnStreamImageAvailable callback when the app has used SetSourceAsync for this image.
        // Note that SetSource disables DecodeToRenderSize so it's not possible to get here in the SetSource case.
        xref_ptr<IImageAvailableCallback> spDecodeCallback;
        if (m_pAsyncAction != NULL)
        {
            spDecodeCallback = make_xref<ImageAvailableCallback<CImageSource>>(this, &CBitmapSource::OnStreamImageAvailable);
        }
        else
        {
            spDecodeCallback = make_xref<ImageAvailableCallback<CImageSource>>(this, &CImageSource::OnDownloadImageAvailable);
        }

        // Decode to the rendered size
        // Software decode must be forced so that it updates the cache with a new entry that has a software surface instead of
        // just a hardware surface.
        IFC_RETURN(DecodeStreamWithSize(
            spDecodeCallback,
            newWidth,
            newHeight,
            FALSE /* synchronousDecode */,
            softwareDecode /* forceDecode */,
            retainPlaybackState,
            true /* decodeToRenderSize */));

        TraceDecodeToRenderSizeEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

        ImagingTelemetry::DecodeToRenderSizeStop(reinterpret_cast<uint64_t>(this));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basically we are going to create a dedicated texture if we can, or if too big, we
//   will tile the image.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::EnsureAndUpdateHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ SurfaceCache *pSurfaceCache
    )
{
    if (m_pImageSurfaceWrapper->IsUpdateRequired())
    {
        // If we don't have a hardware surface yet, but we do have a software surface
        // we need to create a backing texture.  If this fits in a regular atlas, create it there
        // if not try to create a dedicated texture.  If that doesn't work tile.
        // In the case of background image loading, the hardware surface may have been created and decoded
        // to, but it may not have been added to the cache which will happen below.
        xstring_ptr cacheIdentifier;
        if (m_spPendingDecodeParams != nullptr)
        {
            IFC_RETURN(GetCacheIdentifier(*m_spPendingDecodeParams, &cacheIdentifier));
        }

        xref_ptr<ImageHardwareResources> resources;

        // See if someone else already cached this image.
        if (!cacheIdentifier.IsNull())
        {
            IFC_RETURN(pSurfaceCache->GetResources(cacheIdentifier, resources.ReleaseAndGetAddressOf()));

            if (resources != nullptr && resources->IsDiscarded())
            {
                IFC_RETURN(pSurfaceCache->Remove(cacheIdentifier));
                resources.reset();
            }
        }

        if (resources == nullptr)
        {
            // If the HW surface is found in cache it's either good or is going to be good soon when BTIL completes.
            // When resources == nullptr the surface is not good and should never be cached until updated from SW.
            if (m_pImageSurfaceWrapper->HasSoftwareSurface())
            {
                IFC_RETURN(m_pImageSurfaceWrapper->EnsureHardwareResources(pTextureManager, resources.ReleaseAndGetAddressOf()));

                TraceImageCopyToVideoMemoryBegin(reinterpret_cast<XUINT64>(this));

                IFC_RETURN(m_pImageSurfaceWrapper->UpdateSurfaceFromSoftware(pTextureManager));

                TraceImageCopyToVideoMemoryEnd(reinterpret_cast<XUINT64>(this));

                //TRACE(TraceAlways, L"Replacing surface cache image : Identifier = %s",
                //    !cacheIdentifier.IsNull() ? cacheIdentifier.GetBuffer() : L"NULL");
            }

            // Add the bitmap to the video memory cache manager.  It will get deleted from the original
            // cache manager once all references are gone to the original texture.
            if (!cacheIdentifier.IsNull() &&
                (resources != nullptr) &&
                !m_pImageSurfaceWrapper->IsHardwareLocked())
            {
                CNotifyOnDelete *notifyOnDeleteNoRef = nullptr;
                IFC_RETURN(resources->GetNotifyOnDelete(&notifyOnDeleteNoRef));

                const xstring_ptr &cacheIdentifierCurrent = notifyOnDeleteNoRef->GetNotifyOnDeleteToken();

                // When re-decoding for the bigger render size we keep the previous surface to avoid flickering.
                // cacheIdentifier is calculated for the new size. m_pImageSurfaceWrapper may still hold the
                // old size surface which is also being in the cache but under the different key.
                // To avoid the same resource being cached twice only add it to the cache when keys match.
                if (cacheIdentifierCurrent == cacheIdentifier)
                {
                    IFC_RETURN(pSurfaceCache->AddResources(cacheIdentifier, resources.get()));
                }
                else
                {
                    // Special case is when only one dimension is specified in decode params the cacheIdentifier
                    // will look like "width:[ 42 ] height:[ 0 ]" while cacheIdentifierCurrent will be fully specialized
                    // like "width:[ 42 ] height:[ 42 ]". In this rare case do a more expensive lookup to check if
                    // the resource still needs to be added
                    xref_ptr<ImageHardwareResources> spResourcesCurrent;
                    IFC_RETURN(pSurfaceCache->GetResources(cacheIdentifierCurrent, spResourcesCurrent.ReleaseAndGetAddressOf()));
                    if (spResourcesCurrent == nullptr)
                    {
                        IFC_RETURN(pSurfaceCache->AddResources(cacheIdentifier, resources.get()));
                    }
                }
            }
        }
        else
        {
            m_pImageSurfaceWrapper->UseCachedHardwareResources(resources.get());
        }

        // Make sure that any updates to the surface get updated for DComp
        // This method checks if an update is necessary and will queue the update if it is
        TraceImageUpdateHardwareResourcesBegin(reinterpret_cast<XUINT64>(this));

        m_pImageSurfaceWrapper->UpdateHardwareResources();

        TraceImageUpdateHardwareResourcesEnd(reinterpret_cast<XUINT64>(this));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether managed events should be fired for handlers
//      attached to this bitmap image.
//
//      Since image brushes can be attached to bitmap images whenever,
//      events are fired to image brushes regardless of state. However,
//      once fired, the image brush gets moved to the "already fired"
//      list and won't receive any more events until the bitmap image's
//      source changes.
//
//------------------------------------------------------------------------
bool
CImageSource::ShouldFirePublicBitmapImageEvents()
{
    return m_bitmapState == BitmapImageState::Downloading ||
        m_bitmapState == BitmapImageState::Decoding ||
        m_bitmapState == BitmapImageState::DecodePending;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Force the DecodeToRenderSize feature to not be used for this element.
//
//------------------------------------------------------------------------
void
CImageSource::SetDecodeToRenderSizeForcedOff()
{
    // Calling this method is not guaranteed to disable the feature if
    // we've already made decode requests or have a surface.
    // These asserts are here to help catch future uses of this method
    // in unsafe-to-use situations.  Use with care!
    ASSERT(!m_pImageSurfaceWrapper->HasAnySurface());
    ASSERT(m_bitmapState == BitmapImageState::Initial);

    m_fDecodeToRenderSizeForcedOff = TRUE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override for BitmapImage to track internal state and setup the
//      internal state for decoding in common cases like SetSource or
//      Setting the source from a URI.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::PrepareDecode(bool retainPlaybackState)
{
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Decodes an image from memory using size information provided.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::DecodeStreamWithSize(
    _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
    XUINT32 decodeWidth,
    XUINT32 decodeHeight,
    bool synchronousDecode,
    bool forceDecode,
    bool retainPlaybackState,
    bool decodeToRenderSize
    )
{
    bool surfaceCacheHit = false;
    auto core = GetContext();

    ASSERT(m_imageCache != nullptr);

    xref_ptr<ImageProvider> imageProvider(core->GetImageProvider());

    const PixelFormat pixelFormat =
        (core->GetHostSite()->IsHdrOutput() && m_imageMetadataView->GetImageMetadata()->isHdr) ?
            pixelColor64bpp_R16G16B16A16_Float : pixelColor32bpp_A8R8G8B8;

    // For stability and appcompat of DecodeToRenderSize, forceDecode will only be FALSE if we're using DecodeToRenderSize,
    // all other (pre-DecodeToRenderSize) callers will use the old code path which doesn't use the cache and always decodes.
    if (forceDecode)
    {
        xref_ptr<ImageDecodeParams> decodeParams;
        IFC_RETURN(PrepareDecodeParams(
            decodeWidth,
            decodeHeight,
            pixelFormat,
            !!synchronousDecode /* forceUiThreadCopy */,
            retainPlaybackState,
            decodeParams));

        xref_ptr<IAbortableImageOperation> spAbortableImageOperation;

        // For printing purpose we need to do synchronousDecode and it should not affect the
        // already existing long running operation. However for SetSource we may also need to
        // perform it synchronously but in this case we'll keep the operation because SetSource
        // will clear the existing one.
        if (synchronousDecode || m_spAbortableImageOperation == nullptr)
        {
            // TODO: Why does this create another ImageCache just to do the decode? This ImageSource might already have
            // an m_imageCache with the encoded bits in it. This call creates another one that picks up the same encoded
            // bits and has the new one issue the decode.
            IFC_RETURN(imageProvider->GetImage(
                m_imageCache->GetEncodedImageData(),
                decodeParams,
                synchronousDecode ? GetImageOptions::Synchronous : GetImageOptions::None,
                spImageAvailableCallback,
                core,
                spAbortableImageOperation));
        }

        // Only keep the operation when we did not have one as in case of the SetSource path.
        // For printing we always execute the synchronous decode on top of the existing operation.
        // The result of the synchronous decode is immediately sent to the callback so it's OK to
        // have that operation released at the end of this scope.
        if (m_spAbortableImageOperation == nullptr)
        {
            m_spAbortableImageOperation = std::move(spAbortableImageOperation);
        }
        else if (!synchronousDecode) // synchronousDecode should not affect the existing operation
        {
            IFC_RETURN(m_spAbortableImageOperation->SetDecodeParams(spImageAvailableCallback, decodeParams));
        }
    }
    else
    {
        GetImageOptions imageGetOptions = synchronousDecode ? GetImageOptions::Synchronous : GetImageOptions::None;
        if ((m_nCreateOptions & DirectUI::BitmapCreateOptions::IgnoreImageCache) != DirectUI::BitmapCreateOptions::None)
        {
            imageGetOptions = imageGetOptions | GetImageOptions::IgnoreCache;
        }
        else
        {
            IFC_RETURN(CheckCachedHardwareResources(
                decodeWidth,
                decodeHeight,
                pixelFormat,
                &surfaceCacheHit));
        }

        xref_ptr<ImageDecodeParams> decodeParams;
        IFC_RETURN(PrepareDecodeParams(
            decodeWidth,
            decodeHeight,
            pixelFormat,
            synchronousDecode || surfaceCacheHit /* forceUiThreadCopy */,
            retainPlaybackState,
            decodeParams));

        // Cache identifier must be invalidated so a new one can be generated based on the updated size.
        ResetCacheIdentifier();

        // RS5 Bug #18515061:  It's possible for the ImageDecodeRequest to not have an image decoder, particularly
        // in the case that the same URI is being used across multiple BitmapImages.  The order of operations that leads
        // to this look like this:
        // BitmapImage #1 gets a DecodeStreamWithSize first.  It gets the first entry in the ImageCache's m_decodeRequests.
        // BitmapImage #2 gets DecodeStreamWithSize.  It gets the second entry in the ImageCache's m_decodeRequests.
        // BitmapImage #2's ImageDecodeRequest will get chosen as the "winner" since this list is process back-to-front.
        // This leaves BitmapImage #1's ImageDecodeRequest with a NULL image decoder.
        // After resuming, we lose all our surfaces and come back through this sequence again.
        // In this case, though, BitmapImage #1's ImageDecodeRequest is still around from the last round of decoding.
        // In this case if we call SetDecodeParams on it, it won't actually decode the image, which causes a blank texture
        // to be rendered.
        // The fix is to detect there's no decoder present and create a new ImageDecodeRequest from ImageCache,
        // which will also ensure an image decode is created, assigned and decodes the image.
        if (m_spAbortableImageOperation != nullptr && !m_spAbortableImageOperation->HasDecoder())
        {
            m_spAbortableImageOperation.reset();
        }

        if (m_spAbortableImageOperation == nullptr)
        {
            if (m_strSource.IsNull() ||
                synchronousDecode)
            {
                IFC_RETURN(imageProvider->GetImage(
                    m_imageCache->GetEncodedImageData(),
                    decodeParams,
                    imageGetOptions,
                    spImageAvailableCallback,
                    core,
                    m_spAbortableImageOperation));
            }
            else
            {
                bool getEncodedImageOnly = false;
                if (core->GetSurfaceCache() != nullptr)
                {
                    IFC_RETURN(TryUseCachedHardwareResources(*decodeParams, core->GetSurfaceCache(), &getEncodedImageOnly));
                }

                // If we find a hardware surface in the SurfaceCache getEncodedImageOnly will come back true,
                // indicating that we should not issue a full decode request.
                // However we still need to call the callback immediately with
                // the encoded image so the listener can receive the encoded image pointer and run all of its notification logic.
                // Without this, CBitmapImage will not store the encoded image, dirty the element, fire the image events, etc.
                //
                // It is possible getEncodedImageOnly could be true without an available EncodedImageData if background thread image loading
                // was used to pre-allocate hardware surfaces and subsequent decodes use the same cached resources while the original image is being
                // downloaded.  This is expected and we should ignore the getEncodedImageOnly flag and proceed down the code path that will return
                // the encoded image when the download is complete.
                if (getEncodedImageOnly)
                {
                    // Special case to handle only getting an encoded image.
                    IFC_RETURN(spImageAvailableCallback->OnImageAvailable(make_xref<AsyncDecodeResponse>(S_OK)));
                }
                else
                {
                    IFC_RETURN(m_imageCache->GetImage(decodeParams, spImageAvailableCallback, m_spAbortableImageOperation));
                }
            }
        }
        else
        {
            IFC_RETURN(m_spAbortableImageOperation->SetDecodeParams(spImageAvailableCallback, decodeParams));
        }

        // Update the hardware cache if the decode is directly to a hardware surface and
        // it should be cached.  If it is not, then it will decode to a software surface
        // and reset the hardware surface when it is finished in OnStreamImageAvailable.
        // It will then create and copy to a hardware surface in EnsureAndUpdateHardwareResources
        // at which point it will cache the new hardware surface.
        // IsHardwareOutput() was added because of TFS bug 2155285 in which the hardware surface
        // could be available from a previous operation because it is reset after the decode operation
        // completes.
        if (decodeParams->IsHardwareOutput() &&
            !surfaceCacheHit &&
            !flags_enum::is_set(imageGetOptions, GetImageOptions::IgnoreCache))
        {
            IFC_RETURN(UpdateHardwareCacheIfAvailable());
        }

        m_spPendingDecodeParams = std::move(decodeParams);
    }

    m_forceDecodeRequest = false;

    // Limit when we fire this ETW event to match RS2 behavior, which the AppAnalysis code currently relies on.
    if (retainPlaybackState || decodeToRenderSize || m_strSource.IsNull() || synchronousDecode)
    {
        CImageSource::TraceDecodeStreamForImageEtw(this, m_imageCache->GetEncodedImageData(), decodeWidth, decodeHeight, surfaceCacheHit);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the new state of the BitmapImage
//
//------------------------------------------------------------------------
void
CImageSource::SetBitmapState(BitmapImageState newState)
{
    switch (newState)
    {
        case BitmapImageState::Initial:
            ASSERT(m_imageCache == nullptr);
            ASSERT(m_pImageSurfaceWrapper->GetSoftwareSurface() == NULL);
            break;

        case BitmapImageState::Failed:
            ASSERT(m_bitmapState == BitmapImageState::Initial // Uri parse errors
                || m_bitmapState == BitmapImageState::Downloading
                || m_bitmapState == BitmapImageState::Decoding
                || m_bitmapState == BitmapImageState::Decoded    // via ReloadReleasedSoftwareImage
                || m_bitmapState == BitmapImageState::HasEncodedImageOnly); // via HandleLostResources
            break;

        case BitmapImageState::DecodePending:
            ASSERT(m_bitmapState == BitmapImageState::Initial
                || m_bitmapState == BitmapImageState::Downloading
                || m_bitmapState == BitmapImageState::Decoded // via HandleLostResources
                || (m_bitmapState == BitmapImageState::Decoding && !this->IsActive())
                || m_bitmapState == BitmapImageState::HasEncodedImageOnly); // via HandleLostResources
            ASSERT(m_imageCache != nullptr && m_imageCache->GetEncodedImageData() != nullptr);
            ASSERT(m_fDecodeToRenderSize);
            break;

        case BitmapImageState::Decoded:
            ASSERT(m_bitmapState == BitmapImageState::Downloading
                || m_bitmapState == BitmapImageState::Decoding
                || m_bitmapState == BitmapImageState::Decoded // via HandleLostResources
                || m_bitmapState == BitmapImageState::HasEncodedImageOnly); // via HandleLostResources
            ASSERT(m_imageCache != nullptr && m_imageCache->GetEncodedImageData() != nullptr);
            // If we picked up a cached hardware surface and encoded bits during ReloadImage, there won't be a software surface.
            ASSERT(m_pImageSurfaceWrapper->GetSoftwareSurface() != NULL
                || m_pImageSurfaceWrapper->HasHardwareSurfaces());

            break;

        case BitmapImageState::HasEncodedImageOnly:
            ASSERT(m_bitmapState == BitmapImageState::Decoded // via HandleLostResources
                 || m_bitmapState == BitmapImageState::HasEncodedImageOnly); // via HandleLostResources
            ASSERT(m_imageCache != nullptr && m_imageCache->GetEncodedImageData() != nullptr);
            ASSERT(!m_pImageSurfaceWrapper->HasHardwareSurfaces());
            ASSERT(m_pImageSurfaceWrapper->GetSoftwareSurface() == NULL);
            break;
    }

    m_bitmapState = newState;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Search HW surface cache for a match and use it if found.
//      The HW surface cache should be merged into ImageProvider. Then
//      this won't be needed anymore.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CImageSource::TryUseCachedHardwareResources(
    _In_ const ImageDecodeParams& imageDecodeParams,
    _In_ SurfaceCache* pSurfaceCache,
    _Out_ bool* pGetEncodedImageOnly
    )
{
    HRESULT hr = S_OK;
    ImageHardwareResources *pResources = NULL;
    xstring_ptr cacheIdentifier;

    ASSERT(pGetEncodedImageOnly != nullptr);

    *pGetEncodedImageOnly = FALSE;

    IFC(GetCacheIdentifier(imageDecodeParams, &cacheIdentifier));

    // See if someone else already cached this image.
    if (!cacheIdentifier.IsNull())
    {
        IFC(pSurfaceCache->GetResources(
            cacheIdentifier,
            &pResources
            ));
        if (pResources)
        {
            m_pImageSurfaceWrapper->UseCachedHardwareResources(pResources);

            *pGetEncodedImageOnly = TRUE;
        }
    }

Cleanup:
    ReleaseInterface(pResources);
    RRETURN(hr);
}

_Check_return_ HRESULT CImageSource::CheckCachedHardwareResources(
    unsigned long width,
    unsigned long height,
    PixelFormat pixelFormat,
    _Out_ bool* pCacheHit)
{
    bool cacheHit = false;

    if (!m_strSource.IsNull())
    {
        xref_ptr<ImageHardwareResources> resources;

        xstring_ptr cacheIdentifier;
        IFC_RETURN(GetCacheIdentifier(
            width,
            height,
            pixelFormat,
            &cacheIdentifier));

        IFC_RETURN(GetContext()->GetSurfaceCache()->GetResources(
            cacheIdentifier,
            resources.ReleaseAndGetAddressOf()
            ));

        cacheHit = (resources != nullptr);
    }

    *pCacheHit = cacheHit;

    return S_OK;
}

_Check_return_ HRESULT
CImageSource::UpdateHardwareCacheIfAvailable()
{
    // Before Background-Thread-Image-Loading, the software cache was catching multiple decodes and the hardware cache was being
    // hit in EnsureAndUpdateHardwareResources.  Since BTIL is now bypassing the software cache by decoding directly to hardware,
    // the hardware surface will be cached here after allocating the hardware resources. Any subsequent decodes will check for a
    // cache hit via CheckCachedHardwareResources and will find the previously cached entry.
    if (m_pImageSurfaceWrapper->CheckForHardwareResources() &&
        !m_strSource.IsNull())
    {
        xstring_ptr cacheIdentifier;
        IFC_RETURN(GetCacheIdentifier(
            m_pImageSurfaceWrapper->GetWidth(),
            m_pImageSurfaceWrapper->GetHeight(),
            m_pImageSurfaceWrapper->GetPixelFormat(),
            &cacheIdentifier));

        auto core = GetContext();
        xref_ptr<ImageHardwareResources> resources;
        IFC_RETURN(m_pImageSurfaceWrapper->EnsureHardwareResources(
            core->GetHWTextureManagerNoRef(),
            resources.ReleaseAndGetAddressOf()));

        if (!cacheIdentifier.IsNull() &&
            (resources != nullptr))
        {
            IFC_RETURN(core->GetSurfaceCache()->AddResources(
                cacheIdentifier,
                resources.get()
                ));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CImageSource::OnHdrChanged()
{
    if (m_pImageSurfaceWrapper->HasAnySurface())
    {
        auto decodeCallback = make_xref<ImageAvailableCallback<CImageSource>>(this, &CImageSource::OnStreamImageAvailable);
        IFC_RETURN(DecodeStreamWithSize(
            decodeCallback,
            m_pImageSurfaceWrapper->GetWidth(),
            m_pImageSurfaceWrapper->GetHeight(),
            FALSE /* synchronousDecode */,
            FALSE /* forceDecode */,
            true /* retainPlaybackState */,
            false /* decodeToRenderSize */));
    }

    return S_OK;
}

_Check_return_ HRESULT CImageSource::ReloadOnResourceInvalidation(ResourceInvalidationReason reason)
{
    // This method is called from the CImageReloadManager which will reload the images on a scale factor change.
    // As such, this object is tracked in a reload list which is not tied to the objects external lifetime.
    // If all object references are released but there is still an active decode (which internally holds onto
    // a reference), then OnUriSourceChange will eventually call DisconnectImageOperation to abort the decode.  This abort
    // will release the final reference on the object and will delete the object.  Therefore, this method must
    // AddRef itself until it is finished.
    xref_ptr<CImageSource> spHoldRef(this);

    bool shouldReload;
    IFC_RETURN(ShouldReloadOnResourceInvalidation(reason, &shouldReload));

    if (shouldReload)
    {
        switch (reason)
        {
        case ResourceInvalidationReason::ScaleChanged:
            // We implement reload by following the same code path as when the UriSource property changes -
            // except in the reload case, the UriSource property remains the same.
            //
            // When we re-calculate the image cache keys, the new keys will contain data that's different
            // for this reload (namely the resource invalidation ID and/or decode height/width will be different).
            IFC_RETURN(OnUriSourceChanged(true /*retainPlaybackState*/));
            break;

        case ResourceInvalidationReason::HdrChanged:
            IFC_RETURN(OnHdrChanged());
            break;

        default:
            ASSERT(false);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Private function used in several places. Tests whether this image
//      is eligible for reloading when a resource invalidation occurs.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageSource::ShouldReloadOnResourceInvalidation(ResourceInvalidationReason reason, _Out_ bool* shouldReload)
{
    *shouldReload = false;

    if ((ResourceInvalidationReason::HdrChanged == reason || ResourceInvalidationReason::Any == reason) &&
        IsMetadataAvailable() && m_imageMetadataView->GetImageMetadata()->isHdr)
    {
        *shouldReload = true;
        return S_OK;
    }

    if (m_strSource.IsNull())
    {
        // If we don't have a URI source property set, we shouldn't reload.
        return S_OK;
    }

    if (ResourceInvalidationReason::ScaleChanged == reason || ResourceInvalidationReason::Any == reason)
    {
        if (HasLogicalDecodeSize())
        {
            // If the app has explicitly specified a decode height/width in logical pixels, then we should reload on scale changes.
            // This allows us to re-decode the image using a decode height/width based on the new scale.
            *shouldReload = true;
            return S_OK;
        }

        // Check if our resource Uri can be invalidated. We should reload if so.
        xref_ptr<IPALResourceManager> resourceManager;
        IFC_RETURN(GetContext()->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
        IFC_RETURN(EnsureAbsoluteUri());
        IFC_RETURN(resourceManager->CanResourceBeInvalidated(m_absoluteUri, shouldReload));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Registers this image with the core ImageReloadManager. The
//      registration is only done if we haven't already registered and
//      this image is eligible for reloading.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageSource::RegisterWithReloadManager()
{
    if (!m_fRegisteredWithReloadManager)
    {
        bool shouldReload = false;
        IFC_RETURN(ShouldReloadOnResourceInvalidation(ResourceInvalidationReason::Any, &shouldReload));

        if (shouldReload)
        {
            auto core = GetContext();
            IFCEXPECT_RETURN(core);
            // Warning: This may not work correctly with AppWindows
            //     Task 19490172: CImageSource::RegisterWithReloadManager must be AppWindow aware
            if (auto contentRoot = VisualTree::GetContentRootForElement(this))
            {
                if (const auto rootScale = RootScale::GetRootScaleForContentRoot(contentRoot))
                {
                    CImageReloadManager& imageReloadManager = rootScale->GetImageReloadManager();
                    imageReloadManager.AddImage(this);
                    m_fRegisteredWithReloadManager = true;
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Unregisters this image with the core ImageReloadManager. This
//      is only done if we previously registered.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CImageSource::UnregisterWithReloadManager()
{
    if (m_fRegisteredWithReloadManager)
    {
        auto core = GetContext();
        IFCEXPECT_RETURN(core);
        if (auto root = VisualTree::GetContentRootForElement(this))
        {
            if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
            {
                CImageReloadManager& imageReloadManager = rootScale->GetImageReloadManager();
                imageReloadManager.RemoveImage(this);
                m_fRegisteredWithReloadManager = false;
            }
        }
    }

    return S_OK;
}

HRESULT CBitmapImage::GetAutoPlay(
    _Out_ bool& isAutoPlay
    )
{
    CValue autoPlay;
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::BitmapImage_AutoPlay), &autoPlay));

    isAutoPlay = autoPlay.AsBool();

    return S_OK;
}

// Considers input parameters and object state to form a ImageDecodeParams object
// to use with the ImageProvider
 _Check_return_ HRESULT CImageSource::PrepareDecodeParams(
    unsigned int width,
    unsigned int height,
    PixelFormat pixelFormat,
    bool forceUiThreadCopy,
    bool retainPlaybackState,
    xref_ptr<ImageDecodeParams>& decodeParams
    )
{
    auto core = GetContext();
    decodeParams = nullptr;

    ASSERT(IsMetadataAvailable());

    const bool isExplicitDecodeSizeSet = GetDecodeWidth() || GetDecodeHeight();
    if (isExplicitDecodeSizeSet)
    {
        TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::DecodeSizeSpecified);
    }

    bool isAutoPlay = false;
    if (OfTypeByIndex<KnownTypeIndex::BitmapImage>())
    {
        if (retainPlaybackState)
        {
            CValue isPlaying;
            IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::BitmapImage_IsPlaying), &isPlaying));
            isAutoPlay = isPlaying.AsBool();
        }
        else
        {
            IFC_RETURN(GetAutoPlay(isAutoPlay));
        }
    }

    const bool isDeviceInitialized =
        (core->GetBrowserHost() != nullptr) &&
        (core->GetBrowserHost()->GetGraphicsDeviceManager() != nullptr) &&
        core->GetBrowserHost()->GetGraphicsDeviceManager()->IsInitialized();
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    ASSERT(m_imageCache != nullptr);

    // Use the background loading feature only if the following conditions are met:
    // - Feature is enabled via RuntimeEnabledFeatures
    // - Caller didn't force it to use UI thread copy (useful for Synchronous Decoding and cases where the
    //   Software surface is needed)
    // - Width and Height is specified.
    // - DecodePixelWidth/DecodePixelHeight is not set.  This is a tricky case that requires some thought.
    //   It can be supported by using the CCoreServices EnqueueImageDecodeRequest method to queue a decode
    //   request to an explicit size but this requires logic to handle caching, and compat risk.  It is a
    //   consideration for post-Threshold to support this.
    // - The image surface wrapper isn't already marked as requring a system memory copy.
    // - The image surface wrapper isn't already using a hardware surface which would require a reallocation
    //   and thus will result in a flicker.  This restriction can be removed at a future time if the
    //   hardware surface can be made to swap in during the render walk but care will need to be taken
    //   for the hardware surface cache in such a scenario.  It is generally a low occurrence case.
    // - The graphics device must be in an initialized state.  Shell start menu can create images before
    //   the graphics device initialization thread completes.  In this case, we should disable BTIL so there
    //   is no stall.
    // - There are no presenter hosts in the visual tree.
    //   If the visual tree has presenter hosts, the render target for the host will be different from that of the DXamlCore,
    //   and we need the render target of the host to be walked by HwWalk.
    // - The device isn't suspended or in a device lost state (which would cause the allocation to fail).
    //   Note that if the device is suspended, it will return E_FAIL during allocation.  We check ahead of time
    //   from the core so the hardware allocation logic can be skipped, but it is possible that a device lost
    //   hasn't been discovered yet since it is only updated during the render walk.  If this is the case, we
    //   handle the device lost return code from AllocateHardwareResources.
    // - We don't already have a software decode with the same parameters in progress.  In this case, the ImageCache
    //   code isn't smart enough to tell the difference between this request and a new request to decode to hardware,
    //   and confusion will occur when the software decode completes.  It's also bad for perf to decode with the same exact
    //   size/format multiple times.  See VSO Bug #24547577.

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::BackgroundThreadImageLoading) &&
        !forceUiThreadCopy &&
        !m_pImageSurfaceWrapper->MustKeepSystemMemory() &&
        !m_pImageSurfaceWrapper->HasHardwareSurfaces() &&
        !isExplicitDecodeSizeSet &&
        (width != 0) &&
        (height != 0) &&
        isDeviceInitialized &&
        !core->IsSuspended() &&
        !core->IsDeviceLost() &&
        !m_imageCache->HasSoftwareImageDecodeInProgressWithParams(width, height, pixelFormat))
    {
        // Must reset surfaces before decoding so that new hardware resources can be allocated here
        // This is typically done in the decoding callback.  However, that behavior is preferable when decoding
        // to softwares surfaces to minimize blank time when updating the hardware surface.  The decoding callback
        // won't ResetSurfaces because it will never return a surface in the response object since the surfaces are
        // provided as input.
        ResetSurfaces(false /* mustKeepSoftwareSurface */, false /* mustKeepHardwareSurfaces */);

        bool isOpaque = !m_imageMetadataView->GetImageMetadata()->supportsAlpha;

        // First allocate the hardware resources for background thread decoding
        // Note that autoUpdate is set to false so that the hardware surface can be locked/unlocked several times
        // to update the surface incrementally without queuing the update to dcomp.  This is important
        // for synchronization as well since an update requires access to the HWTextureManager which we want to
        // prevent for background thread loading.
        auto hr = m_pImageSurfaceWrapper->AllocateHardwareResources(
            core->GetHWTextureManagerNoRef(),
            width,
            height,
            pixelFormat,
            isOpaque);

        // Device lost error should fallback to outputting a software surface.
        if (!GraphicsUtility::IsDeviceLostError(hr))
        {
            IFC_RETURN(hr);

            // Get the DComp surfaces that were created and the rects associated with them
            // and populate the decodeParams for the background thread to decode to
            SurfaceUpdateList surfaceUpdateList;
            IFC_RETURN(m_pImageSurfaceWrapper->GetHWSurfaceUpdateList(surfaceUpdateList));

            // Locks the hardware resources so they cannot be accessed by any source until Unlocked;
            IFC_RETURN(m_pImageSurfaceWrapper->LockHardwareMutex());

            decodeParams = make_xref<ImageDecodeParams>(
                pixelFormat,
                width,
                height,
                isAutoPlay,
                surfaceUpdateList,
                false /* isLoadedImageSurface */,
                reinterpret_cast<XUINT64>(this),
                m_strSource);
        }
    }

    // If decodeParams was not configured to output a hardware surface
    // then configure it to output a software surface.
    if (!decodeParams)
    {
        decodeParams = make_xref<ImageDecodeParams>(
            pixelFormat,
            width,
            height,
            isAutoPlay,
            false /* isLoadedImageSurface */,
            reinterpret_cast<XUINT64>(this),
            m_strSource);
    }

    return S_OK;
}

void CImageSource::TraceDecodeStreamForImageEtw(
    _In_ CImageSource* image,
    _In_ const std::shared_ptr<EncodedImageData>& encodedImageData,
    _In_ UINT32 decodeWidth,
    _In_ UINT32 decodeHeight,
    _In_ bool retrievedFromCache)
{
    if (!EventEnabledDecodeStreamForImageInfo() || image == nullptr || encodedImageData == nullptr)
    {
        return;
    }

    // We fire this event (if enabled) along with the TraceDecodeStreamForImageInfo
    // so that AppAnalysis can correlate these events.
    if (EventEnabledImageSourceRelationInfo())
    {
        TraceImageSourceRelationEtw(image);
    }

    auto& imageMetadata = encodedImageData->GetMetadata();
    TraceDecodeStreamForImageInfo(
        reinterpret_cast<UINT64>(image),
        decodeWidth,
        decodeHeight,
        imageMetadata.width,
        imageMetadata.height,
        retrievedFromCache);
}

std::uint64_t CImageSource::GetEstimatedSurfaceCommitSize() const
{
    std::uint64_t size = 0;
    if (m_pImageSurfaceWrapper)
    {
        size += m_pImageSurfaceWrapper->GetEstimatedSurfaceCommitSize();
    }

    if (m_imageCache)
    {
        auto encodedData = m_imageCache->GetEncodedImageData();

        if (encodedData)
        {
            size += m_imageCache->GetEncodedImageData()->GetRawDataSize();
        }
    }
    return size;
}

bool CImageSource::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    bool walked = __super::ReferenceTrackerWalkCore(walkType, isRoot, shouldWalkPeer);

    if (walked)
    {
        if (walkType == DirectUI::EReferenceTrackerWalkType::RTW_TotalCompressedImageSize)
        {
            auto oldSize = DirectUI::DXamlServices::GetDXamlCore()->GetRTWTotalCompressedImageSize();
            DirectUI::DXamlServices::GetDXamlCore()->SetRTWTotalCompressedImageSize(oldSize + GetEstimatedSurfaceCommitSize());
        }
    }

    return walked;
}

wf::Size CImageSource::GetMaxRootSize()
{
    return m_imageMetadataView->GetMaxRootSize();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create a new bitmap image.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapImage::Create(
    _Outptr_ CDependencyObject** retObject,
    _In_ CREATEPARAMETERS* createParameters
    )
{
    // Note: make_xref is not used because it requires the constructor it is calling to be public
    //       which we want to avoid in favor of consumers using the Create method.  Thus it must
    //       use xref_ptr to attach to it and detach when it is done.
    xref_ptr<CBitmapImage> bitmapImage;
    bitmapImage.attach(new CBitmapImage(createParameters->m_pCore));

    IFC_RETURN(bitmapImage->SetupImageSource(false /*mustKeepSoftwareSurface*/, createParameters));

    *retObject = bitmapImage.detach();

    return S_OK;
}

_Check_return_ HRESULT
CBitmapImage::PrepareDecode(bool retainPlaybackState)
{
    IFC_RETURN(SetImageCache(nullptr));
    SetBitmapState(BitmapImageState::Initial);

    // If we should not use decode to render size, disable it for this object
    // A case that we would want to do this is when the API specifies the
    // DecodePixelWidth or DecodePixelHeight.  If it is later modified, it should only
    // re-enable decode to render size on the next source set.
    m_fDecodeToRenderSize = ShouldDecodeToRenderSize();

    if (!retainPlaybackState)
    {
        bool isAutoPlay = false;
        IFC_RETURN(GetAutoPlay(isAutoPlay));

        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsPlaying, isAutoPlay));
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsAnimatedBitmap, false));
        m_frameIndex = 0;
    }
    return S_OK;
}

_Check_return_ HRESULT
CBitmapImage::OnDownloadImageAvailableImpl(
    _In_ IImageAvailableResponse* pResponse
    )
{
    HRESULT hr = S_OK;
    HRESULT hrImageResult = pResponse->GetDecodingResult();
    auto core = GetContext();

    // Reset the hardware surface for the DecodeToRenderSize case since it may have been decoded to a new size
    if (m_fDecodeToRenderSize &&
        (pResponse->GetSurface() != nullptr))
    {
        // Preserve the existing "must keep system memory" flag on the image surface wrapper.
        // This is required in a scenario where this CBitmapImage is being used as the source
        // of multiple brushes, one using hardware and the other using software.  If we clear
        // the "must keep system memory" flag, the surface will get released as soon as we
        // update the hardware resources for the hardware path, making the software surface
        // unavailable for the software path, causing it to not render anything.
        // The image surface wrapper already has the proper flag set depending on how it's being
        // used, so we simply preserve that flag as we're resetting the surfaces.
        // Additionally, pSurface is available (software surface) which means that hardware surfaces
        // must be reset in order to re-allocate the hardware surfaces to match the software surface size.
        ResetSurfaces(!!m_pImageSurfaceWrapper->MustKeepSystemMemory(), false);
    }

    IFC(__super::OnDownloadImageAvailableImpl(pResponse));

    if (FAILED(hrImageResult))
    {
        DisconnectImageOperation();
    }
    else
    {
        core->StartTrackingAnimatedImage(this);
    }

    if (m_pendingDecodeForLostSoftwareSurface)
    {
        // Image data was recovered for lost software surface, put the image back in the decoded state.
        // It is possible for any decode to be aborted if an API call to set a new source interrupts the decode.
        if (m_pImageSurfaceWrapper->HasAnySurface() && IsMetadataAvailable())
        {
            SetBitmapState(BitmapImageState::Decoded);
        }
    }
    else
    {
        if (SUCCEEDED(hrImageResult) && (m_frameIndex == 0))
        {
            bool isAnimatedImage = m_imageMetadataView->GetImageMetadata()->IsAnimatedImage();
            bool autoPlay = false;
            IFC(GetAutoPlay(autoPlay));

            IFC(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsAnimatedBitmap, isAnimatedImage));
            IFC(SetValueByKnownIndex(KnownPropertyIndex::BitmapImage_IsPlaying, isAnimatedImage && autoPlay));

            // Despite being named "download" the pupose of this event is to request the measure pass on the parent
            // ImageBrush(es) to account for the physical surface size now available. It may not match neither the
            // natural size nor the requested decode params size so we can only find out now as the decode is complete.
            IFC(FireInternalDownloadEvent());

            // Retain the natural size of the image
            // This was introduced to preserve the natural size for hit-testing when applications use an
            // ImageBrush for a button.  This originated from a bug pre-RS1 that apps took a dependency on
            // whereby the image natural size was used for layout and hit-testing even if the source was changed.
            // It would use the size of the previous source until the new image is done decoding.
            // Refer to TFS 7302806 for info on the bug and an attached min-repro app.
            m_retainedNaturalWidth = m_imageMetadataView->GetImageMetadata()->width;
            m_retainedNaturalHeight = m_imageMetadataView->GetImageMetadata()->height;

            if (m_fDecodeToRenderSize &&
                !m_pImageSurfaceWrapper->HasAnySurface())
            {
                // Check to see if the decode size has changed since the source was set and now.  If it has
                // then, decode should be issued.  This was done because the ability to set the DecodePixelWidth
                // and DecodePixelHeight at any time was added with the DecodeToRenderSize feature (including mid-download).
                m_fDecodeToRenderSize = ShouldDecodeToRenderSize();
                if (!m_fDecodeToRenderSize)
                {
                    SetBitmapState(BitmapImageState::Decoding);
                    IFC(RedecodeEncodedImage(FALSE));
                }
                else
                {
                    SetBitmapState(BitmapImageState::DecodePending);
                }
            }
            else
            {
                // This will get called from OnStreamImageAvailable after any decoding is complete to
                // set the decoded state and fire the image opened event if the decoded surface is available.
                IFC(FireImageOpened());
            }
        }
        else if (FAILED(hrImageResult))
        {
            IFC(FireImageFailed(hrImageResult));
        }

        if (m_pImageSurfaceWrapper->HasAnySurface())
        {
            m_frameIndex++;
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basically we are going to create a dedicated texture if we can, or if too big, we
//   will tile the image.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapImage::EnsureAndUpdateHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ SurfaceCache *pSurfaceCache
    )
{
    HRESULT hr = S_OK;

    TraceImageEnsureAndUpdateHardwareResourcesBegin(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    // Since this CBitmapImage is being render walked it's not a candidate to be suspended.
    // Also make sure the animation is resumed if it has been suspended in the past.
    GetContext()->StopTrackingAnimatedImage(this);
    IFC(ResumeAnimation());

    // Whether we had device lost or not, check and see if we have lost
    // hardware and software surfaces but can restore them from the encoded image
    if (!m_pImageSurfaceWrapper->CheckForHardwareResources() &&
        (GetSoftwareSurface() == NULL) &&
        IsMetadataAvailable() &&
        (m_bitmapState == BitmapImageState::Decoded))
    {
        SetBitmapState(BitmapImageState::HasEncodedImageOnly);
        IFC(RedecodeEncodedImage(TRUE));
    }

    // If we don't have hardware resources, but do have a software surface
    // we're in the decoded state
    if (!m_pImageSurfaceWrapper->CheckForHardwareResources() &&
        GetSoftwareSurface() != NULL)
    {
        ASSERT(IsMetadataAvailable()); // Can't get a software surface without an encoded image
        SetBitmapState(BitmapImageState::Decoded);
    }

    IFC(__super::EnsureAndUpdateHardwareResources(
            pTextureManager,
            pRenderTarget,
            pSurfaceCache));
Cleanup:

    TraceImageEnsureAndUpdateHardwareResourcesEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    RRETURN(hr);
}

void CImageSource::TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::FallbackReason reason)
{
    const char* description = "";
    switch (reason)
    {
    case ImageDecodeBoundsFinder::FallbackReason::SynchronousDecode:
        description = "SynchronousDecode";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::NotInLiveTree:
        description = "NotInLiveTree";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::BitmapIcon:
        description = "BitmapIcon";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::SoftwareRendering:
        description = "SoftwareRendering";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::UsesImageTiling:
        description = "UsesImageTiling";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::EmptyBoundsPostRenderWalk:
        description = "EmptyBoundsPostRenderWalk";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::NineGrid:
        // Note:  Even though NineGrid is already optimized, it is still useful to log this info
        description = "NineGrid";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::DecodeSizeSpecified:
        description = "DecodeSizeSpecified";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::DragAndDrop:
        description = "DragAndDrop";
        break;
    case ImageDecodeBoundsFinder::FallbackReason::AnimatedGIF:
        description = "AnimatedGIF";
        break;
    default:
        ASSERT(FALSE);
        break;
    }

    ImagingTelemetry::DecodeToRenderSizeDisqualified(reinterpret_cast<uint64_t>(this), description);
}
