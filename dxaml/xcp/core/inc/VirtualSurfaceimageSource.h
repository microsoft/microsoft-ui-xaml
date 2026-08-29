// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CVirtualSurfaceImageSource;

// Describes a single tile which a VSIS could free if necessary
struct VirtualSurfaceImageSourceStandbyListRecord
{
    CVirtualSurfaceImageSource *pVSIS;
    CTiledSurface::Tile *pTile;
};

static const XUINT32 VSIS_STANDBY_LIST_COUNT = 8;
static const XUINT32 VSIS_STANDBY_LIST_BUCKET_SIZE = 256; // milliseconds - a power of 2

// The set of all tiles in all VSIS objects which could be freed if necessary
struct VirtualSurfaceImageSourceStandbyLists
{
    // Each list corresponds to a certain age of tile.
    // Tiles which are < VSIS_STANDBY_LIST_BUCKET_SIZE miliseconds old go into list[0]
    // Tiles which are < VSIS_STANDBY_LIST_BUCKET_SIZE * 2 miliseconds old go into list[1]
    xvector<VirtualSurfaceImageSourceStandbyListRecord> list[VSIS_STANDBY_LIST_COUNT];

    // The amount of memory (bytes) consumed by all tiles in all lists
    XUINT64 totalSize{};
};

class IVirtualSurfaceImageSourceCallbacks
{
public:
    virtual _Check_return_ HRESULT UpdatesNeeded() = 0;
};

namespace VisibleBoundsMotionFlags
{
    enum Enum
    {
        Left  = 0x1,
        Up    = 0x2,
        Right = 0x4,
        Down  = 0x8
    };
}

class CVirtualSurfaceImageSource final : public CSurfaceImageSource
{
public:
    CVirtualSurfaceImageSource(_In_ CCoreServices *pCore);
    ~CVirtualSurfaceImageSource() override;

    DECLARE_CREATE(CVirtualSurfaceImageSource);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVirtualSurfaceImageSource>::Index;
    }

    _Check_return_ HRESULT Initialize(
        _In_ XINT32 width,
        _In_ XINT32 height,
        _In_ bool isOpaque
        );

    _Check_return_ HRESULT Invalidate(
        _In_ XRECT updateRect
        );

    _Check_return_ HRESULT GetUpdateRectCount(_Out_ XDWORD *pCount);
    _Check_return_ HRESULT GetUpdateRects(_Out_writes_(count) XRECT *pUpdates,_In_ XDWORD count);
    _Check_return_ HRESULT CalculateBounds(_In_ const XRECTF_RB *pWindowBounds);
    _Check_return_ HRESULT Resize(_In_ XINT32 newWidth,_In_ XINT32 newHeight);
    _Check_return_ HRESULT RegisterCallbacks(_In_ IVirtualSurfaceImageSourceCallbacks *pCallbacks);

    bool IsInLiveTree();
    _Ret_maybenull_ CUIElement* GetFirstParentUIElement();
    static _Check_return_ HRESULT CacheCommonAncestorVisibleRect(
        _In_ CVirtualSurfaceImageSource* pVSIS1,
        _In_ CVirtualSurfaceImageSource* pVSIS2,
        _In_ const XRECTF_RB* pWindowBounds
        );

    _Check_return_ HRESULT PreRender(_In_ const XRECTF_RB *pWindowBounds, _Out_ bool *pWantAdditionalFrame);

    void GetVisibleBounds(_Out_ XRECT_RB *pBounds) { *pBounds = m_visibleBounds; }

    void HandleLostResources();

    _Check_return_ HRESULT UpdateStandbyLists(
        _Inout_ VirtualSurfaceImageSourceStandbyLists *pStandbyLists
        );

    _Check_return_ HRESULT FreeTile(_In_ CTiledSurface::Tile *pTile);

    _Check_return_ HRESULT TrimTiles();
    _Check_return_ HRESULT TrimTilesIfPossible();

protected:
    _Check_return_ HRESULT PreUpdateVirtual(
        _In_ const xvector<XRECT> *pUpdatedRects
        ) override;

    bool IsVirtual() override { return true; }

private:
    _Check_return_ HRESULT AddTile(
        XPOINT origin,
        XSIZE size,
        XUINT64 timestamp,
        bool initializeToTransparent
        );

    XUINT32 GetTileCount();
    CTiledSurface::TileIterator BeginTiles();
    CTiledSurface::TileIterator EndTiles();

    XUINT32 GetMaxTextureDimension();

    _Check_return_ HRESULT CalculateNeededUpdates();

    _Check_return_ HRESULT RaiseCallbacks();

    _Check_return_ HRESULT RequestTickForUpdatesNeeded();

    static void CalculateDesiredBounds(
        _Out_ XRECT_RB *pDesired,
        _Out_ XRECT_RB *pHighPriorityDesired,
        _In_ const XRECT_RB *pVisible,
        XUINT32 motionFlags
        );

    bool TileInsideDesiredBounds(_In_ const CTiledSurface::Tile *pTile);

    void AssertValidRegion();

private:
    static const XUINT32 sc_oldTileTime = 2048; // tiles that are 2 sec old are freed

    IVirtualSurfaceImageSourceCallbacks *m_pCallbacks;

    xvector<XRECT> m_neededUpdates;

    // The region on the VSIS which is visible
    XRECT_RB m_visibleBounds;

    // m_visibleBounds aligned to tile boundaries, plus padding in the direction of motion
    // A superset of m_visibleBounds
    XRECT_RB m_highPriorityDesiredBounds;

    // m_visibleBounds plus padding in all directions
    // A superset of m_highPriorityDesiredBounds
    XRECT_RB m_desiredBounds;

    // Bits from VisibleBoundsMotionFlags determine the direction that the visible bounds is moving
    XUINT32 m_motionFlags;

    IPALRegion *m_pValidRegion;

    // This data structure is used to track and free regions in the backing virtual composition texture.
    // The individual tiles in the data structure are not backed with separate allocations.
    CTiledSurface *m_pTiles;

    // Tracks whether the invalid area in the VSIS might have changed.
    // If TRUE, CalculateNeededUpdates should be called to determine the up-to-date invalid area.
    bool m_neededUpdatesValid       : 1;
    bool m_onGlobalVSISList         : 1;
    bool m_wantHighPriorityUpdates  : 1; // TRUE if only rects inside of m_highPriorityDesiredBounds are desired
    bool m_hasOutstandingFreedTiles : 1;
};
