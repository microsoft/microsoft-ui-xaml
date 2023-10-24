// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FontAndScriptServices.h"
#include "FontFamily.h"
#include "inline.h"
#include "AlphaMask.h"
#include "GraphicsUtility.h"

class BlockNode;
class IContentRenderer;
class BlockLayoutEngine;
class TextSelectionManager;
struct ITextContainer;
class CHyperlink;
class CInputPointEventArgs;
class CFocusRectangle;
class TextBlockView;

class CTextHighlighterCollection;
#include "TextHighlightMerge.h"

interface IDWriteTextLayout;
interface IDWriteTextFormat;
interface IDWriteFactory;
class DWriteFontAndScriptServices;
class D2DTextDrawingContext;
enum DWRITE_READING_DIRECTION;
enum DWRITE_TEXT_ALIGNMENT;
struct DWRITE_TEXT_METRICS;

namespace RichTextServices
{
    class TextFormatter;
}

enum class FastPathOptOutConditions
{
    CharacterSpacing_Not_Default = 0x01,
    TextTrimming_Is_Clip = 0x02,
    HasComplexContent = 0x04,
};

enum class TextMode : uint32_t
{
    Normal = 0,
    DWriteLayout = 1,
};

//------------------------------------------------------------------------
//
//  Class:  CTextBlock
//
//  Synopsis:
//
//      Object created for <TextBlock> tag.
//
//      The CTextBlock object encapsulates all that is required to layout and
//      display one or more lines of text with simple paragraph semantics.
//
//      The CTextBlock allows nested runs of characters in differing styles.
//
//------------------------------------------------------------------------

class CTextBlock final : public CFrameworkElement
{
    friend class JupiterTextHelper;

private:
    CTextBlock(_In_ CCoreServices* pCore);
    ~CTextBlock() override;

public:

    DECLARE_CREATE(CTextBlock);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextBlock>::Index;
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

    void ClearPCRenderData() override;

    _Check_return_ HRESULT AddText(
        _In_ const xstring_ptr& strCharacters,
        _In_ RunFlags flags
        );

    bool GetUseLayoutRounding() const final;

    _Check_return_ HRESULT TextRangeAdapterAssociated();
    _Check_return_ HRESULT NeedNumberSubsitution(_Out_ BOOL* pIsNeeded);
    _Check_return_ HRESULT GetDWriteFontAndScriptServices(_Outptr_ DWriteFontAndScriptServices** ppDWriteFontServices);

    // Pulls all non-locally set inherited properties from the parent.
    _Check_return_ HRESULT PullInheritedTextFormatting() override;

    // CTextBlock can selection grippers as children.
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

    ITextContainer* GetTextContainer() const
    {
        if (m_pInlines != nullptr)
        {
            return m_pInlines->GetTextContainer();
        }
        return nullptr;
    }
    ITextView* GetTextView() const;

    BlockNode* GetPageNode() const { return m_pPageNode; }

    CFontContext* GetFontContext() const { return m_pFontContext; }
    _Check_return_ HRESULT GetBaselineOffset(_Out_ float* pBaselineOffset);

    // Called by InlineCollection when it is dirtied.
    _Check_return_ HRESULT OnContentChanged(_In_opt_ const CDependencyProperty* pdp);

    // Focus and selection handling related overrides and helpers.
    bool IsFocusable() final
    {
        return IsActive() &&
            IsVisible() &&
            IsEnabled() && (m_isTextSelectionEnabled || IsTabStop()) &&
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
        uint32_t previousSelectionStartOffset,
        uint32_t previousSelectionEndOffset,
        uint32_t newSelectionStartOffset,
        uint32_t newSelectionEndOffset);
    _Check_return_ HRESULT OnSelectionVisibilityChanged(
        uint32_t selectionStartOffset,
        uint32_t selectionEndOffset);
    // Selection / TextPointer Public API methods.
    _Check_return_ HRESULT SelectAll();
    TextSelectionManager* GetSelectionManager();
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

    void RecursiveInvalidateMeasure() override;
    void InvalidateRender();
    void InvalidateContentMeasure();
    void InvalidateContent();
    bool BaseRealizationHasSubPixelOffsets() const;
    void ClearBaseRealization();

    // Hyperlink handling.
    _Check_return_ HRESULT GetFocusableChildren(
        _Outptr_result_maybenull_ CDOCollection** ppFocusableChildren
        );

    CInlineCollection* GetInlineCollection() const { return m_pInlines; }

    _Check_return_ HRESULT GetTextElementBoundRect(
        _In_ CTextElement* pElement,
        _Out_ XRECTF* pRectFocus,
        _In_ bool ignoreClip
        );

    _Check_return_ HRESULT HitTestLink(
        _In_ XPOINTF* ptPointer,
        _Outptr_ CHyperlink** ppLink
        );

    xref_ptr<CSolidColorBrush> GetSelectionHighlightColor() const;

