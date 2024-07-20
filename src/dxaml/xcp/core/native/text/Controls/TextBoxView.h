// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichEditPal.h"

struct D2DRenderContext;
class CTextBoxBase;
class HWRenderTarget;
struct ID2D1RenderTarget;
struct IDWriteFontFace;
class CTransformToRoot;

inline bool
DoRectsIntersectOrTouch(
    _In_ const XRECT_RB &a,
    _In_ const XRECT_RB &b
    )
{
    return((a.left <= b.right) &&
           (a.top <= b.bottom) &&
           (a.right >= b.left) &&
           (a.bottom >= b.top));
}

    // Parameter type used by the Scroll method to specify the scroll unit and direction.
enum class CTextBoxView_ScrollCommand
{
    LineUp,
    LineDown,
    LineLeft,
    LineRight,
    PageUp,
    PageDown,
    PageLeft,
    PageRight,
    MouseWheelUp,
    MouseWheelDown,
    MouseWheelLeft,
    MouseWheelRight,
};

// IScrollInfo state, shared with DXaml framework through pinvokes.
struct CTextBoxView_ScrollData
{
    bool CanVerticallyScroll    : 1;
    bool CanHorizontallyScroll  : 1;
    XFLOAT ExtentWidth;
    XFLOAT ExtentHeight;
    XFLOAT ViewportWidth;
    XFLOAT ViewportHeight;
    XDOUBLE HorizontalOffset; // XDOUBLE because fx can set this and values must round-trip.
    XDOUBLE VerticalOffset;   // XDOUBLE because fx can set this and values must round-trip.
};

//---------------------------------------------------------------------------
//
//  The FrameworkElement responsible for measuring and rendering and scrolling
//  the content of a CTextBoxBase derived control.
//
//  This element builds off of Window's ITextServices, the windowless RichEdit
//  control.
//
//---------------------------------------------------------------------------
class CTextBoxView final : public CFrameworkElement, public IGripper
{
public:
    _Check_return_ HRESULT static Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    KnownTypeIndex GetTypeIndex() const override;
    bool GetIsLayoutElement() const final { return true; }
    bool IsRightToLeft() final;
    _Check_return_ HRESULT PullInheritedTextFormatting() override;
    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    // IGripper
    _Check_return_ HRESULT UpdateVisibility() override;

    // ITextHost
    bool TxCreateCaret(_In_ XHANDLE bitmap, _In_ XINT32 width, _In_ XINT32 height);
    bool TxSetCaretPos(_In_ XINT32 x, _In_ XINT32 y);
    HRESULT TxGetClientRect(_Out_ XRECT_RB *pClientRect);
    HRESULT TxGetViewportRect(_Out_ XRECT_RB *pViewportRect);
    HRESULT TxGetContentPadding(_Out_ XRECT_RB *pContentPadding);
    void TxInvalidateRect(_In_ const XRECT_RB *pRect, _In_ BOOL eraseBackground);
    _Check_return_ HRESULT TxGetCharFormat(_Outptr_ const WCHARFORMAT **ppCharFormat);
    _Check_return_ HRESULT TxGetParaFormat(_Outptr_ const XPARAFORMAT **ppParagraphFormat);

    // IProvideFontInfo overrides.
    WCHAR* GetDefaultFont();

    WCHAR* GetSerializableFontName(_In_ XUINT32 fontFaceId);

    XUINT32 GetRunFontFaceId(
        _In_z_ const WCHAR *pCurrentFontName,
        _In_ XUINT32 weight,
        _In_ XUINT32 stretch,
        _In_ XUINT32 style,
        _In_ XUINT32 lcid,
        _In_reads_opt_(charCount) const WCHAR *pText,
        _In_ XUINT32 charCount,
        _In_ XUINT32 fontFaceIdCurrent,
        _Out_ XUINT32 *pRunCount
        );

    IDWriteFontFace* GetFontFace(
        _In_ XINT32 fontId
        );

    // DXaml framework pinvokes.
    const CTextBoxView_ScrollData *GetScrollData() const;

