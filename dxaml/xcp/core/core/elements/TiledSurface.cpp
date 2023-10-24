// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AAEdgeFlags.h>
#include <PixelFormat.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Given the origin of a tile, computes the bounds of that tile (clipped to the bounds of the tiled surface).
//      Returns TRUE if there is any intersection between the tile and the surface
//
//------------------------------------------------------------------------
static bool
GetClippedTileBoundsHelper(
    const XPOINT &origin,
    const XSIZE &surfaceSize,
    _Out_ XRECT *pBounds
    )
{
    // Tile origin must be positive
    ASSERT(origin.x >= 0);
    ASSERT(origin.y >= 0);

    // Tile origin must be aligned to tile grid
    ASSERT(0 == (origin.x % SWAlphaMaskAtlas::TileSize));
    ASSERT(0 == (origin.y % SWAlphaMaskAtlas::TileSize));

    XRECT tileBounds =
    {
        origin.x,
        origin.y,
        SWAlphaMaskAtlas::TileSize,
        SWAlphaMaskAtlas::TileSize
    };

    XRECT surfaceBounds =
    {
        0,
        0,
        surfaceSize.Width,
        surfaceSize.Height
    };

    bool result = IntersectRect(&tileBounds, &surfaceBounds);

    *pBounds = tileBounds;

    return result;
}


// -----------------------------------------------------------------------
//
// Constructor
// Tiled surfaces always know their height and width, even before being back by tiles
//
//-----------------------------------------------------------------------
CTiledSurface::CTiledSurface(XINT32 width, XINT32 height, bool isOpaque)
    : m_tiledWidth(width)
    , m_tiledHeight(height)
    , m_isOpaque(isOpaque)
{
    ASSERT(width >= 0);
    ASSERT(height >= 0);
}


