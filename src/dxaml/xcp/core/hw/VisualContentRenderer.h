// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BaseContentRenderer.h"
#include <AAEdgeFlags.h>
#include "BrushParams.h"

class VisualContentRenderer : public BaseContentRenderer
{
public:
    VisualContentRenderer(uint32_t maxTextureSize);

    void PushRenderDataList(
        _In_opt_ PCRenderDataList* pRenderDataList,
        _In_ bool fAppendOnly) override;

    void PopRenderDataList() override;

    _Check_return_ HRESULT RenderShape(_In_ CShape* pShapeElement) override;

    wrl::ComPtr<WUComp::ICompositor5> GetCompositor5() const;

    _Check_return_ HRESULT PopulateVisual(
        _In_ const XRECTF& rect,
        _Inout_ WUComp::IVisual* pVisual) const;

    CD2DFactory* GetSharedD2DFactoryNoRef() const;

    wrl::ComPtr<WUComp::ICompositionBrush> GetCompositionBrush(_In_ CBrush* brush, _In_ CShape* pShapeElement);

protected:
    wrl::ComPtr<WUComp::ICompositionBrush> GetCompositionBrush(
        _In_ const XRECTF& rect,
        _In_ const XRECTF& brushBounds,
        _In_ AAEdge antialias,
        _In_ CBrush* brush,
        const BrushParams& brushParams,
        _In_opt_ HWTexture* maskTexture,
        _In_opt_ HWTexture* brushTexture,
        _In_ const CMILMatrix* brushTransform,
        _In_opt_ const XTHICKNESS* nineGrid,
        _In_ bool isCenterHollow,
        _In_opt_ WUComp::ICompositionBrush* existingBrush,
        _In_ const CUIElement* element);

    _Check_return_ HRESULT RenderSolidColorRectangle(
        _In_ const XRECTF& rect,
        _In_ CSolidColorBrush* pBrush
        ) override;

    _Check_return_ HRESULT RenderTextRealization(
        _In_ const XRECTF& brushBounds,
        _In_ HWTextRealization *pHwTextRealization,
        bool allowAnimatedColor
        ) override;

    _Check_return_ HRESULT GeneralImageRenderContent(
        _In_ const XRECTF& rect,
        _In_ const XRECTF& brushBounds,
        _In_ CBrush *pBrush,
        const BrushParams& brushParams,
        _In_opt_ CUIElement *pUIElement,
        _In_opt_ const XTHICKNESS *pNinegrid,
        _In_opt_ HWTexture *pShapeHwTexture,
        _In_ bool fIsHollow
        ) override;

    _Check_return_ HRESULT CalculateTileBrushTransform(
        _In_ CTileBrush *pTileBrush,
        _In_ const XRECTF *pBounds,
        _Out_ CMILMatrix *pSourceTextureToElement,
        _Out_ CMILMatrix *pBrushTextureToElement
        ) const override;

private:
    bool DoesBrushTextureFillRect(
        _In_ const XRECTF& rect,
        _In_ CBrush* brush,
        _In_opt_ HWTexture* maskTexture,
        _In_opt_ HWTexture* brushTexture,
        _In_ const CMILMatrix* brushTransform,
        _In_opt_ const XTHICKNESS* nineGrid);

    _Check_return_ HRESULT RenderPrimitive(
        const CUIElement* element,
        _In_ const XRECTF& rect,
        _In_ const XRECTF& brushBounds, // Different from rect for text + linear gradient brushes, where the brush bounds are different from the rect of the rendered text
        _In_ AAEdge antialias,
        _In_ CBrush* brush,
        const BrushParams& brushParams,
        _In_opt_ HWTexture* maskTexture,
        _In_opt_ HWTexture* brushTexture,
        _In_ const CMILMatrix* brushTransform,
        _In_opt_ const XTHICKNESS* nineGrid,
        _In_ bool isCenterHollow);

    _Check_return_ HRESULT EnsureSpriteVisual(
        _In_ WUComp::ICompositor * pCompositor,
        _In_ const XRECTF& rect,
        _In_ AAEdge antialias,
        _Outptr_ WUComp::ISpriteVisual ** ppContentVisual,
        _Outptr_ WUComp::IVisual ** ppVisual
        ) const;

    void EnsureShapeVisual(
        _In_ CShape * pShapeElement,
        _Outptr_ WUComp::IShapeVisual ** ppContentVisual,
        _Outptr_ WUComp::IVisual ** ppVisual) const;

    _Check_return_ HRESULT EnsureClip(
        _In_ WUComp::ICompositor * pCompositor,
        _In_ WUComp::IVisual * pVisual,
        _In_ const XRECTF & rect,
        _In_ const HWClip & localClip
        ) const;

    _Check_return_ HRESULT LinkVisual(
        _In_opt_ CBrush* brush1,
        _In_opt_ CBrush* brush2,
        _In_ WUComp::IVisual* visual);

    void UnparentVisual(_In_ WUComp::IVisual* visual);

    void UnlinkVisual(_In_ WUComp::IVisual* visual);

    void TargetByLights(
        _In_opt_ CBrush* brush1,
        _In_opt_ CBrush* brush2,
        _In_ WUComp::IVisual* visual);

    bool IsRecycledVisual() const;

    void CheckVisualTypeConsistency(_In_ WUComp::IVisual* newVisual) const;

    bool ShouldSpriteVisualBeTransparentForInput() const;

private:
    // This list of visuals is used to recycle the SpriteVisuals held in a UIElement.
    // UIElement's ref-counted render data list is swapped into this collection, and SpriteVisuals
    // are retrieved starting at m_idxNextReusable. This collection is ref-counted starting at
    // m_idxNextReusable (inclusive).
    PCRenderDataList m_RecycledRenderData;
    XUINT32 m_idxNextReusable;

    wrl::ComPtr<WUComp::IVisualCollection> m_spCurrentParentCollection;
};

