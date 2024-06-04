// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "hwrealization.h"
#include "ColorUtil.h"

HWRealization::HWRealization(HWRealizationType::Enum realizationType)
    : m_realizationType(realizationType)
    , m_renderedForAnimation(FALSE)
    , m_surfaceOffsetX(0.0f)
    , m_surfaceOffsetY(0.0f)
{
    m_realizationTransform.SetToIdentity();
}

bool HWRealization::NeedsUpdate(
    _In_ const CMILMatrix *pRealizationTransform,
    bool isTransformAnimating
    )
{
    // If the scale changed, we need to update the realization.  Don't bother if animating, though.
    // TODO: HWPC: We should consider a better heuristic here when scale is animating, ideas...
    // TODO: HWPC: - Only re-realize after the percentage scale change reaches a threshold
    // TODO: HWPC: - Limit re-realization to every x frames, so fast large-delta animations don't hit the first case too often
    // TODO: HWPC: - Determine the final scale from the animation, and re-realize at the target scale ahead of time to minimize 'snap in'
    if (!isTransformAnimating)
    {
        XFLOAT oldScaleX, oldScaleY, newScaleX, newScaleY;
        m_realizationTransform.GetScaleDimensions(&oldScaleX, &oldScaleY);
        pRealizationTransform->GetScaleDimensions(&newScaleX, &newScaleY);

        return !IsCloseReal(oldScaleX, newScaleX) || !IsCloseReal(oldScaleY, newScaleY);
    }

    return false;
}

void HWRealization::UpdateRealizationParameters(
    _In_ const CMILMatrix *pRealizationTransform,
    bool isTransformAnimating,
    XFLOAT surfaceOffsetX,
    XFLOAT surfaceOffsetY,
    const bool renderCollapsedMask)
{
    m_realizationTransform = *pRealizationTransform;
    m_renderedForAnimation = isTransformAnimating;

    m_surfaceOffsetX = surfaceOffsetX;
    m_surfaceOffsetY = surfaceOffsetY;

    m_renderedCollapsedMask = renderCollapsedMask;
}

void HWRealization::GetRealizationScale(
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY
    )
{
    m_realizationTransform.GetScaleDimensions(pScaleX, pScaleY);
}

bool HWRealization::HasSubPixelOffsets() const
{
    return (FractionReal(m_realizationTransform.GetDx()) != 0) || (FractionReal(m_realizationTransform.GetDy()) != 0);
}

HWShapeRealization::HWShapeRealization()
    : HWRealization(HWRealizationType::Shape)
    , m_pStrokeHwTexture(nullptr)
    , m_pFillHwTexture(nullptr)
    , m_fStrokeWasClipped(FALSE)
    , m_fFillWasClipped(FALSE)
{
}

HWShapeRealization::~HWShapeRealization()
{
    ReleaseInterface(m_pStrokeHwTexture);
    ReleaseInterface(m_pFillHwTexture);
}

void HWShapeRealization::SetStrokeHwTexture(
    _In_opt_ HWTexture *pNewStrokeHwTexture
    )
{
    // Existing HWTextures should always be freed before allocating a new one for the same realization.
    ASSERT(m_pStrokeHwTexture == nullptr || pNewStrokeHwTexture == nullptr);
    ReplaceInterface(
        m_pStrokeHwTexture,
        pNewStrokeHwTexture);
}

void HWShapeRealization::SetFillHwTexture(_In_opt_ HWTexture *pNewFillHwTexture)
{
    // Existing HWTextures should always be freed before allocating a new one for the same realization.
    ASSERT(m_pFillHwTexture == nullptr || pNewFillHwTexture == nullptr);
    ReplaceInterface(
        m_pFillHwTexture,
        pNewFillHwTexture);
}

bool HWShapeRealization::HasLostRealizationTexture()
{
    return (m_pFillHwTexture != nullptr && m_pFillHwTexture->IsSurfaceLost())
        || (m_pStrokeHwTexture != nullptr && m_pStrokeHwTexture->IsSurfaceLost());
}

bool HWShapeRealization::MaskNeedsUpdate(
    const bool renderCollapsedMask,
    _In_opt_ CBrush *pFillBrush,
    bool isFillBrushDirty,
    bool isFillBrushAnimated,
    _Out_ bool* pIsFillForHitTestOnly,
    _In_opt_ CBrush *pStrokeBrush,
    bool isStrokeBrushDirty,
    bool isStrokeBrushAnimated,
    _Out_ bool* pIsStrokeForHitTestOnly
    )
{
    if (m_renderedCollapsedMask != renderCollapsedMask)
    {
        return true;
    }

    bool fillDirty = MaskPartNeedsUpdate(pFillBrush, isFillBrushDirty, isFillBrushAnimated, m_pFillHwTexture, &m_fFillWasClipped, pIsFillForHitTestOnly);
    bool strokeDirty = MaskPartNeedsUpdate(pStrokeBrush, isStrokeBrushDirty, isStrokeBrushAnimated, m_pStrokeHwTexture, &m_fStrokeWasClipped, pIsStrokeForHitTestOnly);

    return fillDirty || strokeDirty;
}

