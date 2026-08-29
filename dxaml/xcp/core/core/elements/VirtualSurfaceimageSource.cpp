// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <UIThreadScheduler.h>

static const XUINT32 sc_hardMaximumTileCount = 150;

CVirtualSurfaceImageSource::CVirtualSurfaceImageSource(
    _In_ CCoreServices *pCore
    )
    : CSurfaceImageSource(pCore)
    , m_motionFlags(0)
    , m_pValidRegion(NULL)
    , m_pTiles(NULL)
    , m_neededUpdatesValid(FALSE)
    , m_onGlobalVSISList(FALSE)
    , m_wantHighPriorityUpdates(TRUE)
    , m_hasOutstandingFreedTiles(FALSE)
{
    EmptyRect(&m_visibleBounds);
    EmptyRect(&m_highPriorityDesiredBounds);
    EmptyRect(&m_desiredBounds);

    m_pCallbacks = NULL;
}

CVirtualSurfaceImageSource::~CVirtualSurfaceImageSource()
{
    // Remove the VSIS from the global list if necessary
    if (m_onGlobalVSISList)
    {
        VERIFYHR(GetContext()->RemoveVirtualSurfaceImageSource(this));
    }

    ReleaseInterface(m_pValidRegion);
    ReleaseInterface(m_pTiles);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new VirtualSurfaceImageSource by specifying Width and Height
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::Initialize(
    _In_ XINT32 width,
    _In_ XINT32 height,
    _In_ bool isOpaque
    )
{
    // The VSIS should not already be on the global list
    ASSERT(!m_onGlobalVSISList);

    // Initialize the valid region (initial value is empty)
    IFC_RETURN(gps->CreateRegion(&m_pValidRegion));

    // Add the VSIS to the global list of VSIS objects
    IFC_RETURN(GetContext()->AddVirtualSurfaceImageSource(this));

    m_onGlobalVSISList = TRUE;

    IFC_RETURN(CSurfaceImageSource::Initialize(width, height, isOpaque));

    m_pTiles = new CTiledSurface(width, height, isOpaque);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Invalidates the passed in rectangle because the content has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::Invalidate(_In_ XRECT invalidateRect)
{
    XRECT surfaceBounds = { 0, 0, GetWidth(), GetHeight() };

    // Can't call Invalidate while there is a pending BeginDraw
    if (IsDrawing())
    {
        IFC_RETURN(E_FAIL);
    }

    // The input rect must not exceed the surface bounds
    if (!DoesRectContainRect(&surfaceBounds, &invalidateRect))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // Remove the rect from the valid region
    IFC_RETURN(m_pValidRegion->Remove(&invalidateRect));

    // The output of CalculateNeededUpdates depends on the valid region
    m_neededUpdatesValid = FALSE;

    // Request a tick so that UpdatesNeeded is raised to reflect the potential change in
    // invalid area. Note that if Invalidate is called in or after the UpdatesNeeded callback
    // during the tick, the requested UpdatesNeeded callback will fire on the next tick.
    IFC_RETURN(RequestTickForUpdatesNeeded());

    // On debug builds - check the valid region against set of tiles
    AssertValidRegion();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the number of rects that need updating.  Allows the caller to
//      allocate space before calling GetUpdateRects.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::GetUpdateRectCount(
    _Out_ XDWORD *pCount
    )
{
    // Return the cached update rect count.
    *pCount = m_neededUpdates.size();

    // RRETURN_REMOVAL
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the rects that need updating, but no more than count.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::GetUpdateRects(
    _Out_writes_(count) XRECT *pUpdates,
    _In_ XDWORD count
    )
{
    // Return the cached update rects.
    for (XUINT32 i = 0; i < count; i++)
    {
        if (i < m_neededUpdates.size())
        {
            pUpdates[i] = m_neededUpdates[i];
        }
        else
        {
            pUpdates[i].X = 0;
            pUpdates[i].Y = 0;
            pUpdates[i].Width = 0;
            pUpdates[i].Height = 0;
        }
    }

    // RRETURN_REMOVAL
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread during EndDraw. This ensures that tiles have been allocated
//      for all regions that are affected by an update.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::PreUpdateVirtual(
    _In_ const xvector<XRECT> *pUpdatedRects
    )
{
    XUINT64 timeStamp = gps->GetCPUMilliseconds();

    ASSERT(m_pTiles);

    for (XUINT32 i = 0; i < pUpdatedRects->size(); i++ )
    {
        const XRECT *pUpdateRect = &(*pUpdatedRects)[i];

        // Compute the set of tiles that intersect the update rect
        XRECT alignedUpdateRect;
        CTiledSurface::AlignToTiles(
            &alignedUpdateRect,
            pUpdateRect
            );

        for (XINT32 y = alignedUpdateRect.Y; y < (alignedUpdateRect.Y + alignedUpdateRect.Height); y += SWAlphaMaskAtlas::TileSize)
        {
            for (XINT32 x = alignedUpdateRect.X; x < (alignedUpdateRect.X + alignedUpdateRect.Width); x += SWAlphaMaskAtlas::TileSize)
            {
                bool validateRect = false;
                XPOINT origin = { x, y };
                CTiledSurface::Tile *pTile = m_pTiles->GetTile(origin);
                XRECT tileBounds;

                if (pTile)
                {
                    // Update the timestamp for the tile
                    // indicating that it was recently used
                    pTile->timeStamp = timeStamp;

                    validateRect = TRUE;

                    tileBounds = pTile->bounds;
                }
                else
                {
                    // Only add a new tile if hard maximum tile count has not been reached (which prevents unbounded memory growth in scaling scenarios)
                    if (GetTileCount() < sc_hardMaximumTileCount)
                    {
                        m_pTiles->GetClippedTileBounds(
                            origin,
                            &tileBounds
                            );

                        // If the update rect only covers a portion of the tile
                        // then intialize it to transparent to ensure
                        // that contents are always well-defined
                        bool shouldInitializeToTransparent = !DoesRectContainRect(pUpdateRect, &tileBounds);

                        XSIZE tileSize = { tileBounds.Width, tileBounds.Height };

                        IFC_RETURN(AddTile(origin, tileSize, timeStamp, shouldInitializeToTransparent));

                        validateRect = TRUE;
                    }
                }

                if (validateRect)
                {
                    // If a tile was updated, then add the intersection of the tile bounds and the update rect to the valid region
                    if (IntersectRect(&tileBounds, pUpdateRect))
                    {
                        IFC_RETURN(m_pValidRegion->Add(&tileBounds));
                    }
                }
            }
        }
    }

    // Invalidate m_neededUpdates because a new tile may have been added
    // or an existing tile may have been validated
    // or an existing tile may have been removed.
    m_neededUpdatesValid = FALSE;

    // Trim the virtual surface down to the valid tiles.
    // Note that this needs to happen after we've EndDraw'd the DComp surface
    // as Trim is not allowed while a DComp surface is in the drawing state.
    IFC_RETURN(TrimTiles());

    // On debug builds - check the valid region against set of tiles
    AssertValidRegion();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Calculates the visible & desired regions of the surface image source.
//            This is called on the UI thread before each render walk.
//            This re-computes the visible bounds each time it is called.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CVirtualSurfaceImageSource::CalculateBounds(
    _In_ const XRECTF_RB *pWindowBounds)
{
    XRECTF_WH combinedBounds;
    EmptyRectF(&combinedBounds);

    XRECTF_RB inflatedBounds;

    XRECT_RB visibleBounds;

    XRECT_RB surfaceBounds = { 0, 0, GetWidth(), GetHeight() };

    // for each parent
    auto parentCount = GetParentCount();

    for (size_t parentIndex = 0; parentIndex < parentCount; parentIndex++)
    {
        CDependencyObject *pParentNoRef = GetParentItem(parentIndex);

        XRECTF boundsForThisElement;
        EmptyRectF(&boundsForThisElement);

        // Compute the bounds of the visible region of the VSIS for this parent

        if (pParentNoRef->OfTypeByIndex<KnownTypeIndex::Image>())
        {
            CImage *pImageNoRef = static_cast<CImage *>(pParentNoRef);

            IFC_RETURN(pImageNoRef->GetVisibleImageSourceBounds(
                pWindowBounds,
                &boundsForThisElement
                ));
        }
        else if (pParentNoRef->OfTypeByIndex<KnownTypeIndex::ImageBrush>())
        {
            CImageBrush *pImageBrushNoRef = static_cast<CImageBrush *>(pParentNoRef);

            IFC_RETURN(pImageBrushNoRef->GetVisibleImageSourceBounds(
                pWindowBounds,
                &boundsForThisElement
                ));
        }
        else
        {
            // No part of an image source is visible when it is contained in a resource dictionary
            ASSERT(pParentNoRef->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>());
        }

        // Union bounds visible from this parent with all other parents
        UnionRectF(&combinedBounds, &boundsForThisElement);
    }

    // Inflate the bounds to integer
    inflatedBounds = ToXRectFRB(combinedBounds);
    InflateRectF(&inflatedBounds);

    visibleBounds.left = static_cast<XINT32>(inflatedBounds.left);
    visibleBounds.top = static_cast<XINT32>(inflatedBounds.top);
    visibleBounds.right = static_cast<XINT32>(inflatedBounds.right);
    visibleBounds.bottom = static_cast<XINT32>(inflatedBounds.bottom);

    // clip to the logical bounds of the image source
    // This will set visibleBounds = { 0,0,0,0 } if there is no intersection
    IntersectRect(&visibleBounds, &surfaceBounds);

    if (visibleBounds != m_visibleBounds)
    {
        // Compute motion flags
        m_motionFlags = 0;

        // If the area of the visible rectangle did not change, then compute motion flags based on how it translated.
        // If the area changed, then do not try to predict which direction the rectangle is moving.
        if (((visibleBounds.right - visibleBounds.left) == (m_visibleBounds.right - m_visibleBounds.left)) &&
            ((visibleBounds.bottom - visibleBounds.top) == (m_visibleBounds.bottom - m_visibleBounds.top)))
        {
            XPOINT oldOrigin = { m_visibleBounds.left, m_visibleBounds.top };
            XPOINT newOrigin = { visibleBounds.left, visibleBounds.top };

            XPOINT direction = newOrigin - oldOrigin;

            if (direction.x > 0)
            {
                m_motionFlags |= VisibleBoundsMotionFlags::Right;
            }
            else if (direction.x < 0)
            {
                m_motionFlags |= VisibleBoundsMotionFlags::Left;
            }

            if (direction.y < 0)
            {
                m_motionFlags |= VisibleBoundsMotionFlags::Up;
            }
            else if (direction.y > 0)
            {
                m_motionFlags |= VisibleBoundsMotionFlags::Down;
            }
        }
    }

    // Compute the desired bounds
    XRECT_RB newDesiredBounds;
    XRECT_RB newHighPriorityDesiredBounds;
    CalculateDesiredBounds(&newDesiredBounds, &newHighPriorityDesiredBounds, &visibleBounds, m_motionFlags);

    // If the visible or desired bounds changed - then trace an event
    if (EventEnabledComputeVsisBoundsInfo())
    {
        if ((visibleBounds != m_visibleBounds) ||
            (newDesiredBounds != m_desiredBounds) ||
            (newHighPriorityDesiredBounds != m_highPriorityDesiredBounds))
        {
            TraceComputeVsisBoundsInfo(
                (UINT64) this,
                visibleBounds.left,
                visibleBounds.top,
                visibleBounds.right,
                visibleBounds.bottom,
                newDesiredBounds.left,
                newDesiredBounds.top,
                newDesiredBounds.right,
                newDesiredBounds.bottom,
                newHighPriorityDesiredBounds.left,
                newHighPriorityDesiredBounds.top,
                newHighPriorityDesiredBounds.right,
                newHighPriorityDesiredBounds.bottom,
                m_motionFlags
                );
        }
    }

    // If the high-priority bounds have changed, then (for this tick only) request high-priority updates.
    // Otherwise, request all updates.
    bool wantHighPriorityUpdates = (newHighPriorityDesiredBounds != m_highPriorityDesiredBounds);

    if (wantHighPriorityUpdates != m_wantHighPriorityUpdates)
    {
        // This invalidates the cache of needed update rects.
        m_wantHighPriorityUpdates = wantHighPriorityUpdates;
        m_neededUpdatesValid = FALSE;
    }

    if ((newDesiredBounds != m_desiredBounds) ||
        (newHighPriorityDesiredBounds != m_highPriorityDesiredBounds))
    {
        // The desired bounds have changed, this invalidates the cache of needed update rects
        m_highPriorityDesiredBounds = newHighPriorityDesiredBounds;
        m_desiredBounds = newDesiredBounds;
        m_neededUpdatesValid = FALSE;
    }

    // Save the visible bounds in case the application queries for them
    m_visibleBounds = visibleBounds;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread to determine the set of RECTs that
//      this objects thinks should be generated.
//      This policy tries to optimize 2 competing variables:
//      1. The total amount of memory allocated for tiles
//      2. The number of frames where the user does not see content
//         because the coressponding tiles have not been rendered
//
//      This function aligns the visible bounds to tile boundaries,
//      inflates the result by 1 tile,
//      and then computes the set of tiles which are not present
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::CalculateNeededUpdates()
{
    ASSERT(!m_neededUpdatesValid);

    // Remove any stale data from previous computations
    m_neededUpdates.clear();

    // Compute the intersection of the desired bounds and the bounds of the VSIS.
    // Only rects inside of this intersection will be returned.
    XRECT_RB surfaceBoundsRB = { 0, 0, GetWidth(), GetHeight() };

    // If only high-priority updates are desired, then request only those.
    XRECT_RB maximumRect = m_wantHighPriorityUpdates ? m_highPriorityDesiredBounds : m_desiredBounds;

    if (IntersectRect(&maximumRect, &surfaceBoundsRB))
    {
        xvector<XRECT> unsortedRects;

        // Query for the maximum size supported by the underlying D3D device.
        // This function should not produce a rect with a width/height that exceed this limit.
        XINT32 maxTextureDimension = GetMaxTextureDimension();

        // Compute the inverse of the valid region (constrained to maximumRect)
        // and stream it into an array of rects
        IFC_RETURN(m_pValidRegion->GetInverseRects(&unsortedRects, &maximumRect));

        for (XUINT32 i = 0; i < unsortedRects.size(); i++)
        {
            const XRECT &srcRect = unsortedRects[i];

            // Tile srcRect to fit within the maximum texture dimension
            for (XINT32 y = 0; y < srcRect.Height; y += maxTextureDimension)
            {
                for (XINT32 x = 0; x < srcRect.Width; x += maxTextureDimension)
                {
                    XRECT dstRect =
                    {
                        x + srcRect.X,
                        y + srcRect.Y,
                        maxTextureDimension,
                        maxTextureDimension,
                    };

                    if (IntersectRect(&dstRect, &srcRect))
                    {
                        IFC_RETURN(m_neededUpdates.push_back(dstRect));
                    }
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread.  Determines if a particular tile is within the desired bounds.
//
//------------------------------------------------------------------------
bool
CVirtualSurfaceImageSource::TileInsideDesiredBounds(
    _In_ const CTiledSurface::Tile *pTile
    )
{
    // compute the intersection of the tile bounds with the desired bounds
    XRECT_RB intersection =
    {
        pTile->bounds.X,
        pTile->bounds.Y,
        pTile->bounds.X + pTile->bounds.Width,
        pTile->bounds.Y + pTile->bounds.Height,
    };

    // If there is any intersection, then return TRUE
    return IntersectRect(&intersection, &m_desiredBounds);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the bounds of the desired region and the high-priority desired region given the bounds of the visible region.
//      This sets *pDesired and *pHighPriorityDesired = { 0,0,0,0 } if the visible bounds are empty
//
//------------------------------------------------------------------------
/* static */ void
CVirtualSurfaceImageSource::CalculateDesiredBounds(
    _Out_ XRECT_RB *pDesired,
    _Out_ XRECT_RB *pHighPriorityDesired,
    _In_ const XRECT_RB *pVisible,
    XUINT32 motionFlags
    )
{
    XRECT_RB desiredBounds;
    XRECT_RB highPriorityDesiredBounds;

    // Align the visible bounds to tile boundaries
    XRECT_RB visibleAlignedToTiles;
    CTiledSurface::AlignToTiles(
        &visibleAlignedToTiles,
        pVisible
        );

    // If nothing is visible, then visibleAlignedToTiles will have 0 area
    // in this case, no update rects should be computed
    if ((visibleAlignedToTiles.right > visibleAlignedToTiles.left) &&
        (visibleAlignedToTiles.bottom > visibleAlignedToTiles.top))
    {
        desiredBounds = visibleAlignedToTiles;

        // Inflate by 1 tile to compute desired bounds
        desiredBounds.left -= SWAlphaMaskAtlas::TileSize;
        desiredBounds.top -= SWAlphaMaskAtlas::TileSize;
        desiredBounds.right += SWAlphaMaskAtlas::TileSize;
        desiredBounds.bottom += SWAlphaMaskAtlas::TileSize;

        // Inflate according to the direction of motion for high-priority desired bounds
        highPriorityDesiredBounds = visibleAlignedToTiles;

        if (motionFlags & VisibleBoundsMotionFlags::Left)
        {
            highPriorityDesiredBounds.left -= SWAlphaMaskAtlas::TileSize;
        }

        if (motionFlags & VisibleBoundsMotionFlags::Up)
        {
            highPriorityDesiredBounds.top -= SWAlphaMaskAtlas::TileSize;
        }

        if (motionFlags & VisibleBoundsMotionFlags::Right)
        {
            highPriorityDesiredBounds.right += SWAlphaMaskAtlas::TileSize;
        }

        if (motionFlags & VisibleBoundsMotionFlags::Down)
        {
            highPriorityDesiredBounds.bottom += SWAlphaMaskAtlas::TileSize;
        }

        // desiredBounds is a superset of highPriorityDesiredBounds
        ASSERT(DoesRectContainRect(&desiredBounds, &highPriorityDesiredBounds));

        // highPriorityDesiredBounds is a superset of pVisible
        ASSERT(DoesRectContainRect(&highPriorityDesiredBounds, pVisible));
    }
    else
    {
        EmptyRect(&desiredBounds);
        EmptyRect(&highPriorityDesiredBounds);
    }

    *pDesired = desiredBounds;
    *pHighPriorityDesired = highPriorityDesiredBounds;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resize the virtual space of the surface
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CVirtualSurfaceImageSource::Resize(_In_ XINT32 newWidth,_In_ XINT32 newHeight)
{
    XRECT newBounds = { 0, 0, newWidth, newHeight };

    auto guard = GetMultithreadedLock();

    // Can't call Resize while there is a pending BeginDraw
    if (IsDrawing())
    {
        IFC_RETURN(E_FAIL);
    }

    // If there's an underlying hardware surface, resize it.
    HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
    if (pHWSurfaceNoRef != nullptr)
    {
        ASSERT(pHWSurfaceNoRef->GetCompositionSurface());

        IFC_RETURN(pHWSurfaceNoRef->GetCompositionSurface()->Resize(newWidth, newHeight));
    }

    // Remove anything outside of the new bounds from the valid region
    IFC_RETURN(m_pValidRegion->Intersect(&newBounds));

    // TODO: JCOMP: Do we need a tile data structure?
    // m_pValidRegion is passed into Resize so that Resize can remove rectangles
    // coressponding to any tiles it chooses to discard.
    IFC_RETURN(m_pTiles->Resize(newWidth, newHeight, m_pValidRegion));

    // Update the width/height of the image source.
    SetSize(newWidth, newHeight);

    // Cached content in m_neededUpdates is no longer valid
    m_neededUpdatesValid = FALSE;

    // A new UI thread render walk is required after resizing a virtual surface image source
    // since some tiles might have been discarded.
    IFC_RETURN(SetDirty());

    // Request a tick so that the callback can be called if there are updates that are needed.
    // This is required in addition to marking the VSIS as dirty. If Resize is called during the
    // tick, but during or after the UpdatesNeeded callback, the VSIS is marked dirty for that
    // frame in case tiles are discarded. Dirtying the tree won't cause an additional tick.
    // However, the Resize call must also request a _subsequent_ additional tick in order to
    // raise the UpdatesNeeded callback again, in case new area must be filled as a result of
    // the new VSIS size.
    IFC_RETURN(RequestTickForUpdatesNeeded());

    // On debug builds - check the valid region against set of tiles
    AssertValidRegion();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Register any callbacks
// Note this method does not match the signature of the public RegisterForNeededUpdates.
// This makes it easier to add callbacks in the future by simply adding parameters rather than
// new "C" exports.
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::RegisterCallbacks(_In_ IVirtualSurfaceImageSourceCallbacks *pCallbacks)
{
    if (pCallbacks)
    {
        // UpdatesNeeded needs to be raised again for the new callback.
        m_neededUpdatesValid = FALSE;

        // Request a tick so that the callback can be called if there are updates that are needed
        IFC_RETURN(RequestTickForUpdatesNeeded());
    }

    m_pCallbacks = pCallbacks;

    return S_OK;
}

// Helper function to determine if this VSIS is in the live tree
bool CVirtualSurfaceImageSource::IsInLiveTree()
{
    // It is insufficient to check this->IsActive()
    // because this->IsActive() can return TRUE in cases
    // where the VSIS is not in the tree.
    auto parentCount = GetParentCount();
    for (size_t parentIndex = 0; parentIndex < parentCount; parentIndex++)
    {
        CDependencyObject *pParentNoRef = GetParentItem(parentIndex);
        if (pParentNoRef->IsActive())
        {
            return true;
        }
    }

    return false;
}

// Helper function to retrieve the first CUIElement parent of this VSIS
_Ret_maybenull_ CUIElement*
CVirtualSurfaceImageSource::GetFirstParentUIElement()
{
    // First look at our immediate parents.
    auto parentCount = GetParentCount();
    for (size_t i = 0; i < parentCount; i++)
    {
        // An immediate parent might be an Image element, which is a UIElement.
        CDependencyObject *pParentDO = GetParentItem(i);
        if (pParentDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            return static_cast<CUIElement*>(pParentDO);
        }
        else if (pParentDO->OfTypeByIndex<KnownTypeIndex::ImageBrush>())
        {
            // An ImageBrush is not a UIElement.  Move up and check our grand-parents.
            auto grandParentCount = pParentDO->GetParentCount();
            for (size_t j = 0; j < grandParentCount; j++)
            {
                // The grand-parent can be any element that can have an ImageBrush set on it.
                CDependencyObject *pGrandParentDO = pParentDO->GetParentItem(j);
                if (pGrandParentDO->OfTypeByIndex<KnownTypeIndex::UIElement>())
                {
                    return static_cast<CUIElement*>(pGrandParentDO);
                }
            }
        }
    }
    return nullptr;
}

// Perf optimization to cache the visible bounds of the common ancestor of 2 VSIS's
_Check_return_ HRESULT
CVirtualSurfaceImageSource::CacheCommonAncestorVisibleRect(
    _In_ CVirtualSurfaceImageSource* pVSIS1,
    _In_ CVirtualSurfaceImageSource* pVSIS2,
    _In_ const XRECTF_RB *pWindowBounds
    )
{
    // This function searches for the common ancestor of pVSIS1 and pVSIS2, and if found,
    // we cache the visible bounds of this common ancestor in a hashtable managed by the Core.
    //
    // Notes:  For the optimization to take effect these conditions must be first met:
    // - Both VSIS's must be in the live tree.
    // - pVSIS1 and pVSIS2 must be at the same depth relative to their common ancestor.
    // - We only consider the first UIElement parent of each VSIS, as it is very uncommon for there to be more than one parent.
    // - The common ancestor must be no more than 10 levels away from the two VSIS's first UIElement parent.
    // - The common ancestor must not be in a 3D subtree (this check is done inside CacheVisibleBounds)
    if (pVSIS1->IsInLiveTree() && pVSIS2->IsInLiveTree())
    {
        CUIElement* pParent1 = pVSIS1->GetFirstParentUIElement();
        CUIElement* pParent2 = pVSIS2->GetFirstParentUIElement();
        const int maxLevels = 10;
        int level = 0;

        while (pParent1 != nullptr && pParent2 != nullptr && level < maxLevels)
        {
            if (pParent1 == pParent2)
            {
                // We found the common ancestor!
                IFC_RETURN(pParent1->CacheVisibleBounds(pWindowBounds));
                break;
            }
            pParent1 = pParent1->GetUIElementAdjustedParentInternal(FALSE);
            pParent2 = pParent2->GetUIElementAdjustedParentInternal(FALSE);
            level++;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis: Called on the UI thread.
//  This computes the set of update rects, and calls the application callback
//  if this set has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::PreRender(
    _In_ const XRECTF_RB *pWindowBounds,
    _Out_ bool *pWantAdditionalFrame
    )
{
    HRESULT hr = S_OK;
    bool wantAdditionalFrame = false;

    // On debug builds - validate the valid region
    AssertValidRegion();

    // If the VSIS is not currently active in the tree then do nothing.
    // It is impossible to know which tiles in the VSIS are visible and hence should be rendered
    if (IsInLiveTree())
    {
        XUINT64 timeStamp = gps->GetCPUMilliseconds();
        bool prevUpdatePriority = m_wantHighPriorityUpdates;

        // Recalculate visible & desired bounds
        IFC(CalculateBounds(pWindowBounds));

        // Update the timestamps of all tiles that are inside the desired region
        for (CTiledSurface::TileIterator it = BeginTiles();
            it != EndTiles();
            ++it)
        {
            CTiledSurface::Tile *pTile = it->second;

            if (TileInsideDesiredBounds(pTile))
            {
                pTile->timeStamp = timeStamp;
            }
        }

        // If the VSIS only wants high-priority updates this tick.
        // Then see if there are any high-priority updates that the VSIS needs.
        // If there are none, then switch the VSIS to ask for all updates instead.
        if (m_wantHighPriorityUpdates)
        {
            XDWORD updateRectCount;
            IFC(GetUpdateRectCount(&updateRectCount));

            if (0 == updateRectCount)
            {
                m_wantHighPriorityUpdates = FALSE;
                m_neededUpdatesValid = FALSE;
            }
            else
            {
                // There are actually high-priority updates this frame.
                // Request an additional tick for the low priority updates.
                wantAdditionalFrame = TRUE;
            }
        }

        // Trace the update priority requested this tick
        if (prevUpdatePriority != m_wantHighPriorityUpdates)
        {
            TraceVirtualSurfaceImageSourceUpdatePriorityInfo(m_wantHighPriorityUpdates);
        }

        // Request tiles from the application for needed tiles
        IFC(RaiseCallbacks());
    }
    else
    {
        // Set the desired bounds to empty to ensure that VSIS objects that are not in the tree
        // will release all of their tiles.
        EmptyRect(&m_desiredBounds);
        EmptyRect(&m_highPriorityDesiredBounds);
    }

Cleanup:
    // If the VSIS is only requesting high-priority updates this tick.
    // Then request another UI thread tick which will trigger the low-priority updates.
    *pWantAdditionalFrame = wantAdditionalFrame;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread - after PreRender has been called for all VSIS objects.
//      For each cached (non-desired) tile:
//          1.  If the tile is too old, free it immediately
//          2.  Otherwise - add the tile to the approriate standby list (which may be freed momentarily)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CVirtualSurfaceImageSource::UpdateStandbyLists(
    _Inout_ VirtualSurfaceImageSourceStandbyLists *pStandbyLists)
{
    // Tiles older than 2 seconds are freed immediately.  Tiles younger than this are put onto a standby list.
    STATIC_ASSERT((ARRAY_SIZE(pStandbyLists->list) * VSIS_STANDBY_LIST_BUCKET_SIZE) == sc_oldTileTime, ThereMustBeEnoughBuckets);

    // The bucket size is a power of 2 to avoid a 64-bit divide in this function
    STATIC_ASSERT(VSIS_STANDBY_LIST_BUCKET_SIZE == 256, BucketSizeMustBePow2);

    XUINT64 currentTime = gps->GetCPUMilliseconds();

    // The list of all tiles which should be freed immediately
    CTiledSurface::TileList tilesToFree;

    // For each tile
    for (CTiledSurface::TileIterator it = BeginTiles();
        it != EndTiles();
        ++it)
    {
        CTiledSurface::Tile *pTile = it->second;

        ASSERT(currentTime >= pTile->timeStamp);

        if (!TileInsideDesiredBounds(pTile))
        {
            // The tile is outside of the desired bounds - so it can potentially be freed

            // Compute which standby list that the tile should be placed on
            XUINT64 age = currentTime - pTile->timeStamp;

            XUINT64 bucketIndex = (age / static_cast<XUINT64>(VSIS_STANDBY_LIST_BUCKET_SIZE));

            if (bucketIndex >= ARRAY_SIZE(pStandbyLists->list))
            {
                // The tile is so old that it should be freed immediately
                tilesToFree.PushBack(&pTile->tempListEntry);
            }
            else
            {
                // The tile is not too old, put it on the correct standby list
                VirtualSurfaceImageSourceStandbyListRecord record = { this, pTile };

                IFC_RETURN(pStandbyLists->list[bucketIndex].push_back(record));

                // Update the total size of all cached tiles
                pStandbyLists->totalSize += pTile->GetAllocationSize();
            }
        }
    }

    // free all tiles that are outside of the bounds and too old
    // This was deferred from the loop above
    // because freeing a tile invalidates any outstanding iterators
    const XUINT32 offset = OFFSET(CTiledSurface::Tile, tempListEntry);

    while (!tilesToFree.IsEmpty())
    {
        CTiledSurface::Tile *pTile = tilesToFree.GetHead(offset);

        tilesToFree.Remove(&pTile->tempListEntry);

        IFC_RETURN(FreeTile(pTile));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an iterator pointing to the begining of the tile collection
//
//------------------------------------------------------------------------
CTiledSurface::TileIterator
CVirtualSurfaceImageSource::BeginTiles()
{
    return m_pTiles->Begin();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an iterator pointing past the end of the tile collection
//
//------------------------------------------------------------------------
CTiledSurface::TileIterator
CVirtualSurfaceImageSource::EndTiles()
{
    return m_pTiles->End();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a new tile to the list of tiles.  This assumes (and asserts) that there
//      does not already exist a tile with the same origin.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::AddTile(
    XPOINT origin,
    XSIZE size,
    XUINT64 timestamp,
    bool initializeToTransparent
    )
{
    // Tile origins must be aligned to tile grid
    ASSERT(0 == (origin.x % SWAlphaMaskAtlas::TileSize));
    ASSERT(0 == (origin.y % SWAlphaMaskAtlas::TileSize));

    ASSERT(size.Width > 0);
    ASSERT(size.Height > 0);

    // Tile origin must be within the bounds of the surface image source
    ASSERT(origin.x >= 0);
    ASSERT(origin.x < static_cast<XINT32>(GetWidth()));
    ASSERT(origin.y >= 0);
    ASSERT(origin.y < static_cast<XINT32>(GetHeight()));

    IFC_RETURN(m_pTiles->AddTile(origin, timestamp, initializeToTransparent, size));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Frees a tile and removes the bounds of the tile from the valid region.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::FreeTile(_In_ CTiledSurface::Tile *pTile)
{
    // Remove the tile bounds from the valid region.
    // There is no way for the caller to correctly handle an error from this function.
    IFC_RETURN(m_pValidRegion->Remove(&pTile->bounds));

    // Free the tile in the tracking data structure as well.
    m_pTiles->FreeTile(pTile);

    m_hasOutstandingFreedTiles = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Trims the underlying surface of any freed tiles.
//      This needs to happen after we've EndDraw'd the DComp surface
//      as Trim is not allowed while a DComp surface is in the drawing state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::TrimTiles()
{
    HRESULT hr = S_OK;

    XRECT *pValidTiles = NULL;
    XUINT32 tileCount = 0;

    ASSERT(!IsDrawing());

    if (m_hasOutstandingFreedTiles)
    {
        if (!m_pImageSurfaceWrapper->CheckForLostHardwareResources())
        {
            // If tiles were freed, then there were previously valid tiles.
            // That implies there must be an underlying surface unless it was lost.
            HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
            ASSERT(pHWSurfaceNoRef != NULL && pHWSurfaceNoRef->GetCompositionSurface());

            // Trim the underlying surface to the current valided tiles, removing all freed tiles from it.
            IFC(m_pTiles->GetTileRects(&pValidTiles, &tileCount));

            IFC(pHWSurfaceNoRef->GetCompositionSurface()->TrimTo(pValidTiles, tileCount));

            // Mark the image source as dirty, which will force a re-render on the UI thread.
            // This is necessary because freeing tiles in the composition surface requires that it
            // be re-submitted in a new command list.
            IFC(SetDirty());
        }

        m_hasOutstandingFreedTiles = FALSE;
    }

Cleanup:
    SAFE_DELETE_ARRAY(pValidTiles);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to trim tiles only under certain circumstances
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::TrimTilesIfPossible()
{
    // Lock to avoid racing with BeginDraw from bk thread which will put us in the drawing state.
    auto guard = GetMultithreadedLock();

    // DComp will fail the call to TrimTo on a virtual surface if it has an outstanding BeginDraw.
    // Only TrimTiles if we're not in the drawing state.  If we are in the drawing state,
    // we will TrimTiles at EndDraw time (see PreUpdateVirtual).
    if (!IsDrawing())
    {
        IFC_RETURN(TrimTiles());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the number of tiles in the surface
//
//------------------------------------------------------------------------
XUINT32
CVirtualSurfaceImageSource::GetTileCount()
{
    return m_pTiles->GetTileCount();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the maximum safe dimension of the associated D3D device
//
//------------------------------------------------------------------------
XUINT32
CVirtualSurfaceImageSource::GetMaxTextureDimension()
{
    // For compatibility with Windows 8, maintain a hard-coded max of 2048.
    return 2048;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This computes the set of update rects, and calls the application callback
//      if this set has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::RaiseCallbacks()
{
    // Raise the callback if listeners are registered and the VSIS has changed.
    if (m_pCallbacks != NULL && !m_neededUpdatesValid)
    {
        // The needed rectangles are only re-calculated when UpdatesNeeded is raised.
        // GetUpdateRectCount/GetUpdateRects will return the cached set of rectangles
        // from this point forward until the next UpdatesNeeded callback is triggered.
        IFC_RETURN(CalculateNeededUpdates());

        m_neededUpdatesValid = TRUE;

        // Don't call the callbacks if the set of needed updates is empty.
        if (!m_neededUpdates.empty())
        {
            IFC_RETURN(m_pCallbacks->UpdatesNeeded());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Request an additional UI thread frame to raise UpdatesNeeded.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CVirtualSurfaceImageSource::RequestTickForUpdatesNeeded()
{
    ASSERT(!m_neededUpdatesValid);

    IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::VSISUpdate));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks and handles device lost. Invalidates the entire VSIS on
//      device lost.
//
//------------------------------------------------------------------------
void
CVirtualSurfaceImageSource::HandleLostResources()
{
    if (m_pImageSurfaceWrapper != NULL &&
        m_pImageSurfaceWrapper->CheckForLostHardwareResources())
    {
        XRECT surfaceBounds = { 0, 0, GetWidth(), GetHeight() };
        IFCFAILFAST(Invalidate(surfaceBounds));

        // Device lost recovery frees all tiles
        // which invalidates the cached set of needed update rects.
        m_neededUpdatesValid = FALSE;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      On debug builds - verifies that the valid region is a subset of the
//      region defined by the union of all tiles.  This catches bugs where
//      a tile is removed but the coressponding rect is not removed from the valid region.
//
//------------------------------------------------------------------------
void
CVirtualSurfaceImageSource::AssertValidRegion()
{
#if DBG
    HRESULT hr = S_OK;
    bool isSubset = false;
    IPALRegion *pTileRegion = NULL;

    // Construct a region which represents the union of all tile boundaries
    IFC(gps->CreateRegion(&pTileRegion));

    for (CTiledSurface::TileIterator it = BeginTiles();
        it != EndTiles();
        ++it)
    {
        CTiledSurface::Tile *pTile = it->second;

        IFC(pTileRegion->Add(&pTile->bounds));
    }

    // Assert that the valid region is a subset of the tile region.
    // In other words, assert that all points in the valid region coresspond to active tiles in the VSIS.
    IFC(m_pValidRegion->IsSubsetOf(pTileRegion, &isSubset));
    ASSERT(isSubset);

Cleanup:
    ReleaseInterface(pTileRegion);
#endif
}

namespace CoreImports
{
    //------------------------------------------------------------------------
    // "C" wrappers that are imported by the wrapper layer
    //------------------------------------------------------------------------
    _Check_return_ HRESULT VirtualSurfaceImageSource_Initialize(
        _In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ XINT32 iWidth,
        _In_ XINT32 iHeight,
        _In_ bool isOpaque)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->Initialize(iWidth, iHeight, isOpaque));

        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_Invalidate(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _In_ XRECT updateRect)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->Invalidate(updateRect));


        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetUpdateRectCount(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
            _Out_ XDWORD* pNumOfUpdates)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->GetUpdateRectCount(pNumOfUpdates));

        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetUpdateRects(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
        _Out_writes_(count) XRECT *pUpdates,
        _In_ XDWORD count)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->GetUpdateRects(pUpdates, count));

        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_GetVisibleBounds(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
            _Out_ XRECT_RB* pBounds)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        pVirtualSurfaceImageSource->GetVisibleBounds(pBounds);

        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_RegisterCallbacks(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
                                                                    _In_ IVirtualSurfaceImageSourceCallbacks *pCallback)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->RegisterCallbacks(pCallback));

        return S_OK;
    }

    _Check_return_ HRESULT VirtualSurfaceImageSource_Resize(_In_ CVirtualSurfaceImageSource* pVirtualSurfaceImageSource,
                                                                    _In_ XINT32 newWidth,_In_ XINT32 newHeight)
    {
        IFCPTR_RETURN(pVirtualSurfaceImageSource);

        IFC_RETURN(pVirtualSurfaceImageSource->Resize(newWidth, newHeight));

        return S_OK;
    }
    }