    XTHICKNESS GetPadding() const final { return m_padding; }

    bool IsSelectionEnabled() const;
    CFlyoutBase* GetSelectionFlyoutNoRef() const;

    _Check_return_ HRESULT OnPointerCaptureLost(_In_ CEventArgs* pEventArgs) final;

    _Check_return_ HRESULT GetDWriteTextLayout(_Out_ IDWriteTextLayout** textLayout) const;
    TextMode GetTextMode() const { return m_textMode; }
    const xstring_ptr GetText() const { return m_strText; }
    _Check_return_ HRESULT GetFlowDirection(_Out_ DirectUI::FlowDirection* flowDirection);
    _Check_return_ HRESULT GetContentRenderTransform(_Out_ CMILMatrix* pContentRenderTransform);

    _Check_return_ HRESULT NotifyRenderContent(
        HWRenderVisibility visibility);

    _Check_return_ HRESULT GetAlphaMask(
        _Outptr_ WUComp::ICompositionBrush** ppReturnValue);

    void SetBackPlateForegroundOverride(bool shouldOverride);
    void SetUseHyperlinkForegroundOnBackPlate(bool useHyperlinkForegroundOnBackPlate);
    void SetBackPlateVisibility(bool visible);

    _Check_return_ HRESULT FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const wf::Point& point);

    _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

