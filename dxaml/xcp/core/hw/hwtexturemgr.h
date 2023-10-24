// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Create textures.  This class exists to eventually texture atlas
//      our resources.

#pragma once
#include <palgfx.h>
#include <PALNotify.h>
#include <mutex>
#include <dxgiformat.h>
#include <MaxTextureSizeProvider.h>
#include <AtlasRequestProvider.h>

class HWTextRealization;
class HWTexture;
class CLinearGradientBrush;
class SystemMemoryBits;
class DCompSurface;
class DCompSurfaceFactory;
class CD3D11Device;
class HWTextureManager;
class CWindowRenderTarget;
enum HWTextureFlags;

// The amount of padding needed between texture allocations to prevent samples from other textures bleeding in due to bilinear filtering.
#define BILINEAR_FILTER_EDGE 2

// The minimum amount of padding in an atlas texture.  The largest dimension for an allocation that can fit in an atlas is
// the texture's allocated dimension minus ATLAS_TEXTURE_MIN_PADDING.
// The padding is the bilinear edge for the allocation, plus 1 pixels to account for the white pixel (which has no padding).
#define ATLAS_TEXTURE_MIN_PADDING (BILINEAR_FILTER_EDGE + 1)

extern bool g_dbgEnableTextureAtlasTrace;
extern bool g_dbgEnableTextureAtlasVerboseTrace;