// -----------------------------------------------------------------------
//
// Destructor
//
//-----------------------------------------------------------------------
CTiledSurface::~CTiledSurface()
{
    FreeAllTiles();
    FireOnDelete();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the number of tiles in the surface
//
//------------------------------------------------------------------------
XUINT32
CTiledSurface::GetTileCount()
{
    return m_tiles.Count();
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an iterator pointing to the begining of the tile collection
//
//------------------------------------------------------------------------
CTiledSurface::TileIterator
CTiledSurface::Begin()
{
    return m_tiles.begin();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an iterator pointing past the end of the tile collection
//
//------------------------------------------------------------------------
CTiledSurface::TileIterator
CTiledSurface::End()
{
    return m_tiles.end();
}

//------------------------------------------------------------------------
//
//  Synopsis:
// Copy one tile out of an already locked input surface
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::UpdateTileFromSoftware(
    _In_ HWTextureManager *pTextureManager,
    _In_  CTiledSurface::Tile *pTile,
    _In_ IPALSurface *pSource,
    _In_ const XPOINT &srcOrigin
    )
{
    IFC_RETURN(EnsureTileTexture(pTextureManager, pSource->GetPixelFormat(), pTile));
    IFCEXPECT_RETURN(pTile->spCompositorTexture);
    IFC_RETURN(pTile->spCompositorTexture->UpdateTextureFromSoftware(pSource, srcOrigin));

    return S_OK;
}

_Check_return_ HRESULT
CTiledSurface::EnsureHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    PixelFormat pixelFormat)
{
    IFC_RETURN(EnsureTiles());

    for (CTiledSurface::TileMap::iterator tileIterator = m_tiles.begin(); tileIterator != m_tiles.end(); ++tileIterator)
    {
        CTiledSurface::Tile* pTile = tileIterator->second;
        IFC_RETURN(EnsureTileTexture(pTextureManager, pixelFormat, pTile));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
// Copy out of the input surface, creating tiles as necessary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::UpdateSurfaceFromSoftware(_In_ HWTextureManager *pTextureManager,
                                _In_ IPALSurface *pSurface)
{
    IFC_RETURN(EnsureTiles());

    for (XINT32 y = 0; y < GetHeight(); y += SWAlphaMaskAtlas::TileSize)
    {
        for (XINT32 x = 0; x < GetWidth(); x += SWAlphaMaskAtlas::TileSize)
        {
            XPOINT origin = { x, y };

            Tile *pTile = GetTile(origin);

            IFC_RETURN(UpdateTileFromSoftware(pTextureManager, pTile, pSurface, origin));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
// Allocate tiles for the surface.  This is not the same as ensure they have textures (see EnsureTileTexture)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::EnsureTiles()
{
    // Allocate tiles if this hasn't been done already
    if (0 == GetTileCount())
    {
        XSIZE tileSize = { SWAlphaMaskAtlas::TileSize, SWAlphaMaskAtlas::TileSize };
        for (XINT32 y = 0; y < GetHeight(); y += SWAlphaMaskAtlas::TileSize)
        {
            for (XINT32 x = 0; x < GetWidth(); x += SWAlphaMaskAtlas::TileSize)
            {
                XPOINT origin = { x, y };

                IFC_RETURN(AddTile(origin, 0, FALSE, tileSize));
            }
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a new tile to the list of tiles.  This assumes (and asserts) that there
//      does not already exist a tile with the same origin.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::AddTile(
    XPOINT origin,
    XUINT64 timestamp,
    bool initializeToTransparent,
    XSIZE size
    )
{
    HRESULT hr = S_OK;

    // Tile origin must be within the bounds of the surface image source
    ASSERT(origin.x >= 0);
    ASSERT(origin.x < GetWidth());
    ASSERT(origin.y >= 0);
    ASSERT(origin.y < GetHeight());

    // Compute the intersection of the tile bounds and the surface bounds
    XRECT tileBounds = { origin.x, origin.y, size.Width, size.Height };
    XRECT surfaceBounds = { 0, 0, GetWidth(), GetHeight() };

    bool intersectResult = IntersectRect(&tileBounds, &surfaceBounds);

    // Clipping should produce a rect with positive area
    ASSERT(intersectResult);
    UNREFERENCED_PARAMETER(intersectResult);

    // Clipping should not change the origin, nor increase the size
    ASSERT(tileBounds.X == origin.x);
    ASSERT(tileBounds.Y == origin.y);
    ASSERT(tileBounds.Width <= size.Width);
    ASSERT(tileBounds.Height <= size.Height);

    // Tiles cannot extend beyond the bounds
    ASSERT((tileBounds.X + tileBounds.Width) <= GetWidth());
    ASSERT((tileBounds.Y + tileBounds.Height) <= GetHeight());

    CTiledSurface::Tile *pNewTile = new CTiledSurface::Tile;

    pNewTile->bounds = tileBounds;

    pNewTile->timeStamp = timestamp;

    pNewTile->shouldInitializeToTransparent = initializeToTransparent;

    IFC(m_tiles.Add(
        origin,
        pNewTile
        ));

    // No failures after this point

    pNewTile = NULL; // ownership transferred to m_tiles

    // Assert that a tile does not already exist at this (x,y)
    ASSERT(hr != S_FALSE);

Cleanup:
    SAFE_DELETE(pNewTile);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Frees all memory associated with all tiles
//
//------------------------------------------------------------------------
void
CTiledSurface::FreeAllTiles()
{
    for (CTiledSurface::TileMap::iterator tileIterator = m_tiles.begin(); tileIterator != m_tiles.end(); ++tileIterator)
    {
        CTiledSurface::Tile* pTile = tileIterator->second;

        SAFE_DELETE(pTile);
    }

    m_tiles.Clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the pixel format of the tiles
//
//------------------------------------------------------------------------
PixelFormat
CTiledSurface::GetPixelFormat()
{
    if (GetTileCount() > 0)
    {
        xref_ptr<HWTexture> spFirstTileCompositorTexture = m_tiles.begin()->second->spCompositorTexture;

        ASSERT(spFirstTileCompositorTexture != nullptr);

        return spFirstTileCompositorTexture->GetPixelFormat();
    }

    return pixelUnknown;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Frees memory associated with a specific tile.
//      Note that this invalidates any outstanding iterators.
//
//------------------------------------------------------------------------
void
CTiledSurface::FreeTile(
    _In_ CTiledSurface::Tile *pTile
    )
{
    // Remove the tile from the map
    XPOINT origin = { pTile->bounds.X, pTile->bounds.Y };

    CTiledSurface::Tile *pRemovedTile = NULL;
    VERIFYHR(m_tiles.Remove(origin, pRemovedTile));
    ASSERT(pRemovedTile == pTile);

    SAFE_DELETE(pTile);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the tile with a particular origin (if it exists).  Returns NULL otherwise.
//
//------------------------------------------------------------------------
_Out_opt_ CTiledSurface::Tile *
CTiledSurface::GetTile(
    _In_ const XPOINT &origin
    )
{
    HRESULT hr = S_OK;

    // The input coordinates should be aligned to the tile grid
    ASSERT(0 == (origin.x % SWAlphaMaskAtlas::TileSize));
    ASSERT(0 == (origin.y % SWAlphaMaskAtlas::TileSize));

    CTiledSurface::Tile *pTile = NULL;
    VERIFYHR(m_tiles.Get(origin, pTile));

    if (hr != S_OK)
    {
        // S_FALSE can be returned when there is no neighbor
        ASSERT(S_FALSE == hr);
        ASSERT(!pTile);
        hr = S_OK;
    }

    return pTile;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the anti-aliasing edge flags for a particular tile.
//      Edges that are on the border of the logical size of the surface are antialiased.
//      This information is not cached inside of the Tile structure
//      because the cache becomes invalidated when the logical size of the surface changes.
//
//------------------------------------------------------------------------

XUINT32
CTiledSurface::GetAAMask(
    _In_ const CTiledSurface::Tile *pTile
    )
{
    XUINT32 aaMask = 0;

    if (0 == pTile->bounds.X)
    {
        aaMask |= AAEdge_Left; // this tile is on the left edge
    }

    if ((pTile->bounds.X + pTile->bounds.Width) >= m_tiledWidth)
    {
        aaMask |= AAEdge_Right; // this tile is on the right edge
    }

    if (0 == pTile->bounds.Y)
    {
        aaMask |= AAEdge_Top; // this tile is on the top edge
    }

    if ((pTile->bounds.Y + pTile->bounds.Height) >= m_tiledHeight)
    {
        aaMask |= AAEdge_Bottom; // this tile is on the bottom edge
    }

    return aaMask;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called during the UI thread render walk.
//      This allocates a HWTexture for a tile if necessary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::EnsureTileTexture(
    _In_ HWTextureManager *pTextureManager,
    PixelFormat pixelFormat,
    _Inout_ CTiledSurface::Tile *pTile
    )
{
    HRESULT hr = S_OK;

    HWTexture *pNewTexture = NULL;

    // Allocate space in the atlas if this hasn't been done already
    if (!pTile->spCompositorTexture)
    {
        // HWTextureFlags_IncludePadding indicates that there should be a gutter region allocated around each tile.
        HWTextureFlags flags = static_cast<HWTextureFlags>(HWTextureFlags_IncludePadding);

        // If the surface is opaque then the HWTexture can be marked opaque too
        // as long as the entire tile will be filled in by the caller with opaque pixels.
        // If pTile->shouldInitializeToTransparent = TRUE, then some pixels will not be
        // filed in by the caller, and hence the HWTexture should not be marked as opaque.
        if (m_isOpaque && !pTile->shouldInitializeToTransparent)
        {
            flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsOpaque);
        }

        IFC(pTextureManager->CreateTexture(
            pixelFormat,
            pTile->bounds.Width,
            pTile->bounds.Height,
            flags,
            &pNewTexture
            ));

        pTile->spCompositorTexture = xref_ptr<HWTexture>(pNewTexture);

        if (pTile->shouldInitializeToTransparent)
        {
            IFC(ClearTile(pTile));

            pTile->shouldInitializeToTransparent = FALSE;
        }
    }

Cleanup:
    ReleaseInterface(pNewTexture);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears an entire tile tile to transparent.
//      This assumes that a texture has been allocated for the tile,
//      callers must ensure this.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::ClearTile(
    _In_ CTiledSurface::Tile *pTile
    )
{
    void *pLockedBits = NULL;
    XINT32 lockedStride = 0;
    XUINT32 lockedWidth = 0;
    XUINT32 lockedHeight = 0;

    IFCEXPECT_RETURN(pTile->spCompositorTexture);

    ASSERT(pixelColor32bpp_A8R8G8B8 == pTile->spCompositorTexture->GetPixelFormat());

    IFC_RETURN(pTile->spCompositorTexture->Lock(
        &pLockedBits,
        &lockedStride,
        &lockedWidth,
        &lockedHeight
        ));

    for (XINT32 y = 0; y < pTile->bounds.Height; y++)
    {
        XUINT32 *pScanLine = reinterpret_cast<XUINT32 *>(
            static_cast<XBYTE *>(pLockedBits) + (y * lockedStride)
            );

        for (XINT32 x = 0; x < pTile->bounds.Width; x++)
        {
            pScanLine[x] = 0;
        }
    }

    IFC_RETURN(pTile->spCompositorTexture->Unlock());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether a tile was created on a lost atlas texture.
//
//------------------------------------------------------------------------
bool
CTiledSurface::IsSurfaceLost()
{
    for (CTiledSurface::TileMap::iterator tileIterator = m_tiles.begin(); tileIterator != m_tiles.end(); ++tileIterator)
    {
        CTiledSurface::Tile* pTile = tileIterator->second;

        if (pTile->spCompositorTexture != NULL && pTile->spCompositorTexture->IsSurfaceLost())
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis: Resize the virtual space of the surface
//  This only occurs for VirtualSurfaceImageSource
//  This is a partially destructive operations - some tiles may be freed.
//  If a tile is freed, it will be removed from pValidRegion
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::Resize(_In_ XINT32 newWidth,_In_ XINT32 newHeight, _In_ IPALRegion *pValidRegion)
{
    ASSERT(newWidth >= 0);
    ASSERT(newHeight >= 0);

    XSIZE newLogicalSize = { newWidth, newHeight };

    // Find the set of tiles whose bounds change after the resize operation
    CTiledSurface::TileList tilesToFree;

    // for each tile
    for (CTiledSurface::TileIterator it = Begin();
        it != End();
        ++it)
    {
        CTiledSurface::Tile *pTile = it->second;

        // Compute the bounds of the tile after the resize operation
        XPOINT origin = { pTile->bounds.X, pTile->bounds.Y };
        XRECT newTileBounds;

        bool intersection = GetClippedTileBoundsHelper(
            origin,
            newLogicalSize,
            &newTileBounds
            );

        if (!intersection || (newTileBounds != pTile->bounds))
        {
            // The resize operation changes the tile size
            // So new content must be generated.
            tilesToFree.PushBack(&pTile->tempListEntry);

            // Remove the tile from the valid region
            IFC_RETURN(pValidRegion->Remove(&pTile->bounds));
        }
    }

    // Free all tiles that are outside of the bounds
    // This was deferred from the loop above
    // because freeing a tile invalidates any outstanding iterators
    const XUINT32 offset = OFFSET(CTiledSurface::Tile, tempListEntry);

    while (!tilesToFree.IsEmpty())
    {
        CTiledSurface::Tile *pTile = tilesToFree.GetHead(offset);

        tilesToFree.Remove(&pTile->tempListEntry);

        FreeTile(pTile);
    }

    // save the new size
    m_tiledWidth = newWidth;
    m_tiledHeight = newHeight;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Decides whether a surface is so large it must be broken into tiles
//
//------------------------------------------------------------------------
/* static */
bool
CTiledSurface::NeedsToBeTiled(
    _In_ XSIZE size,
    uint32_t maxTextureSize)
{
    const INT maxTextureSizeInt = static_cast<INT>(maxTextureSize);
    return (size.Width > maxTextureSizeInt || size.Height > maxTextureSizeInt);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the largest multiple of TextureAtlas::TileSize that is <= than the input
//
//------------------------------------------------------------------------
/* static */
XINT32
CTiledSurface::AlignToTilesRoundDown(
    XINT32 in
    )
{
    return (in / SWAlphaMaskAtlas::TileSize) * SWAlphaMaskAtlas::TileSize;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the smallest multiple of TextureAtlas::TileSize that is >= than the input
//
//------------------------------------------------------------------------
/* static */
XINT32
CTiledSurface::AlignToTilesRoundUp(
    XINT32 in
    )
{
    return ((in + SWAlphaMaskAtlas::TileSize - 1) / SWAlphaMaskAtlas::TileSize) * SWAlphaMaskAtlas::TileSize;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Aligns a rectangle to the tile grid
//      There are 2 versions of this function (for both types of rects)
//
//------------------------------------------------------------------------
/* static */ void
CTiledSurface::AlignToTiles(
    _Out_ XRECT_WH *pAlignedRect,
    _In_ const XRECT_WH *pRect
    )
{
    XRECT_RB input = ToXRectRB(*pRect);

    XRECT_RB output;

    AlignToTiles(
        &output,
        &input
        );

    *pAlignedRect = ToXRect(output);
}

/* static */ void
CTiledSurface::AlignToTiles(
    _Out_ XRECT_RB *pAlignedRect,
    _In_ const XRECT_RB *pRect
    )
{
    ASSERT(pRect->left >= 0);
    ASSERT(pRect->top >= 0);

    pAlignedRect->left = AlignToTilesRoundDown(pRect->left);
    pAlignedRect->top = AlignToTilesRoundDown(pRect->top);
    pAlignedRect->right = AlignToTilesRoundUp(pRect->right);
    pAlignedRect->bottom = AlignToTilesRoundUp(pRect->bottom);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Given the origin of a tile, computes the bounds of that tile (clipped to the bounds of the tiled surface)
//
//------------------------------------------------------------------------
void
CTiledSurface::GetClippedTileBounds(
    XPOINT origin,
    _Out_ XRECT *pBounds
    )
{
    XSIZE surfaceSize = { GetWidth(), GetHeight() };

    bool intersection = GetClippedTileBoundsHelper(
        origin,
        surfaceSize,
        pBounds
        );

    // The input tile should always be within the bounds of the surface
    ASSERT(intersection);
    UNREFERENCED_PARAMETER(intersection);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get all tile bounds
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTiledSurface::GetTileRects(
    _Outptr_result_buffer_(*pRectCount) XRECT **ppRects,
    _Out_ XUINT32 *pRectCount
    )
{
    HRESULT hr = S_OK;
    XRECT *pRects = NULL;
    XUINT32 index = 0;
    XUINT32 tileCount = GetTileCount();

    pRects = new XRECT[tileCount];

    for (CTiledSurface::TileMap::iterator tileIterator = m_tiles.begin(); tileIterator != m_tiles.end(); ++tileIterator, index++)
    {
        CTiledSurface::Tile* pTile = tileIterator->second;
        pRects[index] = pTile->bounds;
    }

    *ppRects = pRects;
    *pRectCount = tileCount;
    RRETURN(hr);//RRETURN_REMOVAL
}

xref_ptr<HWTexture>
CTiledSurface::GetTileSurface(
    TileIterator tileIterator
    )
{
    return xref_ptr<HWTexture>(tileIterator->second->spCompositorTexture.get());
}

const XRECT&
CTiledSurface::GetTileRect(
    TileIterator tileIterator
    ) const
{
    return tileIterator->second->bounds;
}
