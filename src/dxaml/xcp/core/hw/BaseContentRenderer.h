// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentRenderer.h"

class CLinearGradientBrush;

class __declspec(novtable) BaseContentRenderer : public IContentRenderer
{
public:
    void SetRenderParams(_In_ const HWRenderParams* pRenderParams) override;
    const HWRenderParams* GetRenderParams() const override;
    void SetElementRenderParams(_In_ HWElementRenderParams* pElementRenderParams) override;
    HWElementRenderParams* GetElementRenderParams() const override;

    void SetTransformToRoot(_In_ const CTransformToRoot* pTransformToRoot) override
    {
        m_pTransformToRoot = pTransformToRoot;
    }

    const CTransformToRoot* GetTransformToRoot() const override
    {
        return m_pTransformToRoot;
    }

    void SetUIElement(_In_ CUIElement* pUIElement) override;
    CUIElement* GetUIElement() override;
    void SetUpdatedShapeRealization(bool value) override;
    bool GetUpdatedShapeRealization() const override;

    _Check_return_ HRESULT MaskCombinedRenderHelper(
        _In_ CUIElement *pUIElement,
        _In_opt_ IMaskPartRenderParams *pStrokeMaskPart,
        _In_opt_ IMaskPartRenderParams *pFillMaskPart,
        bool forceMaskDirty
    ) override;

    _Check_return_ HRESULT PopupRootRenderContent(
        _In_ CPopupRoot* pPopupRoot
    ) override;

    _Check_return_ HRESULT PanelRenderContent(_In_ CPanel *pPanel) override;

    _Check_return_ HRESULT ListViewBaseItemChromeRenderLayer(
        _In_ CListViewBaseItemChrome *pChrome,
        _In_ ListViewBaseItemChromeLayerPosition layer
    ) override;

    _Check_return_ HRESULT ContentPresenterRenderContent(
        _In_ CContentPresenter *pContentPresenter
    ) override;

    _Check_return_ HRESULT UserControlRenderBackground(
        _In_ CUserControl *pUserControl
    ) override;

    _Check_return_ HRESULT ImageRenderContent(
        _In_ CImage *pImage
    ) override;

    _Check_return_ HRESULT GlyphsRenderContent(
        _In_ CGlyphs *pGlyphs
    ) override;

    _Check_return_ HRESULT TextBlockRenderContent(
        _In_ CTextBlock *pTextBlock
    ) override;

    _Check_return_ HRESULT RichTextBlockRenderContent(
        _In_ CRichTextBlock *pRichTextBlock
    ) override;

    _Check_return_ HRESULT RichTextBlockOverflowRenderContent(
        _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
    ) override;

    _Check_return_ HRESULT TextBoxViewRenderContent(
        _In_ CTextBoxView *pTextBoxView
    ) override;

    _Check_return_ HRESULT RectangleRenderContent(
        _In_ CRectangle *pRectangle
    ) override;

    _Check_return_ HRESULT BorderRenderContent(
        _In_ CBorder *pBorderElement
    ) override;

    _Check_return_ HRESULT ContentControlItemRender(
        _In_ CContentControl *pContentControl
    ) override;

    _Check_return_ HRESULT CalendarViewBaseItemChromeRenderLayer(
        _In_ CCalendarViewBaseItemChrome *pChrome,
        _In_ CalendarViewBaseItemChromeLayerPosition layer
    ) override;

    _Check_return_ HRESULT TextBlockPostChildrenRender(
        _In_ CTextBlock *pTextBlock
    ) override;

    _Check_return_ HRESULT RichTextBlockPostChildrenRender(
        _In_ CRichTextBlock *pRichTextBlock
    ) override;

    _Check_return_ HRESULT RichTextBlockOverflowPostChildrenRender(
        _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
    ) override;

    _Check_return_ HRESULT ContentControlPostChildrenRender(
        _In_ CContentControl *pContentControl
    ) override;

    _Check_return_ HRESULT BorderLikeElementPostChildrenRender(
        _In_ CFrameworkElement *element
    ) override;

    _Check_return_ HRESULT RenderFocusRectangle(
        _In_ CUIElement *pUIElement,
        _In_ FocusRectangleOptions &focusOptions
    ) override;

    _Check_return_ HRESULT RenderFocusRectangle(
        _In_ CCoreServices *pCore,
        _In_ XRECTF bounds,
        _In_ bool isContinuous,
        _In_ float dashOffset,
        _In_ float strokeThickness,
        _In_ CBrush *brush
    ) override;

    DCompTreeHost* GetDCompTreeHost() const { return m_dcompTreeHostNoRef; }

