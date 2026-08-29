// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <PCRenderDataList.h>
#include "BrushParams.h"

class CContentPresenter;
class CUserControl;
class CImage;
class CGlyphs;
class CTextBlock;
class CRichTextBlock;
class CRichTextBlockOverflow;
class CTextBoxView;
class CRectangle;
class CBorder;
class CMediaBase;
class CContentControl;
class CCoreServices;
class CSolidColorBrush;
class CBrush;
class CCalendarViewBaseItemChrome;
class CListViewBaseItemChrome;
class CMILMatrix;
class CPanel;
class CPopupRoot;
class CUIElement;
class HWTextRealization;
class HWTexture;
class IMaskPartRenderParams;
struct HWRenderParams;
struct HWElementRenderParams;
struct FocusRectangleOptions;
enum class CalendarViewBaseItemChromeLayerPosition;
enum ListViewBaseItemChromeLayerPosition;

// Interface for content rendering implementations:
// This abstracts away the details of exactly which composition APIs are being used to generate content
// We currently have two implementations:
// VisualContentRenderer:  Uses WinRT Composition (SpriteVisual)

// TODO_WinRT:  Move this into Components
class __declspec(novtable) IContentRenderer
{
public:
    virtual void PushRenderDataList(_In_opt_ PCRenderDataList* pRenderData, _In_ bool fAppendOnly) = 0;
    virtual void PopRenderDataList() = 0;
    virtual void SetRenderParams(_In_ const HWRenderParams* pRenderParams) = 0;
    virtual const HWRenderParams* GetRenderParams() const = 0;
    virtual void SetElementRenderParams(_In_ HWElementRenderParams* pElementRenderParams) = 0;
    virtual HWElementRenderParams* GetElementRenderParams() const = 0;
    virtual void SetTransformToRoot(_In_ const CTransformToRoot* pTransformToRoot) = 0;
    virtual const CTransformToRoot* GetTransformToRoot() const = 0;
    virtual void SetUIElement(_In_ CUIElement* pUIElement) = 0;
    virtual CUIElement* GetUIElement() = 0;
    virtual void SetUpdatedShapeRealization(bool value) = 0;
    virtual bool GetUpdatedShapeRealization() const = 0;

    virtual _Check_return_ HRESULT MaskCombinedRenderHelper(
        _In_ CUIElement *pUIElement,
        _In_opt_ IMaskPartRenderParams *pStrokeMaskPart,
        _In_opt_ IMaskPartRenderParams *pFillMaskPart,
        bool forceMaskDirty
        ) = 0;

    virtual _Check_return_ HRESULT PopupRootRenderContent(
        _In_ CPopupRoot* pPopupRoot
        ) = 0;

    virtual _Check_return_ HRESULT PanelRenderContent(_In_ CPanel *pPanel) = 0;

    virtual _Check_return_ HRESULT ListViewBaseItemChromeRenderLayer(
        _In_ CListViewBaseItemChrome *pChrome,
        _In_ ListViewBaseItemChromeLayerPosition layer
        ) = 0;

    virtual _Check_return_ HRESULT ContentPresenterRenderContent(
        _In_ CContentPresenter *pContentPresenter
        ) = 0;

    virtual _Check_return_ HRESULT UserControlRenderBackground(
        _In_ CUserControl *pUserControl
        ) = 0;

    virtual _Check_return_ HRESULT ImageRenderContent(
        _In_ CImage *pImage
        ) = 0;

    virtual _Check_return_ HRESULT GlyphsRenderContent(
        _In_ CGlyphs *pGlyphs
        ) = 0;

    virtual _Check_return_ HRESULT TextBlockRenderContent(
        _In_ CTextBlock *pTextBlock
        ) = 0;

    virtual _Check_return_ HRESULT RichTextBlockRenderContent(
        _In_ CRichTextBlock *pRichTextBlock
        ) = 0;

    virtual _Check_return_ HRESULT RichTextBlockOverflowRenderContent(
        _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
        ) = 0;

    virtual _Check_return_ HRESULT TextBoxViewRenderContent(
        _In_ CTextBoxView *pTextBoxView
        ) = 0;

    virtual _Check_return_ HRESULT RectangleRenderContent(
        _In_ CRectangle *pRectangle
        ) = 0;

    virtual _Check_return_ HRESULT BorderRenderContent(
        _In_ CBorder *pBorderElement
        ) = 0;
        
    virtual _Check_return_ HRESULT ContentControlItemRender(
        _In_ CContentControl *pContentControl
        ) = 0;

    virtual _Check_return_ HRESULT CalendarViewBaseItemChromeRenderLayer(
        _In_ CCalendarViewBaseItemChrome *pChrome,
        _In_ CalendarViewBaseItemChromeLayerPosition layer
        ) = 0;

    virtual _Check_return_ HRESULT TextBlockPostChildrenRender(
        _In_ CTextBlock *pTextBlock
        ) = 0;

    virtual _Check_return_ HRESULT RichTextBlockPostChildrenRender(
        _In_ CRichTextBlock *pRichTextBlock
        ) = 0;

