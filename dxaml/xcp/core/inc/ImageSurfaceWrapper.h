// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//
//      Definition of an ImageSurfaceWrapper object.
//
// Notes:  Images are stored in different ways at different points in their lifetime and depending
// on their size and what underlying graphics technology they use.  This class wraps up some of
// this state and adds helper functions that make the various options more uniform.  As of 10/25/2011
// this is leaky abstraction and could be further unified, but it is a lot of work to do without sacrificing
// performance or breaking the existing code.
//  Basically, most images start with a SoftwareSurface.  This is simply a chunk of system memory that
// contains the bits of the image as originally presented to the UI layer.  Most likely these are the decoded
// image bits for a bitmap, but they may instead by an array of bytes set by the user (as in WriteableBitmap).
// During the render walk we create HardwareResources for this SoftwareSurface.  The implementation
// of these is a little bit abstracted by HardwareTextureManager, but represents the bits as they are going to
// be used by the compositor (a better name than HardwareSurface might be CompositorSurface:
// these may not be (are currently not (10/25/2011)) GPU resources but simply
// system memory most likely allocated from an Atlas, that will be used to populate GPU resources).
// When D2D rendering is implemented we may instead contain a D2D Bitmap.
// If the ImageSurfaceWrapper contains either a HardwareSurface or a D2D Bitmap we say it HasHardwareResources.
//
// SurfaceImageSources never have a SoftwareSurface (except a 1x1 white pixel used to keep the
// abstraction for cases like jail where the code expects to get back a software surface).
//
// The overall image hierarchy is fairly complex.  For example, an Image is a FrameworkElement.  It
// contains ImageSource.  ImageSource contains an ImageSurfaceWrapper that it turn abstracts
// out the various backing stores mentioned above.  ImageSource can also go into an ImageBrush.
// Meanwhile, the SurfaceCache holds ImageHardwareResources objects which have weak references
// to the same objects that ImageSurfaceWrapper holds.

class HWTexture;
class SurfaceDecodeParams;
using SurfaceUpdateList = std::vector<xref_ptr<SurfaceDecodeParams>>;

//----------------------------------------------------------------------------------
// We need to cache hardware resources so they can be shared between images rather
// than recreated each time (see the SurfaceCache class in hw).  ImageHardwareResources
// holds the resources we need to cache.  SurfaceCache does not need to be aware of what
// is in this class (IPALSurface, D2DBitmap, etc.) but merely needs to stick them
// in its store and take them out when they are deleted.  ImageSurfaceWrapper can look inside
// to get the resources it wants.
// One might think ImageSurfaceWrapper should hold on to an ImageHardwareResources object,
// but the entry in the SurfaceCache must not ref the resources it holds, otherwise they will
// never get deleted, because only when deleted to they get removed from the cache:  Catch-22.
// This type lives here rather than alongside SurfaceCache, because it is ImageSurfaceWrapper, not
// SurfaceCache that needs to look inside it
//
// For BackgroundThreadImageLoading feature, it is okay for this object to have a
// reference to the hardware surface based on the current implementation.  Currently it
// uses the methods IsSurfaceLost and GetNotifyOnDelete which doesn't touch the internal
// surface contents outside accessing the backing DComp surface.
// Additionally, the ImageSurfaceWrapper should not provide an ImageHardwareResources
// for caching until the lock is released.  After an ImageHardwareResources has been
// provided, it can no longer be locked.
//----------------------------------------------------------------------------------
class ImageHardwareResources :  public CInterlockedReferenceCount
{
friend class ImageSurfaceWrapper;
public:
    ImageHardwareResources(_In_ HWTexture *pSurface) :
        m_pSurfaceNoRef(pSurface)
    {
        XCP_WEAK(&m_pSurfaceNoRef);
    }

    _Check_return_ HRESULT GetNotifyOnDelete(_Outptr_ CNotifyOnDelete **ppNotifyOnDelete);

    bool IsDiscarded();

private:
    ~ImageHardwareResources() override
    {
        m_pSurfaceNoRef = NULL;
    }

private:
    _Maybenull_ HWTexture *m_pSurfaceNoRef;
    // _Maybenull_ IPALAcceleratedBitmap *m_pD2DBitmap; // When we reenable D2D we also need to add support for these
};


