// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      DirectX device PAL interface implementation

#pragma once
#include <xref_ptr.h>
#include <xvector.h>
#include <palgfx.h>
#include <HWTextureMgr.h>
#include <Rendertypes.h>
#include <fwd/windows.ui.composition.h>

class CD3D11Device;
class DCompTreeHost;
class SystemMemoryBits;
struct IDCompositionSurface;
struct IDCompositionVirtualSurface;
struct IDCompositionSurfaceFactory;
struct IDCompositionSurfacePartner;
struct IDXGISurface;

class DCompSurface : public CXcpObjectBase<>
{
public:
    XUINT32 GetWidthWithGutters() const
    {
        return m_includesGutters
            ? m_widthWithoutGutters + BILINEAR_FILTER_EDGE
            : m_widthWithoutGutters;
    }

    XUINT32 GetHeightWithGutters() const
    {
        return m_includesGutters
            ? m_heightWithoutGutters + BILINEAR_FILTER_EDGE
            : m_heightWithoutGutters;
    }

    XUINT32 GetWidthWithoutGutters() const
    {
        return m_widthWithoutGutters;
    }

    XUINT32 GetHeightWithoutGutters() const
    {
        return m_heightWithoutGutters;
    }

    bool IncludesGutters() const { return m_includesGutters; }

    std::uint32_t GetTextureSizeInBytes() const;

private:

    struct DCompSurfaceUpdate
    {
        xref_ptr<SystemMemoryBits> spSysMemBits; // for SysMemLock & ReturnSysMemBitsToPool only
        XRECT region{};
    };

public:
    static _Check_return_ HRESULT Create(
        _In_ CD3D11Device *pDevice,
        _In_ DCompTreeHost *pDCompHost,
        bool isOpaque,
        bool isAlphaMask,
        bool isVirtual,
        bool isHDR,
        bool requestAtlas,
        XUINT32 widthWithoutGutters,
        XUINT32 heightWithoutGutters,
        _Outptr_ DCompSurface **ppTexture
        );

     static _Check_return_ HRESULT Create(
        _In_ DCompTreeHost *pDCompHost,
        _In_ IDCompositionSurfaceFactory *pSurfaceFactory,
        bool isOpaque,
        bool isVirtual,
        bool isHDR,
        bool requestAtlas,
        XUINT32 widthWithoutGutters,
        XUINT32 heightWithoutGutters,
        _Outptr_ DCompSurface** ppTexture
        );

     static xref_ptr<DCompSurface> CreateWithNoHardware(
        _In_ DCompTreeHost *pDCompHost,
        bool isVirtual
        );

    _Check_return_ HRESULT Lock(
        _Outptr_ void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight);

    _Check_return_ HRESULT LockRect(
        _In_ const XRECT& lockRegion,
        _Outptr_ void** address,
        _Out_ int32_t* stride,
        _Out_ uint32_t* width,
        _Out_ uint32_t* height
        );

    _Check_return_ HRESULT Unlock();

    _Check_return_ HRESULT FlushUpdates();

    PixelFormat GetPixelFormat() const;

    _Check_return_ HRESULT QueueUpdate();

    _Check_return_ HRESULT BeginDraw(
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppSurface,
        _Out_ XPOINT *pOffsetIntoSurface
        );

    _Check_return_ HRESULT BeginDrawWithGutters(
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppSurface,
        _Out_ XPOINT *pOffsetIntoSurface
        );

    _Check_return_ HRESULT EndDraw();

    _Check_return_ HRESULT SuspendDraw();

    _Check_return_ HRESULT ResumeDraw();

    _Check_return_ HRESULT Resize(
        XUINT32 width,
        XUINT32 height
        );

    void ReleaseLegacyDCompResources(bool resetWucSurface);

    bool IsDiscarded();

    bool IsVirtual();

    bool IsOpaque() const { return m_fIsOpaque; }

    _Check_return_ HRESULT TrimTo(
        _In_reads_(rectCount) XRECT *pRectsToKeep,
        XUINT32 rectCount
        );

    _Check_return_ HRESULT CopyToSurface(
        _Outptr_ IPALByteAccessSurface **ppByteAccessSurface
        );

