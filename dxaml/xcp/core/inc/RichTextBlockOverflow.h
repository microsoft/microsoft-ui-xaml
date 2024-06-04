// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FontAndScriptServices.h"
class CRichTextBlock;
class PageNode;
class RichTextBlockBreak;
class IContentRenderer;

#include "ILinkedTextContainer.h"
#include "TextHighlightMerge.h"

//------------------------------------------------------------------------
//
//  Class:  CRichTextBlockOverflow
//
//  Synopsis:
//
//  Overflow target for CRichTextBlock's content.
//
//------------------------------------------------------------------------
class CRichTextBlockOverflow final : public CFrameworkElement,
                              public RichTextServices::ILinkedTextContainer
{
private:
    CRichTextBlockOverflow(_In_ CCoreServices* pCore);
    ~CRichTextBlockOverflow() override;

public:
    // Public properties

    // Fields unique to RichTextBlockOverflow
    CRichTextBlock* m_pMaster;

    // Fields common to RichTextBlock and RichTextBlockOverflow
    CRichTextBlockOverflow* m_pOverflowTarget;

    // Fields common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    XTHICKNESS m_padding = {};
    uint32_t m_maxLines;
    float m_layoutRoundingHeightAdjustment;
    bool m_isTextTrimmed = false;
    std::vector<HighlightRegion> m_highlightRegions;

    DECLARE_CREATE(CRichTextBlockOverflow);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRichTextBlockOverflow>::Index;
    }

    bool IsRightToLeft() final;

    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject* pNamescopeOwner,
        EnterParams params
        ) override;
    _Check_return_ HRESULT LeaveImpl(
        _In_ CDependencyObject* pNamescopeOwner,
        LeaveParams params
        ) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // The method to clean up all the device related resources like brushes, textures etc. on this element.
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    // Handle desired size changes in embedded UIElements.
    void OnChildDesiredSizeChanged(_In_ CUIElement* pElement) override;

    // Returns TextView for just this element that has no knowledge of linked layout.
    RichTextBlockView* GetSingleElementTextView() const { return m_pTextView; }

    // CRichTextBlockOverflow can have children - embedded elements, and needs to be a layout element in order to host them.
    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }

    _Check_return_ HRESULT GetActualWidth(_Out_ float* pWidth) override;
    _Check_return_ HRESULT GetActualHeight(_Out_ float* pHeight) override;

    // CUIElement override to update the gripper positions
    _Check_return_ HRESULT ArrangeCore(XRECTF finalRect) override;

    CRichTextBlock* GetMaster() const { return m_pMaster; }

    PageNode* GetPageNode() const { return m_pPageNode; }

    _Check_return_ HRESULT GetBaselineOffset(_Out_ float* pBaselineOffset);

    // ILinkedTextContainer public methods.
    RichTextServices::ILinkedTextContainer* GetPrevious() const override;
    RichTextServices::ILinkedTextContainer* GetNext() const override;
    RichTextServices::TextBreak* GetBreak() const override;
    bool IsBreakValid() const override { return m_isBreakValid; }
    RichTextServices::Result::Enum PreviousBreakUpdated(
        _In_ RichTextServices::ILinkedTextContainer* pPrevious) override;
    RichTextServices::Result::Enum PreviousLinkAttached(
        _In_ RichTextServices::ILinkedTextContainer* pPrevious) override;
    RichTextServices::Result::Enum NextLinkDetached(
        _In_ RichTextServices::ILinkedTextContainer* pNext) override;
    RichTextServices::Result::Enum PreviousLinkDetached(
        _In_ RichTextServices::ILinkedTextContainer* pPrevious) override;
    bool IsMaster() const override { return false; }

    // Focus and selection handling related overrides and helpers.
    bool IsFocusable() final
    {
        return IsActive() &&
            IsVisible() &&
            IsEnabled() &&
            AreAllAncestorsVisible();
    }
    _Check_return_ HRESULT UpdateFocusState(DirectUI::FocusState focusState) override
    {
        CValue value;
        value.Set(focusState);
        VERIFYHR(SetValueByIndex(KnownPropertyIndex::UIElement_FocusState, value));
        return S_OK;
    }

    // Selection changed notification.
    _Check_return_ HRESULT OnSelectionChanged(
        _In_ uint32_t previousSelectionStartOffset,
        _In_ uint32_t previousSelectionEndOffset,
        _In_ uint32_t newSelectionStartOffset,
        _In_ uint32_t newSelectionEndOffset);
    _Check_return_ HRESULT OnSelectionVisibilityChanged(
        _In_ uint32_t selectionStartOffset,
        _In_ uint32_t selectionEndOffset);

    // Static helpers for invalidating content and other information through the full chain of overflow links. Called by
    // the element where invalidation occurs and propagated down the chain of overflow elements non-recursively.
    static void InvalidateAllOverflowContent(_In_opt_ CRichTextBlockOverflow* pFirst, const bool clearCachedLinks);
    static void InvalidateAllOverflowContentMeasure(_In_opt_ CRichTextBlockOverflow* pFirst);
    static void InvalidateAllOverflowContentArrange(_In_opt_ CRichTextBlockOverflow* pFirst);
    static void InvalidateAllOverflowRender(_In_opt_ CRichTextBlockOverflow* pFirst);
    static void ResetAllOverflowMasters(_In_opt_ CRichTextBlockOverflow* pFirst);
    static _Check_return_ HRESULT NotifyAllOverflowContentSelectionChanged(
        _In_opt_ CRichTextBlockOverflow* pFirst,
        _In_ uint32_t previousSelectionStartOffset,
        _In_ uint32_t previousSelectionEndOffset,
        _In_ uint32_t newSelectionStartOffset,
        _In_ uint32_t newSelectionEndOffset
        );
    static _Check_return_ HRESULT NotifyAllOverflowContentSelectionVisibilityChanged(
        _In_opt_ CRichTextBlockOverflow *pFirst,
        _In_ uint32_t selectionStartOffset,
        _In_ uint32_t selectionEndOffset
        );

    // Selection and public API helpers, called through P/Invoke.

    _Check_return_ HRESULT CopySelectedText();
    _Check_return_ HRESULT GetContentStart(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);
    _Check_return_ HRESULT GetContentEnd(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);

    uint32_t GetContentStartPosition() const;
    uint32_t GetContentLength() const;

    bool HasOverflowContent() const { return (m_pBreak != nullptr); }
    static _Check_return_ HRESULT HasOverflowContent(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult);

    _Check_return_ HRESULT GetTextPositionFromPoint(
        _In_ XPOINTF point,
        _Outptr_ CTextPointerWrapper** ppTextPointerWrapper
        );

    _Check_return_ HRESULT GetTextElementBoundRect(
        _In_ CTextElement* pElement,
        _Out_ XRECTF* pRectFocus,
        _In_ bool ignoreClip
        );

    _Check_return_ HRESULT HitTestLink(
        _In_ XPOINTF* ptPointer,
        _Outptr_ CHyperlink** ppLink
        );

    XTHICKNESS GetPadding() const final { return m_padding; }

    xref_ptr<CFlyoutBase> GetContextFlyout() const override;

    _Check_return_ XUINT32 GetLinkAPChildrenCount() override;