class ImageSurfaceWrapper : public CXcpObjectBase<>,
                        IPLMListener
{
public:
    static _Check_return_ HRESULT
    Create(
        _In_ CCoreServices *pCore,
        bool mustKeepSoftwareSurface,
        _Outptr_ ImageSurfaceWrapper **ppImageSurface
        );

    ImageSurfaceWrapper(_In_ CCoreServices *pCore,
                             bool mustKeepSoftwareSurface);
    ~ImageSurfaceWrapper() override;

    _Ret_maybenull_ HWTexture* GetHardwareSurface() const { return m_hardwareMutex ? nullptr : m_pImageSurfaceHardware; }
    _Ret_maybenull_ IPALSurface* GetSoftwareSurface() const { return m_pImageSurfaceSoftware; }
    _Ret_maybenull_ IPALAcceleratedBitmap* GetD2DBitmap() const { return m_pD2DBitmap; }

    PixelFormat GetPixelFormat() const;
    XUINT32 GetWidth() const;
    XUINT32 GetHeight() const;
    XUINT32 GetHardwareWidth() const;
    XUINT32 GetHardwareHeight() const;
    bool HasRetainedSize() const { return m_retainedSize; }
    bool IsOpaque();
    bool HasAnySurface() { return HasSoftwareSurface() || HasHardwareSurfaces(); }
    bool MustKeepSystemMemory() { return m_mustKeepSystemMemory; }

    void ResetSurfaces(
        bool mustKeepSoftwareSurface,
        bool mustKeepHardwareSurfaces
        );

    void SetSoftwareSurface(_In_opt_ IPALSurface *pSoftwareSurface);

    void SetHardwareSurface(_In_ HWTexture *pHWSurface);

    void UseCachedHardwareResources(_In_ ImageHardwareResources *pCachedResources);

    void SetD2DBitmap(
        _In_ IPALAcceleratedBitmap *pHardwareSurface
        );

    void SetKeepSystemMemory() { m_mustKeepSystemMemory = TRUE; }

    bool CheckForHardwareResources();

    // Helper method to see if hardware surfaces are set or not. Mostly used in debug asserts.
    bool HasSoftwareSurface() { return m_pImageSurfaceSoftware != nullptr; }
    bool HasHardwareSurfaces()
    {
        return
            (m_pImageSurfaceHardware != nullptr ||
            m_pD2DBitmap != nullptr);
    }

    _Check_return_ HRESULT GetHWSurfaceUpdateList(_In_ SurfaceUpdateList& surfaceUpdateList);

    bool DoesHardwareSurfaceMatch(_In_ HWTextureManager* hwTextureManager) const;

    _Check_return_ HRESULT AllocateHardwareResources(
        _In_ HWTextureManager *pHWTextureManagerNoRef,
        unsigned int width,
        unsigned int height,
        PixelFormat pixelFormat,
        bool opaque,
        bool forceVirtual = false,
        bool persistent = false);

    void AllocateHWTextureWithNoHardware(_In_ HWTextureManager* hwTextureManager, bool isVirtual);

    _Check_return_ HRESULT EnsureHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _Outptr_opt_ ImageHardwareResources **ppResources);

    _Check_return_ HRESULT UpdateSurfaceFromSoftware(
        _In_ HWTextureManager *pHWTextureManagerNoRef);

    void UpdateHardwareResources();

    _Check_return_ HRESULT LockHardwareMutex()
    {
        if (m_isHardwareMutexValid)
        {
            m_hardwareMutex = true;
            return S_OK;
        }
        else
        {
            return E_ACCESSDENIED;
        }
    }

    void UnlockHardwareMutex()
    {
        ASSERT(m_hardwareMutex);
        m_hardwareMutex = false;
    }

    bool IsHardwareLocked() { return m_hardwareMutex; }

    void SetUpdateRequired(bool requiresUpdate) { m_isUpdateRequired = requiresUpdate; }

    bool IsUpdateRequired() { return m_isUpdateRequired; }

    bool CheckForLostHardwareResources();

    bool HasLostHardwareResources();

    void ReleaseHardwareResources();

    // Implement IPLMListener methods
    _Check_return_ HRESULT OnSuspend(_In_ bool isTriggeredByResourceTimer) override;
    _Check_return_ HRESULT OnResume() override;
    void OnLowMemory() override;

    void CleanupDeviceRelatedResources();
    void RegisterForDeviceCleanup();
    void UnregisterForDeviceCleanup();
    IntrusiveList<ImageSurfaceWrapper>::ListEntry* GetDeviceCleanupLink()
    {
        return &m_deviceCleanupListEntry;
    }
    static XUINT32 DeviceCleanupLinkOffset()
    {
        return OFFSET(ImageSurfaceWrapper, m_deviceCleanupListEntry);
    }

    UINT64 GetDeviceCleanupTimestamp() const
    {
        return m_deviceCleanupTimestamp;
    }

    void SetForceRecreateHardwareResources()
    {
        m_forceRecreateHardwareResources = true;
    }

    std::uint64_t GetEstimatedSurfaceCommitSize() const;