    //
    // DCompSurface methods
    //
    _Check_return_ HRESULT Ensure32BitSource(
        _In_ SystemMemoryBits *p8BitSource,
        _Outptr_ SystemMemoryBits **pp32BitSource
        );

    IDCompositionSurface* GetIDCompSurface() const { return m_pCompositionSurface; }
    WUComp::ICompositionSurface* GetWinRTSurface() const { return m_spWinRTSurface.Get(); }

    DXGI_FORMAT GetCompositionSurfaceFormat() const
    {
        DXGI_FORMAT compositionSurfaceFormat = GetDxgiFormat();
        if (compositionSurfaceFormat == DXGI_FORMAT_A8_UNORM && !m_hasNative8BitSurfaceSupport)
        {
            compositionSurfaceFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        }

        return compositionSurfaceFormat;
    }

    static const XUINT32 MIN_GRADIENT_SURFACE_WIDTH = 2048;

    HRESULT TrackSurfaceDrawn();
    HRESULT TraceDCompSurface(UINT32 frameNumber);
    HRESULT DumpDCompSurface(XUINT32 frameNumber, _In_opt_ RECT* pRect);

    _Check_return_ HRESULT InitializeSurface(
        _In_ CD3D11Device *pDevice,
        _In_ DCompTreeHost *pDCompTreeHost,
        bool isVirtual
        );

protected:
    DCompSurface(
        XUINT32 width,
        XUINT32 height,
        bool isOpaque,
        bool isAlphaMask,
        bool includesGutters,
        bool hasNative8BitSupport,
        bool isHDR,
        bool requestAtlas
        );

    ~DCompSurface() override;

    DXGI_FORMAT GetDxgiFormat() const
    {
        return m_fAlphaMask ? DXGI_FORMAT_A8_UNORM :
            (m_isHDR ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM);
    }

    XUINT32 GetPixelStride() const
    {
        return m_fAlphaMask ? sizeof(uint8_t) :
            (m_isHDR ? 4*sizeof(uint16_t) : sizeof(uint32_t));
    }

private:
    void static TextureTrace(
        XUINT64 sourceSurface,
        XUINT64 destinationSurface,
        _In_ const XRECT &sourceRect,
        _In_ const XPOINT &destination,
        XUINT32 bitDepth,
        bool isExtendEndEvent, // if FALSE, is UpdateStart
        bool usesUpdateSubresource
        );
    _Check_return_ HRESULT InitializeSurface(
        _In_ DCompTreeHost *pDCompTreeHost,
        _In_ IDCompositionSurfaceFactory *pSurfaceFactory,
        bool isVirtual
        );

    void UpdateMemoryFootprint(
        bool increase
        );

    _Check_return_ HRESULT CopySubresource(
        _In_ const XRECT& destRect,
        _In_ SystemMemoryBits *pSource
        ) noexcept;


    xref_ptr<CD3D11Device> m_d3dDevice;

    XUINT32 m_widthWithoutGutters;
    XUINT32 m_heightWithoutGutters;

    XUINT32 m_cLocks;

    // TODO: Consider removing this list: It's only going to contain the single element
    //       copied from m_pSystemMemoryBitsForUIThread so take it from there directly.
    xvector<DCompSurfaceUpdate> m_surfaceUpdates;

private:

    // Note: This staging texture includes space for gutters. This is where we do the gutter copies in XAML.
    _Maybenull_ SystemMemoryBits *m_pSystemMemoryBitsForUIThread;

    XRECT m_lockRegion = {};

    IDCompositionSurface *m_pCompositionSurface;
    IDCompositionVirtualSurface *m_pVirtualCompositionSurface;
    IDCompositionSurfacePartner *m_pCompositionSurfacePrivate; // For gutter copy API

    // WinRT composition objects
    wrl::ComPtr<WUComp::ICompositionSurface> m_spWinRTSurface;

    bool m_includesGutters : 1;
    bool m_fIsOpaque : 1;
    bool m_fAlphaMask : 1;
    bool m_hasNative8BitSurfaceSupport : 1;
    bool m_discarded : 1;
    bool m_hasDrawnAtleastOnce : 1;
    bool m_isVirtual : 1;
    bool m_isHDR : 1;
    bool m_requestAtlas : 1;

