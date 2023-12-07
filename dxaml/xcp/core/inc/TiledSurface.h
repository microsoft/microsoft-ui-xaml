// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HWTexture;
class HWTextureManager;

// Notes:  This class is basically used to break large surfaces up into smaller tiles.  We may do this
// because the surface is simply too large to be supported by the hardware (MaxTextureSize is 2k or 4k or 8k or 16k)
// or too large to be supported any conceivable hardware (like an infinite canvas or scrolling list).
class CTiledSurface :  public CInterlockedReferenceCount, public CNotifyOnDelete
{
public:
    CTiledSurface(XINT32 width, XINT32 height, bool isOpaque);

private:
    CTiledSurface();

protected:
    ~CTiledSurface() override;

// Inner Types
public:
    struct Tile
    {
        xref_ptr<HWTexture>             spCompositorTexture; // Space in the compositor atlas
        XRECT                           bounds;
        XUINT64                         timeStamp;
        IntrusiveList<Tile>::ListEntry  tempListEntry;
        bool                           shouldInitializeToTransparent;

        // Returns the number of bytes of atlas space used by this tile - even if atlas space has not yet been allocated.
        XUINT64 GetAllocationSize() const
        {
            const XUINT32 bytesPerPixel = 4;

            return static_cast<XUINT64>(bounds.Width * bounds.Height * bytesPerPixel);
        }
    };

    typedef IntrusiveList<Tile> TileList;

    // Maps tile origin (x,y) to Tile structure
    typedef xchainedmap<XPOINT, Tile*> TileMap;

    typedef TileMap::const_iterator TileIterator;

public:
    XUINT32 GetTileCount();

    _Check_return_ HRESULT GetTileRects(_Outptr_result_buffer_(*pRectCount) XRECT **ppRects, _Out_ XUINT32 *pRectCount);

    xref_ptr<HWTexture> GetTileSurface(TileIterator tileIterator);
    const XRECT& GetTileRect(TileIterator tileIterator) const;

    CTiledSurface::TileIterator Begin();
    CTiledSurface::TileIterator End();

    _Check_return_ HRESULT EnsureHardwareResources(_In_ HWTextureManager *pTextureManager, PixelFormat pixelFormat);

    _Check_return_ HRESULT UpdateSurfaceFromSoftware(_In_ HWTextureManager *pTextureManager, _In_ IPALSurface *pSurface);

    _Check_return_ HRESULT AddTile(
        XPOINT origin,
        XUINT64 timestamp,
        bool initializeToTransparent,
        XSIZE size
        );

    void FreeTile(
        _In_ CTiledSurface::Tile *pTile
        );

    void FreeAllTiles();

    PixelFormat GetPixelFormat();

    _Ret_maybenull_ CTiledSurface::Tile *GetTile(_In_ const XPOINT &origin);

    _Check_return_ HRESULT Resize(_In_ XINT32 newWidth,_In_ XINT32 newHeight, _In_ IPALRegion *pValidRegion);

    XUINT32 GetAAMask(_In_ const CTiledSurface::Tile *pTile);

    _Check_return_ HRESULT ClearTile(
        _In_ CTiledSurface::Tile *pTile
        );

    bool IsOpaque() { return m_isOpaque; }
    XINT32 GetWidth() { return m_tiledWidth; }
    XINT32 GetHeight() { return m_tiledHeight; }

    bool IsSurfaceLost();

    void GetClippedTileBounds(
        XPOINT origin,
        _Out_ XRECT *pBounds
        );

private:
    _Check_return_ HRESULT EnsureTileTexture(
        _In_ HWTextureManager *pTextureManager,
        PixelFormat pixelFormat,
        _Inout_ CTiledSurface::Tile *pTile
        );

    _Check_return_ HRESULT UpdateTileFromSoftware(_In_ HWTextureManager *pTextureManager,
                                        _In_ CTiledSurface::Tile *pTile,
                                        _In_ IPALSurface *pSource,
                                        _In_ const XPOINT &srcOrigin
                                        );
    _Check_return_ HRESULT EnsureTiles();

// Static Helpers
public:
    // Returns TRUE if size is so big the texture must be tiled.
    static bool NeedsToBeTiled(
        _In_ XSIZE size,
        uint32_t maxTextureSize
        );
    static XINT32 AlignToTilesRoundDown(XINT32 in);
    static XINT32 AlignToTilesRoundUp(XINT32 in);

    static void AlignToTiles(
        _Out_ XRECT_RB *pAlignedRect,
        _In_ const XRECT_RB *pRect
        );

    static void AlignToTiles(
        _Out_ XRECT_WH *pAlignedRect,
        _In_ const XRECT_WH *pRect
        );

private:
    CTiledSurface::TileMap m_tiles;

    // The width and height of the tiled region.  May be virtual (larger than what actually has tiles)
    XINT32 m_tiledWidth;
    XINT32 m_tiledHeight;
    bool m_isOpaque;
};
