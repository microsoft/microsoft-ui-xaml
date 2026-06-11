// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class BlockNodeBreak;
class CTextSelectionGripper;
class CUIElement;
class CPopup;
class CPointerEventArgs;
class CInputPointEventArgs;
enum class FindBoundaryType;
enum class TagConversion;

#define SNAPPING_ASSERT ASSERT
#include "TouchSelectionSnappingCalculator.h"
#include <TextSelectionNotify.h>
#include "timer.h"
#include "CaretBrowsingCaret.h"

//---------------------------------------------------------------------------
//
//  TextSelectionManager
//
//  Performs basic selection services for CRichTextBlock and
//  CRichTextBlockOverflow - extending selection on mouse input, etc.
//  These are similar to TextEditor services but without typing handling.
//
//---------------------------------------------------------------------------
class TextSelectionManager: public ITextSelectionNotify
{
    friend class JupiterTextHelper;

public:
    static _Check_return_ HRESULT Create(
        _In_ CUIElement* pOwnerUIElement,
        _In_ ITextContainer* pTextContainer,
        _Outptr_ TextSelectionManager** ppSelectionManager
        );

    static _Check_return_ HRESULT Destroy(
        _Inout_ TextSelectionManager **ppSelectionManager
        );

    _Check_return_ HRESULT TextViewChanged(
        _In_opt_ ITextView* pOldTextView,
        _In_opt_ ITextView* pNewTextView
        );