    void SetScrollEnabled(
        _In_ bool canHorizontallyScroll,
        _In_ bool canVerticallyScroll
        );
    _Check_return_ HRESULT Scroll(
        _In_ CTextBoxView_ScrollCommand command,
        _In_ bool moveCaret,
        _In_ bool expandSelection,
        _In_ XUINT32 mouseWheelDelta,
        _Out_opt_ bool *pScrolled
        );
    _Check_return_ HRESULT SetScrollOffsets(
        _In_ XFLOAT horizontalOffset,
        _In_ XFLOAT verticalOffset
        );
    _Check_return_ HRESULT ScrollView(
        _In_ XFLOAT xDelta,
        _In_ XFLOAT yDelta);
    _Check_return_ HRESULT MakeVisible(
        const XRECTF& targetRect,
        double horizontalAlignmentRatio,
        double verticalAlignmentRatio,
        double horizontalOffset,
        double verticalOffset,
        _Out_opt_ XRECTF* visibleBounds,
        _Out_opt_ double* appliedOffsetX,
        _Out_opt_ double* appliedOffsetY);
    void OnDirectManipulationStarted();
    void OnDirectManipulationCompleted();
    void OnDirectManipulationDelta(
        XFLOAT xCumulativeTranslation,
        XFLOAT yCumulativeTranslation,
        XFLOAT zCumulativeFactor);

    _Check_return_ HRESULT GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset);

    _Check_return_ HRESULT SetOwnerTextControl(_In_ CTextBoxBase *pTextBox);
    _Check_return_ HRESULT RefreshDefaultFormat();

    void ClearOwnerTextControl();

    _Check_return_ HRESULT OnLostFocus();
    _Check_return_ HRESULT OnGotFocus();

    _Check_return_ HRESULT HwRender(_In_ IContentRenderer* contentRenderer);

    XRECTF GetCaretRect() const
    {
        return m_caretRect;
    }

    bool IsCaretInViewport() const;
    bool IsPixelSnapped() const { return m_pixelSnapped; }

    _Check_return_ bool TxShowCaret(_In_ BOOL isCaretVisible);
    _Check_return_ HRESULT ShowOrHideCaret();
    _Check_return_ HRESULT ShowCaretElement();
    _Check_return_ HRESULT HideCaretElement();

    _Check_return_ HRESULT UpdateDefaultParagraphFormat(
        _In_ XUINT32 mask
        );

    _Check_return_ HRESULT OnInheritedPropertyChanged();

    bool IsDMActive() const;

    void CaretChanged();
    void CaretVisibilityChanged();
    void InvalidateViewDispatcherCallback();
    void ShowGripperOnGotFocus(){ m_fShowGrippersOnGotFocus = true;}
    _Check_return_ HRESULT InvalidateView();
    bool IsRendering() const { return !!m_isRendering; };
    XRECTF GetViewportContentRect() const;
    XRECTF_RB GetViewportGlobalRect();

    float GetRightToLeftOffset();

    CShape* GetCaretElementNoRef()
    {
        return m_spCaretElement;
    }

    _Check_return_ HRESULT OnContextMenuOpen(_In_ bool isTouchInvoked);
    _Check_return_ HRESULT OnContextMenuDismiss();

    virtual bool ShouldRaiseEvent(_In_ const CDependencyProperty* pEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL)
    {
        // TODO: Only return TRUE for the events we implicitly handle (OnGotFocus, etc.).
        return true;
    }

    _Check_return_ HRESULT GripperPauseCaretBlink();
    _Check_return_ HRESULT GripperResumeCaretBlink();

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    // Queries the current caret rect and updates its cached position.
    _Check_return_ HRESULT UpdateCaretElement();

    _Check_return_ HRESULT OnBidiOptionsChanged();

    bool AreGrippersVisible();

    _Check_return_ HRESULT ShowSelectionGrippers(bool ignoreDrag = false);
    _Check_return_ HRESULT HideSelectionGrippers();

    UINT32 GetCurrentTextColor() const { return m_charFormat.TextColor; }

    CTransformToRoot* GetTransformToRoot() const { return m_pLastTransformToRoot; }

    _Check_return_ HRESULT GetSelectionEdgeRects(
        _Out_ XRECT_RB* pBeginRect,
        _Out_ XRECT_RB* pEndRect
    );

    _Check_return_ HRESULT GetSelectionRects(_Out_ std::vector<RECT>& rects);