    void AddDirtyElementForNextFrame(_In_ CUIElement* element) override;
    void DirtyElementsForNextFrame();

protected:
    BaseContentRenderer(uint32_t maxTextureSize);

    virtual _Check_return_ HRESULT CalculateTileBrushTransform(
        _In_ CTileBrush *pTileBrush,
        _In_ const XRECTF *pBounds,
        _Out_ CMILMatrix *pBrushTransform,
        _Out_ CMILMatrix *pBrushTextureToElement
        ) const = 0;

    static _Check_return_ HRESULT MaskPartEnsureRealizationTexture(
        _In_ const HWRenderParamsBase &myRP,
        _In_ IMaskPartRenderParams* pParams,
        XUINT32 width,
        XUINT32 height,
        _In_ HWShapeRealization *pHwShapeRealization,
        bool hasBrushAnimation,
        _Out_ bool *pIsForHitTestOnly);

     _Check_return_ HRESULT GetBrushParameters(
        _In_ HWTextureManager *pTextureManager,
        _In_ SurfaceCache *pSurfaceCache,
        _In_ CBrush *pBrush,
        bool setColorOnPrimitive,
        _In_opt_ CUIElement *pUIElement,
        _In_opt_ const XTHICKNESS *pNinegrid,
        _In_ const XRECTF* primitiveBounds, // Used for WUC linear gradient brush + Xaml RichTextBlock
        _In_ const XRECTF* brushBounds,
        _Out_ XFLOAT *pLocalOpacity,
        _Out_ XUINT32 *pBrushMask,
        _Out_ bool *pHasBrush,
        _Out_ HWTexture** ppBrushTexture,
        _Out_opt_ CMILMatrix *pBrushTextureTransform);

    _Check_return_ HRESULT PanelRenderContentHelper(
        _In_ CPanel *pPanel
        );

    _Check_return_ HRESULT BorderRenderHelper(
        _In_ CFrameworkElement *pFrameworkElement
        );

    _Check_return_ HRESULT BorderHWRenderHelper(
        _In_ CFrameworkElement *pFrameworkElement
        );

    _Check_return_ HRESULT MaskPartRenderHelper(
        _In_opt_ CUIElement *pUIElement,
        _In_ const XPOINTF& shapeOffset,
        XFLOAT realizationScaleX,
        XFLOAT realizationScaleY,
        _In_ IMaskPartRenderParams* pParams,
        const BrushParams& brushParams,
        _In_opt_ HWTexture *pHwPartTexture,
        bool isForHitTestOnly,
        const bool renderCollapsedMask,
        const XTHICKNESS* nineGrid);

    _Check_return_ HRESULT SwapChainPanelRenderContent(
        _In_ CSwapChainPanel *pSwapChainPanel
        );

    _Check_return_ HRESULT RenderFocusRectangleWithThickness(
        _In_ CCoreServices *pCore,
        _In_ const XRECTF& bounds,
        _In_ CBrush *brush,
        _In_ const XTHICKNESS& thickness
        );

    uint32_t GetMaxTextureSize() const
    {
        return m_maxTextureSize;
    }

protected:
    PCRenderDataList* m_pRenderDataList;
    const HWRenderParams* m_pRenderParams;
    HWElementRenderParams* m_pElementRenderParams;
    const CTransformToRoot* m_pTransformToRoot;
    CUIElement* m_pUIElementNoRef;
    DCompTreeHost* m_dcompTreeHostNoRef;
    uint32_t m_maxTextureSize;
    bool m_updatedShapeRealization;

    //
    // A collection used to handle render walk error scenarios.
    //
    // Sometimes we have an element that fails to render, and we'd like to try again next frame. The way to do that
    // is to mark the element with content dirty, then when the dirty flag propagates up to the tree root, we request
    // another frame automatically. The problem is that we encounter rendering errors in the middle of the render walk,
    // and we can't set dirty flags in the middle of the render walk, which itself consumes those dirty flags. Even
    // if we ignored asserts and forced the flag to be set, when the render walk leaves an element, it will clean dirty
    // flags on that element, which will clear the flag we just set.
    //
    // To avoid these problems, we build a list of elements that encountered rendering errors during the walk. At the
    // end of the render walk, when the tree is freshly cleaned, we'll go mark those elements as dirty to trigger them
    // to render again next frame.
    //
    // This list is built up during the render walk and is consumed and cleared right afterwards. No elements are
    // expected to be released during the render walk, so we can use NoRef here.
    //
    std::vector<CUIElement*> m_dirtyElementsForNextFrameNoRef;
};
