// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// TODO: Refactor and remove the need to include the marked header files below
//       marked with "Shouldn't be necessary".  Since imagesource.h is fairly monolithic and
//       requires the world, this requires a fair bit of refactoring of imagesource.h in order
//       to remove these include dependencies.

#include "precomp.h"
#include <MUX-ETWEvents.h>
#include <Windows.Foundation.h>
#include <GraphicsUtility.h>
#include "ImageProviderInterfaces.h"
#include "palgfx.h"
#include "corep.h" // Shouldn't be necessary
#include "depends.h" // Shouldn't be necessary
#include "framework.h" // Shouldn't be necessary
#include "brush.h" // Shouldn't be necessary
#include "tilebrush.h" // Shouldn't be necessary
#include "imagebrush.h" // Shouldn't be necessary
#include "mediabase.h" // Shouldn't be necessary
#include "imagebase.h" // Shouldn't be necessary
#include "ImageMetadata.h"
#include "TiledSurface.h"
#include "ImageSurfaceWrapper.h"
#include "imagesource.h"
#include "ImageProvider.h"
#include <windows.graphics.imaging.h>
#include "SurfaceDecodeParams.h"
#include "ImageCopyParams.h"
#include "ImageAvailableCallback.h"
#include "CoreAsyncAction.h"
#include "SoftwareBitmapSource.h"
#include "host.h"
#include <WindowsGraphicsDeviceManager.h>
#include <PixelFormat.h>

_Check_return_ HRESULT
CSoftwareBitmapSource::Create(
    _Outptr_ CDependencyObject** ppObject,
    _In_ CREATEPARAMETERS* pCreate
    )
{
    HRESULT hr = S_OK;
    CSoftwareBitmapSource *pSoftwareBitmapSource = NULL;

    pSoftwareBitmapSource = new CSoftwareBitmapSource(pCreate->m_pCore);
    IFC(ImageSurfaceWrapper::Create(pCreate->m_pCore, FALSE /* mustKeepSoftwareSurface */, &pSoftwareBitmapSource->m_pImageSurfaceWrapper));

    SetInterface(*ppObject, pSoftwareBitmapSource);

Cleanup:
    ReleaseInterface(pSoftwareBitmapSource);
    RRETURN(hr);
}

// Closes the SoftwareBitmap reference by explicitly calling Close
// and releases the software/hardware surfaces which will cause an early abort
// of any async operations in progress.  If the async operation is in progress
// it will close after the async operation returns.
_Check_return_ HRESULT CSoftwareBitmapSource::Close()
{
    if (m_spSoftwareBitmap != nullptr)
    {
        if (m_pAsyncAction != nullptr)
        {
            // Async operation in progress, we have to close the software bitmap after
            // the async operation is complete.
            // Note: Don't call CancelImageGet which will stop the callback... the callback
            //       needs to come back so the software bitmap can be closed.
            m_closeOnCompletion = true;
            CompleteAsyncAction(E_ABORT);
        }
        else
        {
            IFC_RETURN(CloseSoftwareBitmap());
        }
    }

    // If there is an async operation in progress, releasing the reference on the surface it is operating on
    // will cause an early exit condition... this should also trigger a callback which can be used
    // to close the SoftwareBitmap.
    ResetSurfaces();

    return S_OK;
}