protected:
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;
    _Check_return_ HRESULT OnPropertySetImpl(
        _In_ const CDependencyProperty* pDP,
        _In_ const CValue& oldValue,
        _In_ const CValue& value,
        bool propertyChangedValue) override;

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
    CHyperlink* GetCurrentLinkNoRef() const;
    xref_ptr<CHyperlink> GetPressedHyperlink() const;

    // Determine the layout mode, either Normal(LS) or DWriteTextLayout.
    _Check_return_ HRESULT ResolveTextMode();

    _Check_return_ HRESULT DeferredCreateInlineCollection();

    XPOINTF GetContentRenderingOffset(DirectUI::FlowDirection flowDirection) const;

    _Check_return_ HRESULT GetLineStackingOffset(uint32_t lineCount, _Out_ float* offset);

    void SetFastPathOptOutConditions (FastPathOptOutConditions mask);
    void SetFastPathOptOutConditions (FastPathOptOutConditions mask, bool isSet);
    void ClearFastPathOptOutConditions (FastPathOptOutConditions mask);
    _Check_return_ HRESULT UpdateFastPathOptOutConditions(_In_ const CDependencyProperty* pDP);

    _Check_return_ HRESULT EnsureTextDrawingContext();

    _Check_return_ HRESULT SetTextToInlineCollection();

    _Check_return_ HRESULT ConfigureDWriteTextLayout(
        const XSIZEF availableSize,
        const float baseline,
        const float lineAdvance) noexcept;
    _Check_return_ HRESULT GetDWriteTextMetricsOffset(_Out_ XPOINTF* offset);
    _Check_return_ HRESULT GetLineHeight(_Out_ float* baseline, _Out_ float* lineAdvance);

    _Check_return_ HRESULT DetermineTextReadingOrderAndAlignment(
        _In_ IDWriteFactory* pDWriteFactory,
        _In_ const TextFormatting* pTextFormatting,
        _Out_ DWRITE_READING_DIRECTION* dwriteReadingDirection,
        _Out_ DWRITE_TEXT_ALIGNMENT* dwriteTextAlignment
        );

    void EnsureTextBlockView();
    _Check_return_ HRESULT EnsureBackPlateDependencies();
    _Check_return_ HRESULT EnsureFontContext();
    _Check_return_ HRESULT EnsureBlockLayout();
    _Check_return_ HRESULT EnsureTextSelectionManager();
    _Check_return_ HRESULT CreateInlines();

    _Check_return_ HRESULT InvalidateFontSize() override;
    void InvalidateContentArrange();

    // Selection and Highlight helpers
    _Check_return_ HRESULT CreateTextHighlighters();
    _Check_return_ HRESULT CreateSelectionHighlightColor();
    void UpdateSelectionHighlightColor();
    _Check_return_ HRESULT UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions);
    _Check_return_ HRESULT OnSelectionEnabledChanged(bool oldValue);
    _Check_return_ HRESULT RaiseSelectionChangedEvent();

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
    _Check_return_ HRESULT UpdateIsTextTrimmedFastPath(_In_opt_ DWRITE_TEXT_METRICS* m);
    _Check_return_ HRESULT UpdateIsTextTrimmedSlowPath();

    _Check_return_ HRESULT RedrawText();

    _Check_return_ XUINT32 GetLinkAPChildrenCount() override;

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

    _Check_return_ HRESULT GetTightGlobalBounds(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping = false,
        _In_ bool useTargetInformation = false
        ) override;

    _Check_return_ HRESULT GetTightInnerBounds(
        _Out_ XRECTF_RB* pBounds
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

protected:
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

public:

    // Selection rendering.
    _Check_return_ HRESULT HWRenderSelection(
        _In_ IContentRenderer* pContentRenderer,
        _In_ bool isHighContrast,
        _In_ uint32_t highlightRectCount,
        _In_reads_opt_(highlightRectCount) XRECTF* pHighlightRects,
        _In_ bool isBackPlate);

    _Check_return_ HRESULT RenderBackplateIfRequired(_In_opt_ IContentRenderer* pContentRenderer);

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

public:
    // unique to TextBlock
    CInlineCollection*                          m_pInlines;

    // common to TextBlock and RichTextBlock
    CTextHighlighterCollection*                 m_textHighlighters = nullptr;

    // unique to TextBlock
    xstring_ptr                                 m_strText;

private:
    // unique to TextBlock
    BlockNode*                                  m_pPageNode;
    TextBlockView*                              m_pTextView;
    wrl::ComPtr<IDWriteTextLayout>              m_pTextLayout;
    xref_ptr<D2DTextDrawingContext>             m_pTextDrawingContext;
    AlphaMask                                   m_alphaMask;
    CFontContext*                               m_pFontContext;
    BlockLayoutEngine*                          m_pBlockLayout;

    // common to TextBlock and RichTextBlock
    TextSelectionManager*                       m_pSelectionManager;
    xref_ptr<CDOCollection>                     m_focusableChildrenCollection;

    // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    std::shared_ptr<HighlightRegion>            m_selectionHighlight;
    xref_ptr<CHyperlink>                        m_pressedHyperlink;
    xref_ptr<CHyperlink>                        m_currentLink;

public:
    // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    std::vector<HighlightRegion>                m_highlightRegions;
    XTHICKNESS                                  m_padding = {};
    uint32_t                                    m_maxLines;

    // common to TextBlock and RichTextBlock
    float                                       m_eLineHeight;

    // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    float                                       m_layoutRoundingHeightAdjustment;

private:
    // unique to TextBlock
    uint32_t                                    m_isDWriteTextLayoutDirty                           : 1;
    uint32_t                                    m_shouldAddTextOnDeferredInlineCollectionCreation   : 1;

    // unique to TextBlock
    TextMode                                    m_textMode                                          : 1;

    // Determines if a BackPlate must be drawn in HighContrast. There are controls that benefit from not drawing a BackPlate in their TextBlocks.
    // unique to TextBlock
    uint32_t                                    m_drawBackPlate                                     : 1;

    // Internal  flags used to record properties that prevents using fast-fast path with IDWriteTextLayout.
    // unique to TextBlock
    uint32_t                                    m_fFastPathOptOutConditions                         : 10;

    // unique to TextBlock
    uint32_t                                    m_hasBeenMeasured                                   : 1;

    // Determines if the control has to override the foreground to the hyperlink foreground brush when HighContrastAdjustment is enabled.
    // unique to TextBlock
    uint32_t                                    m_useHyperlinkForegroundOnBackPlate                 : 1;

// ApplicationHighContrastAdjustment is an enum with 2 values: 0 and 0xffffffff
// We store this enum as 1 bits in CTextBlock. In order to do that we map ApplicationHighContrastAdjustment to ApplicationHighContrastAdjustmentConsecutive
private: DirectUI::ApplicationHighContrastAdjustmentConsecutive m_applicationHighContrastAdjustment : 1; /* type: DirectUI::ApplicationHighContrastAdjustment */ // common to TextBlock and RichTextBlock

// ElementHighContrastAdjustment is an enum with 3 values: 0, 0x80000000 and 0xffffffff
// We store this enum as 2 bits in CTextBlock. In order to do that we map ElementHighContrastAdjustment to ElementHighContrastAdjustmentConsecutive
private: DirectUI::ElementHighContrastAdjustmentConsecutive m_highContrastAdjustment : 2; /* type: DirectUI::ElementHighContrastAdjustment */ // common to TextBlock and RichTextBlock

private: bool m_redrawForArrange : 1; // commonTo TextBlock, RichTextBlock, and RichTextBlockOverflow

public:
    // common to TextBlock and RichTextBlock
    DirectUI::TextWrapping                      m_textWrapping;
    DirectUI::TextTrimming                      m_textTrimming;
    DirectUI::TextAlignment                     m_textAlignment;
    DirectUI::TextLineBounds                    m_textLineBounds;
    DirectUI::LineStackingStrategy              m_lineStackingStrategy;
    DirectUI::OpticalMarginAlignment            m_opticalMarginAlignment;
    DirectUI::TextReadingOrder                  m_textReadingOrder;

public:
    // common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    bool                                        m_isTextTrimmed = false;

    // common to TextBlock and RichTextBlock
    bool                                        m_isTextSelectionEnabled;
    bool                                        m_isColorFontEnabled;

};
