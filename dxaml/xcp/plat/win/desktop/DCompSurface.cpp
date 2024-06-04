// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <GraphicsUtility.h>
#include <DCompTreeHost.h>
#include <MUX-ETWEvents.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <PixelFormat.h>

extern UINT32 g_uD3D11TextureMemoryUsage;
extern UINT32 g_uD3D11TextureMemoryUsageNPOT;

using namespace RuntimeFeatureBehavior;

bool DCompSurface::s_traceDCompSurfacesInitialized = false;
bool DCompSurface::s_traceDCompSurfaces = false;
bool DCompSurface::s_traceDCompSurfaceUpdates = false;

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a DCompSurface object using XAML's D3D device
//
//-------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
DCompSurface::Create(
    _In_ CD3D11Device *pDevice,
    _In_ DCompTreeHost *pDCompHost,
    bool isOpaque,
    bool fAlphaMask,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    _Outptr_ DCompSurface** ppTexture
    )
{
    IFC_RETURN(pDCompHost->UpdateAtlasHintForRequest(widthWithoutGutters, heightWithoutGutters));

    xref_ptr<DCompSurface> texture;
    texture.init(new DCompSurface(
        widthWithoutGutters,
        heightWithoutGutters,
        isOpaque,
        fAlphaMask,
        isVirtual ? FALSE : TRUE, /* includesGutters, virtual doesn't support gutter drawing */
        pDCompHost->HasNative8BitSurfaceSupport(),
        isHDR,
        requestAtlas
        ));

    IFC_RETURN(texture->InitializeSurface(pDevice, pDCompHost, isVirtual));

    *ppTexture = texture.detach();

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a DCompSurface object for SIS/VSIS using the surface factory
//      for the app's D3D device
//
//-------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
DCompSurface::Create(
    _In_ DCompTreeHost *pDCompHost,
    _In_ IDCompositionSurfaceFactory *pSurfaceFactory,
    bool isOpaque,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    _Outptr_ DCompSurface** ppTexture
    )
{
    IFC_RETURN(pDCompHost->UpdateAtlasHintForRequest(widthWithoutGutters, heightWithoutGutters));

    xref_ptr<DCompSurface> texture;
    texture.init(new DCompSurface(
        widthWithoutGutters,
        heightWithoutGutters,
        isOpaque,
        FALSE /*fAlphaMask*/,
        isVirtual ? FALSE : TRUE /* includesGutters, virtual doesn't support gutter drawing */,
        FALSE /* hasNative8BitSurfaceSupport - unused since this can never be an alpha mask */,
        isHDR,
        requestAtlas
        ));

    IFC_RETURN(texture->InitializeSurface(pDCompHost, pSurfaceFactory, isVirtual));

    *ppTexture = texture.detach();

    return S_OK;
}

// Create a DCompSurface and a WinRT surface wrapper but no actual hardware surface.
/* static */ xref_ptr<DCompSurface>
DCompSurface::CreateWithNoHardware(
    _In_ DCompTreeHost *dcompTreeHost,
    bool isVirtual
    )
{
    xref_ptr<DCompSurface> dcompSurface;
    dcompSurface.init(new DCompSurface(
        0,
        0,
        FALSE,  // isOpaque
        FALSE,  // fAlphaMask
        FALSE,  // includesGutters
        FALSE,  // hasNative8BitSurfaceSupport - unused since this can never be an alpha mask
        false,  // isHDR
        true    // requestAtlas
        ));

    IFCFAILFAST(dcompTreeHost->GetCompositorPrivate()->CreateCompositionSurfaceForDCompositionSurface(
        nullptr, &dcompSurface->m_spWinRTSurface));

    dcompSurface->m_isVirtual = isVirtual;

    return dcompSurface;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the underlying surface from the provided device.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::InitializeSurface(
    _In_ CD3D11Device *pDevice,
    _In_ DCompTreeHost *pDCompTreeHost,
    bool isVirtual
    )
{
    ASSERT(m_pCompositionSurface == NULL);
    m_d3dDevice.reset(pDevice);

    // DComp does not guarantee support for 8-bit textures. Support was checked when the DComp device was
    // initialized. If this surface is 8-bit, but 8-bit isn't natively supported, the underlying DComp surface will be
    // created as 32-bit and an extra copy will occur at upload time to do the format conversion (see FlushUpdates).
    DXGI_FORMAT compositionSurfaceFormat = GetCompositionSurfaceFormat();

    m_isVirtual = !!isVirtual;

    // If the device was lost when we initialized DCompTreeHost::EnsureResources, and that device lost HR was swallowed
    // by CCoreServices::GetMaxTextureSize, then we're left with a DCompTreeHost without a surface factory in it. Check
    // for one explicitly so we don't AV when we try to create the surface from a null factory. All three surface factories
    // are created and released together, so checking for just one of them is enough.
    if (!pDCompTreeHost->HasSurfaceFactory())
    {
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(DXGI_ERROR_DEVICE_REMOVED);
    }

    if (m_isVirtual)
    {
        ASSERT(m_requestAtlas);
        IFCFAILFAST(pDCompTreeHost->GetMainSurfaceFactory()->CreateVirtualSurface(
            GetWidthWithoutGutters(),   // DComp takes surface sizes without gutters
            GetHeightWithoutGutters(),
            compositionSurfaceFormat,
            m_fIsOpaque ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED,
            &m_pVirtualCompositionSurface
            ));

        SetInterface(m_pCompositionSurface, m_pVirtualCompositionSurface);
    }
    else
    {
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDCompTreeHost->GetMainSurfaceFactory2()->CreateSurface(
            GetWidthWithoutGutters(),   // DComp takes surface sizes without gutters
            GetHeightWithoutGutters(),
            compositionSurfaceFormat,
            m_fIsOpaque ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED,
            !m_requestAtlas,
            &m_pCompositionSurface
            ));

        //NOTE: If this is a shared surface this may inflate reported memory footprint.
        UpdateMemoryFootprint(TRUE);
    }

    if (m_spWinRTSurface == nullptr)
    {
        // Wrap with an ICompositionSurface
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDCompTreeHost->GetCompositorPrivate()->CreateCompositionSurfaceForDCompositionSurface(
            m_pCompositionSurface, &m_spWinRTSurface));
    }
    else
    {
        wrl::ComPtr<WUComp::ICompositionSurfaceWrapperPartner> winRTSurfacePartner;
        IFC_RETURN(m_spWinRTSurface.As(&winRTSurfacePartner));
        IFC_RETURN(winRTSurfacePartner->Reset(m_pCompositionSurface));
    }

    IFCFAILFAST(m_pCompositionSurface->QueryInterface(&m_pCompositionSurfacePrivate));

    // Since it's possible to reuse a DCompSurface across discards, reset m_discarded after re-initializing.
    m_discarded = FALSE;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the underlying surface from the provided device.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::InitializeSurface(
    _In_ DCompTreeHost *pDCompTreeHost,
    _In_ IDCompositionSurfaceFactory *pSurfaceFactory,
    bool isVirtual
    )
{
    ASSERT(m_pCompositionSurface == NULL);
    ASSERT(m_pVirtualCompositionSurface == NULL);

    ASSERT(!m_fAlphaMask);

    // Warning!  Do not fail-fast on errors in this function as we need to propagate
    // errors back through the public API calling us (from SIS/VSIS customers).
    // See RS1 bug #8991952
    if (isVirtual)
    {
        ASSERT(m_requestAtlas);
        IFC_RETURN(pSurfaceFactory->CreateVirtualSurface(
            GetWidthWithoutGutters(),   // DComp takes surface sizes without gutters
            GetHeightWithoutGutters(),
            DXGI_FORMAT_B8G8R8A8_UNORM, //GetDxgiFormat()
            m_fIsOpaque ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED,
            &m_pVirtualCompositionSurface
            ));

        SetInterface(m_pCompositionSurface, m_pVirtualCompositionSurface);

        // Wrap with an ICompositionSurface
        auto hr = pDCompTreeHost->GetCompositorPrivate()->CreateCompositionSurfaceForDCompositionSurface(
            m_pVirtualCompositionSurface, &m_spWinRTSurface);

        // TODO_WinRTSprites: Handle not implemented on DComp.  Don't fail hard, just ignore for now.
        //                              This should be removed later in RS2 after DComp support is working.
        if (hr != E_NOINTERFACE)
        {
            IFC_RETURN(hr);
        }

        // TODO: JCOMP: Memory footprint estimate for VSIS needs to happen in VSIS itself.
    }
    else
    {
        wrl::ComPtr<IDCompositionSurfaceFactoryPartner2> surfaceFactory2;
        VERIFYHR(pSurfaceFactory->QueryInterface(IID_PPV_ARGS(&surfaceFactory2)));
        IFC_RETURN(surfaceFactory2->CreateSurface(
            GetWidthWithoutGutters(),   // DComp takes surface sizes without gutters
            GetHeightWithoutGutters(),
            DXGI_FORMAT_B8G8R8A8_UNORM, //GetDxgiFormat()
            m_fIsOpaque ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED,
            !m_requestAtlas,
            &m_pCompositionSurface
            ));

        UpdateMemoryFootprint(TRUE);

        // Wrap with an ICompositionSurface
        IFC_RETURN(pDCompTreeHost->GetCompositorPrivate()->CreateCompositionSurfaceForDCompositionSurface(
            m_pCompositionSurface, &m_spWinRTSurface));
    }

    IFC_RETURN(m_pCompositionSurface->QueryInterface(&m_pCompositionSurfacePrivate));

    return S_OK;
}

DCompSurface::DCompSurface(
    XUINT32 width,
    XUINT32 height,
    bool isOpaque,
    bool fAlphaMask,
    bool includesGutters,
    bool hasNative8BitSupport,
    bool isHDR,
    bool requestAtlas
    )
    : m_widthWithoutGutters(width)
    , m_heightWithoutGutters(height)
    , m_includesGutters(includesGutters)
    , m_pSystemMemoryBitsForUIThread(NULL)
    , m_pCompositionSurface(NULL)
    , m_pVirtualCompositionSurface(NULL)
    , m_pCompositionSurfacePrivate(NULL)
    , m_fIsOpaque(isOpaque)
    , m_fAlphaMask(fAlphaMask)
    , m_hasNative8BitSurfaceSupport(hasNative8BitSupport)
    , m_cLocks(0)
    , m_discarded(FALSE)
    , m_hasDrawnAtleastOnce(FALSE)
    , m_isHDR(isHDR)
    , m_requestAtlas(requestAtlas)
{
    if (!s_traceDCompSurfacesInitialized)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        s_traceDCompSurfaceUpdates = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::TraceDCompSurfaceUpdates);
        s_traceDCompSurfaces = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::TraceDCompSurfaces);
        s_traceDCompSurfacesInitialized = true;
    }
}

