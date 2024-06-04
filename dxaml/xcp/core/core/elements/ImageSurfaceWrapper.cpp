// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GraphicsUtility.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <PixelFormat.h>
#include <SurfaceDecodeParams.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Create method
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
ImageSurfaceWrapper::Create(
    _In_ CCoreServices *pCore,
    bool mustKeepSoftwareSurface,
    _Outptr_ ImageSurfaceWrapper **ppImageSurface
    )
{
    HRESULT hr = S_OK;
    ImageSurfaceWrapper *pImageSurface = NULL;

    pImageSurface = new ImageSurfaceWrapper(pCore, mustKeepSoftwareSurface);

    IFC(pCore->RegisterPLMListener(pImageSurface));

    *ppImageSurface = pImageSurface;
    pImageSurface = nullptr;

Cleanup:
    ReleaseInterfaceNoNULL(pImageSurface);
    RRETURN(hr);
}

// The user of this object sets mustKeepSoftwareSurface flag when it is not going to restore SW bits if they are lost.
// In addition to that the user may decide to change that flag later to keep SW bits for performance reasons. However
// he is still able to recover SW surface if it's lost so when we run low on memory we can stomp over the performance
// and use that knowledge of the user's ability to restore the surface to decide which surfaces are safe to kill.
ImageSurfaceWrapper::ImageSurfaceWrapper(
    CCoreServices *pCore,
    bool mustKeepSoftwareSurface
    )
    : m_pImageSurfaceSoftware(NULL)
    , m_pImageSurfaceHardware(NULL)
    , m_pD2DBitmap(NULL)
    , m_deviceCleanupTimestamp(0LL)
    , m_hardwareMutex(false)
    , m_isHardwareMutexValid(true)
    , m_releaseSoftwareSurfaceOnLowMemory(!mustKeepSoftwareSurface)
    , m_isUpdateRequired(false)
    , m_isReferencingCache(false)
    , m_mustKeepSystemMemory(mustKeepSoftwareSurface)
    , m_lostHardwareResources(FALSE)
    , m_retainedWidth(0)
    , m_retainedHeight(0)
    , m_retainedSize(false)
    , m_pCoreNoRef(pCore)
    , m_forceRecreateHardwareResources(false)
{
    XCP_WEAK(&m_pCoreNoRef);
}