    // DComp, and SurfaceImageSource both allow calling BeginDraw multiple
    // times while an EndDraw is outstanding (for performance reasons).
    // To handle this case we book-keep all the updated rects.
    std::vector<RECT> m_updateRects;    // Holds rects that were updated on the surface

    static bool s_traceDCompSurfacesInitialized;
    static bool s_traceDCompSurfaces;
    static bool s_traceDCompSurfaceUpdates;

    //
    // Lock/Unlock/FlushUpdates
    //
    // Xaml batches up its surface updates until the end of the frame where they're flushed in FlushUpdates. This is legacy
    // behavior from when Xaml did its own atlasing and could update subrects independently. Now every surface update covers
    // the entire rect. This batching also guarantees coherency between the surface updates and the frame that consumes them.
    //
    // There are potential caching benefits here from D2D for surfaces packed into the same atlas. We might be getting them
    // already, or we might need to switch to BeginDraw/SuspendDraw with a single EndDraw at the end of the frame.
    //

    //
    // Gutters
    //
    // Gutters are needed for antialiased primitives filled with atlased surfaces - the antialiasing on the edge of the
    // primitive could cause us to sample a pixel beyond the bounds of the texture, which could be in an adjacent texture
    // packed in the same atlas, so we leave a gutter region around each texture allocation. In order for the antialiasing
    // to look correct, we duplicate the edge pixel into this gutter region.
    //
    // The gutters themselves are reserved by DComp. The BeginDraw API updates the texture, and DComp handles the gutter
    // copies automatically. This is a perf hit, however, because the gutter copies are done with multiple expensive
    // CopySubresourceRegion calls. As an optimization, Xaml will handle the gutter copies in the staging texture on the
    // CPU, then call BeginDrawWithGutters to update everything. DComp then skips its own gutter copies.
    //
    // Gradient textures are all sampled across the middle of the pixel, so there's no need for any gutters. When Xaml did
    // its own atlasing, we could skip gutters and allocate 1px tall textures. When we went over to DComp, we kept the flag
    // that said "no gutters", except DComp will automatically reserve 1px around the edges of its surfaces. Instead, this
    // flag is causing Xaml to call BeginDraw instead of BeginDrawWithGutters, which hits the performance penalty. Since
    // gutters are never needed anyway, Xaml can just call BeginDrawWithGutters and leave them blank.
    // TODO: BeginDrawWithGutters for gradients
    //
    // Text has empty gutters. A text mask is mostly transparent around the edges, which means there's no point antialiasing
    // it, so we leave the gutters empty and turn off antialiasing for text primitives as a perf optimization.
    //

    //
    // Off-thread surface updates for image decode
    //
    // Xaml can update surfaces off of the UI thread for image decodes. This needs to keep working. We're also trying to
    // offload image loading from the UI thread even more, so there could be more multithreaded updating in the future.
    //

    //
    // HWTexture existed to handle atlas subrects back when Xaml did atlasing. Now it can probably be merged with DCompSurface.
    //
};

//-------------------------------------------------------------------------
//
//  Synopsis:
//      A wrapper over IDXGISurface from bytes can be read.
//      This is used by DCompSurface::CopyToSurface api.
//
//-------------------------------------------------------------------------
class ByteAccessDxgiSurface : public CXcpObjectBase<IPALByteAccessSurface>
{
public:
#if DBG
        FORWARD_ADDREF_RELEASE(CXcpObjectBase<IPALByteAccessSurface>);
#endif /* DBG */

    static _Check_return_ HRESULT Create(
        _In_ CD3D11Device *pD3DDevice,
        _In_ IDXGISurface *pDXGISurface,
        XUINT32 stride,
        _Outptr_ ByteAccessDxgiSurface **ppByteAccessDxgiSurface
        );

    _Check_return_ HRESULT ReadBytes(
        _Out_ XUINT32 &length,
        _Outptr_result_bytebuffer_(length) XBYTE **ppBytes) override;

private:
    ByteAccessDxgiSurface(_In_ CD3D11Device *pD3DDevice, XUINT32 stride);

    ~ByteAccessDxgiSurface() override;

    CD3D11Device *m_pD3DDevice;
    IDXGISurface *m_pDxgiSurface;
    XUINT32 m_stride;
};