DCompSurface::~DCompSurface()
{
    ReleaseLegacyDCompResources(false /* resetWucSurface */);

    DCompSurfaceMonitor::RemoveSurface(this);
}

// Release all D3D device-dependent resources.  This includes all "legacy" DComp resource references, but does not
// include the WUC surface (m_spWinRTSurface).  This surface can be managed separately from the legacy resources in
// order to preserve the object across device-loss scenarios.  This is optional and only done by certain features
// (currently, only LoadedImageSurface uses this capability).
// For consumers of this capability, a DCompSurface will re-create a new legacy surface after device recovery and attach it
// to the persistent WUC surface via a call to ICompositionSurfaceWrapperPartner::Reset(surface), see InitializeSurface().
//
// When called in a context not related to LIS device lost handling - specifically from DCompSurface destructor -
// we should not call ICompositionSurfaceWrapperPartner::Reset. This breaks the "set it and forget it" usages of
// DCompSurface where Xaml counts on COMP to keep a surface alive via the WUC wrapper (one example is ListViewItemChrome's
// management of alpha mask for RevealBorderBrush rendering). In such usages Xaml creates a DCompSurface,
// associates the underlying WUC surface wrapper with some Comp object (eg Visual), and then releases the DCompSurface
// since the real surface is now being held by DComp/DWM for rendering. If the DCompSurface dtor Reset the wrapper,
// the legacy surface would be lost and rendered transparent by DWM.
void DCompSurface::ReleaseLegacyDCompResources(bool resetWucSurface)
{
    ASSERT(m_cLocks == 0);

    // Keep m_spWinRTSurface so it can be reattached later
    if (m_spWinRTSurface != nullptr && resetWucSurface)
    {
        wrl::ComPtr<WUComp::ICompositionSurfaceWrapperPartner> winRTSurfacePartner;
        IFCFAILFAST(m_spWinRTSurface.As(&winRTSurfacePartner));
        HRESULT hrReset = winRTSurfacePartner->Reset(nullptr);

        // It's possible the app closed the surface out from under us.  Ignore RO_E_CLOSED in this situation.
        if (hrReset != RO_E_CLOSED)
        {
            IFCFAILFAST(hrReset);
        }
    }

    m_d3dDevice.reset();
    m_surfaceUpdates.clear();

    if (m_pVirtualCompositionSurface == nullptr)
    {
        UpdateMemoryFootprint(FALSE);
    }

    if (m_pSystemMemoryBitsForUIThread != NULL)
    {
        ReleaseInterface(m_pSystemMemoryBitsForUIThread);
    }

    ReleaseInterface(m_pCompositionSurface);
    ReleaseInterface(m_pVirtualCompositionSurface);
    ReleaseInterface(m_pCompositionSurfacePrivate);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Locks the surface for write. Does not account for gutters. If this
//      texture includes gutters, then they are returned in the address.
//      Used by HWTexture to fill the staging texture, including the gutter
//      copy.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::Lock(
    _Outptr_ void **ppAddress,
    _Out_ XINT32 *pnStride,
    _Out_ XUINT32 *puWidth,
    _Out_ XUINT32 *puHeight)
{
    HRESULT hr = S_OK;

    const XUINT32 validRgnWidth = GetWidthWithGutters();
    const XUINT32 validRgnHeight = GetHeightWithGutters();

    // For BackgroundThreadImageLoading, since the surface is locked
    // on a background thread, m_pSystemMemoryBitsForUIThread should
    // always be non-Null, otherwise synchronization for our pool
    // logic may be necessary.
    if (m_pSystemMemoryBitsForUIThread == nullptr)
    {
        IFC(m_d3dDevice->AllocateFromSysMemBitsPool(
            validRgnWidth,      // The staging texture needs gutters
            validRgnHeight,
            GetDxgiFormat(),
            m_hasNative8BitSurfaceSupport,
            &m_pSystemMemoryBitsForUIThread
            ));

        // Sizes should match
        ASSERT(m_pSystemMemoryBitsForUIThread->GetWidth() >= validRgnWidth);
        ASSERT(m_pSystemMemoryBitsForUIThread->GetHeight() >= validRgnHeight);
        ASSERT(m_pSystemMemoryBitsForUIThread->GetDxgiFormat() == GetDxgiFormat());

        // The surface should not be in the pool
        ASSERT(!m_pSystemMemoryBitsForUIThread->GetPoolLink()->IsOnList());
    }

    XUINT32 stagingWidth, stagingHeight;
    m_pSystemMemoryBitsForUIThread->Lock(
        ppAddress,
        pnStride,
        &stagingWidth,
        &stagingHeight
        );

    ASSERT(stagingWidth >= validRgnWidth);
    ASSERT(stagingHeight >= validRgnHeight);

    *puWidth = validRgnWidth;
    *puHeight = validRgnHeight;

    m_lockRegion.X = 0;
    m_lockRegion.Y = 0;
    m_lockRegion.Width = GetWidthWithoutGutters();
    m_lockRegion.Height = GetHeightWithoutGutters();

    m_cLocks++;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DCompSurface::LockRect(
    _In_ const XRECT& lockRegion,
    _Outptr_ void** address,
    _Out_ int32_t* stride,
    _Out_ uint32_t* width,
    _Out_ uint32_t* height
    )
{
    // For BackgroundThreadImageLoading, since the surface is locked
    // on a background thread, m_pSystemMemoryBitsForUIThread should
    // always be non-Null, otherwise synchronization for our pool
    // logic may be necessary.
    if (m_pSystemMemoryBitsForUIThread == nullptr)
    {
        IFC_RETURN(m_d3dDevice->AllocateFromSysMemBitsPool(
            lockRegion.Width,
            lockRegion.Height,
            GetDxgiFormat(),
            m_hasNative8BitSurfaceSupport,
            &m_pSystemMemoryBitsForUIThread
            ));

        // Sizes should match
        ASSERT(m_pSystemMemoryBitsForUIThread->GetWidth() >= static_cast<uint32_t>(lockRegion.Width));
        ASSERT(m_pSystemMemoryBitsForUIThread->GetHeight() >= static_cast<uint32_t>(lockRegion.Height));
        ASSERT(m_pSystemMemoryBitsForUIThread->GetDxgiFormat() == GetDxgiFormat());

        // The surface should not be in the pool
        ASSERT(!m_pSystemMemoryBitsForUIThread->GetPoolLink()->IsOnList());
    }

    m_pSystemMemoryBitsForUIThread->Lock(
        address,
        stride,
        width,
        height
        );

    m_lockRegion = lockRegion;

    m_cLocks++;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Unlocks the surface.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::Unlock()
{
    ASSERT(m_cLocks > 0);

    m_pSystemMemoryBitsForUIThread->Unlock();

    m_cLocks--;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Add a dirty rect to the texture, updates the video memory texture.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::QueueUpdate()
{
    FAIL_FAST_ASSERT(m_pSystemMemoryBitsForUIThread); // Lock must have succeeded

    DCompSurfaceUpdate update;

    update.spSysMemBits = xref_ptr<SystemMemoryBits>(m_pSystemMemoryBitsForUIThread);
    update.region = m_lockRegion;

    m_lockRegion = {};

    IFC_RETURN(m_surfaceUpdates.push_back(update));

    // If there are gutters, keep the system memory bits around so that they can be filled
    // in the same staging buffer, otherwise release the reference to the staging texture so
    // another one can be used.
    if (!IncludesGutters())
    {
        ReleaseInterface(m_pSystemMemoryBitsForUIThread);
        m_pSystemMemoryBitsForUIThread = nullptr;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Given an 8-bit alpha surface, returns a 32-bit one with color channels initialized to white.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::Ensure32BitSource(
    _In_ SystemMemoryBits *p8BitSource,
    _Outptr_ SystemMemoryBits **pp32BitSource
    )
{
    HRESULT hr = S_OK;

    TracePrimitiveCompositionTexture8BitExpandBegin();

    SystemMemoryBits *p32BitSurface = NULL;
    const XUINT32 validRgnWidth = GetWidthWithGutters();
    const XUINT32 validRgnHeight = GetHeightWithGutters();

    ASSERT(p8BitSource->GetDxgiFormat() == DXGI_FORMAT_A8_UNORM);

    IFC(m_d3dDevice->AllocateFromSysMemBitsPool(
        validRgnWidth,      // The staging texture needs gutters
        validRgnHeight,
        DXGI_FORMAT_B8G8R8A8_UNORM,
        m_hasNative8BitSurfaceSupport,
        &p32BitSurface
        ));

    XUINT8 *pSourceAddress;
    XINT32 sourceStride;
    XUINT32 sourceWidth;
    XUINT32 sourceHeight;

    p8BitSource->Lock(
        reinterpret_cast<void**>(&pSourceAddress),
        &sourceStride,
        &sourceWidth,
        &sourceHeight
        );

    ASSERT(0 < sourceWidth);
    ASSERT(0 < sourceHeight);

    ASSERT(sourceWidth >= validRgnWidth);
    ASSERT(sourceHeight >= validRgnHeight);

    XUINT32 *pDestAddress;
    XINT32 destStride;
    XUINT32 destWidth;
    XUINT32 destHeight;

    p32BitSurface->Lock(
        reinterpret_cast<void**>(&pDestAddress),
        &destStride,
        &destWidth,
        &destHeight
        );

    ASSERT(destWidth >= validRgnWidth);
    ASSERT(destHeight >= validRgnHeight);

    for (XUINT32 nRow = 0; nRow < validRgnHeight; nRow++)
    {
        for (XUINT32 nCol = 0; nCol < validRgnWidth; nCol++)
        {
            // Copy alpha channel, write other channels.
            XUINT8 alpha = *(pSourceAddress + nCol);
            XUINT32 bgra = MIL_COLOR(alpha, 0xff, 0xff, 0xff);
            *(pDestAddress + nCol) = bgra;
        }

        pSourceAddress += sourceStride;
        pDestAddress = reinterpret_cast<XUINT32*>((reinterpret_cast<XUINT8*>(pDestAddress) + destStride));
    }

    p8BitSource->Unlock();
    p32BitSurface->Unlock();

    if (EventEnabledPrimitiveCompositionTexture8BitExpandEnd())
    {
        XPOINT dstPoint;
        dstPoint.x = 0;
        dstPoint.y = 0;

        XRECT sourceRect =
        {
            0,
            0,
            validRgnWidth,
            validRgnHeight
        };

        TextureTrace(
            reinterpret_cast<XUINT64>(p8BitSource),
            reinterpret_cast<XUINT64>(p32BitSurface),
            sourceRect,
            dstPoint,
            GetPixelStride(),
            TRUE, // is expand end event
            FALSE // usesUpdateSubresource
            );
    }

    *pp32BitSource = p32BitSurface;
    p32BitSurface = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(p32BitSurface);

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//    Implements a sys->vid copy.
//    Called on the UI thread.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::CopySubresource(
    _In_ const XRECT& destRect,
    _In_ SystemMemoryBits *pSource
    ) noexcept
{
    HRESULT hr = S_OK;
    IDXGISurface *pDestDXGISurface = NULL;
    ID3D11Resource *pDestD3DResource = NULL;
    ID3D11Texture2D *pCopySourceNoRef = NULL;
    CD3D11SharedDeviceGuard guard;
    bool beginDrawCalled = false;
    bool endDrawCalled = false;

    if (EventEnabledPrimitiveCompositionTextureUpdateBegin())
    {
        XPOINT destination = {0};
        XRECT sourceRect =
        {
            0,
            0,
            GetWidthWithGutters(),
            GetHeightWithGutters()
        };

        TextureTrace(
            reinterpret_cast<XUINT64>(pSource),
            reinterpret_cast<XUINT64>(m_pCompositionSurface),
            sourceRect,
            destination,
            GetPixelStride(),
            FALSE, // this is an update start event
            pSource->IsDriverVisible() ? FALSE : TRUE // usesUpdateSubresource
            );
    }

    // Call BeginDrawWithGutters with the size of the DComp surface, which excludes the gutters.
    RECT dstRectNoGutters;
    dstRectNoGutters.left = destRect.X;
    dstRectNoGutters.top = destRect.Y;
    dstRectNoGutters.right = destRect.X + destRect.Width;
    dstRectNoGutters.bottom = destRect.Y + destRect.Height;

    D3D11_BOX sourceBoxWithGutters = { 0 };
    sourceBoxWithGutters.back = 1;

    POINT dstOffsetNoGutters;
    POINT dstOffsetWithGutters;

    const bool includesGutters = IncludesGutters();
    if (includesGutters)
    {
        // BeginDrawWithGutters is not supported on virtual surfaces so fail fast if that is the case.  This should
        // be prevented during allocation time by setting includesGutters to false if the surface is virtual.
        ASSERT(!IsVirtual());

        // The private interface exposes gutters. This allows XAML to fill them along with this single
        // CopySubresourceRegion, which avoids overhead of having DComp call CopySubresourceRegion an additional time
        // internally for each gutter it needs.
        ASSERT(m_pCompositionSurfacePrivate != NULL);

        DCOMPOSITION_GUTTERS gutters = {0};

        IFC(m_pCompositionSurfacePrivate->BeginDrawWithGutters(
            &dstRectNoGutters,
            __uuidof(IDXGISurface),
            reinterpret_cast<void**>(&pDestDXGISurface),
            &dstOffsetNoGutters,
            &gutters
            ));
        beginDrawCalled = true;

        IFC(pDestDXGISurface->QueryInterface(__uuidof(ID3D11Resource), reinterpret_cast<void**>(&pDestD3DResource)));

        // BeginDrawWithGutters doesn't apply the gutters to the offset that it returns. Apply it here.
        dstOffsetWithGutters.x = dstOffsetNoGutters.x - gutters.leftWidth;
        dstOffsetWithGutters.y = dstOffsetNoGutters.y - gutters.topWidth;

        //
        // (0,0) in the source texture refers to the top-left gutter. DComp may not actually require all
        // gutters, so undo all the gutters and expand only the ones that we need to copy.
        //
        sourceBoxWithGutters.left = BILINEAR_FILTER_EDGE / 2 - gutters.leftWidth;
        sourceBoxWithGutters.top = BILINEAR_FILTER_EDGE / 2 - gutters.topWidth;
        sourceBoxWithGutters.right = GetWidthWithGutters() - BILINEAR_FILTER_EDGE / 2 + gutters.rightWidth;
        sourceBoxWithGutters.bottom = GetHeightWithGutters() - BILINEAR_FILTER_EDGE / 2 + gutters.bottomWidth;
    }
    else
    {
        IFC(m_pCompositionSurface->BeginDraw(
            &dstRectNoGutters,
            __uuidof(IDXGISurface),
            reinterpret_cast<void**>(&pDestDXGISurface),
            &dstOffsetNoGutters
            ));
        beginDrawCalled = true;

        IFC(pDestDXGISurface->QueryInterface(__uuidof(ID3D11Resource), reinterpret_cast<void**>(&pDestD3DResource)));

        dstOffsetWithGutters = dstOffsetNoGutters;

        //
        // (0,0) in the source texture refers to the top-left pixel of the content. Copy all of it.
        //
        sourceBoxWithGutters.left = 0;
        sourceBoxWithGutters.top = 0;
        sourceBoxWithGutters.right = pSource->GetWidth();
        sourceBoxWithGutters.bottom = pSource->GetHeight();
    }

    // Nobody should be staging from heap memory
    ASSERT(pSource->IsDriverVisible());

    {
        // If IsDriverVisible, we can safely down-cast to a SystemMemoryBitsDriver
        SystemMemoryBitsDriver *pSourceDriver = static_cast<SystemMemoryBitsDriver*>(pSource);

        // Format of system bits must match composition surface format
        ASSERT(pSourceDriver->GetDxgiFormat() == GetCompositionSurfaceFormat());

        // The source surface must be unmapped before issuing the copy operation
        pSourceDriver->EnsureUnmapped();
        pCopySourceNoRef = pSourceDriver->GetTexture();

        // Assert that both textures (source and dest) are on the same D3D device
#if DBG
        {
            ID3D11Device *pSourceDevice = NULL;
            ID3D11Device *pDestDevice = NULL;

            pSourceDriver->GetTexture()->GetDevice(&pSourceDevice);
            pDestD3DResource->GetDevice(&pDestDevice);

            ASSERT(pSourceDevice == pDestDevice);

            ReleaseInterface(pSourceDevice);
            ReleaseInterface(pDestDevice);
        }
#endif
    }

    IFC(m_d3dDevice->TakeLockAndCheckDeviceLost(&guard));

    // On some hardware it is better to perform this copy in two steps so that
    // we don't go directly from a sysmem staging texture to a shared video mem
    // texture.  Believe it.

    if (ID3D11Texture2D* pIntermediateSurfaceNoRef = m_d3dDevice->GetIntermediateUploadSurfaceNoRef(GetCompositionSurfaceFormat()))
    {
        // Assert that source and intermediate surfaces are on the same device.
#if DBG
        {
            ID3D11Device *pSourceDevice = NULL;
            ID3D11Device *pDestDevice = NULL;

            pCopySourceNoRef->GetDevice(&pSourceDevice);
            pIntermediateSurfaceNoRef->GetDevice(&pDestDevice);

            ASSERT(pSourceDevice == pDestDevice);

            ReleaseInterface(pSourceDevice);
            ReleaseInterface(pDestDevice);
        }
#endif

        m_d3dDevice->GetLockedDeviceContext(&guard)->CopySubresourceRegion(
            pIntermediateSurfaceNoRef,  // pDstResource
            0,                      // DestSubresource
            0,                      // DstX
            0,                      // DstY
            0,                      // DstZ
            pCopySourceNoRef,       // pSrcResource
            0,                      // SrcSubresource
            &sourceBoxWithGutters   // pSrcBox
            );

        pCopySourceNoRef = pIntermediateSurfaceNoRef;

        sourceBoxWithGutters.right -= sourceBoxWithGutters.left;
        sourceBoxWithGutters.bottom -= sourceBoxWithGutters.top;
        sourceBoxWithGutters.left = 0;
        sourceBoxWithGutters.top = 0;
    }

    m_d3dDevice->GetLockedDeviceContext(&guard)->CopySubresourceRegion(
        pDestD3DResource,       // pDstResource
        0,                      // DestSubresource
        dstOffsetWithGutters.x, // DstX
        dstOffsetWithGutters.y, // DstY
        0,                      // DstZ
        pCopySourceNoRef,       // pSrcResource
        0,                      // SrcSubresource
        &sourceBoxWithGutters   // pSrcBox
        );

    // Mark this flag before calling EndDraw. If the EndDraw call itself returns a device lost error, we don't want to
    // call it a second time during cleanup.
    endDrawCalled = true;
    if (includesGutters)
    {
        IFC(m_pCompositionSurfacePrivate->EndDraw());
    }
    else
    {
        IFC(m_pCompositionSurface->EndDraw());
    }

    m_hasDrawnAtleastOnce = TRUE;

    if (s_traceDCompSurfaceUpdates)
    {
        m_updateRects.push_back(dstRectNoGutters);
    }

    IGNOREHR(TrackSurfaceDrawn());

Cleanup:
    // If we encountered a device lost while trying to copy the surface to video memory after calling BeginDraw on the
    // composition surface, make sure we call EndDraw on the composition surface. This puts the surface back in the
    // correct state and avoids calling BeginDraw again later while we're in the middle of drawing.
    if (beginDrawCalled && !endDrawCalled && GraphicsUtility::IsDeviceLostError(hr))
    {
        if (includesGutters)
        {
            IGNOREHR(m_pCompositionSurfacePrivate->EndDraw());
        }
        else
        {
            IGNOREHR(m_pCompositionSurface->EndDraw());
        }
    }

    ReleaseInterfaceNoNULL(pDestDXGISurface);
    ReleaseInterfaceNoNULL(pDestD3DResource);

    TracePrimitiveCompositionTextureUpdateEnd();

    RRETURN(hr);
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a pointer and offset into the underlying hardware surface so the caller can write
//      the requested update rect.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::BeginDraw(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppSurface,
    _Out_ XPOINT *pOffsetIntoSurface
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurface != NULL);

    RECT rect;
    rect.left = pUpdateRect->X;
    rect.top = pUpdateRect->Y;
    rect.right = pUpdateRect->X + pUpdateRect->Width;
    rect.bottom = pUpdateRect->Y + pUpdateRect->Height;

    POINT offset;

    IFC_NOTRACE(m_pCompositionSurface->BeginDraw(
        &rect,
        iid,
        reinterpret_cast<void**>(ppSurface),
        &offset
        ));

    pOffsetIntoSurface->x = offset.x;
    pOffsetIntoSurface->y = offset.y;

    if (s_traceDCompSurfaceUpdates)
    {
        m_updateRects.push_back(rect);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a pointer and offset into the underlying hardware surface so the caller can write
//      the requested update rect. Caller must update entire surface, including gutters.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::BeginDrawWithGutters(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppSurface,
    _Out_ XPOINT *pOffsetIntoSurface
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurface != NULL);

    RECT rect;
    rect.left = pUpdateRect->X;
    rect.top = pUpdateRect->Y;
    rect.right = pUpdateRect->X + pUpdateRect->Width;
    rect.bottom = pUpdateRect->Y + pUpdateRect->Height;

    POINT offset;
    DCOMPOSITION_GUTTERS gutters;

    IFC(m_pCompositionSurfacePrivate->BeginDrawWithGutters(
        &rect,
        iid,
        reinterpret_cast<void**>(ppSurface),
        &offset,
        &gutters
        ));

    // DComp's BeginDrawWithGutters doesn't apply the gutters to the offset that it returns,
    // and neither do we. We don't even bother to return it to the caller, since currently callers
    // don't care how big the gutters are - they're going to clear the entire region anyway
    // before drawing to the 'real' offset into the texture, not into the gutter region.
    pOffsetIntoSurface->x = offset.x;
    pOffsetIntoSurface->y = offset.y;

    if (s_traceDCompSurfaceUpdates)
    {
        m_updateRects.push_back(rect);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Acknowledges that the caller has completed their pending update.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::EndDraw()
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurface != NULL);

    IFC(m_pCompositionSurface->EndDraw());

    m_hasDrawnAtleastOnce = TRUE;

    IGNOREHR(TrackSurfaceDrawn());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Tells the surface to transition to "suspended" drawing state.
//      A suspended surface cannot be drawn to.  DComp requires this state
//      in order to allow drawing multiple updates to the same atlas surface.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::SuspendDraw()
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurface != NULL);

    IFC(m_pCompositionSurface->SuspendDraw());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Tells the surface to transition from "suspended" to "drawing" state.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::ResumeDraw()
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurface != NULL);

    IFC(m_pCompositionSurface->ResumeDraw());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if the composition surface was discarded due to a previous offer.
//
//------------------------------------------------------------------------------
bool
DCompSurface::IsDiscarded()
{
    if (!m_discarded &&
        m_hasDrawnAtleastOnce &&
        m_pCompositionSurfacePrivate != nullptr)
    {
        BOOL hasValidPixels = FALSE;

        // As per DComp team HasValidPixels would never fail.
        // Hence for simplicity, we do not propagate it.
        // In case the implementation changes in future, the failfast
        // should indicate it.
        HRESULT hasValidPixelsHR = m_pCompositionSurfacePrivate->HasValidPixels(&hasValidPixels);
        XCP_FAULT_ON_FAILURE(SUCCEEDED(hasValidPixelsHR));

        m_discarded = !hasValidPixels;
    }
    return m_discarded || (m_pCompositionSurface == nullptr);
}

bool
DCompSurface::IsVirtual()
{
    return m_isVirtual;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Resizes the surface. Any existing contents persist for virtual surfaces.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::Resize(
    XUINT32 width,
    XUINT32 height
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_pCompositionSurfacePrivate);

    if (!IsVirtual())
    {
        UpdateMemoryFootprint(FALSE);
    }

    IFC(m_pCompositionSurfacePrivate->Resize(width, height));

    // If the surface size changed, then we clear out the pending update list. If the surface became smaller,
    // the existing surface updates can draw to a part of the surface that exceeds the new smaller bounds. If
    // the surface became larger, the existing surface updates may no longer update the entire surface, which is
    // required when first drawing into the larger surface.
    //
    // Note: We could walk through the update list and clear out only those that don't exactly match the new
    // bounds. To keep things simple, we don't bother.
    if (width != m_widthWithoutGutters || height != m_heightWithoutGutters)
    {
        m_surfaceUpdates.clear();
    }

    m_widthWithoutGutters = width;
    m_heightWithoutGutters = height;

    if (!IsVirtual())
    {
        UpdateMemoryFootprint(TRUE);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Hints to the underlying hardware surface that the area outside the given rects is no longer
//      required, and can be freed. This area will not be rendered again (until included in another
//      BeginDraw).
//      This is only supported for virtual surfaces.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::TrimTo(
    _In_reads_(rectCount) XRECT *pRectsToKeep,
    XUINT32 rectCount
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_pVirtualCompositionSurface);

    RECT *pRects = NULL;

    pRects = new RECT[rectCount];
    for (XUINT32 i = 0; i < rectCount; ++i)
    {
        pRects[i].left = pRectsToKeep[i].X;
        pRects[i].top = pRectsToKeep[i].Y;
        pRects[i].right = pRectsToKeep[i].X + pRectsToKeep[i].Width;
        pRects[i].bottom = pRectsToKeep[i].Y + pRectsToKeep[i].Height;
    }

    IFC(m_pVirtualCompositionSurface->Trim(pRects, rectCount));

Cleanup:
    SAFE_DELETE_ARRAY(pRects);

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Copies the surface contents to a staging texture.
//  Note:
//      Uses IDCompositionSurfaceDebug. DComp team allowed us
//      to use if for the time being, but we would like an api with no
//      debug in it.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::CopyToSurface(
    _Outptr_ IPALByteAccessSurface **ppByteAccessSurface
    )
{
    HRESULT hr = S_OK;
    ASSERT(m_pCompositionSurface != NULL);
    IDCompositionSurfaceDebug *pDCompositionSurfaceDebug = NULL;
    IDXGISurface *pDXGISurface = NULL;
    ByteAccessDxgiSurface *pByteAccessSurface = NULL;

    IFC(m_pCompositionSurface->QueryInterface(__uuidof(IDCompositionSurfaceDebug), reinterpret_cast<void**>(&pDCompositionSurfaceDebug)));
    IFC(pDCompositionSurfaceDebug->CopySurface(
        NULL,
        &pDXGISurface));

#if DBG
    DXGI_SURFACE_DESC desc;
    IFC(pDXGISurface->GetDesc(&desc));
    // The gutters should not be copied out.
    ASSERT(desc.Width == GetWidthWithoutGutters() && desc.Height == GetHeightWithoutGutters());
#endif

    IFC(ByteAccessDxgiSurface::Create(m_d3dDevice, pDXGISurface, GetPixelStride(), &pByteAccessSurface));
    *ppByteAccessSurface = pByteAccessSurface;
    pByteAccessSurface = NULL;

Cleanup:
    ReleaseInterface(pDXGISurface);
    ReleaseInterface(pByteAccessSurface);
    ReleaseInterface(pDCompositionSurfaceDebug);
    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Flushes the pending updates to the DComp surface.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurface::FlushUpdates()
{
    HRESULT hr = S_OK;

    SystemMemoryBits *pWrapped32BitSource = NULL;

    xvector<DCompSurfaceUpdate>::const_iterator iter = m_surfaceUpdates.begin();

    while (iter != m_surfaceUpdates.end())
    {
        ASSERT(m_d3dDevice != NULL);
        ASSERT(iter->spSysMemBits != NULL);

        // Emulate native 8-bit surface support by expanding the 8-bit staging texture to 32-bits, and then
        // uploading it to the underlying 32-bit surface.
        if (    iter->spSysMemBits->GetDxgiFormat() == DXGI_FORMAT_A8_UNORM
            && !m_hasNative8BitSurfaceSupport)
        {
            // TODO: JCOMP: Can we ever queue more than one update per surface?
            if (pWrapped32BitSource != NULL)
            {
                m_d3dDevice->ReturnToSysMemBitsPool(pWrapped32BitSource);
                ReleaseInterface(pWrapped32BitSource);
            }

            // This should be safe for virtual surfaces since the system memory bits was allocated lower
            // than the max texture size and will then be copied via CopySubresource to the spot in
            // the hardware virtual surface which can be larger than max texture size.
            IFC(Ensure32BitSource(
                iter->spSysMemBits,
                &pWrapped32BitSource
                ));

            IFC(CopySubresource(iter->region, pWrapped32BitSource));
        }
        else
        {
            IFC(CopySubresource(iter->region, iter->spSysMemBits.get()));
        }

        // In the case of gutters aren't supported (such as the case with virtual surfaces), it will hand off
        // the system memory bits to the update queue and work with a new m_pSystemMemoryBitsForUIThread.
        // In the case that gutters are supported, a single surface texture write will re-use the
        // m_pSystemMemoryBitsForUIThread for the purpose of doing gutter updates later and it is more
        // efficient to re-use the staging buffer.  Since the surface wasn't return the the pool previously,
        // do it here.  See complementary logic in QueueUpdate().
        if (!IncludesGutters())
        {
            ASSERT(m_pSystemMemoryBitsForUIThread != iter->spSysMemBits.get());
            ASSERT(!iter->spSysMemBits->GetPoolLink()->IsOnList());

            m_d3dDevice->ReturnToSysMemBitsPool(iter->spSysMemBits.get());

            // iter->spSysMemBits will be cleared in m_surfaceUpdates.clear() since it is managed by a
            // smart pointer.
        }

        ++iter;
    }

    m_surfaceUpdates.clear();

    // This covers the case that m_pSystemMemoryBitsForUIThread wasn't queued for update after lock or
    // if the staging surface was re-used for the purposes of filling gutters.
    if (m_pSystemMemoryBitsForUIThread != nullptr)
    {
        // The surface should not be in the pool
        ASSERT(!m_pSystemMemoryBitsForUIThread->GetPoolLink()->IsOnList());

        m_d3dDevice->ReturnToSysMemBitsPool(m_pSystemMemoryBitsForUIThread);
        ReleaseInterface(m_pSystemMemoryBitsForUIThread);
    }

Cleanup:
    if (pWrapped32BitSource != NULL)
    {
        m_d3dDevice->ReturnToSysMemBitsPool(pWrapped32BitSource);
        ReleaseInterfaceNoNULL(pWrapped32BitSource);
    }
    RRETURN(hr);
}

PixelFormat DCompSurface::GetPixelFormat() const
{
    return m_fAlphaMask ? pixelGray8bpp :
        (m_isHDR ? pixelColor64bpp_R16G16B16A16_Float : pixelColor32bpp_A8R8G8B8);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Updates texture memory usage, which is reported by the frame-rate
//      counter.
//
//-------------------------------------------------------------------------
void
DCompSurface::UpdateMemoryFootprint(bool increase)
{
    XINT32 textureSize = static_cast<XINT32>(GetTextureSizeInBytes());

    if (!increase)
    {
        textureSize = -textureSize;
    }

    g_uD3D11TextureMemoryUsage += textureSize;
    g_uD3D11TextureMemoryUsageNPOT += textureSize;

    // TODO: JCOMP: Need to separate the logic here for sysmem vs vidmem into different methods again
    if (m_pCompositionSurface != NULL)
    {
        TraceMemoryUpdateAllocationDCompSurfaceInfo(static_cast<int32_t>(textureSize));
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the size, in bytes, of the entire texture.
//
//-------------------------------------------------------------------------
std::uint32_t
DCompSurface::GetTextureSizeInBytes() const
{
    return GetWidthWithGutters() * GetHeightWithGutters() * GetPixelStride();
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Debugging aid, tracks this surface for dumping to disk
//
//-------------------------------------------------------------------------
HRESULT DCompSurface::TrackSurfaceDrawn()
{
    HRESULT hr = S_OK;

    if (s_traceDCompSurfaces || s_traceDCompSurfaceUpdates)
    {
        IFC(DCompSurfaceMonitor::TrackSurfaceDrawn(this));
    }

Cleanup:
    RRETURN(hr);
}

// Wrapper for DComp surface dumper debugging aid
HRESULT
DCompSurface::TraceDCompSurface(XUINT32 frameNumber)
{
    if (s_traceDCompSurfaces)
    {
        IGNOREHR(DumpDCompSurface(frameNumber, nullptr));
    }
    if (s_traceDCompSurfaceUpdates)
    {
        for (unsigned int i = 0; i < m_updateRects.size(); i++)
        {
            IGNOREHR(DumpDCompSurface(frameNumber, &m_updateRects[i]));
        }
        m_updateRects.clear();
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Debugging aid, dumps this surface's contents to a PNG file on disk.
//
//-------------------------------------------------------------------------
HRESULT
DCompSurface::DumpDCompSurface(XUINT32 frameNumber, _In_opt_ RECT* pRect)
{
    HRESULT hr = S_OK;
    bool mapped = false;
    IDCompositionSurfaceDebug *pSurfaceDebug = NULL;
    IDXGISurface *pDXGISurface = NULL;
    IWICImagingFactory *pWICFactory = NULL;
    IWICBitmap *pWICBitmap = NULL;

    IFC(m_pCompositionSurface->QueryInterface(&pSurfaceDebug));
    IFC(pSurfaceDebug->CopySurface(pRect, &pDXGISurface));

    DXGI_SURFACE_DESC desc;
    IFC(pDXGISurface->GetDesc(&desc));

    DXGI_MAPPED_RECT mappedRect;
    IFC(pDXGISurface->Map(&mappedRect, DXGI_MAP_READ));
    mapped = TRUE;

    IFC(CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWICFactory)
        ));

    IFC(pWICFactory->CreateBitmapFromMemory(
        desc.Width,
        desc.Height,
        (desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM) ? GUID_WICPixelFormat32bppBGRA : GUID_WICPixelFormat8bppGray,   // pixelFormat
        mappedRect.Pitch,
        mappedRect.Pitch * desc.Height,
        mappedRect.pBits,
        &pWICBitmap
        ));

    // Put the file in this app packages temp directory
    // Typically this is c:\users\<user>\appdata\local\Packages\<Package>\AC\Temp
    // The filename is of form DComSurfaceDump_<this pointer>_<counter>.png
    WCHAR dumpPath[MAX_PATH];

    if (pRect == nullptr)
    {
        IFC(::StringCchPrintf(
            dumpPath,
            ARRAYSIZE(dumpPath),
            L"%%temp%%\\DCompSurfaceDump_%05d_%p_Whole.png",
            frameNumber,
            this
            ));
    }
    else
    {
        IFC(::StringCchPrintf(
            dumpPath,
            ARRAYSIZE(dumpPath),
            L"%%temp%%\\DCompSurfaceDump_%05d_%p_%d_%d_%dX%d.png",
            frameNumber,
            this,
            pRect->left,
            pRect->top,
            pRect->right - pRect->left,
            pRect->bottom - pRect->top
            ));
    }

    WCHAR expandedDumpPath[MAX_PATH];
    if (!::ExpandEnvironmentStrings(dumpPath, expandedDumpPath, ARRAYSIZE(expandedDumpPath)))
    {
        IFC(E_FAIL);
    }

    IFC(ImageUtils::SaveWICBitmapAsPNG(pWICBitmap, expandedDumpPath));

Cleanup:
    ReleaseInterfaceNoNULL(pWICBitmap);
    ReleaseInterfaceNoNULL(pWICFactory);
    if (mapped)
    {
        pDXGISurface->Unmap();
    }
    ReleaseInterfaceNoNULL(pDXGISurface);
    ReleaseInterfaceNoNULL(pSurfaceDebug);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Traces a texture.
//
//  Static
//
//-------------------------------------------------------------------------

void
DCompSurface::TextureTrace(
    XUINT64 sourceSurface,
    XUINT64 destinationSurface,
    _In_ const XRECT &sourceRect,
    _In_ const XPOINT &destination,
    XUINT32 bitDepth,
    bool isExtendEndEvent, // if FALSE, is UpdateStart
    bool usesUpdateSubresource
    )
{
    XUINT32 area = sourceRect.Height * sourceRect.Width;

    if (isExtendEndEvent)
    {
        TracePrimitiveCompositionTexture8BitExpandEnd(
            sourceSurface,
            destinationSurface,
            usesUpdateSubresource /* isUpdateSubResource */,
            sourceRect.X,
            sourceRect.Y,
            sourceRect.Width,
            sourceRect.Height,
            destination.x,
            destination.y,
            area,
            bitDepth
            );
    }
    else
    {
        TracePrimitiveCompositionTextureUpdateBegin(
            sourceSurface,
            destinationSurface,
            usesUpdateSubresource /* isUpdateSubResource */,
            sourceRect.X,
            sourceRect.Y,
            sourceRect.Width,
            sourceRect.Height,
            destination.x,
            destination.y,
            area,
            bitDepth
            );
    }
}

/*static*/ _Check_return_ HRESULT
ByteAccessDxgiSurface::Create(
    _In_ CD3D11Device *pD3DDevice,
    _In_ IDXGISurface *pDXGISurface,
    XUINT32 stride,
    _Outptr_ ByteAccessDxgiSurface **ppByteAccessDxgiSurface)
{
    HRESULT hr = S_OK;
    ByteAccessDxgiSurface *pSurface = new ByteAccessDxgiSurface(pD3DDevice, stride);
    SetInterface(pSurface->m_pDxgiSurface, pDXGISurface);
    *ppByteAccessDxgiSurface = pSurface;
    RRETURN(hr);//RRETURN_REMOVAL
}

ByteAccessDxgiSurface::ByteAccessDxgiSurface(_In_ CD3D11Device *pD3DDevice, XUINT32 stride)
    : m_pDxgiSurface(NULL)
    , m_stride(stride)
{
    SetInterface(m_pD3DDevice, pD3DDevice);
}

ByteAccessDxgiSurface::~ByteAccessDxgiSurface()
{
    ReleaseInterface(m_pD3DDevice);
    ReleaseInterface(m_pDxgiSurface);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//     Allocates a byte array and reads pixels into it.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ByteAccessDxgiSurface::ReadBytes(
    _Out_ XUINT32 &length,
    _Outptr_result_bytebuffer_(length) XBYTE **ppBytes)
{
    HRESULT hr = S_OK;
    XBYTE *pBytes = NULL;

    DXGI_SURFACE_DESC desc;
    IFC(m_pDxgiSurface->GetDesc(&desc));

    DXGI_MAPPED_RECT mappedRect;
    {
        // The DXGI Surface may access the Context under the covers so take a lock.
        {
            CD3D11SharedDeviceGuard guard;
            IFC(m_pD3DDevice->TakeLockAndCheckDeviceLost(&guard));
            auto lockedDeviceContext = m_pD3DDevice->GetLockedDeviceContext(&guard);
            IFC(m_pDxgiSurface->Map(&mappedRect, DXGI_MAP_READ));
        }

        XUINT32 sourcePitch = static_cast<XUINT32>(mappedRect.Pitch);
        XUINT32 destPitch = (desc.Width * m_stride);
        ASSERT(destPitch <= sourcePitch);
        length = destPitch * desc.Height;
        pBytes = new(NO_FAIL_FAST) XBYTE[length];
        IFCOOM(pBytes);

        XUINT32 sourceLocationIndex = 0;
        XUINT32 destLocationIndex = 0;
        for (XUINT32 rowIndex = 0; rowIndex < desc.Height; rowIndex++)
        {
            memcpy((pBytes + destLocationIndex), (mappedRect.pBits + sourceLocationIndex), destPitch);
            destLocationIndex += destPitch;
            sourceLocationIndex += sourcePitch;
        }

        // The DXGI Surface may access the Context under the covers so take a lock.
        {
            CD3D11SharedDeviceGuard guard;
            IFC(m_pD3DDevice->TakeLockAndCheckDeviceLost(&guard));
            auto lockedDeviceContext = m_pD3DDevice->GetLockedDeviceContext(&guard);
            IFC(m_pDxgiSurface->Unmap());
        }
    }

    *ppBytes = pBytes;
    pBytes = NULL;
Cleanup:
    SAFE_DELETE_ARRAY(pBytes);
    RRETURN(hr);

}