bool HWShapeRealization::MaskPartNeedsUpdate(
    _In_opt_ CBrush *pBrush,
    bool isBrushDirty,
    bool isBrushAnimated,
    _In_opt_ HWTexture *pHwTexture,
    _Inout_ bool *pWasClipped,
    _Out_ bool *pIsForHitTestOnly
    )
{
    bool dirty = false;
    bool isForHitTestOnly = false;

    bool brushDrawsSomething = (pBrush != nullptr);
    if (brushDrawsSomething && !isBrushAnimated && pBrush->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
    {
        CSolidColorBrush *pSolidColorBrush = static_cast<CSolidColorBrush*>(pBrush);

        // Primitives drawn with unanimated transparent brushes aren't visible, so we can just skip generating them.
        // Or we would be able to, if it weren't for hit testing. As an small optimization, rather than generating a
        // transparent primitive with a mask texture, we'll generate a transparent primitive with no alpha mask. The
        // only purpose of the primitive is to provide bounds to DComp, and DComp doesn't consider alpha masks when
        // bounding anyway.
        //
        // This doesn't apply if there's a color animation or a brush transition playing. The transparent brush might
        // be animating to something that's opaque, so we still need a mask for it.
        isForHitTestOnly = ColorUtils::IsTransparentColor(pSolidColorBrush->m_rgb);
        brushDrawsSomething = !isForHitTestOnly;
    }

    if (brushDrawsSomething)
    {
        // If there is a brush but no mask, then create the mask
        dirty = (pHwTexture == nullptr);

        if (isBrushDirty)
        {
            bool clipped = pBrush->HasClipRect();

            // always update if there is a clip but also update if brush has gone from clipped to unclipped
            dirty = dirty || *pWasClipped || clipped;

            *pWasClipped = !!clipped;
        }
    }
    else
    {
        // If there is a mask but no need for one, then release the mask
        dirty = (pHwTexture != nullptr);

        *pWasClipped = FALSE;
    }

    *pIsForHitTestOnly = isForHitTestOnly;
    return dirty;
}

HWTextRealization::HWTextRealization()
    : HWRealization(HWRealizationType::Text)
{
}

HWTextRealization::~HWTextRealization()
{
    ReleaseInterface(m_pTextHwTexture);
    ReleaseInterface(m_pForegroundBrush);
}

bool HWTextRealization::NeedsUpdate(
    _In_ const CMILMatrix *pWorldTransform,
    bool isTransformAnimating
    )
{
    // Text realizations depend on sub-pixel positioning in order for text to remain sharp.
    // If that changes, we need to re-realize, but don't bother if we're animating.
    if (!isTransformAnimating)
    {
        const XFLOAT oldSubX = FractionReal(m_realizationTransform.GetDx());
        const XFLOAT newSubX = FractionReal(pWorldTransform->GetDx());

        const XFLOAT oldSubY = FractionReal(m_realizationTransform.GetDy());
        const XFLOAT newSubY = FractionReal(pWorldTransform->GetDy());

        if (!IsCloseReal(oldSubX, newSubX) || !IsCloseReal(oldSubY, newSubY))
        {
            return true;
        }
    }

    return HWRealization::NeedsUpdate(pWorldTransform, isTransformAnimating);
}

XFLOAT HWTextRealization::GetSurfaceOffsetX() const
{
    return m_surfaceOffsetX - FractionReal(m_realizationTransform.GetDx());
}

XFLOAT HWTextRealization::GetSurfaceOffsetY() const
{
    return m_surfaceOffsetY - FractionReal(m_realizationTransform.GetDy());
}

void HWTextRealization::SetTextHwTexture(_In_ HWTexture *pNewTextHwTexture)
{
    ReplaceInterface(m_pTextHwTexture, pNewTextHwTexture);
}

void HWTextRealization::SetForegroundBrush(_In_ CBrush *pForegroundBrush)
{
    ReplaceInterface(m_pForegroundBrush, pForegroundBrush);
}

bool HWTextRealization::HasLostRealizationTexture()
{
    return m_pTextHwTexture != nullptr && m_pTextHwTexture->IsSurfaceLost();
}

void HWTextRealization::SetIsColorBitmap(bool isColorBitmap)
{
    m_isColorBitmap = isColorBitmap;
}