    _Check_return_ HRESULT OnPointerMoved(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnPointerPressed(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnPointerReleased(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnHolding(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
    );
    _Check_return_ HRESULT OnTapped(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
    );
    _Check_return_ HRESULT OnGotFocus(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnLostFocus(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnKeyUp(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnKeyDown(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnRightTapped(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT OnDoubleTapped(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
        );
    _Check_return_ HRESULT HWRender(
        _In_ IContentRenderer* pContentRenderer,
        _In_ ITextView* pTextView,
        bool isHighContrast,
        uint32_t highlightRectCount,
        _In_reads_opt_(highlightRectCount) XRECTF* pHighlightRects,
        bool isBackPlate = false
        );

    _Check_return_ HRESULT D2DRender(
        _In_ const D2DRenderParams& rp,
        _In_ ITextView* pTextView,
        bool isHighContrast,
        uint32_t highlightRectCount,
        _In_reads_opt_(highlightRectCount) XRECTF* pHighlightRects
        );

    bool IsSelectionVisible() const;
    IJupiterTextSelection* GetTextSelection() const;
    CSolidColorBrush* GetSelectionBackgroundBrush() const;
    _Check_return_ HRESULT GetSelectedText(_Out_ xstring_ptr* pstrText);
    _Check_return_ HRESULT GetSelectionBoundingRect(_Out_ wf::Rect* selectionBoundingRect);
    _Check_return_ HRESULT GetSelectionHighlightRects(
        _In_ ITextView* pTextView,
        _Out_ uint32_t* pCount,
        _Outptr_result_buffer_(*pCount) XRECTF** ppRectangles
        );
    _Check_return_ HRESULT GetBackPlateSelectionHighlightRects(
        _In_ ITextView* pTextView,
        _Out_ uint32_t* pCount,
        _Outptr_result_buffer_(*pCount) XRECTF** ppRectangles
        );
    _Check_return_ HRESULT CopySelectionToClipboard();

    // Gets the start and end offsets, as well as foreground and background brushes for a selected region.
    // If nothing is selected, selection pointer is empty.
    _Check_return_ HRESULT GetSelectionHighlightRegion(
        _In_ bool isHighContrast,
        _Out_ std::shared_ptr<HighlightRegion>& textSelection
        );

    // Selection public APIs.
    _Check_return_ HRESULT SelectAll();
    _Check_return_ HRESULT Select(
        _In_ const CPlainTextPosition &anchorTextPosition,
        _In_ const CPlainTextPosition &movingTextPosition
    );

    _Check_return_ HRESULT NotifyGripperPositionChanged(_In_ CTextSelectionGripper* pGripper, _In_ XPOINTF worldPos, _In_ int32_t pointerId);
    _Check_return_ HRESULT NotifyGripperPressed(_In_ CTextSelectionGripper* pGripper);
    _Check_return_ HRESULT NotifyPenPressedWithBarrelButton(_In_ CTextSelectionGripper *pGripper, _In_ CEventArgs* pEventArgs);
    _Check_return_ HRESULT SnapGrippersToSelection();

    _Check_return_ HRESULT ShowContextMenu(
        _In_ XPOINTF point,
        _In_ bool   isSelectionEmpty,
        _In_ bool   showGrippersOnDismiss);
    _Check_return_ HRESULT UpdateGripperParents();

    _Check_return_ HRESULT OnContextMenuDismiss();

    CTextSelectionGripper* GetOtherGripper(_In_ const CTextSelectionGripper* pGripper) const;

    _Check_return_ HRESULT SetSelectionHighlightColor(_In_ uint32_t selectionColor);

    static bool AreSelectionHighlightOffsetsEqual(
        _In_ std::shared_ptr<HighlightRegion> oldSelection,
        _In_ std::shared_ptr<HighlightRegion> newSelection
    );

    _Check_return_ HRESULT UpdateGripperPositions();

    _Check_return_ HRESULT SetActiveGripper(_In_ CTextSelectionGripper* pActiveGripper);

    _Check_return_ HRESULT ReleaseGrippers();

    void OnSelectionChanged() override;

    void RemoveCaretFromTextObject(_In_ CDependencyObject* textOwnerObject);

    _Check_return_ HRESULT ForceFocusLoss();

    _Check_return_ HRESULT DismissAllFlyouts();

private:
    std::unique_ptr<IJupiterTextSelection>                  m_pTextSelection;
    CSolidColorBrush*                                       m_pSelectionBackground;
    ITextContainer*                                         m_pContainer;
    xstring_ptr                                             m_strSelectedText;
    int32_t                                                 m_leftClickPointerId;
    int32_t                                                 m_penPointerId;
    bool                                                   m_hasPointerCapture : 1;
    bool                                                   m_leftClickPressed  : 1;
    bool                                                   m_penPressed        : 1;
    bool                                                   m_isShiftPressed    : 1;
    bool                                                   m_showGrippersOnCMDismiss : 1;
    bool                                                   m_endGripperLastMoved     : 1;
    bool                                                   m_interacting             : 1;
    bool                                                    m_forceFocusedVisualState : 1;
    bool                                                    m_isSelectionFlyoutUpdateQueued : 1;
    TouchTextSelection::CSnappingCalculator<uint32_t, TextSelectionManager>* m_pSnappingCalculator;

    // Due to a limitation the input manager will call OnDoubleTapped before OnPointerReleased
    // hence a PointerMove might happen in between the 2 events and since we do not release the
    // pointer capture until OnPointerRelease is called we mark m_ignorePointerMove = true
    // so that OnPointerMove will have no effect.
    bool m_ignorePointerMove : 1;

    CTextSelectionGripper* m_pGripperElementStart;
    CTextSelectionGripper* m_pGripperElementEnd;
    CTextSelectionGripper* m_pGripperActive;
    CUIElement* m_pOwnerUIElement;
    CPopup* m_pPopupGripperElementStart;
    CPopup* m_pPopupGripperElementEnd;
    DirectUI::PointerDeviceType m_lastInputDeviceType;
    xref_ptr<CCaretBrowsingCaret> m_caretElement;
    xref_ptr<CStoryboard> m_caretBlink;
    xref_ptr<CDispatcherTimer> m_caretTimer;
    CUIElement* m_caretOwnerUIElementNoRef = nullptr;

    static const XUINT32 DefaultCaretColor = 0xffffffff;


    TextSelectionManager();
    ~TextSelectionManager();

    void ShowCaretElement();
    void RemoveCaret();
    void EnsureCaret();
    void UpdateCaretElement();
    void CaretOnKeyDown(_In_ CUIElement* pSender, _In_ CEventArgs* pEventArgs, _In_ ITextView* pTextView);
    void SetupCaretBlinkAnimation();
    static _Check_return_ HRESULT OnCaretTimeout(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    void ResetCaretBlink();


    _Check_return_ HRESULT IsPointOverLinkedView(
        _In_ CUIElement* pSender,
        _In_ ITextView* pSenderView,
        _In_ const XPOINTF* pHitPoint,
        _Out_opt_ XPOINTF* pLocalMousePoint,
        _Outptr_ ITextView** ppPointerOverView,
        _Outptr_result_maybenull_ CUIElement** ppTargetUIElement
        );

    _Check_return_ HRESULT UpdateStartGripperPosition();

    _Check_return_ HRESULT UpdateEndGripperPosition();

    _Check_return_ HRESULT UpdateGripperPosition(
        _In_ bool isFirstGripper);

    _Check_return_ HRESULT GetSelectionCoordinates(
        bool fBegin,
        bool fAdjustForGripperRadius,
        _Out_ XPOINTF* pWorldCoordinate,
        _Out_opt_ XPOINTF* pLineWorldCoordinate,
        _Out_opt_ float* pLineHeight
        );

    _Check_return_ HRESULT GetGripperCoordinatesForTextOffset(
        _In_ bool fBegin,
        _In_ bool fAdjustForGripperPositioning,
        _In_ uint32_t offset,
        _In_ TextGravity gravity,
        _Out_ XPOINTF* pWorldCoordinate,
        _Out_opt_ XPOINTF* pLineWorldCoordinate,
        _Out_opt_ float* pLineHeight
        );

    _Check_return_ HRESULT EnsureGrippers();

    _Check_return_ HRESULT GetTextViewForRichTextBlock(
        _In_ uint32_t iTextPosition,
        _In_ TextGravity gravity,
        _Outptr_ ITextView** ppView,
        _Outptr_ CUIElement** ppOwnerElement
        );

    void SwapGrippers();

    _Check_return_ HRESULT CreateGripper(
        _Out_ CTextSelectionGripper** ppTextSelectionGripper,
        _Out_ CPopup** ppPopup);

    _Check_return_ HRESULT IsPointOverElement(
        _In_  CUIElement* pElement,
        _In_  const XPOINTF* pHitPoint,
        _Out_ XPOINTF* pLocalMousePoint,
        _Out_ bool* pIsPointOverElement,
        _Out_opt_ float* pDistance
        );

    _Check_return_ HRESULT NotifyContextMenuOpening(
        _In_ XPOINTF point,
        _In_ bool   isSelectionEmpty);

    _Check_return_ HRESULT ShowGrippers(_In_ bool fAnimate);

    _Check_return_ HRESULT IsOffsetBetweenSelection(
        _In_ CTextPosition currentOffsetPosition,
        _Out_ bool* pIsBetweenSelection);

    _Check_return_ HRESULT GetTextPosition(
        _In_ CInputPointEventArgs* pPointEventArgs,
        _In_ CUIElement* pSender,
        _In_ ITextView* pTextView,
        _Out_ CTextPosition* pTextPosition,
        _Out_opt_ bool* pbInNextLine);

    // Notifies TextContainer owner that selection or selection's visibility changed.
    _Check_return_ HRESULT GetSelectionStartEndOffsets(
        _Out_ uint32_t* pStartOffset,
        _Out_ uint32_t* pEndOffset
        );
    _Check_return_ HRESULT NotifySelectionVisibilityChanged();
    _Check_return_ HRESULT NotifySelectionChanged(
        _In_ uint32_t previousSelectionStartOffset,
        _In_ uint32_t previousSelectionEndOffset,
        bool fHideGrippers
        );
    _Check_return_ HRESULT NotifySelectionChanged(
        bool selectionPreviouslyEmpty,
        _In_ uint32_t previousSelectionStartOffset,
        _In_ uint32_t previousSelectionEndOffset,
        bool fHideGrippers
        );
    _Check_return_ HRESULT UpdateLastSelectedTextElement();
    _Check_return_ HRESULT GetSelectionHighlightColor(
        bool isHighContrast,
        _Out_ CSolidColorBrush** ppSolidColorBrush);
    _Check_return_ HRESULT GetSystemColorWindowBrush(
        _Outptr_ CSolidColorBrush** ppSolidColorBrush);

    _Check_return_ HRESULT ChangeSelection(
        _In_ const TouchTextSelection::SelectionRange<uint32_t>& currentRange,
        _In_ TouchTextSelection::GripperSide gripperSide,
        uint32_t newPosition);

    _Check_return_ HRESULT ExtendSelectionRange(
        _In_ TouchTextSelection::SnappingMode snappingMode,
        _In_ TouchTextSelection::GripperSide gripperSide,
        _In_ uint32_t newPosition,
        _In_ const TouchTextSelection::SelectionRange<uint32_t>& currentRange,
        _Out_ TouchTextSelection::SelectionRange<uint32_t>* newRange);

    _Check_return_ HRESULT GetStartOfWordInPosition(_In_ uint32_t position, _Out_ uint32_t* wordStart) const;

    _Check_return_ HRESULT GetEndOfWordInPosition(_In_ uint32_t position, _Out_ uint32_t* wordEnd) const;

    _Check_return_ HRESULT GetBoundaryOfWordInPosition(
        _In_ uint32_t position,
        _In_ FindBoundaryType findType,
        _Out_ uint32_t* boundary) const;

    bool ArePositionsInSameWord(_In_ uint32_t first, _In_ uint32_t second);

    bool ArePositionsInSameWordBoundary(
        _In_ uint32_t first,
        _In_ uint32_t second,
        _In_ HRESULT (TextSelectionManager::*GetBoundaryOfWordInPosition)(uint32_t, _Out_ uint32_t*) const) const;

    _Check_return_ HRESULT ClearSelection(_In_ const CTextPosition& caretPosition);

    _Check_return_ HRESULT SetSelectionToWord(_In_ const CTextPosition& tapPosition);

    _Check_return_ HRESULT EnsureSnappingCalculator();

    _Check_return_ HRESULT HideGrippers(bool fAnimate);

    _Check_return_ HRESULT SelectAllText(_In_ IJupiterTextSelection* textSelection, bool sendSelectionChangedNotification);

    _Check_return_ HRESULT GetSelectionHighlightRectsInternal(
        _In_ ITextView* pTextView,
        _Out_ uint32_t* pCount,
        _Outptr_result_buffer_(*pCount) XRECTF** ppRectangles,
        _In_ IJupiterTextSelection* pTextSelection
    );

    // Handling Move,Pressed and Released for selection.
    _Check_return_ HRESULT OnSelectionPointerMove(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
    );
    _Check_return_ HRESULT OnSelectionPointerPressed(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
    );
    _Check_return_ HRESULT OnSelectionPointerReleased(
        _In_ CUIElement* pSender,
        _In_ CEventArgs* pEventArgs,
        _In_ ITextView* pTextView
    );

    // SelectionFlyout helpers
    bool ShouldForceFocusedVisualState();
    CFlyoutBase* GetSelectionFlyoutNoRef() const;
    _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility();

public:
    _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

private:
    XPOINTF m_lastPointerPosition{};
};

inline IJupiterTextSelection* TextSelectionManager::GetTextSelection() const
{
    return m_pTextSelection.get();
}

inline CSolidColorBrush* TextSelectionManager::GetSelectionBackgroundBrush() const
{
    return m_pSelectionBackground;
}
