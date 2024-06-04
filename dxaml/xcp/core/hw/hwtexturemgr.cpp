// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <LinearGradientBrush.h>
#include <PixelFormat.h>

bool g_dbgEnableTextureAtlasTrace = false;
bool g_dbgEnableTextureAtlasVerboseTrace = false;


// Rules concerning Virtual Surfaces
//
// In RS2, we introduced the concept of using HWTexture as a means to operate on
// virtual surfaces in a tiled way.  This includes the addition of a new LockRect
// which allows for locking a region of an image and writing to it with software bits.
//
// Usage of the API's are governed under the following rules:
//     - Any surface that has width or height that exceeds MaxTextureSize
//       must allocate the texture as virtual and use LockRect.
//     - Do not use Lock() to lock a virtual surface, instead use LockRect().
//       This is done so that clients that use the API are forced to implement
//       their algorithm in a scalable way that is not limited by max texture size.
//     - LockRect should not attempt to use a rect with width and height greater
//       than MaxTextureSize.
//
// Internal rules governing virtual surfaces are as follows:
//     - DComp does not support BeginDrawWithGutters() if the surface is virtual.
//       This means that if padding is in use, the texture dimensions must be
//       less than MaxTextureSize.
//     - If padding is not used, then the staging surface should be kept around
//       to easily allow for filling in the gutters without incurring the cost of
//       and additional CopySubresource.
//     - If padding is used, the staging surface can be allocated to full texture
//       size and be reused to fill in padding later.


//------------------------------------------------------------------------------
//
//  Synopsis:
//      Copies 1 rectangle from a surface onto another non-intersecting rectangle
//
//------------------------------------------------------------------------------
template<typename PixelType>
static void
CopyRect(
    const XPOINT &dstPoint,
    const XRECT &srcRect,
     _In_reads_(height*stride) XUINT8 *pAddress,
    XUINT32 width,
    XUINT32 height,
    XINT32 stride
    )
{
    ASSERT(dstPoint.x >= 0);
    ASSERT(dstPoint.y >= 0);
    ASSERT(srcRect.X >= 0);
    ASSERT(srcRect.Y >= 0);
    ASSERT(srcRect.Width >= 0);
    ASSERT(srcRect.Height >= 0);

    ASSERT((srcRect.X + srcRect.Width) <= static_cast<XINT32>(width));
    ASSERT((srcRect.Y + srcRect.Height) <= static_cast<XINT32>(height));

    ASSERT((dstPoint.x + srcRect.Width) <= static_cast<XINT32>(width));
    ASSERT((dstPoint.y + srcRect.Height) <= static_cast<XINT32>(height));

    // Inform OACR about src rect bounds
    _Analysis_assume_((pAddress + ((srcRect.Y + srcRect.Height) * stride) + ((srcRect.X + srcRect.Width) * sizeof(PixelType))) <= (pAddress + (height * stride)));

    // Inform OACR about dst rect bounds
    _Analysis_assume_((pAddress + ((dstPoint.y + srcRect.Height) * stride) + ((dstPoint.x + srcRect.Width) * sizeof(PixelType))) <= (pAddress + (height * stride)));

    const PixelType *pSrcScan = reinterpret_cast<const PixelType *>(pAddress + srcRect.Y * stride);
    PixelType *pDstScan = reinterpret_cast<PixelType *>(pAddress + dstPoint.y * stride);

    for (XINT32 y = 0; y < srcRect.Height; y++)
    {
        for (XINT32 x = 0; x < srcRect.Width; x++)
        {
            pDstScan[x + dstPoint.x] = pSrcScan[x + srcRect.X];
        }

        pSrcScan = reinterpret_cast<const PixelType *>(reinterpret_cast<const XUINT8 *>(pSrcScan) + stride);
        pDstScan = reinterpret_cast<PixelType *>(reinterpret_cast<XUINT8 *>(pDstScan) + stride);
    }
}

HWTexture::HWTexture(
    _In_ DCompSurface *pDeviceSurface,
    _In_ HWTextureManager *pTextureManager
    )
    : m_pDeviceSurface(pDeviceSurface)
    , m_pTextureMgrNoRef(pTextureManager)
{
    m_pDeviceSurface->AddRef();
}

HWTexture::~HWTexture()
{
    ReleaseInterface(m_pDeviceSurface);

    FireOnDelete();
}


HWRgbTexture::HWRgbTexture(
    _In_ DCompSurface *pDeviceSurface,
    bool isOpaque,
    _In_ HWTextureManager *pTextureManager
    )
    : HWTexture(pDeviceSurface, pTextureManager)
    , m_isOpaque(isOpaque)
    , m_holdFlush(FALSE)
{
#if DBG
    // Insert self at the head of the global list.
    m_pNextNoRef = m_pTextureMgrNoRef->m_pGlobalListHeadNoRef;
    if (m_pNextNoRef != NULL)
    {
        m_pNextNoRef->m_pPreviousNoRef = this;
    }
    m_pPreviousNoRef = NULL;
    m_pTextureMgrNoRef->m_pGlobalListHeadNoRef = this;
#endif
}