#define ATLAS_TRACE(verbose, format, ...) \
    do \
    { \
        if (g_dbgEnableTextureAtlasTrace && (!verbose || g_dbgEnableTextureAtlasVerboseTrace)) \
        { \
            IGNOREHR(gps->DebugOutputSzNoEndl(L"TEXTURE ATLAS:" format L"\r\n", ##__VA_ARGS__)); \
        } \
    } while (0, 0);

class SWAlphaMaskAtlas
{
public:
    // Note:  The following logic/comments apply to the Windows 8 implementation which did not use DComp and hard-coded the atlas size.
    // The implementation now uses DComp and atlas sizes are more dynamic, however changing the tile size would cause an app-visible
    // behavior change, so for compatibility, we are keeping the tile size hard-coded to 504.
    // Please read the comments below as historical.
    //
    // The tile size is chosen to optimize 2 competing factors:
    // 1. Minimize internal fragmentation of atlases.  The ideal tile size is 509.
    //    At this size, the padding for gutter regions makes each tile 511x511, which leaves just enough room for the "white" pixel in each atlas.
    //    Sizes larger than 509 are very bad because they force atlases to be larger than 512 pixels high.
    //    Sizes less than 509 are acceptable, as the size gets smaller the atlases become more fragmented.
    // 2. Maximize the usable size of a VSIS.  The idea is to make the coordinates of all VSIS tiles (in local element space) be representable as a float32.
    //    504 is a multiple of 8, which allows the coordinates of a VSIS tile to be representable as a float32 with sizes up to 134,217,727 pixels on a side.
    //    After this point, rounding will cause seams between tiles, and stretching/shrinking of tile contents.
    //    OneNote uses very large VSIS objects (Windows 8 bug 678821).
    static const XINT32 TileSize = 504;
};

//------------------------------------------------------------------------------
//
//  Class:     HWTexture
//
//  Synopsis:  Base class for HWTexture's that attach to a DCompSurface
//
//------------------------------------------------------------------------------
class HWTexture : public CXcpObjectBase<IPALSurface>, public CNotifyOnDelete
{
public:
    // Converts a transform that maps from the relative coordinates [0, 1]
    // into a transform that maps from the absolute coordinates [0, width].
    void AddTextureSpaceToLocalSpaceTransform(
        _Inout_ CMILMatrix *pTextureSpaceToLocalSpaceTransform
        );

    virtual bool IsSurfaceLost() { ASSERT(FALSE); return false; }

    virtual DCompSurface* GetCompositionSurface() const { return nullptr; }

    virtual _Check_return_ HRESULT UpdateTextureFromSoftware(
        _In_ IPALSurface *pSourceTexture,
        _In_ const XPOINT& srcOrigin
        )
    {
        ASSERT(FALSE); return E_NOTIMPL;
    }

    virtual _Check_return_ HRESULT LockRect(
        _In_ const XRECT& lockRegion,
        _Outptr_result_bytebuffer_(*stride * *height) void** address,
        _Out_ int32_t* stride,
        _Out_ uint32_t* width,
        _Out_ uint32_t* height
        ) = 0;

    using CXcpObjectBase<IPALSurface>::Unlock;               // This is needed because overload below hides it.
    virtual _Check_return_ HRESULT Unlock(bool update) = 0;

    virtual _Check_return_ HRESULT QueueUpdate() = 0;

    virtual _Check_return_ HRESULT Clear()
    {
        ASSERT(FALSE); return E_NOTIMPL;
    }

    //
    // Unused IPALSurface methods included here simply to provide some implementation
    //

    void SetIsOpaque(bool fIsOpaque) override { ASSERT(FALSE); }

    HRESULT Offer() override { return S_OK; }
    HRESULT Reclaim(_Out_ bool *pWasDiscarded) override { *pWasDiscarded = FALSE; return S_OK; }

    // This class uses CNotifyOnDelete implementation so that we can live in the bitmapcache
    //
    _Check_return_ HRESULT GetNotifyOnDelete(_Outptr_ CNotifyOnDelete **ppNotifyOnDelete) override
    {
        *ppNotifyOnDelete = this;
        return S_OK;
    }

    bool IsVirtual() override { return false; }

    virtual void SetHoldFlush(bool holdFlush) = 0;

    void SetIsPersistent(bool isPersistent) { m_isPersistent = isPersistent; }
    bool IsPersistent() const { return m_isPersistent; }

protected:

    HWTexture(
        _In_ DCompSurface *pDeviceSurface,
        _In_ HWTextureManager *pTextureManager
        );

    ~HWTexture() override;

    HWTextureManager *m_pTextureMgrNoRef;

protected:

    _Notnull_ DCompSurface *m_pDeviceSurface;
    bool m_isPersistent = false;

#if DBG
public:
    HWTexture *m_pNextNoRef;
    HWTexture *m_pPreviousNoRef;
#endif
};

//------------------------------------------------------------------------------
//
//  Class:     HWRgbTexture
//
//  Synopsis:  Texture creation abstraction to allow for texture atlasing
// TODO: JCOMP: mostly obsolete, even with Xaml doing the atlasing for text.
// AtlasNode is the class handling the atlas splitting, and XamlAtlasedDCompSurface
// is the class handling the subrect offset in the atlas. This class handles
// gutter copies.
//
// TODO: JCOMP: Move the gutters to the DCompSurface wrapper. Delete this class.
//
//  All HWTextures include gutters when forwarding the Lock and Unlock calls
//  to the underlying DCompSurface. We then handle the gutter copy
//  within the XAML staging texture. The exception is SIS and VSIS, which don't
//  have a software surface for XAML to copy gutters. They also always forward
//  the BeginDraw/EndDraw calls directly to the underlying DComp surface, so
//  they never call Lock and Unlock, so they never trigger the gutter updates
//  in XAML.
//
//  Suppose a DComp surface is 10x10, and we add a pixel of gutters in each
//  direction. There are actually 3 different rects that are needed:
//
//   1. The rect for the DComp surface is LTRB = { 0, 0, 10, 10 }. This is what
//      we use to call into IDCompositionSurfacePartner::BeginDrawWithGutters.
//      This is also what we use when calculating the texture transform.
//
//   2. The rect in our staging texture is LTRB = { 1, 1, 11, 11 }. This is what
//      we use to put pixels into the staging texture, while leaving enough
//      space to update the gutters later.
//
//   3. The rect used to actually upload to DComp is LTRB = { 0, 0, 12, 12 }.
//      This is the source rect used for the CopySubresourceRegion call.
//
//------------------------------------------------------------------------------
class HWRgbTexture final : public HWTexture
{
public:
    _Check_return_ static HRESULT Create(
        _In_ DCompSurface *pDeviceSurface,
        bool isOpaque,
        _In_ HWTextureManager *pTextureManager,
        _Outptr_ HWTexture **ppHwTexture
        );

    _Check_return_ HRESULT FlushUpdates(_Out_ bool *flushed);

    DCompSurface* GetCompositionSurface() const override { return m_pDeviceSurface; }

    _Check_return_ HRESULT UpdateTextureFromSoftware(
        _In_ IPALSurface* sourceTexture,
        _In_ const XPOINT& srcOrigin
        ) override;

    bool IsSurfaceLost() override;

    _Check_return_ HRESULT LockRect(
        _In_ const XRECT& lockRegion,
        _Outptr_result_bytebuffer_(*stride * *height) void** address,
        _Out_ int32_t* stride,
        _Out_ uint32_t* width,
        _Out_ uint32_t* height
        ) override;

    _Check_return_ HRESULT Unlock(bool update) override;

    //
    // IPALSurface methods
    //

    _Check_return_ HRESULT Lock(
        _Outptr_result_bytebuffer_(*pnStride * *puHeight) void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight) override;

    _Check_return_ HRESULT Unlock() override;

    _Check_return_ HRESULT QueueUpdate() override;

    _Check_return_ HRESULT Clear() override;

    XUINT32 GetWidth() const override;
    XUINT32 GetHeight() const override;

    PixelFormat GetPixelFormat() override;
    bool       IsOpaque() override { return m_isOpaque; }
    bool        IsVirtual() override;

    // This class uses CNotifyOnDelete implementation so that we can live in the bitmapcache
    //
    _Check_return_ HRESULT GetNotifyOnDelete(_Outptr_ CNotifyOnDelete **ppNotifyOnDelete) override
    {
        *ppNotifyOnDelete = this;
        return S_OK;
    }

    void SetHoldFlush(bool holdFlush) override;

private:
    HWRgbTexture(
        _In_ DCompSurface *pDeviceSurface,
        bool isOpaque,
        _In_ HWTextureManager *pTextureManager
        );

    ~HWRgbTexture() override;

    static void CopyGutters(
        _In_reads_(height*stride) XUINT8 *pAddress,
        XUINT32 width,
        XUINT32 height,
        XINT32 stride,
        XUINT32 pixelStride,
        _In_ const XRECT &lockedRect,
        _In_ const XRECT &atlasSubrect // does include padding
        );

private:
    std::mutex m_mutex; // protects the underlying surface

    bool m_isOpaque : 1;
    bool m_holdFlush : 1;
};

//------------------------------------------------------------------------------
//
//  Class:     HWTextureManager
//
//  Synopsis:  Texture creation abstraction to allow for texture atlasing
//
//------------------------------------------------------------------------------
class HWTextureManager : public CInterlockedReferenceCount
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
        _In_ AtlasRequestProvider& atlasRequestProvider,
        _Outptr_ HWTextureManager **ppHWTextureManager
        );

    _Check_return_ HRESULT EnsureLegacyDeviceSurface(_In_ HWTexture *texture, unsigned int width, unsigned int height);

    _Check_return_ HRESULT CreateTexture(
        PixelFormat pixelFormat,
        XUINT32 widthWithoutGutters,
        XUINT32 heightWithoutGutters,
        HWTextureFlags flags,
        _Outptr_ HWTexture **ppTexture,
        _In_opt_ DCompSurfaceFactory *pSurfaceFactory = NULL
        );

    xref_ptr<HWTexture> CreateTextureWithNoHardware(bool isVirtual);

    _Check_return_ HRESULT CreateTextureForHardware(
        XUINT32 widthWithPadding,
        XUINT32 heightWithPadding,
        _Outptr_ HWTexture **ppTexture
        );

    _Check_return_ HRESULT SubmitTextureUpdates();

    void CleanupDeviceRelatedResources(bool cleanupDComp);

    void QueueTextureUpdate(
        _In_ HWRgbTexture *pRgbTexture,
        DXGI_FORMAT compositionSurfaceFormat,
        XUINT32 widthWithGutters,
        XUINT32 heightWithGutters
        );

    _Check_return_ HRESULT GetHitTestTexture(_Outptr_ HWTexture **ppHitTestTexture);

    uint32_t GetMaxTextureSize() const
    {
        return m_maxTextureSizeProvider.GetMaxTextureSize();
    }

private:
    HWTextureManager(
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
        _In_ AtlasRequestProvider& atlasRequestProvider);

    ~HWTextureManager() override;

    void ClearTextureUpdates();

private:
    CWindowRenderTarget *m_pRenderTargetNoRef;
    MaxTextureSizeProvider& m_maxTextureSizeProvider;
    AtlasRequestProvider& m_atlasRequestProvider;

    // Mutex primarily used for controlling access to texture update queue which can be
    // queued from multiple threads.
    std::mutex m_mutex;

    // Contains a list of all newly allocated textures for the frame.
    std::vector<xref_ptr<HWRgbTexture>> m_texturesToUpdate;

    // Tracks the largest DXGI_FORMAT_B8G8R8A8_UNORM texture size used in the frame
    XUINT32 m_uMaxTextureWidthInFrame;
    XUINT32 m_uMaxTextureHeightInFrame;

    HWTexture *m_pHitTestTexture;

#if DBG
public:
    HWTexture *m_pGlobalListHeadNoRef;
#endif

};