ImageSurfaceWrapper::~ImageSurfaceWrapper()
{
    VERIFYHR(m_pCoreNoRef->UnregisterPLMListener(this));
    ReleaseInterface(m_pImageSurfaceSoftware);
    ReleaseInterface(m_pImageSurfaceHardware);
    ReleaseInterface(m_pD2DBitmap);
    UnregisterForDeviceCleanup();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the software surface if it's available. Otherwise return
//      the PC hardware surface if it's available.
//      Purely a helper function that combines the two IPALSurfaces (software and hardware)
//      TODO:  Refactor to account for D2DBitmap backing
//
//------------------------------------------------------------------------
_Ret_maybenull_ IPALSurface*
ImageSurfaceWrapper::GetSoftwareOrHardwareSurface()
{
    if (m_pImageSurfaceSoftware)
    {
        return m_pImageSurfaceSoftware;
    }
    else if (m_pImageSurfaceHardware)
    {
        return IsHardwareLocked() ? nullptr : m_pImageSurfaceHardware;
    }
    else
    {
        return NULL;
    }
}

PixelFormat ImageSurfaceWrapper::GetPixelFormat() const
{
    if (GetSoftwareSurface() != nullptr)
    {
        return GetSoftwareSurface()->GetPixelFormat();
    }
    else if (m_pImageSurfaceHardware != nullptr)
    {
        return m_pImageSurfaceHardware->GetPixelFormat();
    }
    else
    {
        return pixelUnknown;
    }
}

XUINT32
ImageSurfaceWrapper::GetWidth() const
{
    // For BackgroundThreadImageLoading, it is safe to get the
    // width/height of a surface without a hardware lock since
    // they are immutable after allocation.

    if (GetSoftwareSurface())
    {
        return GetSoftwareSurface()->GetWidth();
    }
    else if (m_pD2DBitmap)
    {
        return m_pD2DBitmap->GetWidth();
    }
    else
    {
        return GetHardwareWidth();
    }
}

XUINT32
ImageSurfaceWrapper::GetHeight() const
{
    if (GetSoftwareSurface())
    {
        return GetSoftwareSurface()->GetHeight();
    }
    else if (m_pD2DBitmap)
    {
        return m_pD2DBitmap->GetHeight();
    }
    else
    {
        return GetHardwareHeight();
    }
}

XUINT32
ImageSurfaceWrapper::GetHardwareWidth() const
{
    if (m_pImageSurfaceHardware != NULL)
    {
        return m_pImageSurfaceHardware->GetWidth();
    }
    else if (HasRetainedSize())
    {
        return m_retainedWidth;
    }
    else
    {
        return 0;
    }
}

XUINT32
ImageSurfaceWrapper::GetHardwareHeight() const
{
    if (m_pImageSurfaceHardware != NULL)
    {
        return m_pImageSurfaceHardware->GetHeight();
    }
    else if (HasRetainedSize())
    {
        return m_retainedHeight;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the image surface is opaque.
//
//------------------------------------------------------------------------
bool
ImageSurfaceWrapper::IsOpaque()
{
    // TODO: PC: opaqueness isn't implemented on HWTexture
    // Either we don't have both surfaces or their opaquenesses match
//    ASSERT(
//        m_pImageSurfaceSoftware == NULL
//        || m_pImageSurfaceHardware == NULL
//        || m_pImageSurfaceSoftware->IsOpaque() == m_pImageSurfaceHardware->IsOpaque()     // not impl on the hardware surface!
//        );

    IPALSurface *pSurface = GetSoftwareOrHardwareSurface();

    // D2D bitmaps are never opaque
    return pSurface != NULL ? pSurface->IsOpaque() : false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets both surfaces
//
//------------------------------------------------------------------------
void
ImageSurfaceWrapper::ResetSurfaces(
    bool mustKeepSoftwareSurface,
    bool mustKeepHardwareSurfaces
    )
{
    // mustKeepSoftwareSurface is intended to be used to indicate that the software
    // surface should be kept around when updating hardware resources.  However,
    // the software resource is reset here for cases like CacheMode="BitmapCache".
    // TODO: This function is a little bit weird right now.  It resets the
    //       software surface but keeps a flag to preserve it later when copying to
    //       hardware.  However, the hardware flag is meant to immediately preserve
    //       the hardware surface during a reset.  It might be best to split this into
    //       two functions next time there is an opportunity to refactor.
    ReleaseInterface(m_pImageSurfaceSoftware);
    m_mustKeepSystemMemory = mustKeepSoftwareSurface;

    if (!mustKeepHardwareSurfaces)
    {
        ReleaseInterface(m_pImageSurfaceHardware);
        ReleaseInterface(m_pD2DBitmap);

        if (m_hardwareMutex)
        {
            UnlockHardwareMutex();
        }
        m_isHardwareMutexValid = true;
    }

    m_isReferencingCache = false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the software surface. Called by the software walk. Can be
//      called with NULL.
//
//------------------------------------------------------------------------
void ImageSurfaceWrapper::SetSoftwareSurface(_In_opt_ IPALSurface *pSoftwareSurface)
{
    //
    // The UI thread sets the software surface. This has either happened as a result of the
    // source changing, or because the software surface was released after it was copied to
    // hardware.
    //
    // If the source changed, then both surfaces should have been released so there should
    // be no hardware surface.
    //
    // If this is the UI thread restoring a software surface that has been copied to hardware,
    // then that hardware surfaces must match the software surface in dimensions. We must also
    // be marked as always requiring a software surface.
    //

    // This assumed that we loaded the decoded software surface before the hardware one. That's no longer true.
    // We could have loaded the decoded hardware surface first from the surface cache.
    // TODO: Update this assert when the surface cache is merged in.
    //ASSERT(((m_pImageSurfaceHardware == NULL) && (m_pD2DBitmap == NULL)) || m_mustKeepSystemMemory);

    ReplaceInterface(m_pImageSurfaceSoftware, pSoftwareSurface);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the hardware surface. Used by SIS/VSIS.
//
//------------------------------------------------------------------------
void
ImageSurfaceWrapper::SetHardwareSurface(
    _In_ HWTexture *pHardwareSurface
    )
{
    FAIL_FAST_ASSERT(!IsHardwareLocked());
    ASSERT(m_pImageSurfaceHardware == NULL && pHardwareSurface != NULL);

    SetInterface(m_pImageSurfaceHardware, pHardwareSurface);
    m_lostHardwareResources = FALSE;

    m_isReferencingCache = false;
    m_forceRecreateHardwareResources = false;

    ResetRetainedSize();
}

void
ImageSurfaceWrapper::ResetRetainedSize()
{
    m_retainedSize = false;
    m_retainedWidth = 0;
    m_retainedHeight = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the hardware resources that we got from the surface cache
//
//------------------------------------------------------------------------
void
ImageSurfaceWrapper::UseCachedHardwareResources(_In_ ImageHardwareResources *pCachedResources)
{
    // Unlock any hardware mutex since the surface is going to be updated to a cached resource and then
    // invalidate the mutex so no lock can occur on a cached resource
    if (IsHardwareLocked())
    {
        UnlockHardwareMutex();
    }
    m_isHardwareMutexValid = false;

    if (pCachedResources->m_pSurfaceNoRef != NULL)
    {
        // Guard against setting it to the same resource
        if (pCachedResources->m_pSurfaceNoRef != m_pImageSurfaceHardware)
        {
            ReleaseInterface(m_pImageSurfaceHardware);

            SetHardwareSurface(pCachedResources->m_pSurfaceNoRef);

            // Mark this surface as referencing cached hardware resources.
            // This must be done after SetHardwareSurface which will set the field to false.
            m_isReferencingCache = true;
        }
    }
    else
    {
        ASSERT(FALSE); // Add support for other surface types
    }

    // If we've transferred the software bits to hardware, try to opportunistically
    // release the software bits.
    ReleaseSoftwareSurfaceIfAllowed();
}

void ImageSurfaceWrapper::SetD2DBitmap(_In_ IPALAcceleratedBitmap *pD2DBitmap)
{
    ASSERT(m_pImageSurfaceSoftware != nullptr && m_pD2DBitmap == nullptr);

    SetInterface(m_pD2DBitmap, pD2DBitmap);

    // If we've transferred the software bits to hardware, try to opportunistically
    // release the software bits.
    ReleaseSoftwareSurfaceIfAllowed();

    ResetRetainedSize();
}

// Gets a list of HW surfaces and the rects associated with them so that
// background thread decoding can decode directly to them
_Check_return_ HRESULT
ImageSurfaceWrapper::GetHWSurfaceUpdateList(
    _In_ SurfaceUpdateList& surfaceUpdateList
    )
{
    HRESULT hr = S_OK;

    FAIL_FAST_ASSERT(!IsHardwareLocked());

    ASSERT(surfaceUpdateList.empty());

    if (m_pImageSurfaceHardware)
    {
        if (m_pImageSurfaceHardware->IsVirtual())
        {
            // Surface update list should consist of the same virtual surface with different tile size update regions.
            auto imageWidth = m_pImageSurfaceHardware->GetWidth();
            auto imageHeight = m_pImageSurfaceHardware->GetHeight();

            for (uint32_t tileY = 0; tileY < imageHeight; tileY += SWAlphaMaskAtlas::TileSize)
            {
                auto tileHeight = MIN(SWAlphaMaskAtlas::TileSize, imageHeight - tileY);

                for (uint32_t tileX = 0; tileX < imageWidth; tileX += SWAlphaMaskAtlas::TileSize)
                {
                    auto tileWidth = MIN(SWAlphaMaskAtlas::TileSize, imageWidth - tileX);

                    XRECT rect = { tileX, tileY, tileWidth, tileHeight };

                    xref_ptr<SurfaceDecodeParams> spNewTile = make_xref<SurfaceDecodeParams>(
                        rect,
                        m_pImageSurfaceHardware);
                    surfaceUpdateList.push_back(spNewTile);
                }
            }
        }
        else
        {
            XRECT rect = { 0, 0, GetWidth(), GetHeight() };

            xref_ptr<SurfaceDecodeParams> spNewTile = make_xref<SurfaceDecodeParams>(
                rect,
                m_pImageSurfaceHardware);
            surfaceUpdateList.push_back(spNewTile);
        }
    }
    else
    {
        // The hardware surfaces should have been pre-allocated with AllocateHardwareResources() prior
        // to calling this function
        FAIL_FAST_ASSERT(false);
    }

//Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
// Update the hardware resources from the software surface, assuming they are
// already created.  Overwrites existing bits.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSurfaceWrapper::UpdateSurfaceFromSoftware(_In_ HWTextureManager *pTextureManager)
{
    if (GetSoftwareSurface())
    {
        FAIL_FAST_ASSERT(!IsHardwareLocked());
        ASSERT(HasHardwareSurfaces());
        ASSERT(DoesHardwareSurfaceMatch(pTextureManager));

        if (m_pImageSurfaceHardware != NULL)
        {
            XPOINT srcOrigin = { 0, 0 };
            // This update could potentially fail when locking the hardware surface to update it.  This can also be
            // handled gracefully in the device lost scenario.
            IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(m_pImageSurfaceHardware->UpdateTextureFromSoftware(GetSoftwareSurface(), srcOrigin));
        }
        else
        {
            FAIL_FAST_ASSERT(false);  // Other types of hardware resources not supported
        }
        ReleaseSoftwareSurfaceIfAllowed();
    }

    return S_OK;
}

static HWTextureFlags ComputeHWTextureFlags(unsigned int width, unsigned int height, bool opaque, bool forceVirtual, uint32_t maxTextureSize)
{
    uint32_t flags = HWTextureFlags_IncludePadding;

    if (opaque)
    {
        flags |= HWTextureFlags_IsOpaque;
    }

    if (forceVirtual || width > maxTextureSize || height > maxTextureSize)
    {
        flags |= HWTextureFlags_IsVirtual;
    }

    return static_cast<HWTextureFlags>(flags);
}

// Check if the size and format of the existing HW surface match the existing SW surface
// so that we could reuse the existing HW surface to copy image bits from the existing SW surface.
bool ImageSurfaceWrapper::DoesHardwareSurfaceMatch(_In_ HWTextureManager* hwTextureManager) const
{
    if (m_pImageSurfaceSoftware != nullptr)
    {
        HWTextureFlags flags = ComputeHWTextureFlags(
            GetWidth(),
            GetHeight(),
            !!m_pImageSurfaceSoftware->IsOpaque(),
            false /* forceVirtual */,
            hwTextureManager->GetMaxTextureSize()
            );

        if ((m_pImageSurfaceHardware != nullptr) &&
            (m_pImageSurfaceHardware->GetWidth() == GetWidth()) &&
            (m_pImageSurfaceHardware->GetHeight() == GetHeight()) &&
            (m_pImageSurfaceHardware->IsVirtual() == !!(flags & HWTextureFlags_IsVirtual)) &&
            (m_pImageSurfaceHardware->IsOpaque() == !!(flags & HWTextureFlags_IsOpaque)) &&
            (m_pImageSurfaceHardware->GetPixelFormat() == m_pImageSurfaceSoftware->GetPixelFormat()))
        {
            return true;
        }
    }

    return false;
}

_Check_return_ HRESULT
ImageSurfaceWrapper::AllocateHardwareResources(
    _In_ HWTextureManager *pHWTextureManagerNoRef,
    unsigned int width,
    unsigned int height,
    PixelFormat pixelFormat,
    bool opaque,
    bool forceVirtual,
    bool persistent
    )
{
    ASSERT(pHWTextureManagerNoRef != nullptr);

    FAIL_FAST_ASSERT(!IsHardwareLocked());

    ASSERT(pixelFormat == pixelGray8bpp
        || pixelFormat == pixelColor32bpp_A8R8G8B8
        || pixelFormat == pixelColor64bpp_R16G16B16A16_Float);

    HWTextureFlags flags = ComputeHWTextureFlags(width, height, opaque, forceVirtual, pHWTextureManagerNoRef->GetMaxTextureSize());

    if (m_pImageSurfaceHardware == nullptr)
    {
        xref_ptr<HWTexture> newTexture;

        // Only fail fast if it wasn't a device lost scenario which can be recovered.
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pHWTextureManagerNoRef->CreateTexture(
            pixelFormat,
            width,
            height,
            flags,
            newTexture.ReleaseAndGetAddressOf()));

        // Persistent textures are allowed to stay alive during device lost
        newTexture->SetIsPersistent(persistent);

        SetHardwareSurface(newTexture.get());
    }
    else
    {
        // Cannot change creation flags so ensure we have the most generic ones already set
        FAIL_FAST_ASSERT(m_pImageSurfaceHardware->IsVirtual() || !(flags & HWTextureFlags_IsVirtual));
        FAIL_FAST_ASSERT(!m_pImageSurfaceHardware->IsOpaque() || !!(flags & HWTextureFlags_IsOpaque));
        FAIL_FAST_ASSERT(m_pImageSurfaceHardware->GetPixelFormat() == pixelFormat);

        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pHWTextureManagerNoRef->EnsureLegacyDeviceSurface(m_pImageSurfaceHardware, width, height));

        m_lostHardwareResources = FALSE;
    }

    // At this point we should have created a texture
    ASSERT(m_pImageSurfaceHardware != nullptr);

    return S_OK;
}

// Allocate a HWTexture and WinRT surface wrapper but no hardware surface.
void ImageSurfaceWrapper::AllocateHWTextureWithNoHardware(_In_ HWTextureManager* hwTextureManager, bool isVirtual)
{
    xref_ptr<HWTexture> newTexture = hwTextureManager->CreateTextureWithNoHardware(isVirtual);

    SetHardwareSurface(newTexture.get());
}

//------------------------------------------------------------------------
//
//  Synopsis:
// Create hardware texture as necessary.  Return what we created
// so we can cache it in the SurfaceCache in case other images use the same source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSurfaceWrapper::EnsureHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    _Outptr_opt_ ImageHardwareResources**ppResources
    )
{
    if (HasSoftwareSurface() && (m_forceRecreateHardwareResources || !DoesHardwareSurfaceMatch(pTextureManager)))
    {
        CleanupDeviceRelatedResources();

        IFC_RETURN(AllocateHardwareResources(
            pTextureManager,
            GetWidth(),
            GetHeight(),
            m_pImageSurfaceSoftware->GetPixelFormat(),
            !!m_pImageSurfaceSoftware->IsOpaque()));
    }

    if (ppResources != nullptr)
    {
        *ppResources = nullptr;
        if (m_pImageSurfaceHardware != nullptr)
        {
            *ppResources = new ImageHardwareResources(m_pImageSurfaceHardware);
        }

        // Cacheable hardware resources have been returned, the ImageSurfaceWrapper
        // is no longer able to lock the hardware resources.
        m_isHardwareMutexValid = false;
    }

    return S_OK;
}

// Ensures that any hardware updates have been queued.  An explicit step to inform DComp of a
// texture update may be necessary if the hardware surfaces were updated directly such as with
// background image decoding.
void
ImageSurfaceWrapper::UpdateHardwareResources()
{
    // This function used to update the hardware texture if it was necessary but a lot of that
    // functionality is now handled after copying on the background thread.  This function
    // keeps track of state now.
    m_isUpdateRequired = !(m_isReferencingCache || HasHardwareSurfaces());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the software surface if we already have a hardware
//      surface (either for PC or D2D) and no longer need the software one.
//
//------------------------------------------------------------------------
void
ImageSurfaceWrapper::ReleaseSoftwareSurfaceIfAllowed()
{
    if (HasHardwareSurfaces() &&
        m_pImageSurfaceSoftware != NULL &&
        !m_mustKeepSystemMemory)
    {
        ReleaseInterface(m_pImageSurfaceSoftware);

        //TRACE(TraceAlways, L"Software surface released.");
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether the hardware surface is discarded.
//
//------------------------------------------------------------------------
bool
ImageSurfaceWrapper::CheckForLostHardwareResources()
{
    if (!m_lostHardwareResources)
    {
        if (HasLostHardwareResources())
        {
            m_lostHardwareResources = TRUE;

            if (m_pImageSurfaceHardware != nullptr)
            {
                if (!m_pImageSurfaceHardware->IsPersistent())
                {
                    ReleaseInterface(m_pImageSurfaceHardware);
                }
            }

            m_isUpdateRequired = HasSoftwareSurface();

            m_isReferencingCache = false;
        }
    }
    else
    {
        ASSERT(m_pImageSurfaceHardware == nullptr || m_pImageSurfaceHardware->IsPersistent());
    }
    return m_lostHardwareResources;
}

bool
ImageSurfaceWrapper::HasLostHardwareResources()
{
    return
        m_lostHardwareResources ||
        ((m_pImageSurfaceHardware != nullptr) && m_pImageSurfaceHardware->IsSurfaceLost());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if there is a valid hardware surface
//
//------------------------------------------------------------------------
bool
ImageSurfaceWrapper::CheckForHardwareResources()
{
    if (m_pImageSurfaceHardware == NULL &&
        m_pD2DBitmap == NULL)
    {
        return false;
    }
    return (!CheckForLostHardwareResources());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the hardware surface, which was created on a device that has been lost.
//
//------------------------------------------------------------------------
void
ImageSurfaceWrapper::ReleaseHardwareResources()
{
    ASSERT(m_pImageSurfaceHardware);

    // D2D is no longer enabled
    ASSERT(m_pD2DBitmap == NULL);

    ReleaseInterface(m_pImageSurfaceHardware);

    // If the hardware resources are released and there is a software
    // surface available then it needs to be marked as requiring an update.
    // If no software surface is available, then it will be marked for update
    // during the next Decode.
    m_isUpdateRequired = HasSoftwareSurface();

    m_isReferencingCache = false;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Handle Process Lifetime Suspend event
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSurfaceWrapper::OnSuspend(_In_ bool isTriggeredByResourceTimer)
{
    HRESULT hr = S_OK;
    // Offer the memory for the software surface.  These are normally released when
    // the hardware resources are created...ie when the image is rendered. But it can happen
    // that  the image has been decoded but not yet rendered (or we've lost the hardware surfaces)
    // For example if the image is not in the tree, or if the app has bad luck and gets
    // suspended between the decode finishing and the image being rendered
    if (m_pImageSurfaceSoftware != NULL &&
        !m_mustKeepSystemMemory)
    {
        // If we can't offer, we are an inconsistent state...fail fast
        XCP_FAULT_ON_FAILURE(SUCCEEDED(m_pImageSurfaceSoftware->Offer()));
        TraceOfferSystemMemorySurfaceInfo(L"Software surface offered.");
        //TRACE(TraceAlways, L"Software surface offered.");
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Handles Process Lifetime Resume event
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSurfaceWrapper::OnResume()
{
    if (m_pImageSurfaceSoftware != NULL)
    {
        bool bContentsDiscarded = false;
        // If we can't reclaim we are in an inconsistent state so fail fast
        XCP_FAULT_ON_FAILURE(SUCCEEDED(m_pImageSurfaceSoftware->Reclaim(&bContentsDiscarded)));

        if (bContentsDiscarded)
        {
            // If we lost the contents we could (should) decode into the same memory
            // but on the assumption that decode cost will swamp memory allocation cost,
            // it is simpler and more reliable to release the surface object and let the normal
            // asynchronous decode path work
            // TODO:  Consider reusing the memory
            ReleaseInterface(m_pImageSurfaceSoftware);
            TraceReclaimSystemMemorySurfaceInfo(L"Software surface contents discarded.");
            //TRACE(TraceAlways, L"Software surface contents discarded.");
        }
        else
        {
            TraceReclaimSystemMemorySurfaceInfo(L"Software surface reclaimed.  Contents preserved.");
            //TRACE(TraceAlways, L"Software surface reclaimed.  Contents preserved.");
        }
    }

    CheckForLostHardwareResources();

    RRETURN(S_OK);
}

void ImageSurfaceWrapper::OnLowMemory()
{
    if (m_releaseSoftwareSurfaceOnLowMemory)
    {
        ReleaseInterface(m_pImageSurfaceSoftware);
    }
}

void ImageSurfaceWrapper::CleanupDeviceRelatedResources()
{
    if (m_pImageSurfaceHardware != NULL)
    {
        m_lostHardwareResources = TRUE;

        uint32_t width = GetWidth();
        uint32_t height = GetHeight();

        if ((width != 0) && (height != 0))
        {
            // Store the size of the hardware surface  for layout to use for elements.
            // The retained size will be reset when a new surface is set.  If a software
            // surface is available, that will be provided instead via GetWidth/GetHeight.
            m_retainedSize = true;
            m_retainedWidth = width;
            m_retainedHeight = height;
        }
    }
    ReleaseInterface(m_pImageSurfaceHardware);
    UnregisterForDeviceCleanup();

    m_isUpdateRequired = HasSoftwareSurface();

    m_isReferencingCache = false;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Registers self for device related cleanup if needed.
//
//-----------------------------------------------------------------------------
void
ImageSurfaceWrapper::RegisterForDeviceCleanup()
{
    ASSERT(!m_deviceCleanupListEntry.IsOnList());
    if (m_pImageSurfaceHardware != NULL)
    {
        m_pCoreNoRef->RegisterSurfaceWrapperForDeviceCleanup(this);
        m_deviceCleanupTimestamp = ::GetTickCount64();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Unregisters self from device related cleanup (if registered earlier)
//
//-----------------------------------------------------------------------------
void
ImageSurfaceWrapper::UnregisterForDeviceCleanup()
{
    if (m_deviceCleanupListEntry.IsOnList())
    {
        m_pCoreNoRef->UnregisterSurfaceWrapperForDeviceCleanup(this);
        m_deviceCleanupTimestamp = 0LL;
    }
}

// Returns the estimated commit size in bytes of the surfaces wrapped by this object
std::uint64_t ImageSurfaceWrapper::GetEstimatedSurfaceCommitSize() const
{
    std::uint64_t size = 0;

    auto softwareSurface = GetSoftwareSurface();
    if (softwareSurface != nullptr)
    {
        size += (std::uint64_t)softwareSurface->GetWidth() * (std::uint64_t)softwareSurface->GetHeight() * (std::uint64_t)::GetPixelStride(softwareSurface->GetPixelFormat());
    }

    auto hardwareSurface = GetHardwareSurface();
    if (hardwareSurface != nullptr)
    {
        auto dcompSurface = hardwareSurface->GetCompositionSurface();
        if (dcompSurface)
        {
            size += (std::uint64_t)dcompSurface->GetTextureSizeInBytes();
        }
    }

    return size;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the hardware surfaces in this cache have been
//      lost.
//
//------------------------------------------------------------------------
bool
ImageHardwareResources::IsDiscarded()
{
    return m_pSurfaceNoRef != NULL && m_pSurfaceNoRef->IsSurfaceLost();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//    The SurfaceCache needs to be notified when the hardware resource it is caching
// is deleted.  The ImageHardwareResources object is just a thin wrapper holding one
// of the resource types we want, each of which sends notifications when it is deleted, so this
// function just digs through the contents and finds which one we need to listen to.
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageHardwareResources::GetNotifyOnDelete(
    _Outptr_ CNotifyOnDelete **ppNotifyOnDelete
    )
{
    if (m_pSurfaceNoRef != NULL)
    {
        IFC_RETURN(m_pSurfaceNoRef->GetNotifyOnDelete(ppNotifyOnDelete));
    }
    else
    {
        // TODO -- Support D2D resources
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}