private:

    void ReleaseSoftwareSurfaceIfAllowed();

    _Ret_maybenull_ IPALSurface* GetSoftwareOrHardwareSurface();

    void ResetRetainedSize();

    CCoreServices *m_pCoreNoRef;
    IPALSurface *m_pImageSurfaceSoftware;   // System memory surface
    HWTexture *m_pImageSurfaceHardware;     // Device texture surface
    IPALAcceleratedBitmap *m_pD2DBitmap;    // D2D bitmap
    UINT64 m_deviceCleanupTimestamp;        // Timestamp indicating when the surface wrapper was registered for device cleanup
    std::atomic<bool> m_hardwareMutex;      // Synchronization lock to provide exclusive access to the hardware resources
    bool m_isHardwareMutexValid;            // Determines whether the hardware surface can be locked
                                            // It will not be valid/usable after the hardware surface has been cached
                                            // This is to ensure there will never be contention over cached hardware resources

    // Indicates that the user of this object is able to restore the SW surface in case it's lost.
    // Usually we will respect m_mustKeepSystemMemory and keep the SW bits but not under low memory conditions
    // where we consider m_releaseSoftwareSurfaceOnLowMemory flag instead.
    bool m_releaseSoftwareSurfaceOnLowMemory;

    // The retained fields store the size of the image if the hardware resources were cleaned up or lost
    uint32_t m_retainedWidth;
    uint32_t m_retainedHeight;
    bool m_retainedSize;

    // This flag tracks the update state of the hardware resources in the ImageSurfaceWrapper.
    // Prior to background thread image loading, the update state was essentially
    // (m_pImageSurfaceSoftware != nullptr) && (m_pImageSurfaceHardware == nullptr).  That assumption
    // is no longer valid since the hardware surface can be decoded to directly without the use of a software
    // surface.
    //
    // This flag is set to true when a decode (whether directly to a hardware or software surface)
    // is complete.  It can also be set to true when hardware resources are released (possible due to device lost)
    // and there is a software surface available for update.  In the scenario that there are no hardware or software
    // surfaces available then CBitmapImage::ReloadImage will initiate a decode to re-populate the hardware or
    // software surface (depending on the constraints around decoding directly to hardware) which will
    // SetUpdateRequired(true) when the decode is complete in CImageSource::OnImageAvailableCommon.
    //
    // It is set to false after the hardware surface has been updated.  It could be updated in one of three ways:
    //   1. Software surface is available so it will EnsureHardwareResources, then UpdateSurfaceFromSoftware.
    //   2. Hardware surface is available in which case UpdateHardwareResources will ensure the updates are queued
    //      to dcomp.
    //   3. Hardware resources have been cached in which case it will call UseCachedHardwareResources
    //
    // Scenarios 1 and 2 will add the resources to the hardware cache.
    // This flag is set to false in scenarios 1,2 and 3 through the UpdateHardwareResources method which
    // checks that hardware resources are available and that their updates have been pushed to DComp.
    bool m_isUpdateRequired;

    // This flag tracks if the image hardware resources were set using UseCachedHardwareResources.  Images
    // that are referencing a cached hardware resource should never call update on that resource since
    // another thread may be operating on it and caching is the only means of sharing that resource.
    bool m_isReferencingCache;

    // When an element owning the surface wrapper leaves the tree,
    // it tracks the surface wrapper for device loss cleanup.
    // When it enters the tree again or when the surface wrapper gets
    // deleted, the entry from the cleanup list needs to be removed.
    // This list entry is used to perform such list cleanups in an efficient manner.
    IntrusiveList<ImageSurfaceWrapper>::ListEntry m_deviceCleanupListEntry;

    // TODO: PC: the sizes of these surfaces don't have to match due to lod. The software
    // surface could be a LocalMemorySurface at full size while the hardware surface is a scaled
    // down version. So getting the surface width is more complicated than getting either surface
    // and returning the width. It depends on who is asking and what they'll do with the width.
    // Once this is fixed, the size asserts can be added back.
    // See Silverlight 101111 <PC: HWTexture::GetWidth/Height include LOD>

    //
    // For images copied to video memory for hardware rendering (either D2D or compositor)
    // usually the system memory surface can be released once we copy it into video memory.
    // But the system memory copy may be needed later
    // (either by the SW rasterizer, or to download the image to video memory for either the compositor or D2D)
    // UI thread as well, then we'll need to keep both surfaces. The heuristic for now is
    // to assume that the image is used only by one component (SW rasterizer, D2D, or compositor)
    // If the SW rasterizer ever needs to use the bitmap, then this bit is set
    // which indicates that the system memory copy should not be released
    //
    bool m_mustKeepSystemMemory : 1;
    bool m_lostHardwareResources : 1;

    bool m_forceRecreateHardwareResources : 1;
};