    virtual _Check_return_ HRESULT RichTextBlockOverflowPostChildrenRender(
        _In_ CRichTextBlockOverflow *pRichTextBlockOverflow
        ) = 0;

    virtual _Check_return_ HRESULT ContentControlPostChildrenRender(
        _In_ CContentControl *pContentControl
        ) = 0;

    virtual _Check_return_ HRESULT BorderLikeElementPostChildrenRender(
        _In_ CFrameworkElement *element
        ) = 0;

    virtual _Check_return_ HRESULT RenderFocusRectangle(
        _In_ CUIElement *pUIElement,
        _In_ FocusRectangleOptions &focusOptions
        ) = 0;

    virtual _Check_return_ HRESULT RenderFocusRectangle(
        _In_ CCoreServices *pCore,
        _In_ XRECTF bounds,
        _In_ bool isContinuous,
        _In_ float dashOffset,
        _In_ float strokeThickness,
        _In_ CBrush *brush
        ) = 0;

    virtual _Check_return_ HRESULT RenderSolidColorRectangle(
        _In_ const XRECTF& rect,
        _In_ CSolidColorBrush* pBrush
        ) = 0;

    virtual _Check_return_ HRESULT RenderTextRealization(
        _In_ const XRECTF& brushBounds,
        _In_ HWTextRealization *pHwTextRealization,
        bool allowAnimatedColor // TODO WinRT: Figure out solid color animation story
        ) = 0;

    virtual _Check_return_ HRESULT GeneralImageRenderContent(
        _In_ const XRECTF& rect,
        _In_ const XRECTF& brushBounds,
        _In_ CBrush *pBrush,
        const BrushParams& brushParams,
        _In_opt_ CUIElement *pUIElement,
        _In_opt_ const XTHICKNESS *pNinegrid,
        _In_opt_ HWTexture *pShapeHwTexture,
        _In_ bool fIsHollow
        ) = 0;

    virtual _Check_return_ HRESULT RenderShape(_In_ CShape *pShapeElement) = 0;

    virtual void AddDirtyElementForNextFrame(_In_ CUIElement* element) = 0;
};

class StackAllocationPattern
{
private:
    // Only stack allocation allowed, prevent new operator from being used
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};

// Stack allocated helper class used to establish a context for rendering a UIElement
class ElementRenderingContext : public StackAllocationPattern
{
public:
    ElementRenderingContext(
        _In_ IContentRenderer* pContentRenderer,
        _In_ CUIElement* pUIElement,
        _In_ const HWRenderParams* pRenderParams,
        _In_opt_ HWElementRenderParams* pElementRenderParams,
        _In_opt_ const CTransformToRoot* p2DTransformToRoot
        );

    ~ElementRenderingContext();

private:
    // Default constructor not allowed
    ElementRenderingContext();

private:
    IContentRenderer* m_pContentRenderer;
    CUIElement* m_pSavedUIElement;
    const HWRenderParams* m_pSavedRenderParams;
    HWElementRenderParams* m_pSavedElementRenderParams;
    const CTransformToRoot* m_pSavedTransformToRoot;
};

// Stack-allocated helper class used to temporarily override ContentRenderer's RenderParams
class HWRenderParamsOverride : public StackAllocationPattern
{
public:
    HWRenderParamsOverride(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const HWRenderParams* pRenderParams
        );

    ~HWRenderParamsOverride();

private:
    // Default constructor not allowed
    HWRenderParamsOverride();

private:
    IContentRenderer* m_pContentRenderer;
    const HWRenderParams* m_pSavedRenderParams;
};

// Stack-allocated helper class used to temporarily override ContentRenderer's TransformToRoot2D
class TransformToRoot2DOverride : public StackAllocationPattern
{
public:
    TransformToRoot2DOverride(
        _In_ IContentRenderer* pContentRenderer,
        _In_ const CTransformToRoot* p2DTransformToRoot
        );

    ~TransformToRoot2DOverride();

private:
    // Default constructor not allowed
    TransformToRoot2DOverride();

private:
    IContentRenderer* m_pContentRenderer;
    const CTransformToRoot* m_pSavedTransformToRoot;
};

// Stack allocated helper class used to collect incremental PCRenderData from multiple render operations
class CaptureRenderData : public StackAllocationPattern
{
public:
    CaptureRenderData(
        _In_ IContentRenderer* pContentRenderer,
        _In_opt_ PCRenderDataList* pRenderDataList,
        _In_ bool fAppendOnly
        );

    ~CaptureRenderData();

private:
    // Default constructor not allowed
    CaptureRenderData();

private:
    IContentRenderer* m_pContentRenderer;
};

class RealizationUpdateContext : public StackAllocationPattern
{
public:
    RealizationUpdateContext(
        _In_ IContentRenderer* pContentRenderer
        );

    ~RealizationUpdateContext();

private:
    // Default constructor not allowed
    RealizationUpdateContext();

private:
    IContentRenderer* m_pContentRenderer;
    bool m_savedUpdatedShapeRealization;
};