protected:
     _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

     _Check_return_ HRESULT NotifyApplicationHighContrastAdjustmentChanged() override;

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    bool CanHaveChildren() const final
    {
        return true;
    }

    _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams
        ) override;

    _Check_return_ HRESULT MeasureOverride(
        XSIZEF availableSize,
        XSIZEF &desiredSize
        ) override;

    _Check_return_ HRESULT ArrangeOverride(
        XSIZEF finalSize,
        XSIZEF &newFinalSize
        ) override;

    _Check_return_ HRESULT MarkInheritedPropertyDirty(
        _In_ const CDependencyProperty* pdp,
        _In_ const CValue* pValue
        ) override;

    _Check_return_ HRESULT InvalidateFontSize() override;

    // Bounds and Hit Testing
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

    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

private:

    // Containing control.
    CTextBoxBase *m_pTextControl;

    // Render target used by RichEdit to draw its content.
    HWRenderTarget *m_pRenderTarget;

    // The caret rect supplied by RichEdit in client space coordinates.
    XRECTF m_caretRect;

    // The natural size of the content reported by RichEdit.
    XSIZEF m_requestedSize;

    // The invalidation rect supplied by RichEdit in client space coordinates.
    XRECT_RB m_invalidRect;

    // Extent, viewport, and scrollbar offsets shared with fx ScrollViewer.
    CTextBoxView_ScrollData m_scrollData;

    // Child UIElement used to render the caret.
    xref_ptr<CShape> m_spCaretElement;

    // Float/opacity animation used to blink the caret.
    xref_ptr<CStoryboard> m_spCaretBlink;

    // Caret blinking period.
    XFLOAT m_rCaretBlinkingPeriod;

    // Caret blink timeout timer
    xref_ptr<CDispatcherTimer> m_spCaretTimer;
    xref_ptr<CDispatcherTimer> m_spCaretStartTimer;

    // Horizontal/Vertical offset set by DM related action.
    XPOINTF m_directManipulationOffset;

    // Default character formatting for ITextServices.
    WCHARFORMAT2 m_charFormat;

    // Default paragraph formatting for ITextServices.
    XPARAFORMAT2 m_paragraphFormat;

    CTransformToRoot *m_pLastTransformToRoot;
    XPOINTF m_lastViewportOffset;
    XRECT m_lastSelectionRect;

    BYTE m_foregroundAlpha;

    // Default color set to white in order to get the destination invert composition mode behavior.
    static const XUINT32 DefaultCaretColor = 0xffffffff;

    DirectUI::ElementHighContrastAdjustmentConsecutive      m_highContrastAdjustment : 2;
    DirectUI::ApplicationHighContrastAdjustmentConsecutive  m_applicationHighContrastAdjustment : 1;

    bool m_forceRedraw                 : 1;
    bool m_canShowCaret                : 1;
    bool m_fInheritedPropertiesDirty   : 1;
    bool m_fShowGrippersOnAnimationComplete : 1;
    bool m_fShowGrippersOnGotFocus     : 1;
    bool m_fShowGrippersOnSelectionChanged : 1;
    bool m_isDMActive                  : 1;
    bool m_isMeasuring                 : 1;
    bool m_isRendering                 : 1;
    bool m_resetCharFormatingMask      : 1;
    bool m_resetParaFormatingMask      : 1;
    bool m_showGrippersOnCMDismiss     : 1;
    bool m_issueDispatcherViewInvalidation : 1;
    bool m_fBlinkPausedForGripper      : 1;
    bool m_shouldShowRTLIndicator      : 1;
    bool m_pixelSnapped                : 1;
    bool m_fLockSetScrollOffsets       : 1;
    bool m_fCaretTimeoutEnabled        : 1; // Caret timeout flag
    bool m_fScrollOffsetsChangedByDraw : 1;

    CTextBoxView(_In_ CCoreServices *pCore);

    ~CTextBoxView() override;

    // Coerces a scrollbar offset to fit within the current viewport and extent limits.
    static XFLOAT NormalizeScrollOffset(
        _In_ XFLOAT offset,
        _In_ XFLOAT viewportLength,
        _In_ XFLOAT extent
        );

    // Gets a new scrollbar offset intended to move a segment into view.
    static XFLOAT CalculateSegmentVisibleScrollOffset(
        _In_ XFLOAT segmentOffset,
        _In_ XFLOAT segmentLength,
        _In_ XFLOAT currentScrollOffset,
        _In_ XFLOAT viewportLength
        );

    // Adds the new extent regions to the invalid render rect.
    void InvalidateExtentAdditions(_In_ const XSIZEF &newExtent);

    _Check_return_ HRESULT SetScrollDataOffsets(XDOUBLE horizontalOffset, XDOUBLE verticalOffset, bool forceInvalidScrollInfo = true);
    _Check_return_ HRESULT GetShouldRenderCaret(_Out_ bool *pShouldRenderCaret);

    bool ShouldShowRTLIndicator();
    float GetRTLIndicatorSize();
    _Check_return_ HRESULT GenerateRTLCaret(XFLOAT width, XFLOAT height);
    _Check_return_ HRESULT AddLineSegmentToSegmentCollection(_In_ CCoreServices *pCore, _In_ CPathSegmentCollection *pSegments, _In_ XPOINTF point);
    void ResetCaretElement();
    void EnsureCaretElement();
    void CreateCaretShape();
    _Check_return_ HRESULT SetupCaretBlinkAnimation();
    _Check_return_ HRESULT ResetCaretBlink();
    _Check_return_ HRESULT PauseCaretBlink();
    _Check_return_ HRESULT ResumeCaretBlink();
    XFLOAT CheckCaretBlinkTimeout();

    static _Check_return_ HRESULT OnCaretTimeout(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        );

    static _Check_return_ HRESULT OnCaretStart(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
    );

    XFLOAT GetCaretOpacity() const;

    _Check_return_ HRESULT D2DRenderCommon(
        _In_ const SharedRenderParams &sharedRP,
        _In_ const D2DRenderParams &d2dRP,
        _In_ ID2D1RenderTarget* pRenderTarget
        );

    void InitializeDefaultCharFormat();
    void InitializeDefaultParagraphFormat();
    _Check_return_ HRESULT UpdateDefaultCharFormat(_In_ XUINT32 mask);
    static XINT16 TextAlignmentToParagraphAlignment(
        _In_ DirectUI::TextAlignment alignment,
        _In_ DirectUI::FlowDirection flowDirection
        );

    _Check_return_ HRESULT GetFontFaceRun(
        _In_ CCompositeFontFamily *pCompositeFontFamily,
        _In_ XUINT32 weight,
        _In_ XUINT32 stretch,
        _In_ XUINT32 style,
        _In_reads_(charCount) const WCHAR *pText,
        _In_ XUINT32 charCount,
        _Out_ XUINT32 *pFontId,
        _Out_ XUINT32 *pRunCount
    );

    _Check_return_ HRESULT GetSystemFontCollection(
        _Outptr_ CTypefaceCollection **ppFontCollection
        );

    _Check_return_ HRESULT GetWinTextCore(
        _Outptr_ WinTextCore **ppWinTextCore
        );

    _Check_return_ HRESULT GetSelectionRect(
        _Out_ XRECT *pSelectionRect
        );

    _Check_return_ HRESULT IsSelectionEdgeVisible(
        _Out_ bool &fVisible
        );

   void GetViewportAndViewportTransform(
       _Out_ XRECTF_RB *pViewport,
       _Out_ CMILMatrix *pViewportTransform
       );

   _Check_return_ HRESULT HWNWRenderCommon(
       _In_ IContentRenderer* contentRenderer,
       XRECT_RB& contentBounds,
       XRECT_RB& updateBounds);

   _Check_return_ HRESULT GetTextNaturalSize(
       XSIZEF availableSize,
       XSIZEF& naturalSize);

   bool IsHighContrastAdjustmentActive() const;

   bool IsHighContrastAdjustmentEnabled() const;

   _Check_return_ HRESULT UpdateHighContrastAdjustments();

   _Check_return_ HRESULT ChangePlaceholderBackPlateVisibility(bool visible);

   _Check_return_ HRESULT OnHighContrastAdjustmentChanged(_In_ DirectUI::ElementHighContrastAdjustment newValue);
   _Check_return_ HRESULT OnApplicationHighContrastAdjustmentChanged(_In_ DirectUI::ApplicationHighContrastAdjustment newValue);
   _Check_return_ HRESULT CalculateBaseLine(_In_ const wrl::ComPtr<ITextRange2>& spRange, _Out_ UINT32& lineHeight, _Out_ UINT32& baselineY);
   _Check_return_ HRESULT DrawBaseLine(_In_ const XRECT_RB& updateBounds, _In_ UINT32 crText);
};

inline bool CTextBoxView::IsDMActive() const
{
    return m_isDMActive;
}
