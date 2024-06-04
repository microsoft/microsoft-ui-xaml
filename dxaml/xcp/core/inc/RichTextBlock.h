// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FontAndScriptServices.h"
#include "FontFamily.h"

class CRichTextBlockOverflow;
class PageNode;
class RichTextBlockBreak;
class IContentRenderer;
class BlockLayoutEngine;
class TextSelectionManager;
struct ITextContainer;
class CHyperlink;
class CInputPointEventArgs;
class CFocusRectangle;
class RichTextBlockView;
class LinkedRichTextBlockView;

#include "TextFormatter.h"
#include "TextView.h"
#include "BlockTextElement.h"
#include "ILinkedTextContainer.h"
class CTextHighlighterCollection;
#include "TextHighlightMerge.h"

class CRichTextBlock final : public CFrameworkElement,
                      public RichTextServices::ILinkedTextContainer
{
    friend class JupiterTextHelper;

private:
    CRichTextBlock(_In_ CCoreServices* pCore);
    ~CRichTextBlock() override;

public:
    // Public properties

    CBlockCollection*                m_pBlocks;                     // unique to RichTextBlock
    IFontCollection*                 m_pFontCollection;             // unique to RichTextBlock
    CSolidColorBrush*                m_pSelectionHighlightColor;    // unique to RichTextBlock
    CTextHighlighterCollection*      m_textHighlighters = nullptr;  // common to TextBlock and RichTextBlock
    CRichTextBlockOverflow*          m_pOverflowTarget;             // common to RichTextBlock and RichTextBlockOverflow

    float                            m_textIndent;                  // unique to RichTextBlock
    DirectUI::TextWrapping           m_textWrapping;                // common to TextBlock and RichTextBlock
    DirectUI::TextTrimming           m_textTrimming;                // common to TextBlock and RichTextBlock
    DirectUI::TextAlignment          m_textAlignment;               // common to TextBlock and RichTextBlock
    DirectUI::TextLineBounds         m_textLineBounds;              // common to TextBlock and RichTextBlock

    float                            m_eLineHeight;                 // common to TextBlock and RichTextBlock
    DirectUI::LineStackingStrategy   m_lineStackingStrategy;        // common to TextBlock and RichTextBlock
    DirectUI::OpticalMarginAlignment m_opticalMarginAlignment;      // common to TextBlock and RichTextBlock
    DirectUI::TextReadingOrder       m_textReadingOrder;            // common to TextBlock and RichTextBlock
    bool                             m_isTextSelectionEnabled;      // common to TextBlock and RichTextBlock

    XTHICKNESS                       m_padding = {};                // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    uint32_t                         m_maxLines;                    // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    float                            m_layoutRoundingHeightAdjustment;  // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    bool                             m_isTextTrimmed = false;       // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    bool                             m_isColorFontEnabled;          // common to TextBlock and RichTextBlock

private:
    DirectUI::ElementHighContrastAdjustmentConsecutive m_highContrastAdjustment : 2;
    DirectUI::ApplicationHighContrastAdjustmentConsecutive m_applicationHighContrastAdjustment : 1;
    bool m_isBreakValid : 1; // common to RichTextBlock and RichTextBlockOverflow
    bool m_redrawForArrange : 1; // commonTo TextBlock, RichTextBlock, and RichTextBlockOverflow
    bool : 3; // unused, padding

public:
    std::vector<HighlightRegion>     m_highlightRegions;            // common to TextBlock, RichTextBlock, and RichTextBlockOverflow

    DECLARE_CREATE(CRichTextBlock);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRichTextBlock>::Index;
    }

    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject* pNamescopeOwner,
        EnterParams params
        ) override;
    _Check_return_ HRESULT LeaveImpl(
        _In_ CDependencyObject* pNamescopeOwner,
        LeaveParams params
        ) override;

    _Check_return_ HRESULT GetValue(
        _In_  const CDependencyProperty* pdp,
        _Out_ CValue* pValue
        ) override;
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // The method to clean up all the device related resources like brushes, textures etc. on this element.
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    // Handle desired size changes in embedded UIElements.
    void OnChildDesiredSizeChanged(_In_ CUIElement* pElement) override;
    // Pulls all non-locally set inherited properties from the parent.
    _Check_return_ HRESULT PullInheritedTextFormatting() override;

    // Returns TextView for just this element that has no knowledge of linked layout.
    RichTextBlockView* GetSingleElementTextView() const { return m_pTextView; }

    // CRichTextBlock can have children - embedded elements, and needs to be a layout element in order to host them.
    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }

    _Check_return_ HRESULT GetActualWidth(_Out_ float* pWidth) override;
    _Check_return_ HRESULT GetActualHeight(_Out_ float* pHeight) override;

    // CUIElement override to update the gripper positions
    _Check_return_ HRESULT ArrangeCore(XRECTF finalRect) override;

    // Get TextFormatter instance.
    _Check_return_ HRESULT GetTextFormatter(
        _Outptr_ RichTextServices::TextFormatter** ppTextFormatter
        );
    // Release TextFormatter instance.
    void ReleaseTextFormatter(
        _In_ RichTextServices::TextFormatter* pTextFormatter
        );

    ITextContainer* GetTextContainer()
    {
        if (m_pBlocks != nullptr)
        {
            return m_pBlocks->GetTextContainer();
        }
        return nullptr;
    }
    // Returns the ITextView that this object exposes publicly for querying. This is the standalone TextView if there are
    // no overflow targets, or the linked view if this is the master in a linked chain.
    ITextView* GetTextView() const;

    PageNode* GetPageNode() const { return m_pPageNode; }

    BlockLayoutEngine* GetBlockLayoutEngine() const { return m_pBlockLayout; }
    CFontContext* GetFontContext() const { return m_pFontContext; }
    _Check_return_ HRESULT GetBaselineOffset(_Out_ float* pBaselineOffset);

    // Called by BlockCollection when it is dirtied.
    _Check_return_ HRESULT OnContentChanged(_In_opt_ const CDependencyProperty* pdp);

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
    bool IsMaster() const override { return true; }

    HRESULT OnPointerCaptureLost(_In_ CEventArgs* pEventArgs) final;

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

    // Selection changed notification called by TextSelectionManager when selection has changed.
    _Check_return_ HRESULT OnSelectionChanged(
        _In_ uint32_t previousSelectionStartOffset,
        _In_ uint32_t previousSelectionEndOffset,
        _In_ uint32_t newSelectionStartOffset,
        _In_ uint32_t newSelectionEndOffset);
    _Check_return_ HRESULT OnSelectionVisibilityChanged(
        _In_ uint32_t selectionStartOffset,
        _In_ uint32_t selectionEndOffset);

    // Static automation peer helpers.
    static _Check_return_ HRESULT GetPlainText(
        _In_ CFrameworkElement* pElement,
        _Out_ xstring_ptr* pstrText
        );

    // Selection and other Public API methods called through P/Invoke.
    _Check_return_ HRESULT SelectAll();
    TextSelectionManager* GetSelectionManager() const { return m_pSelectionManager; }
    static _Check_return_ HRESULT GetSelectedText(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult);

    _Check_return_ HRESULT CopySelectedText();
    _Check_return_ HRESULT GetContentStart(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);
    _Check_return_ HRESULT GetContentEnd(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);
    _Check_return_ HRESULT GetSelectionStart(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);
    _Check_return_ HRESULT GetSelectionEnd(_Outptr_ CTextPointerWrapper** ppTextPointerWrapper);
    _Check_return_ HRESULT Select(
        _In_ CTextPointerWrapper* pAnchorPosition,
        _In_ CTextPointerWrapper* pMovingPosition
        );

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

    void RecursiveInvalidateMeasure() override;
    void InvalidateRender();
    void InvalidateContentMeasure();
    void InvalidateContent(const bool clearCachedLinks = true);

    void Shutdown() override;

    // Hyperlink handling.
    _Check_return_ HRESULT GetFocusableChildren(
        _Outptr_result_maybenull_ CDOCollection** ppFocusableChildren
        );

    // Returns the global Blocks collections containing all the paragraphs.
    CBlockCollection* GetBlockCollection() const { return m_pBlocks; }

    _Check_return_ HRESULT GetTextElementBoundRect(
        _In_ CTextElement* pElement,
        _Out_ XRECTF* pRectFocus,
        _In_ bool ignoreClip
        );

    _Check_return_ HRESULT FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const wf::Point& point);

    _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

    static _Check_return_ HRESULT GetTextElementBoundRect(
        _In_ CTextElement* pElement,
        _In_ ITextView* pTextView,
        _In_ CUIElement* pTextControl,
        _Out_ XRECTF* pRectFocus,
        _In_ bool ignoreClip
        );

    _Check_return_ HRESULT HitTestLink(
        _In_ XPOINTF* ptPointer,
        _Outptr_ CHyperlink** ppLink
        );

    static _Check_return_ HRESULT HitTestLink(
        _In_ ITextContainer* pTextContainer,
        _In_ CUIElement* pSender,
        _In_ XPOINTF* ptPointer,
        _In_ ITextView* pTextView,
        _Outptr_ CHyperlink** ppLink
        );
    static _Check_return_ HRESULT HWRenderFocusRects(
        _In_ IContentRenderer* pContentRenderer,
        _In_ CTextElement* pFocusedElement,
        _In_ ITextView* pTextView
        );

    XTHICKNESS GetPadding() const final { return m_padding; }

    bool IsSelectionEnabled() const;
    CFlyoutBase* GetSelectionFlyoutNoRef() const;

    _Check_return_ XUINT32 GetLinkAPChildrenCount() override;
    static _Check_return_ XUINT32 GetLinkAPChildrenCountHelper(_In_opt_ CInlineCollection* inlines);