protected:
    // Returns 'true' because the ArrangeOverride method always returns a size at least as large as its finalSize parameter
    // when the render size is smaller.
    bool IsFinalArrangeSizeMaximized() override { return true; }
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

private:
    // Fields unique to RichTextBlockOverflow
    PageNode* m_pPageNode;
    RichTextBlockView* m_pTextView;
    ILinkedTextContainer *m_pPreviousLink;
    // Fields common to RichTextBlock and RichTextBlockOverflow
    RichTextBlockBreak* m_pBreak;
    bool m_isBreakValid : 1;

    // Fields common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    bool m_redrawForArrange : 1;
    std::shared_ptr<HighlightRegion> m_selectionHighlight;
    xref_ptr<CHyperlink> m_pressedHyperlink;
    xref_ptr<CHyperlink> m_currentLink;
    CHyperlink* GetCurrentLinkNoRef() const;

    void ResetMaster();

    CAutomationPeer* OnCreateAutomationPeerImpl() override;

    // Ensure and Invalidates
    void InvalidateContent(const bool clearCachedLinks);
    void InvalidateContentMeasure();
    void InvalidateContentArrange();
    void InvalidateRender();
    void InvalidateSelectionRender();

    // Selection and Highlight helpers
    _Check_return_ HRESULT UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions);

    // LinkedTextContainer helpers.
    _Check_return_ HRESULT ResolveNextLink();
    _Check_return_ HRESULT InvalidateNextLink();
    _Check_return_ HRESULT SetBreak(_In_ RichTextBlockBreak* pBreak);
    bool IsPreviousBreakValid() const;
    RichTextBlockBreak* GetPreviousBreak() const;

    _Check_return_ HRESULT SetupLinkedBlockLayout();
    bool ValidateNextLink(_In_ RichTextServices::ILinkedTextContainer *pNewNext);

    // IsTextTrimmed helpers
    void RaiseIsTextTrimmedChangedEvent();
    _Check_return_ HRESULT UpdateIsTextTrimmed();

protected:
    // Since the CRichTextBlock element is a leaf and draws non-overlapping glyph runs,
    // opacity can be applied directly to its edges.
    _Check_return_ bool NWCanApplyOpacityWithoutLayer() override { return true; }

    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) override;

    _Check_return_ HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        ) override;

private:
    _Check_return_ HRESULT D2DPreChildrenRenderVirtual(
        _In_ const SharedRenderParams& sharedRP,
        _In_ const D2DRenderParams& d2dRP
        );

    _Check_return_ HRESULT D2DEnsureResources(
        _In_ const D2DPrecomputeParams &cp,
        _In_ const CMILMatrix* pMyAccumulatedTransform
        );

    CTextElement* GetTextElementForFocusRect();

    _Check_return_ HRESULT RenderHighlighterForegroundCallback(
        _In_ CSolidColorBrush* foregroundBrush,
        _In_ uint32_t highlightRectCount,
        _In_reads_(highlightRectCount) XRECTF* highlightRects
        );

public:
//-----------------------------------------------------------------------------
// Event Handlers from UIElement
//-----------------------------------------------------------------------------
    _Check_return_ HRESULT OnPointerMoved(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerExited(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerPressed(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnPointerReleased(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnGotFocus(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnLostFocus(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnHolding(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnRightTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnDoubleTapped(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnKeyUp(_In_ CEventArgs* pEventArgs) override;
    _Check_return_ HRESULT OnKeyDown(_In_ CEventArgs* pEventArgs) override;

    // Selection rendering.
    _Check_return_ HRESULT HWRenderSelection(
        _In_ IContentRenderer* pContentRenderer,
        _In_ bool isHighContrast,
        _In_ uint32_t highlightRectCount,
        _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects);

//-----------------------------------------------------------------------------
// PC Methods/Fields
//-----------------------------------------------------------------------------
    _Check_return_ HRESULT HWRenderContent(
        _In_ IContentRenderer* pContentRenderer
        );

    _Check_return_ HRESULT HWPostChildrenRender(
        _In_ IContentRenderer* pContentRenderer
        );

    void GetIndependentlyAnimatedBrushes(
        _Outptr_ CSolidColorBrush** ppFillBrush,
        _Outptr_ CSolidColorBrush** ppStrokeBrush
        ) override;
};
