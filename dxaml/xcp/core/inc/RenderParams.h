// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "palgfx.h"

class CWindowRenderTarget;
struct IPALAcceleratedGraphicsFactory;
struct IDirtyRegionAccumulator;
struct IPALAcceleratedRenderTarget;
struct IPALAcceleratedBrush;
struct IDirtyRegionQuery;
class CMILMatrix;

// Parameters for the render walk that are shared between software and D2D rendering.
struct SharedRenderParams
{
public:
    // Use the pCurrentTransform as-is, without appending any other transforms.
    bool overrideTransform = false;

    // For rounded-corner borders, collapse the mask to just the four corners plus an extra pixel in each dimension
    // to reduce memory consumption. The collapsed mask gets nine grid stretched back up to the full size.
    bool renderCollapsedMask { false };

    // The transform from the root of the walk or previous intermediate surface to this point.
    const CMILMatrix *pCurrentTransform = nullptr;
};

struct NWRenderParams
{
public:
    NWRenderParams();

    bool fTransformDirty : 1;              // True if a transform in the ancestor chain has changed

    // Whether to draw the element as opaque regardless of its actual opacity, used for drawing into layers or intermediates
    bool fForceOpaque : 1;

    // Whether cached composition is enabled. This may be disabled from the root or for a subtree only, but can never be
    // re-enabled in a node or subtree below nodes which have disabled it.
    bool fCachedCompositionEnabled : 1;

    // Whether or not to render child elements.
    bool fRenderChildren : 1;

    // The edge vector created during the render is usually
    // cached on the element. If this flag is false one should
    // neither reuse such cached edge vector nor cache the
    // newly created one. This is used for RenderTargetBitmap.
    bool fCacheEdgeVectors : 1;
};

struct D2DPrecomputeParams
{
    D2DPrecomputeParams(
        _In_ IPALAcceleratedGraphicsFactory* pFactory,
        _In_ IDirtyRegionAccumulator* pDirtyRegion
        );

    _Ret_notnull_ IPALAcceleratedGraphicsFactory* GetFactory() const
    {
        return m_pFactoryNoRef;
    }

    void SetAddDirtyRegion(bool fShouldAdd)
    {
        m_fShouldAddDirtyRegion = fShouldAdd;
    }

    bool ShouldAddDirtyRegion() const
    {
        return m_fShouldAddDirtyRegion;
    }

    void Reset();

    void AddSurfaceSpaceRect(const XRECTF_RB& rect);

    void GetCacheSize(
        _Out_ XINT32& width,
        _Out_ XINT32& height
        ) const;

    void GetCacheOffsets(
        _Out_ XINT32& offsetX,
        _Out_ XINT32& offsetY
        ) const;

private:
    IPALAcceleratedGraphicsFactory *m_pFactoryNoRef;
    bool m_fShouldAddDirtyRegion;
    XRECTF_RB m_cacheSize;
};

struct PrintParams
{
    PrintParams();
    bool GetIsPrintTarget() const { return m_fIsPrintTarget; }
    bool m_fForceVector;

protected:
    bool m_fIsPrintTarget;
};

struct D2DRenderParams
    : public PrintParams    // TODO: D2D: consider splitting this
{
public:
    D2DRenderParams(
        _In_ IPALAcceleratedRenderTarget *pRenderTarget,
        _In_ IDirtyRegionQuery *pDirtyRegion,
        bool isPrintTarget
        );

    _Ret_notnull_ IPALAcceleratedRenderTarget* GetD2DRenderTarget() const
    {
        return m_pRenderTargetNoRef;
    }

    _Ret_notnull_ IPALAcceleratedRenderTarget *GetRenderTarget() const
    {
        return m_pRenderTargetNoRef;
    }

    void SetOverrideBrush(_In_ IPALAcceleratedBrush* overrideBrush)
    {
        m_overrideBrush = overrideBrush;
    }

    wrl::ComPtr<IPALAcceleratedBrush> GetOverrideBrush() const
    {
        return m_overrideBrush;
    }

    //
    // Opacity is usually handled by pushing a semitransparent layer and drawing content into
    // the layer at full opacity, but elements that don't have overlap (e.g. text block, a shape
    // with just a fill) can draw opacity by pushing it into the brush. This flag marks whether
    // to draw the element as opaque regardless of its actual opacity, used for pushing an
    // element's opacity into its brush when drawing. Set to FALSE when drawing leaf elements
    // that can handle their own opacity (see NWCanApplyOpacityWithoutLayer), except when they
    // are the root of a HW-cached subtree (in which case the opacity is applied on the compositor
    // side).
    //
    bool m_forceOpaque : 1;

    // These flags are used by the render walk to get separate fill & stroke masks.
    bool m_renderFill : 1;
    bool m_renderStroke : 1;

private:
    _Notnull_ IPALAcceleratedRenderTarget *m_pRenderTargetNoRef;

    // When rendering alpha masks, we don't care about the brush. Just use a solid color brush.
    _Maybenull_ wrl::ComPtr<IPALAcceleratedBrush> m_overrideBrush;
};