HWRgbTexture::~HWRgbTexture()
{
    FireOnDelete();

#if DBG
    // Remove self from the global list.
    if (m_pPreviousNoRef != NULL)
    {
        m_pPreviousNoRef->m_pNextNoRef = m_pNextNoRef;
    }
    if (m_pNextNoRef != NULL)
    {
        m_pNextNoRef->m_pPreviousNoRef = m_pPreviousNoRef;
    }
    if (this == m_pTextureMgrNoRef->m_pGlobalListHeadNoRef)
    {
        m_pTextureMgrNoRef->m_pGlobalListHeadNoRef = m_pNextNoRef;
    }
#endif

}

_Check_return_ HRESULT
HWRgbTexture::Create(
    _In_ DCompSurface *pDeviceSurface,
    bool isOpaque,
    _In_ HWTextureManager *pTextureManager,
    _Outptr_ HWTexture **ppHWTexture
    )
{
    HRESULT hr = S_OK;
    HWRgbTexture *pHWRgbTexture = NULL;

    pHWRgbTexture = new HWRgbTexture(
        pDeviceSurface,
        isOpaque,
        pTextureManager
        );

    *ppHWTexture = pHWRgbTexture;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Lock the surface for write. Returns the address, width, and height
//      without gutters. Used by the rest of XAML to fill the HWRgbTexture with
//      content. The gutters are copied automatically when this HWRgbTexture is
//      unlocked.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
HWRgbTexture::Lock(
    _Outptr_result_bytebuffer_(*pnStride * *puHeight) void **ppAddress,
    _Out_ XINT32 *pnStride,
    _Out_ XUINT32 *puWidth,
    _Out_ XUINT32 *puHeight)
{
    // Virtual surfaces shouldn't use lock since they typically cannot lock the entire surface.
    // They should use LockRect instead to lock a smaller region of the surface.  While it is possible
    // for virtual surfaces to be locked if their size is sufficiently small, the desire is to enforce
    // a pattern that virtual surfaces should always use LockRect in order to detect erroneous behavior
    // with using Lock.  It also promotes coding with scalable algorithms that can exceed max texture size.
    ASSERT(!IsVirtual());

    void *pAddress;
    XUINT32 width;
    XUINT32 height;
    XINT32 stride;

    XUINT32 sizeAdjustment;
    XUINT32 offsetAdjustment;

    std::unique_lock<std::mutex> lock(m_mutex);

    IFC_RETURN(m_pDeviceSurface->Lock(
        &pAddress,
        &stride,
        &width,
        &height
        ));

    ASSERT(width == m_pDeviceSurface->GetWidthWithGutters());
    ASSERT(height == m_pDeviceSurface->GetHeightWithGutters());

    if (m_pDeviceSurface->IncludesGutters())
    {
        // Adjust returned size and offset to account for padding
        sizeAdjustment = BILINEAR_FILTER_EDGE;
        offsetAdjustment = BILINEAR_FILTER_EDGE / 2;
    }
    else
    {
        sizeAdjustment = 0;
        offsetAdjustment = 0;
    }

    *ppAddress =
          reinterpret_cast<XUINT8 *>(pAddress)
        + offsetAdjustment * GetPixelStride(GetPixelFormat())
        + offsetAdjustment * stride;

    *puWidth = width - sizeAdjustment;
    *puHeight = height - sizeAdjustment;

    *pnStride = stride;

    // Leave the mutex locked until HWRgbTexture::Unlock() call
    lock.release();

    return S_OK;
}

_Check_return_ HRESULT
HWRgbTexture::LockRect(
    _In_ const XRECT& lockRegion,
    _Outptr_result_bytebuffer_(*stride * *height) void** address,
    _Out_ int32_t* stride,
    _Out_ uint32_t* width,
    _Out_ uint32_t* height
    )
{
    std::unique_lock<std::mutex> mutexLock(m_mutex);

    if (m_pDeviceSurface->IncludesGutters())
    {
        void* lockAddress = nullptr;
        int32_t lockStride = 0;
        uint32_t lockWidth = 0;
        uint32_t lockHeight = 0;
        IFC_RETURN(m_pDeviceSurface->Lock(
            &lockAddress,
            &lockStride,
            &lockWidth,
            &lockHeight
            ));

        uint32_t offsetAdjustment = BILINEAR_FILTER_EDGE / 2;

        ASSERT(lockWidth == m_pDeviceSurface->GetWidthWithGutters());
        ASSERT(lockHeight == m_pDeviceSurface->GetHeightWithGutters());

        *address =
            reinterpret_cast<uint8_t*>(lockAddress)
            + (offsetAdjustment + lockRegion.X) * GetPixelStride(GetPixelFormat())
            + (offsetAdjustment + lockRegion.Y) * lockStride;

        *stride = lockStride;
        *width = lockRegion.Width;
        *height = lockRegion.Height;

    }
    else
    {
        IFC_RETURN(m_pDeviceSurface->LockRect(
            lockRegion,
            address,
            stride,
            width,
            height
            ));
    }

    // Leave the mutex locked until HWRgbTexture::Unlock() call
    mutexLock.release();

    return S_OK;
}

_Check_return_ HRESULT
HWRgbTexture::Unlock()
{
    // Default behavior is to unlock and queue update immediately.
    return Unlock(true);
}

_Check_return_ HRESULT
HWRgbTexture::Unlock(bool update)
{
    std::unique_lock<std::mutex> mutexLock(m_mutex, std::adopt_lock);

    IFC_RETURN(m_pDeviceSurface->Unlock());

    mutexLock.unlock();

    if (update)
    {
        IFC_RETURN(QueueUpdate());
    }

    return S_OK;
}

// Updates the gutters of an image, marks it dirty and queues the update
// This should be called implicitly when unlocking a surface
_Check_return_ HRESULT
HWRgbTexture::QueueUpdate()
{
    std::unique_lock<std::mutex> mutexLock(m_mutex);

    if (m_pDeviceSurface->IncludesGutters())
    {
        void *pLockedBits = NULL;
        XINT32 stride = 0;
        XUINT32 width = 0;
        XUINT32 height = 0;

        // Recursive locks are supported, this simply returns the same pointer that the last call to Lock returned
        IFC_RETURN(m_pDeviceSurface->Lock(
            &pLockedBits,
            &stride,
            &width,
            &height
            ));

        XRECT contentRect =
        {
            1,
            1,
            m_pDeviceSurface->GetWidthWithoutGutters(),
            m_pDeviceSurface->GetHeightWithoutGutters()
        };

        XRECT gutterRect =
        {
            0,
            0,
            m_pDeviceSurface->GetWidthWithGutters(),
            m_pDeviceSurface->GetHeightWithGutters()
        };

        //
        // Fill it the gutters in the staging texture on the CPU, then update the DComp surface with the texture
        // contents as well as the gutters. Updating gutters here means DComp won't have to issue multiple
        // expensive CopySubresourceRegion calls to update gutters themselves.
        //
        CopyGutters(
            reinterpret_cast<XUINT8*>(pLockedBits),
            width,
            height,
            stride,
            GetPixelStride(GetPixelFormat()),
            contentRect,
            gutterRect
            );

        IFC_RETURN(m_pDeviceSurface->Unlock());
    }

    IFC_RETURN(m_pDeviceSurface->QueueUpdate());

    // Make sure to unlock the mutex before calling QueueTextureUpdate which will take a lock on the texture manager
    // lock.  If this is not done, it could cause a deadlock because HWTextureManager::SubmitTextureUpdates calls
    // HWRgbTexture::FlushUpdates (both of which take locks for their respective classes) thus resulting in a double
    // lock.  If this also takes a lock on both objects in the reverse order, then a deadlock could occur in rare
    // circumstances.
    // It is possible for HWTextureManager::SubmitTextureUpdates to avoid taking a double lock, but it would have to
    // make a copy of the m_texturesToUpdate list inside it's lock and then unlock the mutex before iterating through the
    // elements.  The performance oriented solution is to just unlock this mutex here and allow SubmitTextureUpdates to take
    // both locks.
    mutexLock.unlock();

    // Register for a texture update of the underlying surface at the end of the frame.
    m_pTextureMgrNoRef->QueueTextureUpdate(
        this,
        m_pDeviceSurface->GetCompositionSurfaceFormat(),
        m_pDeviceSurface->GetWidthWithGutters(),
        m_pDeviceSurface->GetHeightWithGutters());

    return S_OK;
}

// Fill the texture with transparent blackness.
_Check_return_ HRESULT
HWRgbTexture::Clear()
{
    void* destAddress = nullptr;
    auto destStride = int32_t{ 0 };
    auto destWidth = uint32_t{ 0 };
    auto destHeight = uint32_t{ 0 };

    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(Lock(
        &destAddress,
        &destStride,
        &destWidth,
        &destHeight));

    auto unlockGuard = wil::scope_exit([&]
    {
        IGNOREHR(Unlock());
    });

    uint8_t* buffer = reinterpret_cast<uint8_t*>(destAddress);
    ZeroMemory(buffer, destStride * destHeight);


    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Fills in the padding regions around a rectangle with the contents from the edges of the rectangle
//
// TODO: JCOMP: Can be simplified now that the rects are guaranteed to be the entire texture.
//
//------------------------------------------------------------------------------
/*static*/ void
HWRgbTexture::CopyGutters(
    _In_reads_(height*stride) XUINT8 *pAddress,
    XUINT32 width,
    XUINT32 height,
    XINT32 stride,
    XUINT32 pixelStride,
    _In_ const XRECT &lockedRect,
    _In_ const XRECT &atlasSubrect // does include padding
    )
{
    ASSERT(  (atlasSubrect.X + atlasSubrect.Width) * static_cast<XINT32>(pixelStride) <= stride
           && atlasSubrect.Y + atlasSubrect.Height <= static_cast<XINT32>(height));

    bool intersectLeft   = lockedRect.X == (atlasSubrect.X + 1);
    bool intersectRight  = (lockedRect.X + lockedRect.Width + 1) == (atlasSubrect.X + atlasSubrect.Width);
    bool intersectTop    = lockedRect.Y == (atlasSubrect.Y + 1);
    bool intersectBottom = (lockedRect.Y + lockedRect.Height + 1) == (atlasSubrect.Y + atlasSubrect.Height);

    struct
    {
        bool   doesIntersect;
        XPOINT dstPoint;
        XRECT  srcRect;
    } gutterRegions[] =
    {
        // left column
        {
            intersectLeft,
            { atlasSubrect.X, lockedRect.Y },
            { lockedRect.X, lockedRect.Y, 1, lockedRect.Height }
        },

        // top-left corner
        {
            intersectLeft && intersectTop,
            { atlasSubrect.X, atlasSubrect.Y },
            { lockedRect.X, lockedRect.Y, 1, 1 }
        },

        // top row
        {
            intersectTop,
            { lockedRect.X, atlasSubrect.Y },
            { lockedRect.X, lockedRect.Y, lockedRect.Width, 1 }
        },

        // top-right corner
        {
            intersectTop && intersectRight,
            { atlasSubrect.X + atlasSubrect.Width - 1, atlasSubrect.Y },
            { lockedRect.X + lockedRect.Width - 1, lockedRect.Y, 1, 1 },
        },

        // right column
        {
            intersectRight,
            { atlasSubrect.X + atlasSubrect.Width - 1, lockedRect.Y },
            { lockedRect.X + lockedRect.Width - 1, lockedRect.Y, 1, lockedRect.Height }
        },

        // bottom-right corner
        {
            intersectBottom && intersectRight,
            { atlasSubrect.X + atlasSubrect.Width - 1, atlasSubrect.Y + atlasSubrect.Height - 1 },
            { lockedRect.X + lockedRect.Width - 1, lockedRect.Y + lockedRect.Height - 1, 1, 1 },
        },

        // bottom row
        {
            intersectBottom,
            { lockedRect.X, atlasSubrect.Y + atlasSubrect.Height - 1 },
            { lockedRect.X, lockedRect.Y + lockedRect.Height - 1, lockedRect.Width, 1 }
        },

        // bottom-left corner
        {
            intersectBottom && intersectLeft,
            { atlasSubrect.X, atlasSubrect.Y + atlasSubrect.Height - 1 },
            { lockedRect.X, lockedRect.Y + lockedRect.Height - 1, 1, 1 }
        },
    };

    // For each of the 8 gutter regions
    for (XUINT32 i = 0; i < ARRAY_SIZE(gutterRegions); i++)
    {
        if (gutterRegions[i].doesIntersect)
        {
#if DBG
            // dst and source rects must not intersect
            XRECT dstRect = { gutterRegions[i].dstPoint.x, gutterRegions[i].dstPoint.y, gutterRegions[i].srcRect.Width, gutterRegions[i].srcRect.Height };
            XRECT intersection = gutterRegions[i].srcRect;
            ASSERT(!IntersectRect(&intersection, &dstRect));

            // dst rect must be contained by the atlas subrect with padding
            ASSERT(DoesRectContainRect(&atlasSubrect, &dstRect));

            // src rect must be contained by the locked rect
            ASSERT(DoesRectContainRect(&lockedRect, &gutterRegions[i].srcRect));
#endif
            switch (pixelStride)
            {
            case 1:
                CopyRect<XBYTE>(
                    gutterRegions[i].dstPoint,
                    gutterRegions[i].srcRect,
                    pAddress,
                    width,
                    height,
                    stride
                    );
                break;

            case 4:
                CopyRect<XUINT32>(
                    gutterRegions[i].dstPoint,
                    gutterRegions[i].srcRect,
                    pAddress,
                    width,
                    height,
                    stride
                    );
                break;

            case 8:
                CopyRect<uint64_t>(
                    gutterRegions[i].dstPoint,
                    gutterRegions[i].srcRect,
                    pAddress,
                    width,
                    height,
                    stride
                    );
                break;

            default:
                ASSERT(FALSE);
            }
        }
    }
}

_Check_return_ HRESULT HWRgbTexture::FlushUpdates(_Out_ bool *flushed)
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    *flushed = false;

    // m_holdFlush is meant to allow background threads time to queue multiple updates to a surface and prevent
    // FlushUpdates from flushing partial updates to a surface.  This is very useful in the virtual surface
    // scenario where there are multiple updates made to a single large texture that should be flushed in the
    // same batch but might be queued separately.
    if (!m_holdFlush)
    {
        IFC_RETURN(m_pDeviceSurface->FlushUpdates());
        *flushed = true;
    }

    return S_OK;
}

_Check_return_ HRESULT
HWRgbTexture::UpdateTextureFromSoftware(
    _In_ IPALSurface* sourceSurface,
    _In_ const XPOINT& sourceOrigin
    )
{
    const PixelFormat sourcePixelFormat = sourceSurface->GetPixelFormat();
    const PixelFormat targetPixelFormat = m_pDeviceSurface->GetPixelFormat();

    const bool pixelFormatsAreCompatible = (sourcePixelFormat == targetPixelFormat);

    const bool supportedPixelFormat = (sourcePixelFormat != pixelGray1bpp);

    ASSERT(pixelFormatsAreCompatible && supportedPixelFormat);

    // If the dest surface exceeds max texture size, ensure that it is virtual.
    uint32_t textureWidth = GetWidth();
    uint32_t textureHeight = GetHeight();
    const uint32_t maxTextureSize = m_pTextureMgrNoRef->GetMaxTextureSize();
    bool needsTiling = ((textureWidth > maxTextureSize) || (textureHeight > maxTextureSize));
    ASSERT(!needsTiling || IsVirtual());

    // Lock the source texture
    uint8_t* sourceBuffer;
    int32_t sourceStride;
    uint32_t sourceWidth;
    uint32_t sourceHeight;

    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(sourceSurface->Lock(
        reinterpret_cast<void**>(&sourceBuffer),
        &sourceStride,
        &sourceWidth,
        &sourceHeight));
    auto autoUnlockSourceGuard = wil::scope_exit([&]
    {
        IFCFAILFAST(sourceSurface->Unlock());
    });

    // Adjust the source offset, the stride will ensure it resumes at the same point.
    sourceBuffer += (sourceOrigin.y * sourceStride) + (sourceOrigin.x * GetPixelStride(GetPixelFormat()));

    // It is expected that pixel formats byte sizes match and is enforced at the top of this method.
    uint32_t pixelSize = GetPixelStride(sourceSurface->GetPixelFormat());

    // Tiling and non-tiling will use the same copy algorithm except it will adjust the tile increment size
    // appropriately.  In the case of non-tiling, it could copy each segment of the image a piece at a time.
    // However, it is currently more optimal to copy it all at once if it can.  In the case of tiling,
    // it is more optimal to use SWAlphaMaskAtlas::TileSize for reasons explained where it is defined.
    uint32_t xTilingIncrement = textureWidth;
    uint32_t yTilingIncrement = textureHeight;

    if (needsTiling)
    {
        xTilingIncrement = SWAlphaMaskAtlas::TileSize;
        yTilingIncrement = SWAlphaMaskAtlas::TileSize;
    }

    // Iterate through the texture by tiles and copy each tile segment individually.
    for (uint32_t tileOffsetY = 0; tileOffsetY < textureHeight; tileOffsetY += yTilingIncrement)
    {
        const uint32_t tileHeight = MIN(yTilingIncrement, textureHeight - tileOffsetY);

        for (uint32_t tileOffsetX = 0; tileOffsetX < textureWidth; tileOffsetX += xTilingIncrement)
        {
            const uint32_t tileWidth = MIN(xTilingIncrement, textureWidth - tileOffsetX);

            XRECT updateRect = { tileOffsetX, tileOffsetY, tileWidth, tileHeight };

            // Between LockRect and Unlock, there should be no IFC!  Otherwise a scope_guard should be used to unlock it.
            uint8_t* destBuffer = nullptr;
            int32_t destStride = 0;
            uint32_t destWidth = 0;
            uint32_t destHeight = 0;

            IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(LockRect(
                updateRect,
                reinterpret_cast<void**>(&destBuffer),
                &destStride,
                &destWidth,
                &destHeight));
            auto autoUnlockDestGuard = wil::scope_exit([&]
            {
                IFCFAILFAST(Unlock())
            });

            // Setup the copy inner-loop
            // The goal is to avoid multiplications and use memcpy_s which typically uses inline assembly to copy fast.
            uint8_t* destLine = destBuffer;
            uint32_t sourceOffset = (tileOffsetX * pixelSize) + (tileOffsetY * sourceStride);
            const uint8_t* sourceLine = sourceBuffer + sourceOffset;
            uint32_t rectWidthInBytes = tileWidth * pixelSize;

            // Copy quickly (hot loop)
            for (uint32_t copyLine = 0; copyLine < tileHeight; copyLine++)
            {
                memcpy_s(destLine, destStride, sourceLine, rectWidthInBytes);

                destLine += destStride;
                sourceLine += sourceStride;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the pixel format.
//
//------------------------------------------------------------------------------
PixelFormat
HWRgbTexture::GetPixelFormat()
{
    return m_pDeviceSurface->GetPixelFormat();
}

bool
HWRgbTexture::IsVirtual()
{
    return m_pDeviceSurface->IsVirtual();
}


void HWRgbTexture::SetHoldFlush(bool holdFlush)
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    m_holdFlush = holdFlush;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the logical width. Assumes the caller is not interested in gutters.
//
//------------------------------------------------------------------------------
XUINT32
HWRgbTexture::GetWidth() const
{
    return m_pDeviceSurface->GetWidthWithoutGutters();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the logical height. Assumes the caller is not interested in gutters.
//
//------------------------------------------------------------------------------
XUINT32
HWRgbTexture::GetHeight() const
{
    return m_pDeviceSurface->GetHeightWithoutGutters();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if the underlying device surface is discarded.
//
//------------------------------------------------------------------------------
bool
HWRgbTexture::IsSurfaceLost()
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    return (m_pDeviceSurface != NULL &&
            m_pDeviceSurface->IsDiscarded());
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     ctor
//
//-------------------------------------------------------------------------
HWTextureManager::HWTextureManager(
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
    _In_ AtlasRequestProvider& atlasRequestProvider
    )
    : m_pRenderTargetNoRef(pRenderTarget)
    , m_maxTextureSizeProvider(maxTextureSizeProvider)
    , m_atlasRequestProvider(atlasRequestProvider)
    , m_pHitTestTexture(NULL)
{
    XCP_WEAK(&m_pRenderTargetNoRef);

    m_uMaxTextureWidthInFrame = 0;
    m_uMaxTextureHeightInFrame = 0;

#if DBG
    m_pGlobalListHeadNoRef = NULL;
#endif
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     dtor
//
//-------------------------------------------------------------------------
HWTextureManager::~HWTextureManager()
{
    ReleaseInterface(m_pHitTestTexture);

    ClearTextureUpdates();
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create a HWTextureManager object
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWTextureManager::Create(
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ MaxTextureSizeProvider& maxTextureSizeProvider,
    AtlasRequestProvider& atlasRequestProvider,
    _Outptr_ HWTextureManager **ppHWTextureManager
    )
{
    HRESULT hr = S_OK;
    HWTextureManager *pHWTextureManager = NULL;

    pHWTextureManager = new HWTextureManager(pRenderTarget, maxTextureSizeProvider, atlasRequestProvider);

    *ppHWTextureManager = pHWTextureManager;
    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT HWTextureManager::EnsureLegacyDeviceSurface(_In_ HWTexture *texture, unsigned int width, unsigned int height)
{
    DCompSurface *compositionSurface = texture->GetCompositionSurface();
    ASSERT(compositionSurface != nullptr);

    DCompTreeHost *dcompTreeHost = m_pRenderTargetNoRef->GetDCompTreeHost();
    IFC_RETURN(dcompTreeHost->EnsureLegacyDeviceSurface(compositionSurface, width, height));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create a HWTextureManager object.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWTextureManager::CreateTexture(
    PixelFormat pixelFormat,
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    HWTextureFlags flags,
    _Outptr_ HWTexture **ppTexture,
    _In_opt_ DCompSurfaceFactory *pChildDevice
    )
{
    HRESULT hr = S_OK;

    ASSERT(pixelFormat == pixelGray8bpp
        || pixelFormat == pixelColor32bpp_A8R8G8B8
        || pixelFormat == pixelColor64bpp_R16G16B16A16_Float);

    HWTexture *pTexture = NULL;
    DCompSurface *pDeviceSurface = NULL;

    const bool isVirtual = !!(flags & HWTextureFlags_IsVirtual);
    const bool isOpaque = !!(flags & HWTextureFlags_IsOpaque);
    const bool isAlphaMask = pixelFormat == pixelGray8bpp;
    const bool isHDR = pixelFormat == pixelColor64bpp_R16G16B16A16_Float;
    const bool requestAtlas = isVirtual ? true : m_atlasRequestProvider.AtlasRequest(widthWithoutGutters, heightWithoutGutters, pixelFormat);

    // Allocate textures using the framework's device, unless another device is specified.
    if (pChildDevice != NULL)
    {
        // There's no technical reason this isn't supported, it just isn't expected.
        ASSERT(!isAlphaMask);

        IFC(pChildDevice->CreateSurface(
            widthWithoutGutters,
            heightWithoutGutters,
            isOpaque,
            isVirtual,
            isHDR,
            requestAtlas,
            &pDeviceSurface
            ));
    }
    else
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pRenderTargetNoRef->GetDCompTreeHost();
        IFC(pDCompTreeHostNoRef->CreateSurface(
            widthWithoutGutters,
            heightWithoutGutters,
            isOpaque,
            isAlphaMask,
            isVirtual,
            isHDR,
            requestAtlas,
            &pDeviceSurface
            ));
    }

    IFC(HWRgbTexture::Create(
        pDeviceSurface,
        isOpaque,
        this,
        &pTexture
        ));

    *ppTexture = pTexture;
    pTexture = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pTexture);
    ReleaseInterfaceNoNULL(pDeviceSurface);

    RRETURN(hr);
}

// Allocate a HWTexture and WinRT surface wrapper but no hardware surface.
xref_ptr<HWTexture> HWTextureManager::CreateTextureWithNoHardware(bool isVirtual)
{
    xref_ptr<HWTexture> hwTexture;
    xref_ptr<DCompSurface> dcompSurface = m_pRenderTargetNoRef->GetDCompTreeHost()->CreateSurfaceWithNoHardware(isVirtual);

    IFCFAILFAST(HWRgbTexture::Create(
        dcompSurface,
        false, // isOpaque
        this,
        hwTexture.ReleaseAndGetAddressOf()
        ));

    hwTexture->SetIsPersistent(true);

    return hwTexture;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create a HWTexture object.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
HWTextureManager::CreateTextureForHardware(
    XUINT32 widthWithPadding,
    XUINT32 heightWithPadding,
    _Outptr_ HWTexture **ppTexture
    )
{
    HRESULT hr = S_OK;
    HWTexture *pTexture = NULL;

    //
    // If the texture is too big to fit into the atlas, find a mip-level that does fit
    //

    const uint32_t maxTextureSize = GetMaxTextureSize();
    if (widthWithPadding > maxTextureSize || heightWithPadding > maxTextureSize)
    {
        //TODO: PC: Generate hardware mip map.
        ASSERT(FALSE);
    }

    //
    // Create the texture
    //
    // For hardware rendered surfaces (ie D2D), we have no way to apply the 1 pixel padding
    // because it would require locking and drawing into the atlas before we update it from the
    // D2D surface. Instead, we have D2D create a surface that is BILINEAR_FILTER_EDGE larger
    // than it would otherwise be, and have it fill the padding for us.
    // Here we pass an adjusted width/height and includesPadding = TRUE, so that we end
    // up with the correctly sized texture, and so that HWRgbTexture::GetUV returns the correct
    // padding adjusted coordinates for rendering.
    //
    IFC(CreateTexture(
        pixelColor32bpp_A8R8G8B8,
        widthWithPadding - BILINEAR_FILTER_EDGE,
        heightWithPadding - BILINEAR_FILTER_EDGE,
        HWTextureFlags_IncludePadding, // TODO: HWPC: Must we assume D2D content is transparent
        &pTexture));

    //
    // Return the texture
    //

    *ppTexture = pTexture;
    pTexture = NULL;

Cleanup:
    ReleaseInterface(pTexture);
    RRETURN(hr);
}

// Test hook to suspend updating surfaces.
void HWTextureManager::SetSuspendSurfaceUpdates(bool isSuspended)
{
    m_areSurfaceUpdatesSuspended = isSuspended;
    // It's up to the test to trigger another frame to flush all the pending surface updates.
}

// Causes every surface that was updated this frame to flush its pending updates.
// This is typically done before DComp submission which is currently done on the UI thread.
// It could be potentially
_Check_return_ HRESULT
HWTextureManager::SubmitTextureUpdates()
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    TracePrimitiveCompositionUpdateSurfacesBegin();
    auto etwStopOnEnd = wil::scope_exit([] { TracePrimitiveCompositionUpdateSurfacesEnd(); });

    if (!m_texturesToUpdate.empty() && !m_areSurfaceUpdatesSuspended)
    {
        WindowsGraphicsDeviceManager *pDeviceManagerNoRef = m_pRenderTargetNoRef->GetGraphicsDeviceManager();
        CD3D11Device *pDeviceNoRef = pDeviceManagerNoRef->GetGraphicsDevice();

        pDeviceNoRef->NotifyLargestTextureInFrame(DXGI_FORMAT_B8G8R8A8_UNORM, m_uMaxTextureWidthInFrame, m_uMaxTextureHeightInFrame);

        auto current = m_texturesToUpdate.begin();
        for (auto it = m_texturesToUpdate.begin(); it != m_texturesToUpdate.end(); it++)
        {
            bool flushed;
            IFC_RETURN((*it)->FlushUpdates(&flushed));

            if (!flushed)
            {
                *current++ = std::move(*it);
            }
        }

        m_texturesToUpdate.erase(current, m_texturesToUpdate.end());

        if (m_texturesToUpdate.empty())
        {
            m_uMaxTextureWidthInFrame = 0;
            m_uMaxTextureHeightInFrame = 0;
        }
    }


    return S_OK;
}

// Notifies the texture manager that a surface was updated this tick.
// This can be called from any thread.
void HWTextureManager::QueueTextureUpdate(
    _In_ HWRgbTexture *pRgbTexture,
    DXGI_FORMAT compositionSurfaceFormat,
    XUINT32 widthWithGutters,
    XUINT32 heightWithGutters
)
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    // Save some work by checking if it is already queued for update.
    bool alreadyQueued = (std::find(m_texturesToUpdate.begin(), m_texturesToUpdate.end(), pRgbTexture) != m_texturesToUpdate.end());

    if (!alreadyQueued)
    {
        m_texturesToUpdate.emplace_back(pRgbTexture);

        // Bug 17486965: ShellExperienceHost is using 4GB of memory
        //
        // In the one dump we had for this bug, there were 104 thousand entries in this
        // textures to update list.  Somehow we were getting them in, and not getting
        // them out.  There are some theories as to how this occurs (such as ShellExperienceHost
        // tiles being updated while we aren't ticking), but without dumps it is hard to
        // narrow down the possible cause.  In the typical case, it is thought that
        // as some point this will just recover and eventually drain, but in other cases it
        // can cause memory failures.  In order to try to get a handle on where these entries
        // are coming from, we will fail fast for a set of internal users and hopefully
        // get a series of dumps that will allow this to be better investigated.
        if (m_texturesToUpdate.size() >= 20000)
        {
            if (Feature_RunawayTextureList::IsEnabled())
            {
                IFCFAILFAST(E_UNEXPECTED);
            }
            MICROSOFT_TELEMETRY_ASSERT_MSG_DISABLED(m_texturesToUpdate.size() != 20000, "Excessive number of texture updates queued");
        }
    }

    if (DXGI_FORMAT_B8G8R8A8_UNORM == compositionSurfaceFormat)
    {
        if (widthWithGutters > m_uMaxTextureWidthInFrame)
        {
            m_uMaxTextureWidthInFrame = widthWithGutters;
        }

        if (heightWithGutters > m_uMaxTextureHeightInFrame)
        {
            m_uMaxTextureHeightInFrame = heightWithGutters;
        }
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Drops pending texture updates.
//
//-------------------------------------------------------------------------
void
HWTextureManager::ClearTextureUpdates()
{
    m_texturesToUpdate.clear();

    m_uMaxTextureWidthInFrame = 0;
    m_uMaxTextureHeightInFrame = 0;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the atlas pools, clears the pending textures for notify.
//
//-----------------------------------------------------------------------------
void
HWTextureManager::CleanupDeviceRelatedResources(bool cleanupDComp)
{
    std::lock_guard<std::mutex> mutexGuard(m_mutex);

    ReleaseInterface(m_pHitTestTexture);

    ClearTextureUpdates();

#if DBG
    if (cleanupDComp)
    {
        // Assert that all the textures are released.
        // TODO 36331906: This assert is being hit in a number of tests following the WinUI 2.6 port.
        // We should either re-enable it if it's still relevant once we figure out what's causing it to be hit,
        // or remove it entirely if it's not still relevant.
        //ASSERT(m_pGlobalListHeadNoRef == NULL);
    }
    else
    {
        // Assert that only persistent textures are present.
        for (auto current = m_pGlobalListHeadNoRef; current != nullptr; current = current->m_pNextNoRef)
        {
            ASSERT(current->IsPersistent());
        }
    }
#endif
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a texture that can be used to make a primitive hit-testable
//      without incurring extra rendering overhead.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
HWTextureManager::GetHitTestTexture(_Outptr_ HWTexture **ppHitTestTexture)
{
    if (m_pHitTestTexture == NULL)
    {
        // Virtual surfaces are hit-testable even if they haven't been drawn into.
        IFC_RETURN(CreateTexture(
            pixelColor32bpp_A8R8G8B8,
            1, /*width*/
            1, /*height*/
            HWTextureFlags_IsVirtual,
            &m_pHitTestTexture
            ));
    }

    SetInterface(*ppHitTestTexture, m_pHitTestTexture);

    return S_OK;
}