_Check_return_ HRESULT
CSoftwareBitmapSource::CloseSoftwareBitmap()
{
    if (m_spSoftwareBitmap)
    {
        wrl::ComPtr<wf::IClosable> spClosableSoftwareBitmap;

        IFC_RETURN(m_spSoftwareBitmap.As<wf::IClosable>(&spClosableSoftwareBitmap));

        spClosableSoftwareBitmap->Close();
        m_spSoftwareBitmap = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT
CSoftwareBitmapSource::SetBitmap(
    _In_ wrl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap,
    _In_ ICoreAsyncAction *pCoreAsyncAction
    )
{
    // Cancel any current image download or decode operation.
    DisconnectImageOperation();
    CompleteAsyncAction(E_ABORT);
    m_pAsyncAction = pCoreAsyncAction;

    // Setup for a new async copy operation
    m_spSoftwareBitmap = spSoftwareBitmap;

    if (m_spSoftwareBitmap == nullptr)
    {
        // Sending a nullptr parameter for the sotware bitmap releases the reference
        // Reset the surfaces and dirty the element to provide expected behavior.
        // This should complete immediately.
        ResetSurfaces();
        IFC_RETURN(SetDirty());
        CompleteAsyncAction(S_OK);
    }
    else
    {
        ASSERT(m_spAbortableImageOperation == nullptr);

        ImagingTelemetry::SetSoftwareBitmap(reinterpret_cast<uint64_t>(this));

        IFC_RETURN(ReloadSource(false /* forceCopyToSoftwareSurface */));
    }

    return S_OK;
}

_Check_return_ HRESULT CSoftwareBitmapSource::OnSoftwareBitmapImageAvailable(_In_ IImageAvailableResponse* pResponse)
{
    HRESULT hr = S_OK;

    //
    // The off-thread surface upload completed. There are three possibilities:
    //
    //   1. The upload completed successfully.
    //      This is the best case. Tell the app that their SetBitmapAsync completed successfully and continue as usual.
    //
    //   2. The upload failed with a device lost error while we're uploading to a hardware surface.
    //      This is a case where we can recover. We can instead copy the SoftwareBitmap out to a software surface that
    //      we keep around, then try to upload to hardware at the end of the next frame. If that upload fails, Xaml's
    //      device lost recovery will kick in and upload again from the same software surface. So in this case, do not
    //      tell the app that their SetBitmapAsync completed, and try to copy to a software surface. Once that operation
    //      completes we'll notify the app.
    //
    //   3. The upload failed due to some other sort of error.
    //      We don't have a way of recovering from this. Tell the app that their SetBitmapAsync failed.
    //
    bool attemptCopyToSoftwareSurfaceInstead = false;

    m_spAbortableImageOperation.reset();

    HRESULT uploadHResult = pResponse->GetDecodingResult();

    if (GraphicsUtility::IsDeviceLostError(uploadHResult))
    {
        // Toss the hardware surface that we failed to upload into. We'll be copying into a software surface, then
        // copying from that into a hardware surface later. This software to hardware copy is done in
        // CImageSource::EnsureAndUpdateHardwareResources, but only if m_pImageSurfaceWrapper is correctly marked
        // as needing an update. We're about to queue up a copy into a software surface, which will run the other
        // branch of this if statement once it succeeds. That will queue an "update" in CImageSource::OnImageAvailableCommon,
        // and ImageSurfaceWrapper::UpdateHardwareResources will respond by just clearing the "update needed flag"
        // without doing the software to hardware copy. UpdateHardwareResources used to do the copy, but that has
        // since moved to a worker thread. Clearing the hardware surface here ensures that UpdateHardwareResources
        // won't clear the "update needed flag", so when we get to CImageSource::EnsureAndUpdateHardwareResources
        // later it can do the software to hardware copy.
        m_pImageSurfaceWrapper->ReleaseHardwareResources();

        attemptCopyToSoftwareSurfaceInstead = true;

        ImagingTelemetry::SoftwareBitmapFallbackAfterUploadError(reinterpret_cast<uint64_t>(this), uploadHResult);

        // Force a copy into a software surface, which will never hit a device lost error. We'll upload this into a
        // hardware surface later on a UI thread frame.
        IFC(ReloadSource(true /* forceCopyToSoftwareSurface */));
    }
    else
    {
        if (m_closeOnCompletion)
        {
            IFC(CloseSoftwareBitmap());
            m_closeOnCompletion = false;
        }

        if (ShouldContinueAsyncAction())
        {
            IFC(OnImageAvailableCommon(pResponse));
        }
    }

Cleanup:
    if (!attemptCopyToSoftwareSurfaceInstead)
    {
        CompleteAsyncAction(uploadHResult);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT
CSoftwareBitmapSource::ReloadReleasedSoftwareImage()
{
    // Pre-condition for calling this method: m_pImageSurfaceWrapper->GetSoftwareSurface() == nullptr
    ASSERT(m_pImageSurfaceWrapper->GetSoftwareSurface() == nullptr);

    // Only request a software surface if there is a SoftwareBitmap already set on the object
    // and if there isn't a pending operation to request the software surface.

    // Logic Table:
    //  op | req || reload
    // --------------------
    //   0 |   0 || 1       <--- No operations issued to get the software surface
    //   0 |   1 || 1       <-/  (Ditto of above)
    //   1 |   0 || 1       <--- Operation has been requested, but not for a software surface, issue another one for the software surface.
    //   1 |   1 || 0       <--- Operation already requested for the software surface.
    bool isOperationInProgress = (m_spAbortableImageOperation != nullptr);
    bool isSoftwareSurfaceRequested = !!m_pImageSurfaceWrapper->MustKeepSystemMemory();

    // TODO: As it stands today for the software copy, imagesource will be holding onto
    //       a SoftwareBitmap that is able to provide the software bitmap at any point in time and
    //       ImageSurfaceWrapper will also be holding onto a software CMemorySurface meaning there
    //       could potentially be two copies of the software bits.  The fix would be to get
    //       ImageSurfaceWrapper to either hold a reference to the SoftwareBitmap and use that
    //       or to change ImageSurfaceWrapper to use SoftwareBitmap as it's primary implementation
    //       for software memory.  This should be addressed in 1502 before the API is made public.
    //       Care should be taken when taking multiple locks on the software bitmap.  A background
    //       thread could be reading from the software bitmap meanwhile the UI thread may need access.
    if (m_spSoftwareBitmap &&
        !(isOperationInProgress && isSoftwareSurfaceRequested))
    {
        m_pImageSurfaceWrapper->SetKeepSystemMemory();

        IFC_RETURN(ReloadSource(false /* forceCopyToSoftwareSurface */));
    }

    return S_OK;
}

_Check_return_ HRESULT
CSoftwareBitmapSource::EnsureAndUpdateHardwareResources(
    _In_ HWTextureManager* pTextureManager,
    _In_ CWindowRenderTarget* pRenderTarget,
    _In_ SurfaceCache* pSurfaceCache
    )
{
    HRESULT hr = S_OK;

    TraceImageEnsureAndUpdateHardwareResourcesBegin(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    // Whether we had device lost or not, check and see if we have lost
    // hardware and software surfaces but can restore them from the stored SoftwareBitmap
    if ((!m_pImageSurfaceWrapper->HasAnySurface() ||
        m_pImageSurfaceWrapper->CheckForLostHardwareResources()) &&
        m_spSoftwareBitmap &&
        (m_spAbortableImageOperation == nullptr))
    {
        IFC(ReloadSource(false /* forceCopyToSoftwareSurface */));
    }

    IFC(__super::EnsureAndUpdateHardwareResources(
        pTextureManager,
        pRenderTarget,
        pSurfaceCache));

Cleanup:

    TraceImageEnsureAndUpdateHardwareResourcesEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    return hr;
}

_Check_return_ HRESULT CSoftwareBitmapSource::PrepareCopyParams(
    bool forceCopyToSoftwareSurface,
    xref_ptr<ImageCopyParams>& spImageCopyParams)
{
    HRESULT hr = S_OK;
    spImageCopyParams = nullptr;

    // Use the background loading feature under the following conditions:
    // - System memory allocation isn't required
    // - The device isn't suspended or in a device lost state (which would cause the allocation to fail).
    //   Note that if the device is suspended, it will return E_FAIL during allocation.
    auto core = GetContext();

    //
    // Note: There's a difference between forceCopyToSoftwareSurface and m_pImageSurfaceWrapper->MustKeepSystemMemory(),
    // although both refer to the software surface.
    //
    // MustKeepSystemMemory() is meant for scenarios where we need the software bits long-term (e.g. for software rendering
    // later). Marking the ImageSurfaceWrapper with SetKeepSystemMemory() will prevent it from _ever_ releasing the software
    // surface later on.
    //
    // forceCopyToSoftwareSurface is a one-time copy to a software surface. We need this software surface until we can
    // successfully upload it to a hardware surface, at which time we can discard the software surface. We don't want to
    // mark the ImageSurfaceWrapper with SetKeepSystemMemory() because we _do_ want to discard the software surface after
    // a successful upload. That discard happens in ImageSurfaceWrapper::UpdateSurfaceFromSoftware via a call to
    // ImageSurfaceWrapper::ReleaseSoftwareSurfaceIfAllowed.
    //
    // Todo: I don't think software rendering scenarios exist anymore. BitmapCache is gone. The only place that still calls
    // CImageBrush::ReloadSoftwareSurfaceIfReleased is in alpha mask rendering in BaseContentRenderer, but even there we've
    // switched away from the Silverlight software rasterizer and moved to D2D. I think this whole flag/mechanism can be
    // deleted.
    //
    if (!forceCopyToSoftwareSurface &&
        !m_pImageSurfaceWrapper->MustKeepSystemMemory() &&
        !core->IsSuspended() &&
        !core->IsDeviceLost())
    {
        // There appear to be cases where we might get called prior to the first tick.  When this happens,
        // make sure that the hardware resources are ready for us.
        IFC(core->GetBrowserHost()->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());

        INT32 width = 0;
        INT32 height = 0;
        wgri::BitmapPixelFormat bitmapPixelFormat;
        wgri::BitmapAlphaMode bitmapAlphaMode;
        SurfaceUpdateList surfaceUpdateList;

        IFC(m_spSoftwareBitmap->get_PixelWidth(&width));
        IFC(m_spSoftwareBitmap->get_PixelHeight(&height));
        IFC(m_spSoftwareBitmap->get_BitmapPixelFormat(&bitmapPixelFormat));
        IFC(m_spSoftwareBitmap->get_BitmapAlphaMode(&bitmapAlphaMode));

        bool opaque = (bitmapAlphaMode == wgri::BitmapAlphaMode_Ignore);

        ResetSurfaces();

        hr = m_pImageSurfaceWrapper->AllocateHardwareResources(
            core->GetHWTextureManagerNoRef(),
            width,
            height,
            pixelColor32bpp_A8R8G8B8,
            opaque /* opaque */);

        // Device lost error should fallback to outputting a software surface.
        if (!GraphicsUtility::IsDeviceLostError(hr))
        {
            IFC(hr);

            // Get the DComp surfaces that were created and the rects associated with them
            // and populate the decodeParams for the background thread to decode to
            IFC(m_pImageSurfaceWrapper->GetHWSurfaceUpdateList(surfaceUpdateList));

            // Locks the hardware resources so they cannot be accessed by any source until Unlocked;
            IFC(m_pImageSurfaceWrapper->LockHardwareMutex());

            spImageCopyParams = make_xref<ImageCopyParams>(
                m_spSoftwareBitmap,
                surfaceUpdateList);
        }
    }

    // If spImageCopyParams was not configured to output a hardware surface
    // then configure it to output a software surface.
    if (!spImageCopyParams)
    {
        spImageCopyParams = make_xref<ImageCopyParams>(
            m_spSoftwareBitmap);

        hr = S_OK;
    }

Cleanup:
    return hr;
}