protected:
    // Returns 'true' because the ArrangeOverride method always returns a size at least as large as its finalSize parameter
    // when the render size is smaller.
    bool IsFinalArrangeSizeMaximized() override { return true; }
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;
    _Check_return_ HRESULT MarkInheritedPropertyDirty(
        _In_ const CDependencyProperty* pdp,
        _In_ const CValue* pValue) override;

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;
    _Check_return_ HRESULT NotifyApplicationHighContrastAdjustmentChanged() override;

private:
    // Fields unique to RichTextBlock
    PageNode* m_pPageNode;
    RichTextBlockView* m_pTextView;
    RichTextServices::TextFormatter* m_pTextFormatter;
    LinkedRichTextBlockView* m_pLinkedView;

    // Fields common to TextBlock and RichTextBlock
    CFontContext* m_pFontContext;
    BlockLayoutEngine* m_pBlockLayout;
    TextSelectionManager* m_pSelectionManager;
    xref_ptr<CDOCollection> m_focusableChildrenCollection;

    // Fields common to RichTextBlock and RichTextBlockOverflow
    RichTextBlockBreak* m_pBreak;

    // Fields common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    std::shared_ptr<HighlightRegion> m_selectionHighlight;
    xref_ptr<CHyperlink> m_pressedHyperlink;
    xref_ptr<CHyperlink> m_currentLink;
    CHyperlink* GetCurrentLinkNoRef() const;

    // Ensure and Invalidates
    _Check_return_ HRESULT EnsureBackPlateDependencies();
    _Check_return_ HRESULT EnsureFontContext();
    _Check_return_ HRESULT EnsureBlockLayout();
    _Check_return_ HRESULT CreateTextSelectionManager();
    _Check_return_ HRESULT CreateBlocks();

    _Check_return_ HRESULT InvalidateFontSize() override;
    void InvalidateContentArrange();
    void InvalidateSelectionRender();

    CAutomationPeer* OnCreateAutomationPeerImpl() override;

    // Selection and Highlight helpers
    _Check_return_ HRESULT CreateTextHighlighters();
    _Check_return_ HRESULT CreateSelectionHighlightColor();
    void UpdateSelectionHighlightColor();
    _Check_return_ HRESULT UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions);
    _Check_return_ HRESULT OnSelectionEnabledChanged(bool oldValue);
    _Check_return_ HRESULT RaiseSelectionChangedEvent();

    // LinkedTextContainer helpers.
    _Check_return_ HRESULT ResolveNextLink();
    _Check_return_ HRESULT InvalidateNextLink();
    _Check_return_ HRESULT SetBreak(_In_ RichTextBlockBreak* pBreak);

    // High contrast helpers
    _Check_return_ HRESULT OnHighContrastAdjustmentChanged(
        _In_ DirectUI::ElementHighContrastAdjustment newValue
        );
    _Check_return_ HRESULT OnApplicationHighContrastAdjustmentChanged(
        _In_ DirectUI::ApplicationHighContrastAdjustment newValue
        );
    _Check_return_ HRESULT UpdateHighContrastAdjustments();
    bool IsHighContrastAdjustmentActive() const;
    bool IsHighContrastAdjustmentEnabled() const;
    void UpdateBackPlateForegroundOverride();

    static _Check_return_ HRESULT GetFocusableChildrenHelper(
        _In_ CDOCollection* pFocusChildren,
        _In_ CTextElementCollection* pCollection);

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
        _In_reads_opt_(highlightRectCount) XRECTF* pHighlightRects,
        _In_ bool isBackPlate);


//-----------------------------------------------------------------------------
// PC Methods/Fields
//-----------------------------------------------------------------------------
    // Renders content in PC rendering walk.
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
