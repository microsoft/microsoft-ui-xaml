// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextSelectionManager.h"
#include "Gripper.h"
#include "LinkedRichTextBlockView.h"
#include "RichTextBlockView.h"
#include "TextContextMenu.h"
#include "TextSelectionSettings.h"
#include <SystemThemingInterop.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <ContentRenderer.h>
#include "HighlightRegion.h"
#include <CaretBrowsingGlobal.h>
#include "TextNavigationHelper.h"
#include "RectUtil.h"
#include "focusmgr.h"
#include "RootScale.h"

using namespace RuntimeFeatureBehavior;

static const TextGravity c_defaultGravity = LineForwardCharacterBackward;

using namespace TouchTextSelection;

_Check_return_ HRESULT XAML_SetClipboardText(_In_ const xstring_ptr_view& strText)
{
    // Create the DataPackage for the text
    ctl::ComPtr<wadt::IDataPackage> dataPackage;
    IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_DataPackage).Get(), dataPackage.GetAddressOf()));

    dataPackage->SetText(wrl_wrappers::HStringReference(strText.GetBuffer(), strText.GetCount()).Get());

    // Set the data package on the clipboard.
    ctl::ComPtr<wadt::IClipboardStatics> clipboard;
    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_Clipboard).Get(), clipboard.GetAddressOf()));

    IFC_RETURN(clipboard->SetContent(dataPackage.Get()));
    IFC_RETURN(clipboard->Flush());

    return S_OK;
}

//------------------------------------------------------------------------
//
//
//  Synopsis: Returns the default text selection highlight color.
//
//------------------------------------------------------------------------
uint32_t GetDefaultSelectionHighlightColor()
{
    SystemThemingInterop themingInterop;
    return themingInterop.GetSystemAccentColor();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::Create
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::Create(
    _In_ CUIElement         *pOwnerUIElement,
    _In_ ITextContainer     *pTextContainer,
    _Outptr_ TextSelectionManager **ppSelectionManager

)
{
    HRESULT               hr = S_OK;
    TextSelectionManager *pManager = nullptr;
    CSolidColorBrush     *pBackground = nullptr;

    CValue valueDefaultSelectionBackgroundColor;
    valueDefaultSelectionBackgroundColor.SetColor(GetDefaultSelectionHighlightColor());
    CREATEPARAMETERS CPDefaultSelectionBackgroundColor(pTextContainer->GetCore(), valueDefaultSelectionBackgroundColor);
    IFC(CreateDO2<CSolidColorBrush>(&pBackground, &CPDefaultSelectionBackgroundColor));

    // Finally, create selection manager with background and selection.
    pManager = new TextSelectionManager();
    pManager->m_pSelectionBackground = pBackground;
    pManager->m_pContainer = pTextContainer;
    pManager->m_pOwnerUIElement = pOwnerUIElement;

    *ppSelectionManager = pManager;
    pManager = nullptr;
    pBackground = nullptr;

Cleanup:
    delete pManager;
    ReleaseInterface(pBackground);
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::Destroy
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::Destroy(
    _Inout_ TextSelectionManager **ppSelectionManager
    )
{
    if (*ppSelectionManager != nullptr)
    {
        (*ppSelectionManager)->RemoveCaret();
        IFC_RETURN((*ppSelectionManager)->ReleaseGrippers());
        delete *ppSelectionManager;
        *ppSelectionManager = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::ReleaseGrippers()
{
    HRESULT hr = S_OK;

    IFC(HideGrippers(FALSE /* fAnimate */));
    // remove gripper from Popup, and remove Popup from owner(TextBlock, RichTextBlock)
    if (m_pGripperElementStart)
    {
        IFC(m_pPopupGripperElementStart->SetValueByKnownIndex(KnownPropertyIndex::Popup_Child, nullptr));

        // There is a possibility that Popup is moved to a different parent (RichTextBlockOverFlow, see function CTextSelectionGripper::UpdateParent)
        // Detach from its current parent in order to release the reference to PopupElement
        CUIElement* pParent = m_pPopupGripperElementStart->GetUIElementParentInternal();
        if (pParent != nullptr)
        {
            IFC(pParent->RemoveChild(m_pPopupGripperElementStart));
        }
    }

    if (m_pGripperElementEnd)  // do the same thing for end gripper popup
    {
        IFC(m_pPopupGripperElementEnd->SetValueByKnownIndex(KnownPropertyIndex::Popup_Child, nullptr));
        CUIElement* pParent = m_pPopupGripperElementEnd->GetUIElementParentInternal();
        if (pParent != nullptr)
        {
            IFC(pParent->RemoveChild(m_pPopupGripperElementEnd));
        }
    }

Cleanup:
    ReleaseInterface(m_pGripperElementStart);
    ReleaseInterface(m_pGripperElementEnd);
    ReleaseInterface(m_pPopupGripperElementStart);
    ReleaseInterface(m_pPopupGripperElementEnd);

    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::EnsureGrippers
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::EnsureGrippers()
{
    if (!m_pGripperElementStart)
    {
        IFC_RETURN(CreateGripper(
            &m_pGripperElementStart,
            &m_pPopupGripperElementStart
        ));
        m_pGripperElementStart->SetStartGripper(TRUE /*isStart*/);

        IFC_RETURN(CreateGripper(
            &m_pGripperElementEnd,
            &m_pPopupGripperElementEnd
        ));
        m_pGripperElementEnd->SetStartGripper(FALSE /*isStart*/);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::CreateGripper
//
//  Synopsis:
//      Creates a text selection gripper and attaches it to a popup
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::CreateGripper(
    _Out_ CTextSelectionGripper **ppTextSelectionGripper,
    _Out_ CPopup **ppPopup)
{
    HRESULT               hr = S_OK;
    CDependencyObject    *pGripperDo = nullptr;
    CTextSelectionGripper *pGripper = nullptr;
    CDependencyObject    *pPopupDo = nullptr;
    CPopup               *pPopup = nullptr;
    CREATEPARAMETERS      createParameters(m_pOwnerUIElement->GetContext());

    IFC(CTextSelectionGripper::Create(&pGripperDo, &createParameters));
    pGripper = static_cast<CTextSelectionGripper*>(pGripperDo);
    IFC(pGripper->InitializeGripper(this));
    IFC(CPopup::Create(&pPopupDo, &createParameters));
    pPopup = static_cast<CPopup*>(pPopupDo);
    pPopupDo = nullptr;

    // Add the child first, so that a peer will be created and kept alive by its parent.
    ASSERT(m_pOwnerUIElement != nullptr);
    IFC(m_pOwnerUIElement->AddChild(pPopup));

    IFC(pPopup->SetValueByKnownIndex(KnownPropertyIndex::Popup_Child, pGripper));
    pGripper->SetPopupNoAddRef(pPopup);

    *ppPopup = pPopup;
    pPopup = nullptr;

    *ppTextSelectionGripper = pGripper;
    pGripper = nullptr;

Cleanup:
    ReleaseInterface(pGripper);
    ReleaseInterface(pPopup);
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::TextSelectionManager
//
//---------------------------------------------------------------------------
TextSelectionManager::TextSelectionManager() :
    m_pSelectionBackground(nullptr),
    m_pContainer(nullptr),
    m_strSelectedText(),
    m_leftClickPointerId(-1),
    m_penPointerId(-1),
    m_hasPointerCapture(FALSE),
    m_leftClickPressed(FALSE),
    m_penPressed(FALSE),
    m_isShiftPressed(FALSE),
    m_showGrippersOnCMDismiss(FALSE),
    m_ignorePointerMove(FALSE),
    m_interacting(FALSE),
    m_forceFocusedVisualState(false),
    m_isSelectionFlyoutUpdateQueued(false),
    m_pGripperElementStart(nullptr),
    m_pGripperElementEnd(nullptr),
    m_pOwnerUIElement(nullptr),
    m_pPopupGripperElementStart(nullptr),
    m_pPopupGripperElementEnd(nullptr),
    m_lastInputDeviceType(static_cast<DirectUI::PointerDeviceType>(-1)),
    m_pSnappingCalculator(nullptr),
    m_pGripperActive(nullptr)
{}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::~TextSelectionManager
//
//---------------------------------------------------------------------------
TextSelectionManager::~TextSelectionManager()
{
    ReleaseInterface(m_pSelectionBackground);
    delete m_pSnappingCalculator;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::TextViewChanged
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::TextViewChanged(
    _In_opt_ ITextView *pOldTextView,
    _In_opt_ ITextView *pNewTextView
)
{
    // If the old text view is different from the new one, selection and all associated text pointers will be different.
    // Delete the current selection and create a new one.

    if (pOldTextView != pNewTextView)
    {
        m_pTextSelection.reset();

        // Create selection object.
        if (pNewTextView != nullptr)
        {
            CTextSelection* pSelection = nullptr;

            IFC_RETURN(CTextSelection::Create(m_pContainer, pNewTextView, static_cast<ITextSelectionNotify*>(this), &pSelection));

            m_pTextSelection.reset(pSelection);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnSelectionPointerMove
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnSelectionPointerMove(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerEventArgs = nullptr;
    XPOINTF mouseMovePoint;
    uint32_t mouseMoveOffset;
    XPOINTF hitPoint;
    TextGravity gravity = LineForwardCharacterBackward;
    CTextPosition mouseMovePosition;
    ITextView *pPointerOverView = nullptr;
    uint32_t previousSelectionStartOffset = 0;
    uint32_t previousSelectionEndOffset = 0;

    pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerEventArgs);

    // MouseMove, etc. are expected to be called only when we know we have a selection.
    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);

    hitPoint = pPointerEventArgs->GetGlobalPoint();

    // If we have mouse capture, in a linked scenario the mouse may be captured by
    // one link but actually be over another link. In that case, we should hit test the
    // linked view, because we want to extend selection.
    IFC_RETURN(IsPointOverLinkedView(pSender,
                              pTextView,
                              &hitPoint,
                              &mouseMovePoint,
                              &pPointerOverView,
                              nullptr));

    // Always use the mouse over view for hit testing.
    IFC_RETURN(pPointerOverView->PixelPositionToTextPosition(
        mouseMovePoint,   // pixel offset
        FALSE,             // Recognise hits after newline
        &mouseMoveOffset,
        &gravity
    ));

    // Get previous selection offsets to pass to NotifySelectionChanged.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &previousSelectionStartOffset,
        &previousSelectionEndOffset));

    // Use the TextView passed in to create the position. Assume it's the same one that selection was created with,
    // it's the callers responsibility to notify us if TextView changed.
    // TODO: add assert?
    mouseMovePosition = CPlainTextPosition(m_pContainer, mouseMoveOffset, gravity);
    IFC_RETURN(m_pTextSelection->ExtendSelectionByMouse(mouseMovePosition));

    pPointerEventArgs->m_bHandled = TRUE;

    bool hideGrippers = true;
    const bool interactionIsPen = m_penPressed;
    if (interactionIsPen)
    {
        IFC_RETURN(ShowGrippers(TRUE /* Animate */));
        hideGrippers = FALSE;
    }

    // Assuming we have capture and are handling the move, we can assume selection will change.
    IFC_RETURN(NotifySelectionChanged(
        previousSelectionStartOffset,
        previousSelectionEndOffset,
        hideGrippers /* fHideGrippers */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnSelectionPointerPressed
//
//  Synopsis: Event handler for MouseLeftButtonDown event on TextBlock.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnSelectionPointerPressed(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerEventArgs = nullptr;
    CTextPosition mouseDownPosition;
    uint32_t previousSelectionStartOffset = 0;
    uint32_t previousSelectionEndOffset = 0;

    pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerEventArgs);

    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);
    bool selectionPreviouslyEmpty = m_pTextSelection->IsEmpty();
    bool isInitiallyFocused = false;

    isInitiallyFocused = IsSelectionVisible();
    if (!pSender->IsFocused())
    {
        bool focusUpdated = false;
        IFC_RETURN(pSender->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusUpdated));
        if (!focusUpdated)
        {
            // We can't obtain focus - do not react to the mouse button.
            return S_OK;
        }
    }

    IFC_RETURN(GetTextPosition(
        pPointerEventArgs,
        pSender,
        pTextView,
        &mouseDownPosition,
        nullptr
    ));

    // Get previous selection offsets to pass to NotifySelectionChanged.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &previousSelectionStartOffset,
        &previousSelectionEndOffset));

    m_isShiftPressed = FALSE;
    bool hasPointerCapture = false;
    if ((pPointerEventArgs->m_keyModifiers & DirectUI::VirtualKeyModifiers::Shift) != DirectUI::VirtualKeyModifiers::None)
    {
        m_isShiftPressed = TRUE;
        IFC_RETURN(m_pTextSelection->ExtendSelectionByMouse(mouseDownPosition));
        IFC_RETURN(pSender->CapturePointer(pPointerEventArgs->m_pPointer, &hasPointerCapture));
        m_hasPointerCapture = hasPointerCapture;
    }
    else
    {
        bool isOffsetBetweenSelection = false;
        IFC_RETURN(IsOffsetBetweenSelection(mouseDownPosition, &isOffsetBetweenSelection));

        // If the pointer is pressed within the the selection we just do nothing.
        // This is the current behavior of IE for mouse.
        // For pen we have to do that. Otherwise, when the user PressHoldAndLift
        // to invoke the Context Menu, the selection will be reset before the CM shows.
        if (!isInitiallyFocused || !isOffsetBetweenSelection)
        {
            IFC_RETURN(m_pTextSelection->SetCaretPositionFromTextPosition(mouseDownPosition));
            IFC_RETURN(pSender->CapturePointer(pPointerEventArgs->m_pPointer, &hasPointerCapture));
            m_hasPointerCapture = hasPointerCapture;
        }
    }

    IFC_RETURN(HideGrippers(FALSE /* fAnimate */));

    IFC_RETURN(NotifySelectionChanged(
        selectionPreviouslyEmpty,
        previousSelectionStartOffset,
        previousSelectionEndOffset,
        TRUE /* fHideGrippers */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetTextPosition
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetTextPosition(
    _In_ CInputPointEventArgs *pPointEventArgs,
    _In_ CUIElement *pSender,
    _In_ ITextView *pTextView,
    _Out_ CTextPosition *pTextPosition,
    _Out_opt_ bool *pbInNextLine
)
{
    wf::Point pointCoords;
    XPOINTF pointCoordsInternal;
    uint32_t pointDownOffset;
    TextGravity gravity = LineForwardCharacterBackward;

    ASSERT(pTextPosition != nullptr);

    IFC_RETURN(pPointEventArgs->GetPosition(pSender, &pointCoords));

    pointCoordsInternal.x = pointCoords.X;
    pointCoordsInternal.y = pointCoords.Y;

    IFC_RETURN(pTextView->PixelPositionToTextPosition(
        pointCoordsInternal,   // pixel offset
        FALSE /* bIncludeNewline */, // Get position before any trailing newlines.
        &pointDownOffset,
        &gravity
    ));

    *pTextPosition = CPlainTextPosition(m_pContainer, pointDownOffset, gravity);
    if (pbInNextLine != nullptr)
    {
        *pbInNextLine = ((gravity & LineBackward) != 0) ? TRUE : FALSE;
    }


    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnDoubleTapped
//
//  Synopsis: Event handler for OnDoubleTapped event on TextBlock.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnDoubleTapped(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CDoubleTappedEventArgs *pPointerEventArgs = nullptr;
    CTextPosition pointerDownPosition;
    uint32_t previousSelectionStartOffset = 0;
    uint32_t previousSelectionEndOffset = 0;
    bool bTextPositionInLineBelow;

    pPointerEventArgs = static_cast<CDoubleTappedEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerEventArgs);

    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);
    bool selectionPreviouslyEmpty = m_pTextSelection->IsEmpty();

    // If the event is already handled or doesn't come from the sender, we shouldn't be doing
    // selection updates.
    if (pPointerEventArgs->m_bHandled || pPointerEventArgs->m_pSource != pSender)
    {
        return S_OK;
    }

    if (!pSender->IsFocused())
    {
        bool focusUpdated = false;
        IFC_RETURN(pSender->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusUpdated));
        if (!focusUpdated)
        {
            // We can't obtain focus - do not react to the mouse button.
            return S_OK;
        }
    }

    m_lastInputDeviceType = pPointerEventArgs->m_pointerDeviceType;

    // Get previous selection offsets to pass to NotifySelectionChanged.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &previousSelectionStartOffset,
        &previousSelectionEndOffset));

    IFC_RETURN(GetTextPosition(
        pPointerEventArgs,
        pSender,
        pTextView,
        &pointerDownPosition,
        &bTextPositionInLineBelow
    ));

    if (pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch ||
        (pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen))
    {
        // If the given position is in the line below the one hit, don't start the selection,
        // and dismiss the existing one so we don't show it when we tap back into the control when the focus was somewhere else
        if (bTextPositionInLineBelow)
        {
            IFC_RETURN(ClearSelection(pointerDownPosition));
        }
        else
        {
            IFC_RETURN(SetSelectionToWord(pointerDownPosition));
        }
    }
    else
    {
        IFC_RETURN(CTextBoxHelpers::SelectWordFromTextPosition(
            m_pContainer,
            pointerDownPosition,
            m_pTextSelection.get(),
            FindBoundaryType::ForwardIncludeTrailingWhitespace,
            TagConversion::None
        ));
    }

    pPointerEventArgs->m_bHandled = TRUE;

    // Due to a limitation the input manager will call OnDoubleTapped before OnPointerReleased
    // hence a PointerMove might happen in between the 2 events and since we do not release the
    // pointer capture until OnPointerRelease is called we mark m_ignorePointerMove = true
    // so that OnPointerMove will have no effect.
    m_ignorePointerMove = TRUE;

    if (pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch ||
        (pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen))
    {
        IFC_RETURN(ShowGrippers(TRUE /* fAnimate */));
    }
    else
    {
        // Manipulating via a mouse. Ensure the grippers are not shown, no animation.
        IFC_RETURN(HideGrippers(FALSE /* fAnimate */));
    }

    IFC_RETURN(NotifySelectionChanged(
        selectionPreviouslyEmpty,
        previousSelectionStartOffset,
        previousSelectionEndOffset,
        FALSE /* fHideGrippers */)); // Already hidden with the code above if necessary

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnSelectionPointerReleased
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnSelectionPointerReleased(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    auto const pPointerEventArgs = static_cast<CPointerEventArgs* const>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerEventArgs);

    // Do not handle the event if we don't have capture.
    // It is OK to call ReleaseCapture unconditionally, but if the event is marked handled,
    // it will not bubble further.
    if (!pPointerEventArgs->m_bHandled && m_hasPointerCapture)
    {
        if (!m_pTextSelection->IsEmpty() && IsSelectionVisible())
        {
            CPointer* pPointer = nullptr;
            CPointerEventArgs* pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
            pPointer = pPointerArgs->m_pPointer;

            if (pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse)
            {
                IFC_RETURN(QueueUpdateSelectionFlyoutVisibility());
            }
        }

        m_hasPointerCapture = false;
        IFC_RETURN(pSender->ReleasePointerCapture(pPointerEventArgs->m_pPointer));
    }

    // MouseUp currently has no effect on selection - no need to notify owner of change.
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnPointerMoved
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnPointerMoved(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerArgs = nullptr;
    CPointer* pPointer = nullptr;

    pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerArgs);

    if (pPointerArgs->m_bHandled || !m_hasPointerCapture || m_ignorePointerMove)
    {
        return S_OK;
    }

    pPointer = pPointerArgs->m_pPointer;

    if ((pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse
         && m_leftClickPressed
         && m_leftClickPointerId == pPointer->m_uiPointerId)
        ||
        (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen
         && m_penPressed
         && m_penPointerId == pPointer->m_uiPointerId))
    {
        IFC_RETURN(OnSelectionPointerMove(
            pSender,
            pEventArgs,
            pTextView));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnPointerPressed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnPointerPressed(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerArgs = nullptr;
    CPointer* pPointer = nullptr;

    pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerArgs);

    pPointer = pPointerArgs->m_pPointer;

    // If the event is already handled or doesn't come from the sender, we shouldn't be doing
    // selection updates.
    if (pPointerArgs->m_bHandled || pPointerArgs->m_pSource != pSender)
    {
        return S_OK;
    }

    // Due to a limitation the input manager will call OnDoubleTapped before OnPointerReleased
    // hence a PointerMove might happen in between the 2 events and since we do not release the
    // pointer capture until OnPointerRelease is called we mark m_ignorePointerMove = true
    // so that OnPointerMove will have no effect.
    m_ignorePointerMove = FALSE;
    m_lastInputDeviceType = pPointer->m_pointerDeviceType;

    if (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse && pPointer->m_bLeftButtonPressed)
    {
        m_leftClickPressed = TRUE;
        m_leftClickPointerId = pPointer->m_uiPointerId;

        IFC_RETURN(OnSelectionPointerPressed(
            pSender,
            pEventArgs,
            pTextView));
    }
    else if (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen && pPointer->m_bBarrelButtonPressed)
    {
        m_penPressed = TRUE;
        m_penPointerId = pPointer->m_uiPointerId;

        IFC_RETURN(OnSelectionPointerPressed(
            pSender,
            pEventArgs,
            pTextView));
    }

    if (GetCaretBrowsingModeEnable() && m_caretElement == nullptr)  //when it get focus first then enable CaretBrowsingMode
    {
        ShowCaretElement();
    }

    pPointerArgs->m_bHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::NotifyPenPressedWithBarrelButton(_In_ CTextSelectionGripper *pGripper, _In_ CEventArgs* pEventArgs)
{

    CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    CPointer* pPointer = pPointerArgs->m_pPointer;

    m_pTextSelection->ResetSelection();
    m_penPressed = TRUE;
    m_penPointerId = pPointer->m_uiPointerId;

    IFC_RETURN(OnSelectionPointerPressed(
        pGripper->GetOwner(),
        pEventArgs,
        pGripper->GetOwnerTextView()));

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnPointerReleased
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnPointerReleased(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerArgs = nullptr;
    CPointer* pPointer = nullptr;

    pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    IFCEXPECT_ASSERT_RETURN(pPointerArgs);

    pPointer = pPointerArgs->m_pPointer;

    if (pPointerArgs->m_bHandled)
    {
        return S_OK;
    }

    if (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse
        && m_leftClickPressed
        && pPointer->m_uiPointerId == m_leftClickPointerId)
    {
        IFC_RETURN(OnSelectionPointerReleased(
            pSender,
            pEventArgs,
            pTextView));

        m_leftClickPressed = FALSE;
        m_leftClickPointerId = -1;
    }
    else if (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen
             && m_penPressed
             && pPointer->m_uiPointerId == m_penPointerId)
    {
        IFC_RETURN(OnSelectionPointerReleased(
            pSender,
            pEventArgs,
            pTextView));

        m_penPointerId = -1;
        m_penPressed = FALSE;

        ShowGrippers(TRUE /*animate*/);
    }

    // Stops the event from bubbling up.
    // When TB or RTB is inside a scroll viewer, not marking this event as handled
    // will cause the control to go out of focus. This will prevent us from
    // successfully showing the CM when the user taps again on a selected word.
    pPointerArgs->m_bHandled = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnHolding
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnHolding(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CPointerEventArgs *pPointerEventArgs = nullptr;

    ASSERT(m_pTextSelection != nullptr);

    pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);

    // If the event is already handled or doesn't come from the sender, we shouldn't be doing
    // selection updates.
    if (pPointerEventArgs->m_bHandled || pPointerEventArgs->m_pSource != pSender)
    {
        return S_OK;
    }

    if (!pSender->IsFocused() && !FxCallbacks::TextControlFlyout_IsOpen(pSender->GetContextFlyout().get()))
    {
        bool focusUpdated = false;
        IFC_RETURN(pSender->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusUpdated))
        if (!focusUpdated)
        {
            // We can't obtain focus - do not react to the mouse button.
            return S_OK;
        }
    }

    if (!m_pTextSelection->IsEmpty())
    {
        if (pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen)
        {
            IFC_RETURN(ShowContextMenu(
                pPointerEventArgs->GetGlobalPoint(),
                FALSE /*isSelectionEmpty*/,
                true /* showGrippersOnDismiss */));

            pPointerEventArgs->m_bHandled = TRUE;
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnRightTapped
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnRightTapped(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CRightTappedEventArgs *pRightTappedEventArgs = nullptr;

    ASSERT(m_pTextSelection != nullptr);

    pRightTappedEventArgs = static_cast<CRightTappedEventArgs*>(pEventArgs);

    // If the event is already handled or doesn't come from the sender, we shouldn't be doing
    // selection updates.
    if (pRightTappedEventArgs->m_bHandled || pRightTappedEventArgs->m_pSource != pSender)
    {
        return S_OK;
    }

    if (!pSender->IsFocused() && !FxCallbacks::TextControlFlyout_IsOpen(pSender->GetContextFlyout().get()))
    {
        bool focusUpdated = false;
        IFC_RETURN(pSender->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusUpdated))
            if (!focusUpdated)
            {
                // We can't obtain focus - do not react to the mouse button.
                return S_OK;
            }
    }

    if (!m_pTextSelection->IsEmpty())
    {
        bool reshowGrippers = pRightTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch ||
                              pRightTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;

        IFC_RETURN(ShowContextMenu(
            pRightTappedEventArgs->GetGlobalPoint(),
            FALSE /*isSelectionEmpty*/,
            reshowGrippers));

        pRightTappedEventArgs->m_bHandled = TRUE;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnGotFocus
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnGotFocus(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    // We call UpdateLastSelectedTextElement here to make sure our state is updated
    // in case the focus was gained as a result on mouse or pen input.
    IFC_RETURN(UpdateLastSelectedTextElement());

    // GotFocus can't change the selection, only make it visible, so only invalidate render if it's non-empty.
    if (m_pTextSelection != nullptr && !(m_pTextSelection->IsEmpty()))
    {
        if (m_lastInputDeviceType == DirectUI::PointerDeviceType::Touch ||
            (m_lastInputDeviceType == DirectUI::PointerDeviceType::Pen))
        {
            if (m_pGripperElementStart)
            {
                IFC_RETURN(ShowGrippers(true /* fAnimate */));
            }
        }

        // Selection went from invisible to visible, notify visibility changed.
        IFC_RETURN(NotifySelectionVisibilityChanged());

        m_forceFocusedVisualState = false;
    }
    if (GetCaretBrowsingModeEnable())
    {
        uint32_t previousSelectionStartOffset = 0;
        uint32_t previousSelectionEndOffset = 0;
        IFCFAILFAST(GetSelectionStartEndOffsets(
            &previousSelectionStartOffset,
            &previousSelectionEndOffset));
        // We don't have way to identify the previous input method.
        // Solution here is to keep the selection as before, only skip the hidden character at very first time got focus.
        // TODO: find a way to identify this focus is got from tab key.
        if (previousSelectionStartOffset == 0 && previousSelectionEndOffset == 0)
        {
            CTextPosition textPosition;
            IFCFAILFAST(m_pTextSelection->GetAnchorTextPosition(&textPosition));
            CPlainTextPosition plainTextPosition = textPosition.GetPlainPosition();
            IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfContent(false, m_pOwnerUIElement, &plainTextPosition));
            IFCFAILFAST(m_pTextSelection->SetCaretPositionFromTextPosition(CTextPosition(plainTextPosition)));
            IFCFAILFAST(NotifySelectionChanged(
                previousSelectionStartOffset,
                previousSelectionEndOffset,
                FALSE /* fHideGrippers */));
        }
        ShowCaretElement();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnLostFocus
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnLostFocus(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    // LostFocus can't change the selection, only make it invisible, so only invalidate render if it's non-empty.
    if (m_pTextSelection != nullptr && !(m_pTextSelection->IsEmpty()))
    {
        // If we're losing focus to the Selection or Context flyouts, then we should not hide the selection or grippers.
        m_forceFocusedVisualState = ShouldForceFocusedVisualState();

        if (!m_forceFocusedVisualState)
        {
            IFC_RETURN(HideGrippers(false /* fAnimate */));            
        }

        IFC_RETURN(NotifySelectionVisibilityChanged());
    }
    RemoveCaret();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnTapped
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnTapped(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CTappedEventArgs *pTappedEventArgs = nullptr;
    CTextPosition     tapPosition;
    uint32_t previousSelectionStartOffset = 0;
    uint32_t previousSelectionEndOffset = 0;
    bool bTextPositionInLineBelow;

    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);
    bool selectionPreviouslyEmpty = m_pTextSelection->IsEmpty();

    pTappedEventArgs = static_cast<CTappedEventArgs*>(pEventArgs);

    // If the event is already handled or doesn't come from the sender, we shouldn't be doing
    // selection updates.
    if (pTappedEventArgs->m_bHandled || pTappedEventArgs->m_pSource != pSender)
    {
        return S_OK;
    }

    // If one gripper is already being manipulated, ignore the tap with any other pointer
    if (((m_pGripperElementStart != nullptr) && m_pGripperElementStart->HasPointerCapture()) ||
        ((m_pGripperElementEnd != nullptr) && m_pGripperElementEnd->HasPointerCapture()))
    {
        pTappedEventArgs->m_bHandled = TRUE;
        return S_OK;
    }

    m_lastInputDeviceType = pTappedEventArgs->m_pointerDeviceType;

    // Get previous selection offsets to pass to NotifySelectionChanged.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &previousSelectionStartOffset,
        &previousSelectionEndOffset));

    IFC_RETURN(GetTextPosition(
        pTappedEventArgs,
        pSender,
        pTextView,
        &tapPosition,
        &bTextPositionInLineBelow
    ));

    if (pTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch ||
       (pTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen))
    {
        IFC_RETURN(EnsureGrippers());
        CTextCore* pTextCore = nullptr;
        IFC_RETURN(m_pOwnerUIElement->GetContext()->GetTextCore(&pTextCore));
        if (!m_pTextSelection->IsEmpty() && IsSelectionVisible())
        {
            bool isOffsetBetweenSelection = false;
            IFC_RETURN(IsOffsetBetweenSelection(tapPosition, &isOffsetBetweenSelection));

            if (isOffsetBetweenSelection)
            {
                // Tapped inside the selection. Show grippers if not shown yet.
                IFC_RETURN(ShowGrippers(TRUE /* fAnimate */));
            }
            else
            {
                // If text is already selected, a tap outside the selected text should
                // just clear the selection.
                IFC_RETURN(ClearSelection(tapPosition));
            }
        }
        else if (!pTextCore->CanSelectText(m_pContainer->GetOwnerUIElement()))
        {
            IFC_RETURN(ClearSelection(tapPosition));
        }
        else
        {
            // If the given position is in the line below the one hit, don't start the selection,
            // and dismiss the existing one so we don't show it when we tap back into the control when the focus was somewhere else
            if (bTextPositionInLineBelow)
            {
                IFC_RETURN(ClearSelection(tapPosition));
            }
            else
            {
                IFC_RETURN(SetSelectionToWord(tapPosition));
            }
        }
    }
    else
    {
        if (!m_isShiftPressed)
        {
            IFC_RETURN(m_pTextSelection->SetCaretPositionFromTextPosition(tapPosition));
        }
    }

    if (!pSender->IsFocused())
    {
        bool wasFocusUpdated;
        IFC_RETURN(pSender->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &wasFocusUpdated));
    }

    pTappedEventArgs->m_bHandled = TRUE;
    m_ignorePointerMove = TRUE;
    IFC_RETURN(NotifySelectionChanged(
        selectionPreviouslyEmpty,
        previousSelectionStartOffset,
        previousSelectionEndOffset,
        FALSE /* fHideGrippers */));

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::GetSelectionStartEndOffsets(
    _Out_ uint32_t *pStartOffset,
    _Out_ uint32_t *pEndOffset
)
{
    CTextPosition     startPosition;
    CTextPosition     endPosition;
    uint32_t           startOffset;
    uint32_t           endOffset;

    IFC_RETURN(m_pTextSelection->GetStartTextPosition(&startPosition));
    IFC_RETURN(m_pTextSelection->GetEndTextPosition(&endPosition));

    IFC_RETURN(startPosition.GetOffset(&startOffset));
    IFC_RETURN(endPosition.GetOffset(&endOffset));

    *pStartOffset = startOffset;
    *pEndOffset = endOffset;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifySelectionVisibilityChanged
//
//  Synopsis: Notifies TextContainer owner that selection visibility changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::NotifySelectionVisibilityChanged()
{
    CRichTextBlock *pRichTextBlock = nullptr;
    CUIElement *pOwner = m_pContainer->GetOwnerUIElement();
    CTextBlock *pTextBlock = nullptr;
    uint32_t selectionStartOffset = 0;
    uint32_t selectionEndOffset = 0;

    pRichTextBlock = do_pointer_cast<CRichTextBlock>(pOwner);
    pTextBlock = do_pointer_cast<CTextBlock>(pOwner);

    // Get updated selection start/end offsets.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &selectionStartOffset,
        &selectionEndOffset));

    if (pRichTextBlock != nullptr)
    {
        IFC_RETURN(pRichTextBlock->OnSelectionVisibilityChanged(
            selectionStartOffset,
            selectionEndOffset));
    }
    else if (pTextBlock != nullptr)
    {
        IFC_RETURN(pTextBlock->OnSelectionVisibilityChanged(
            selectionStartOffset,
            selectionEndOffset));
    }
    else
    {
        // Currently only TextBlock and RichTextBlock are using
        // the text selection Manager.
        IFCEXPECT_ASSERT_RETURN(FALSE);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifySelectionChanged
//
//  Synopsis: Notifies TextContainer owner that selection changed.
//            Deletes stored SelectedText string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::NotifySelectionChanged(
    _In_ uint32_t previousSelectionStartOffset,
    _In_ uint32_t previousSelectionEndOffset,
    _In_ bool fHideGrippers
)
{
    CRichTextBlock *pRichTextBlock = nullptr;
    CUIElement *pOwner = m_pContainer->GetOwnerUIElement();
    CTextBlock *pTextBlock = nullptr;
    uint32_t newSelectionStartOffset = 0;
    uint32_t newSelectionEndOffset = 0;

    // Delete the current stored value of selected text.
    m_strSelectedText.Reset();

    // Get updated selection start/end offsets.
    IFC_RETURN(GetSelectionStartEndOffsets(
        &newSelectionStartOffset,
        &newSelectionEndOffset));

    pRichTextBlock = do_pointer_cast<CRichTextBlock>(pOwner);
    pTextBlock = do_pointer_cast<CTextBlock>(pOwner);

    if (pRichTextBlock != nullptr)
    {
        IFC_RETURN(pRichTextBlock->OnSelectionChanged(
            previousSelectionStartOffset,
            previousSelectionEndOffset,
            newSelectionStartOffset,
            newSelectionEndOffset));
    }
    else if (pTextBlock != nullptr)
    {
        IFC_RETURN(pTextBlock->OnSelectionChanged(
            previousSelectionStartOffset,
            previousSelectionEndOffset,
            newSelectionStartOffset,
            newSelectionEndOffset));
    }
    else
    {
        // Currently only TextBlock and CRichTextBlock are using
        // the text selection Manager.
        IFCEXPECT_ASSERT_RETURN(FALSE);
    }

    IFC_RETURN(UpdateLastSelectedTextElement());
    if (fHideGrippers)
    {
        IFC_RETURN(HideGrippers(FALSE /* fAnimate */));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifySelectionChanged
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::NotifySelectionChanged(
    _In_ bool selectionPreviouslyEmpty,
    _In_ uint32_t previousSelectionStartOffset,
    _In_ uint32_t previousSelectionEndOffset,
    _In_ bool fHideGrippers
)
{
    bool selectionChanged = true;

    // If the selection was empty before and still is, there is no need for callers to re-render
    // highlight, etc. Further optimization is possible by comparing anchor/moving positions, etc.
    // but is not really worth it.
    if (m_pTextSelection->IsEmpty() && selectionPreviouslyEmpty)
    {
        selectionChanged = FALSE;
    }

    if (selectionChanged)
    {
        IFC_RETURN(NotifySelectionChanged(
            previousSelectionStartOffset,
            previousSelectionEndOffset,
            fHideGrippers));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::IsOffsetBetweenSelection
//
//  Synopsis: Checks whether a given offset is between the start
//            and end of the selected text.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::IsOffsetBetweenSelection(
    _In_ CTextPosition currentOffsetPosition,
    _Out_ bool       *pIsBetweenSelection)
{
    CTextPosition     startPosition;
    CTextPosition     endPosition;
    uint32_t           currentOffset;
    uint32_t           startOffset;
    uint32_t           endOffset;

    IFC_RETURN(m_pTextSelection->GetStartTextPosition(&startPosition));
    IFC_RETURN(m_pTextSelection->GetEndTextPosition(&endPosition));

    IFC_RETURN(currentOffsetPosition.GetOffset(&currentOffset));
    IFC_RETURN(startPosition.GetOffset(&startOffset));
    IFC_RETURN(endPosition.GetOffset(&endOffset));

    *pIsBetweenSelection = (currentOffset >= startOffset && currentOffset < endOffset);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::UpdateGripperParents
//
//  Synopsis: Update the grippers' positions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::UpdateGripperParents()
{
    CTextPosition     startTextPosition;
    CTextPosition     endTextPosition;
    ITextView         *pTextView = nullptr;
    CUIElement        *pRTBTarget = nullptr;
    CTextBlock        *pTextBlock = nullptr;
    uint32_t            startOffset;
    uint32_t            endOffset;
    TextGravity        startGravity;
    TextGravity        endGravity;

    IFC_RETURN(m_pTextSelection->GetStartTextPosition(&startTextPosition));
    IFC_RETURN(m_pTextSelection->GetEndTextPosition(&endTextPosition));

    IFC_RETURN(startTextPosition.GetOffset(&startOffset));
    IFC_RETURN(endTextPosition.GetOffset(&endOffset));

    startGravity = m_pTextSelection->GetStartGravity();
    endGravity = m_pTextSelection->GetEndGravity();

    ASSERT(m_pGripperElementStart != nullptr);
    ASSERT(m_pGripperElementEnd != nullptr);

    if (m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        IFC_RETURN(GetTextViewForRichTextBlock(startOffset, startGravity, &pTextView, &pRTBTarget));
        ASSERT(pRTBTarget != nullptr);

        m_pGripperElementStart->SetOwner(pRTBTarget);
        m_pGripperElementStart->SetOwnerTextView(pTextView);
    }
    else
    {
        pTextBlock = do_pointer_cast<CTextBlock>(m_pOwnerUIElement);
        ASSERT(pTextBlock != nullptr);
        pTextView = pTextBlock->GetTextView();
        m_pGripperElementStart->SetOwner(m_pOwnerUIElement);
        m_pGripperElementStart->SetOwnerTextView(pTextView);
    }

    if (m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        IFC_RETURN(GetTextViewForRichTextBlock(endOffset, endGravity, &pTextView, &pRTBTarget));
        ASSERT(pRTBTarget != nullptr);

        m_pGripperElementEnd->SetOwner(pRTBTarget);
        m_pGripperElementEnd->SetOwnerTextView(pTextView);
    }
    else
    {
        pTextBlock = do_pointer_cast<CTextBlock>(m_pOwnerUIElement);
        ASSERT(pTextBlock != nullptr);
        pTextView = pTextBlock->GetTextView();
        m_pGripperElementEnd->SetOwner(m_pOwnerUIElement);
        m_pGripperElementEnd->SetOwnerTextView(pTextView);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnKeyUp
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnKeyUp(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    HRESULT hr = S_OK;
    CKeyEventArgs *pKeyEventArgs = nullptr;
    ITransformer *pTransformer = nullptr;

    pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
    if (pKeyEventArgs->m_bHandled)
    {
        goto Cleanup;
    }

    if (pKeyEventArgs->m_platformKeyCode == TextContextMenu::ContextMenuKeyCode)
    {
        XPOINTF localPoint;
        XPOINTF worldPoint;
        IFC(pSender->TransformToRoot(&pTransformer));
        localPoint.x = 0.0f;
        localPoint.y = 0.0f;
        IFC(pTransformer->Transform(&localPoint, &worldPoint, 1));

        if (!m_pTextSelection->IsEmpty())
        {
            IFC(ShowContextMenu(
                worldPoint,
                FALSE /*isSelectionEmpty*/,
                FALSE /*isTouchInput*/));

            pKeyEventArgs->m_bHandled = TRUE;
        }
    }

Cleanup:
    ReleaseInterface(pTransformer);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnKeyDown
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnKeyDown(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CKeyEventArgs *pKeyEventArgs = nullptr;
    bool handled = false;

    pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
    if (pKeyEventArgs->m_bHandled)
    {
        return S_OK;
    }

    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));

    if (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_F10 && pKeyEventArgs->IsShiftPressed())
    {
        XPOINTF localPoint;
        XPOINTF worldPoint;
        xref_ptr<ITransformer> spTransformer;
        IFC_RETURN(pSender->TransformToRoot(spTransformer.ReleaseAndGetAddressOf()));
        localPoint.x = 0.0f;
        localPoint.y = 0.0f;
        IFC_RETURN(spTransformer->Transform(&localPoint, &worldPoint, 1));

        if (!m_pTextSelection->IsEmpty())
        {
            IFC_RETURN(ShowContextMenu(
                worldPoint,
                FALSE /*isSelectionEmpty*/,
                FALSE /*isTouchInput*/));

            handled = TRUE;
        }
    }

    if (pKeyEventArgs->m_xEditKey != XEDITKEY_NONE) // Is this a recognized edit keystroke/key combination?
    {
        switch (pKeyEventArgs->m_xEditKey)
        {
        case XEDITKEY_COPY:
            IFC_RETURN(CopySelectionToClipboard());
            handled = TRUE;
            break;
        case XEDITKEY_SELECT_ALL:
            IFC_RETURN(SelectAll());
            handled = TRUE;
            break;
        }
    }

    if  (GetCaretBrowsingModeEnable() && m_caretOwnerUIElementNoRef != nullptr &&
        (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Right ||
        pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Left ||
        pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Down ||
        pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Up ||
        pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_End ||
        pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Home))
    {
        CaretOnKeyDown(pSender, pEventArgs, pTextView);
        handled = TRUE;
    }

    if (handled)
    {
        pKeyEventArgs->m_bHandled = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::HWRender
//
//  Synopsis: Compute selection highlight geometry and render it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::HWRender(
    _In_ IContentRenderer* pContentRenderer,
    _In_ ITextView *pTextView,
    _In_ bool isHighContrast,
    _In_ uint32_t highlightRectCount,
    _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects,
    // Optional highlight rects that can be passed in if the caller is aware of them e.g. in high contrast mode where they are
    // also pushed to the drawing context. This avoids recalculation of the rects.
    bool isBackPlate
)
{
    HRESULT hr = S_OK;
    XRECTF *pSelectionHighlightRects = nullptr;
    CSolidColorBrush *pBackgroundBrush = nullptr;

    if ((isBackPlate && pHighlightRects != nullptr) ||
        (m_pTextSelection != nullptr && !m_pTextSelection->IsEmpty() &&
         IsSelectionVisible()))
    {
        uint32_t cSelectionHighlightRects = 0;
        CMILMatrix rpTransform;
        pContentRenderer->GetRenderParams()->pTransformsAndClipsToCompNode->Get2DTransformInLeafmostProjection(&rpTransform);
        bool allowsSnapToPixels = CTextBoxHelpers::TransformAllowsSnapToPixels(&rpTransform);

        if (pHighlightRects == nullptr)
        {
            ASSERT(!isBackPlate);

            // Highlight rects not passed in - query TextView.
            // Generate an array of text bound rectangles from the selection object.
            IFC(pTextView->TextSelectionToTextBounds(
                m_pTextSelection.get(),
                &cSelectionHighlightRects,
                &pSelectionHighlightRects
            ));
        }
        else
        {
            cSelectionHighlightRects = highlightRectCount;
            pSelectionHighlightRects = pHighlightRects;
        }

        if (isBackPlate)
        {
            IFC(GetSystemColorWindowBrush(&pBackgroundBrush));
        }
        else
        {
            IFC(GetSelectionHighlightColor(isHighContrast, &pBackgroundBrush));
        }

        for (uint32_t i = 0; i < cSelectionHighlightRects; i++)
        {
            XRECTF rect = pSelectionHighlightRects[i];

            // Check the applicable RenderTransform to see if it's practical to try to snap to pixels.
            if (allowsSnapToPixels)
            {
                IFC(CTextBoxHelpers::SnapRectToPixel(&rpTransform, 0, SelectionRectangle, &rect));
            }

            IFC(pContentRenderer->RenderSolidColorRectangle(
                rect,
                pBackgroundBrush
            ));
        }
    }

Cleanup:
    ReleaseInterface(pBackgroundBrush);
    if (pHighlightRects == nullptr)
    {
        delete[] pSelectionHighlightRects;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::D2DRender
//
//  Synopsis: Compute selection highlight geometry and render it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::D2DRender(
    _In_ const D2DRenderParams& rp,
    _In_ ITextView *pTextView,
    _In_ bool isHighContrast,
    _In_ uint32_t highlightRectCount,
    _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects
    // Optional highlight rects that can be passed in if the caller is aware of them e.g. in high contrast mode where they are
    // also pushed to the drawing context. This avoids recalculation of the rects.
)
{
    HRESULT hr = S_OK;
    XRECTF *pSelectionHighlightRects = nullptr;
    IPALAcceleratedBrush *pD2DBackgroundBrush = nullptr;
    CSolidColorBrush *pBackgroundBrush = nullptr;
    uint32_t selectColor = 0;

    if (m_pTextSelection != nullptr &&
        !m_pTextSelection->IsEmpty() &&
        IsSelectionVisible())
    {
        uint32_t cSelectionHighlightRects = 0;

        if (pHighlightRects == nullptr)
        {
            // Highlight rects not passed in - query TextView.
            // Generate an array of text bound rectangles from the selection object.
            IFC(pTextView->TextSelectionToTextBounds(
                m_pTextSelection.get(),
                &cSelectionHighlightRects,
                &pSelectionHighlightRects
            ));
        }
        else
        {
            cSelectionHighlightRects = highlightRectCount;
            pSelectionHighlightRects = pHighlightRects;
        }

        IFC(GetSelectionHighlightColor(isHighContrast, &pBackgroundBrush));
        selectColor = pBackgroundBrush->m_rgb;

        IFC(rp.GetD2DRenderTarget()->CreateSolidColorBrush(
            Premultiply(selectColor),
            1.0f,
            &pD2DBackgroundBrush
        ));

        for (uint32_t i = 0; i < cSelectionHighlightRects; i++)
        {
            IFC(rp.GetD2DRenderTarget()->FillRectangle(
                ToXRectFRB(const_cast<const XRECTF&>(pSelectionHighlightRects[i])),
                pD2DBackgroundBrush,
                1.0f));
        }
    }

Cleanup:
    ReleaseInterface(pBackgroundBrush);
    ReleaseInterface(pD2DBackgroundBrush);
    if (pHighlightRects == nullptr)
    {
        delete[] pSelectionHighlightRects;
    }
    return hr;
}

_Check_return_ HRESULT TextSelectionManager::UpdateLastSelectedTextElement()
{
    CUIElement *pOwner = m_pContainer->GetOwnerUIElement();
    CTextCore* pTextCore = nullptr;

    // If UpdateLastSelectedTextElement() was called as a result of
    // programatically setting the selection then we will not update
    // the last selected text element to this one since the selection
    // will not be visible if the control was not in focus.
    if (IsSelectionVisible())
    {
        IFC_RETURN(pOwner->GetContext()->GetTextCore(&pTextCore));
        if (!m_pTextSelection || m_pTextSelection->IsEmpty())
        {
            pTextCore->ClearLastSelectedTextElement();
        }
        else
        {
            pTextCore->SetLastSelectedTextElement(pOwner);
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifyContextMenuOpening
//
//  Synopsis: Notifies TextContainer owner that context menu in showing
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::NotifyContextMenuOpening(
    _In_ XPOINTF point,
    _In_ bool   isSelectionEmpty)
{
    CRichTextBlock *pRichTextBlock = nullptr;
    CTextBlock *pTextBlock = nullptr;
    EventHandle hEvent;
    CUIElement *pOwner = m_pContainer->GetOwnerUIElement();

    pRichTextBlock = do_pointer_cast<CRichTextBlock>(pOwner);
    pTextBlock = do_pointer_cast<CTextBlock>(pOwner);

    if (pRichTextBlock != nullptr)
    {
        hEvent.index = KnownEventIndex::RichTextBlock_ContextMenuOpening;
    }
    else if (pTextBlock != nullptr)
    {
        hEvent.index = KnownEventIndex::TextBlock_ContextMenuOpening;
    }
    else
    {
        // Currently only TextBlock and CRichTextBlock are using
        // the text selection Manager.
        IFCEXPECT_ASSERT_RETURN(FALSE);
    }

    if (!pOwner->GetContextFlyout())
    {
        IFC_RETURN(TextContextMenu::RaiseContextMenuOpeningEvent(
            hEvent,
            pOwner,
            point,
            false, // showCut
            !isSelectionEmpty, // showCopy
            false, // showPaste
            false, // showUndo,
            false, // showRedo,
            false  // showSelectAll
        ));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectedText
//
//  Synopsis: Gets selection serialized as plain text string.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetSelectedText(
    _Out_ xstring_ptr* pstrText
)
{
    if (m_strSelectedText.IsNull() &&
        m_pTextSelection != nullptr)
    {
        IFC_RETURN(m_pTextSelection->GetText(&m_strSelectedText));
    }

    *pstrText = m_strSelectedText;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectionBoundingRect
//
//  Synopsis: Gets the bounding rect for the selected text.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetSelectionBoundingRect(_Out_ wf::Rect* selectionBoundingRect)
{
    std::vector<XRECTF> highlightRectVector;
    wf::Rect selectionBoundingRectLocal = DirectUI::RectUtil::CreateEmptyRect();

    CUIElement* owner = m_pContainer->GetOwnerUIElement();

    if (auto textBlock = do_pointer_cast<CTextBlock>(owner))
    {
        XRECTF *highlightRects = nullptr;
        uint32_t highlightRectCount = 0;

        auto scopeGuard = wil::scope_exit([&highlightRects]()
        {
            if (highlightRects != nullptr)
            {
                delete[] highlightRects;
                highlightRects = nullptr;
            }
        });

        IFC_RETURN(GetSelectionHighlightRects(textBlock->GetTextView(), &highlightRectCount, &highlightRects));

        for (uint32_t i = 0; i < highlightRectCount; i++)
        {
            highlightRectVector.push_back(highlightRects[i]);
        }
    }
    // In the case of RichTextBlock, we need to aggregate the bounding boxes
    // of both it and all of its overflow elements.
    else if (auto richTextBlock = do_pointer_cast<CRichTextBlock>(owner))
    {
        xref_ptr<ITransformer> richTextBlockToRootTransform;
        xref_ptr<ITransformer> currentElementToRootTransform;
        IFC_RETURN(richTextBlock->TransformToRoot(richTextBlockToRootTransform.ReleaseAndGetAddressOf()));
        IFC_RETURN(richTextBlock->TransformToRoot(currentElementToRootTransform.ReleaseAndGetAddressOf()));

        RichTextBlockView* view = richTextBlock->GetSingleElementTextView();
        CRichTextBlockOverflow* overflow = static_cast<CRichTextBlockOverflow *>(richTextBlock->GetNext());

        while (view != nullptr)
        {
            XRECTF *highlightRects = nullptr;
            uint32_t highlightRectCount = 0;

            auto scopeGuard = wil::scope_exit([&highlightRects]()
            {
                if (highlightRects != nullptr)
                {
                    delete[] highlightRects;
                    highlightRects = nullptr;
                }
            });

            IFC_RETURN(GetSelectionHighlightRects(view, &highlightRectCount, &highlightRects));

            for (uint32_t i = 0; i < highlightRectCount; i++)
            {
                XRECTF highlightRect = highlightRects[i];

                // The bounds we get will all be in the element's local coordinate system,
                // so we'll transform them from there to the owning RichTextBlock's coordinates.
                CTransformer::TransformBounds(currentElementToRootTransform.get(), &highlightRect, &highlightRect);
                CTransformer::TransformBounds(richTextBlockToRootTransform.get(), &highlightRect, &highlightRect, TRUE /* fReverse */);

                highlightRectVector.push_back(highlightRect);
            }

            if (overflow != nullptr)
            {
                view = overflow->GetSingleElementTextView();
                IFC_RETURN(overflow->TransformToRoot(currentElementToRootTransform.ReleaseAndGetAddressOf()));

                overflow = static_cast<CRichTextBlockOverflow *>(overflow->GetNext());
            }
            else
            {
                view = nullptr;
            }
        }
    }

    for (XRECTF highlightRect : highlightRectVector)
    {
        selectionBoundingRectLocal.X = std::min(selectionBoundingRectLocal.X, static_cast<float>(highlightRect.X));
        selectionBoundingRectLocal.Y = std::min(selectionBoundingRectLocal.Y, static_cast<float>(highlightRect.Y));
        selectionBoundingRectLocal.Width = std::max(selectionBoundingRectLocal.Width, static_cast<float>(highlightRect.Width));
        selectionBoundingRectLocal.Height = std::max(selectionBoundingRectLocal.Height, static_cast<float>(highlightRect.Height));
    }

    *selectionBoundingRect = selectionBoundingRectLocal;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectionHighlightRects
//
//  Synopsis: Gets rectangles for selection highlight.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetSelectionHighlightRects(
    _In_ ITextView *pTextView,
    _Out_ uint32_t *pCount,
    _Outptr_result_buffer_(*pCount) XRECTF **ppRectangles
)
{
    IFC_RETURN(GetSelectionHighlightRectsInternal(pTextView, pCount, ppRectangles, m_pTextSelection.get()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetBackPlateSelectionHighlightRects
//
//  Synopsis: Gets rectangles for Back Plate.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetBackPlateSelectionHighlightRects(
    _In_ ITextView *pTextView,
    _Out_ uint32_t *pCount,
    _Outptr_result_buffer_(*pCount) XRECTF **ppRectangles
)
{
    CTextSelection* pBackPlateSelection = nullptr;
    std::unique_ptr<IJupiterTextSelection> pTextSelection;

    IFC_RETURN(CTextSelection::Create(m_pContainer, pTextView, nullptr, &pBackPlateSelection));

    pTextSelection.reset(pBackPlateSelection);
    pBackPlateSelection = nullptr;

    SelectAllText(pTextSelection.get(), false);

    IFC_RETURN(GetSelectionHighlightRectsInternal(pTextView, pCount, ppRectangles, pTextSelection.get()));

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::GetSelectionHighlightRectsInternal(
    _In_ ITextView *pTextView,
    _Out_ uint32_t *pCount,
    _Outptr_result_buffer_(*pCount) XRECTF **ppRectangles,
    _In_ IJupiterTextSelection *pTextSelection
)
{
    if (pTextSelection != nullptr &&
        !pTextSelection->IsEmpty())
    {
        // Generate an array of text bound rectangles from the selection object.
        IFC_RETURN(pTextView->TextSelectionToTextBounds(
            pTextSelection,
            pCount,
            ppRectangles
        ));
    }
    else
    {
        *pCount = 0;
        *ppRectangles = nullptr;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::SetSelectionHighlightColor
//
//  Synopsis: Set selection highlight.color.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::SetSelectionHighlightColor(_In_ uint32_t selectionColor)
{
    m_pSelectionBackground->m_rgb = selectionColor;
    // Adjust the color for grippers.
    if (m_pGripperElementStart && m_pGripperElementEnd)
    {
        IFC_RETURN(m_pGripperElementStart->CheckAndAdjustStrokeColor());
        IFC_RETURN(m_pGripperElementEnd->CheckAndAdjustStrokeColor());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectionHighlightRects
//
//  Synopsis: Gets the selection color appropriate for normal and high contrast modes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::
GetSelectionHighlightColor(
    bool isHighContrast,
    _Out_ CSolidColorBrush **ppSolidColorBrush)
{
    HRESULT hr = S_OK;
    CSolidColorBrush *pBackgroundBrush = nullptr;

    // If high contrast, ignore the property set by user. Always return the
    // system theme color.
    if (isHighContrast)
    {
        IFC(m_pOwnerUIElement->GetContext()->GetSystemTextSelectionBackgroundBrush(&pBackgroundBrush));
    }

    else
    {
        SetInterface(pBackgroundBrush, m_pSelectionBackground);
    }

    IFCPTR(pBackgroundBrush);
    *ppSolidColorBrush = pBackgroundBrush;
    pBackgroundBrush = nullptr;

Cleanup:
    ReleaseInterface(pBackgroundBrush);
    return hr;

}

_Check_return_ HRESULT TextSelectionManager::GetSystemColorWindowBrush(
    _Outptr_ CSolidColorBrush **ppSolidColorBrush)
{
    *ppSolidColorBrush = m_pOwnerUIElement->GetContext()->GetSystemColorWindowBrush();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::CopySelectionToClipboard
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::CopySelectionToClipboard()
{
    xstring_ptr strSelectedText;

    IFC_RETURN(GetSelectedText(&strSelectedText));
    if (!strSelectedText.IsNullOrEmpty())
    {
        IFC_RETURN(XAML_SetClipboardText(strSelectedText));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectionHighlightRegion
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetSelectionHighlightRegion(
    _In_ bool isHighContrast,
    _Out_ std::shared_ptr<HighlightRegion>& textSelection
)
{
    textSelection = nullptr;

    if (IsSelectionVisible())
    {
        CTextPosition startPosition;
        CTextPosition endPosition;
        unsigned int startOffset;
        unsigned int endOffset;

        // Get start and end offsets
        IFC_RETURN(GetTextSelection()->GetStartTextPosition(&startPosition));
        IFC_RETURN(GetTextSelection()->GetEndTextPosition(&endPosition));
        IFC_RETURN(startPosition.GetOffset(&startOffset));
        IFC_RETURN(endPosition.GetOffset(&endOffset));

        // Get foreground brush
        xref_ptr<CSolidColorBrush> selectionForegroundBrush;
        IFC_RETURN(m_pOwnerUIElement->GetContext()->GetSystemTextSelectionForegroundBrush(selectionForegroundBrush.ReleaseAndGetAddressOf()));

        // Get background brush -- accounts for high contrast mode
        xref_ptr<CSolidColorBrush> selectionBackgroundBrush;
        GetSelectionHighlightColor(isHighContrast, selectionBackgroundBrush.ReleaseAndGetAddressOf());

        textSelection = std::make_shared<HighlightRegion>(
            startOffset,
            endOffset-1, // Highlight ends are inclusive, selection ends are not
            selectionForegroundBrush,
            selectionBackgroundBrush);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::SelectAll
//
//  Synopsis: Select all content in TextContainer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::SelectAll()
{
    IFC_RETURN(SelectAllText(m_pTextSelection.get(), true));

    return S_OK;
}


_Check_return_ HRESULT TextSelectionManager::SelectAllText(
    _In_ IJupiterTextSelection* textSelection,
    bool sendSelectionChangedNotification)
{
    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
    CTextPosition startTextPosition;
    CTextPosition endTextPosition;
    uint32_t containerStartPosition = 0;
    uint32_t containerEndPosition;

    IFCEXPECT_ASSERT_RETURN(textSelection != nullptr);

    m_pContainer->GetPositionCount(&containerEndPosition);
    if (containerEndPosition > 0)
    {
        containerEndPosition--;
    }

    IFC_RETURN(textSelection->GetStartTextPosition(&startTextPosition));
    IFC_RETURN(textSelection->GetEndTextPosition(&endTextPosition));
    IFC_RETURN(startTextPosition.GetOffset(&startOffset));
    IFC_RETURN(endTextPosition.GetOffset(&endOffset));

    if (startOffset == containerStartPosition &&
        endOffset == containerEndPosition)
    {
        return S_OK;
    }

    // Select all content.
    IFC_RETURN(textSelection->Select(
        containerStartPosition,
        containerEndPosition,
        LineForwardCharacterBackward));

    if (sendSelectionChangedNotification)
    {
        IFC_RETURN(NotifySelectionChanged(
            startOffset,
            endOffset,
            TRUE /* fHideGrippers*/));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::Select
//
//  Synopsis: Select text in the specified range.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::Select(
    _In_ const CPlainTextPosition &anchorTextPosition,
    _In_ const CPlainTextPosition &movingTextPosition
)
{
    uint32_t startOffset = 0;
    uint32_t endOffset = 0;
    uint32_t selectionStartOffset = 0;
    uint32_t selectionEndOffset = 0;
    CTextPosition startTextPosition;
    CTextPosition endTextPosition;

    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);

    IFC_RETURN(anchorTextPosition.GetOffset(&startOffset));
    IFC_RETURN(movingTextPosition.GetOffset(&endOffset));
    IFC_RETURN(CTextBoxHelpers::VerifyPositionPair(m_pContainer, startOffset, endOffset));
    IFCEXPECT_ASSERT_RETURN(anchorTextPosition.GetTextContainer() == m_pContainer);
    IFCEXPECT_ASSERT_RETURN(movingTextPosition.GetTextContainer() == m_pContainer);

    IFC_RETURN(m_pTextSelection->GetStartTextPosition(&startTextPosition));
    IFC_RETURN(m_pTextSelection->GetEndTextPosition(&endTextPosition));
    IFC_RETURN(startTextPosition.GetOffset(&selectionStartOffset));
    IFC_RETURN(endTextPosition.GetOffset(&selectionEndOffset));

    if (startOffset == selectionStartOffset &&
        endOffset == selectionEndOffset)
    {
        // Everything's already selected
        return S_OK;
    }

    // Select all content.
    IFC_RETURN(m_pTextSelection->Select(
        startOffset,
        endOffset,
        LineForwardCharacterBackward));

    IFC_RETURN(NotifySelectionChanged(
        selectionStartOffset,
        selectionEndOffset,
        TRUE /* fHideGrippers*/));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::IsPointOverElement
//
//  Synopsis: Check a global mouse over point and establish whether it's
//            over an given element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::IsPointOverElement(
    _In_  CUIElement      *pElement,
    _In_  const XPOINTF   *pHitPoint,
    _Out_ XPOINTF         *pLocalMousePoint,
    _Out_ bool           *pIsPointOverElement,
    _Out_opt_ XFLOAT      *pDistance
)
{
    HRESULT hr = S_OK;
    ITransformer *pTransformer = nullptr;
    XPOINTF localPoint = { 0 };
    bool isPointOverElement = false;
    XFLOAT distance = 0.0f;

    IFC(pElement->TransformToRoot(&pTransformer));
    IFC(pTransformer->ReverseTransform(pHitPoint, &localPoint, 1));

    if (pElement->HasLayoutStorage())
    {
        // If point is not contained within element's bounds, calculate its closest distance.
        // If the hit-test point horizontally or vertically intersects with the bounding rectangle,
        // the closest distance is the distance to the closest edge.
        // Otherwise, the closest distance is the distance to the closest corner.
        XRECTF_RB elementBounds = { 0.0f, 0.0f, pElement->RenderSize.width, pElement->RenderSize.height };
        if (localPoint.x >= elementBounds.left && localPoint.x <= elementBounds.right)
        {
            if (localPoint.y >= elementBounds.top && localPoint.y <= elementBounds.bottom)
            {
                isPointOverElement = TRUE;
            }
            else if (pDistance != nullptr)
            {
                distance = MIN(XcpAbsF(elementBounds.top - localPoint.y), XcpAbsF(elementBounds.bottom - localPoint.y));
            }
        }
        else if (pDistance != nullptr)
        {
            if (localPoint.y >= elementBounds.top && localPoint.y <= elementBounds.bottom)
            {
                distance = MIN(XcpAbsF(elementBounds.left - localPoint.x), XcpAbsF(elementBounds.right - localPoint.x));
            }
            else
            {
                XPOINTF closestCorner = {
                    localPoint.x < elementBounds.left ? elementBounds.left : elementBounds.right,
                    localPoint.y < elementBounds.top ? elementBounds.top : elementBounds.bottom
                };
                distance = Distance(localPoint, closestCorner);
            }
        }
    }

    *pIsPointOverElement = isPointOverElement;
    *pLocalMousePoint = localPoint;
    if (pDistance != nullptr)
    {
        *pDistance = distance;
    }

Cleanup:
    ReleaseInterface(pTransformer);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::IsPointOverLinkedView
//
//  Synopsis: Check a global mouse over point and establish whether it's
//            over a linked view of the sender. If it is, return the
//            linked view for hit testing use. If not, return the sender's
//            view.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::IsPointOverLinkedView(
    _In_        CUIElement      *pSender,
    _In_        ITextView       *pSenderView,
    _In_        const XPOINTF   *pHitPoint,
    _Out_opt_   XPOINTF         *pLocalMousePoint,
    _Outptr_ ITextView      **ppPointerOverView,
    _Outptr_result_maybenull_ CUIElement **ppTargetUIElement
)
{
    HRESULT hr = S_OK;
    ITextView *pPointerOverView = nullptr;
    CUIElement *pElement = pSender;
    CRichTextBlock *pRichTextBlock = nullptr;
    CRichTextBlockOverflow *pRichTextBlockOverflow = nullptr;
    ITransformer *pTransformer = nullptr;
    XPOINTF localPoint = { 0 };
    bool isPointOverElement;

    if (auto elementCastRichTextBlock = do_pointer_cast<CRichTextBlock>(pElement))
    {
        pRichTextBlock = elementCastRichTextBlock;
    }
    else if (auto elementCastRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pElement))
    {
        pRichTextBlockOverflow = elementCastRichTextBlockOverflow;
        pRichTextBlock = pRichTextBlockOverflow->GetMaster();
    }

    // If the sender is a CRichTextBlock/Overflow, see if the mouse over point is anywhere in
    // the linked chain.
    if (pRichTextBlock)
    {
        IFC(IsPointOverElement(
            pSender,
            pHitPoint,
            &localPoint,
            &isPointOverElement,
            nullptr));

        if (isPointOverElement)
        {
            pElement = pSender;
            pPointerOverView = pSenderView;
        }
        else
        {
            // At this point we are not directly over the sender of the pointer message,
            // and need to iterate though linked text containers to find the element which contains the point.
            // There is a chance that none of the containers contains the point, in such case during
            // iteration process we store the closest container. This container will be returned as match in this case.
            XFLOAT closestElementDistance = XFLOAT_MAX;
            XFLOAT distance = 0.0f;
            CUIElement *pClosestElement = nullptr;

            pElement = pRichTextBlock;
            pRichTextBlockOverflow = nullptr;
            while (pElement != nullptr)
            {
                IFC(IsPointOverElement(
                    pElement,
                    pHitPoint,
                    &localPoint,
                    &isPointOverElement,
                    &distance));

                if (isPointOverElement)
                {
                    if (pRichTextBlockOverflow != nullptr)
                    {
                        pPointerOverView = reinterpret_cast<ITextView *>(pRichTextBlockOverflow->GetSingleElementTextView());
                    }
                    else
                    {
                        pPointerOverView = reinterpret_cast<ITextView *>(pRichTextBlock->GetSingleElementTextView());
                    }

                    // If the container that has this point does not have a content,
                    // continue with the search process to find the closest non-empty container.
                    if (pPointerOverView != nullptr)
                    {
                        break;
                    }
                }
                else
                {
                    if (closestElementDistance > distance)
                    {
                        if (pRichTextBlockOverflow == nullptr ||
                            pRichTextBlockOverflow->GetSingleElementTextView() != nullptr)
                        {
                            closestElementDistance = distance;
                            pClosestElement = pElement;
                        }
                    }
                }

                if (pRichTextBlockOverflow != nullptr)
                {
                    pElement = pRichTextBlockOverflow->m_pOverflowTarget;
                    pRichTextBlockOverflow = pRichTextBlockOverflow->m_pOverflowTarget;
                }
                else
                {
                    pElement = pRichTextBlock->m_pOverflowTarget;
                    pRichTextBlockOverflow = pRichTextBlock->m_pOverflowTarget;
                }
            }

            if (pElement == nullptr)
            {
                ASSERT(pClosestElement != nullptr);
                ASSERT(pPointerOverView == nullptr);

                pElement = pClosestElement;
                if (pClosestElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
                {
                    pPointerOverView = reinterpret_cast<ITextView *>(pRichTextBlock->GetSingleElementTextView());
                }
                else
                {
                    ASSERT(pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>());
                    pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pElement);
                    pPointerOverView = reinterpret_cast<ITextView *>(pRichTextBlockOverflow->GetSingleElementTextView());
                }

                IFC(pElement->TransformToRoot(&pTransformer));
                IFC(pTransformer->ReverseTransform(pHitPoint, &localPoint, 1));
            }
        }
    }
    else
    {
        pElement = pSender;
        pPointerOverView = pSenderView;

        ASSERT(pElement->OfTypeByIndex<KnownTypeIndex::TextBlock>());
        IFC(pElement->TransformToRoot(&pTransformer));
        IFC(pTransformer->ReverseTransform(pHitPoint, &localPoint, 1));
    }

    if (pLocalMousePoint)
    {
        *pLocalMousePoint = localPoint;
    }

    *ppPointerOverView = pPointerOverView;

    if (ppTargetUIElement)
    {
        *ppTargetUIElement = pElement;
    }

Cleanup:
    ReleaseInterface(pTransformer);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifyGripperPositionChanged
//
//  Synopsis: Handler for Gripper move.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::NotifyGripperPositionChanged(
    _In_ CTextSelectionGripper *pGripper,
    _In_ XPOINTF                worldPos,
    _In_ int32_t                pointerId)
{
    TraceNotifyGripperPositionChangedBegin();
    HRESULT     hr = S_OK;
    uint32_t     pointerMoveOffset;
    TextGravity gravity = LineForwardCharacterBackward;
    ITextView  *pTappedView = nullptr;
    CUIElement *pTargetUIElement = nullptr;
    XPOINTF     localPos;
    CTextPosition startPosition;
    CTextPosition endPosition;
    SelectionRange<uint32_t> currentSelection;
    bool       isStartGripper = (pGripper == m_pGripperElementStart);

    // If we have mouse capture, in a linked scenario the mouse may be captured by
    // one link but actually be over another link. In that case, we should hit test the
    // linked view, because we want to extend selection.
    IFC(IsPointOverLinkedView(pGripper->GetOwner(),
                              pGripper->GetOwnerTextView(),
                              &worldPos,
                              &localPos,
                              &pTappedView,
                              &pTargetUIElement));

    pGripper->SetOwner(
        pTargetUIElement,
        TRUE //isGripperMoving
    );
    pGripper->SetOwnerTextView(pTappedView);

    IFC(pTappedView->PixelPositionToTextPosition(
        localPos,          // pixel offset
        FALSE,             // Recognise hits after newline
        &pointerMoveOffset,
        &gravity));

    IFC(m_pTextSelection->GetStartTextPosition(&startPosition));
    IFC(m_pTextSelection->GetEndTextPosition(&endPosition));

    IFC(startPosition.GetOffset(&currentSelection.begin));
    IFC(endPosition.GetOffset(&currentSelection.end));

    ASSERT(m_pGripperElementStart != nullptr);
    ASSERT(m_pGripperElementEnd != nullptr);

    if (isStartGripper)
    {
        auto gripperSide = GripperSide::Left;
        // Crossover occurred, normalize the Grippers
        if (pointerMoveOffset > currentSelection.end)
        {
            SwapGrippers();
            isStartGripper = !isStartGripper;
            currentSelection.begin = currentSelection.end;
            currentSelection.end = pointerMoveOffset;
            gripperSide = GripperSide::Right;
        }
        // Prevent empty selections.
        else if (pointerMoveOffset == currentSelection.end)
        {
            pointerMoveOffset = currentSelection.end - 1;
        }

        IFC(ChangeSelection(currentSelection, gripperSide, pointerMoveOffset));
    }
    else
    {
        auto gripperSide = GripperSide::Right;
        // Crossover occurred, normalize the Grippers
        if (pointerMoveOffset < currentSelection.begin)
        {
            SwapGrippers();
            isStartGripper = !isStartGripper;
            currentSelection.end = currentSelection.begin;
            currentSelection.begin = pointerMoveOffset;
            gripperSide = GripperSide::Left;
        }
        // Prevent empty selections.
        else if (pointerMoveOffset == currentSelection.begin)
        {
            pointerMoveOffset = currentSelection.begin + 1;
        }

        IFC(ChangeSelection(currentSelection, gripperSide, pointerMoveOffset));
    }

    if ((pGripper->GetOwner() != nullptr) &&
        (pGripper->GetOwnerTextView() != nullptr))
    {
        // Update the current gripper coordinates based on its calculated text position (pointerMoveOffset)
        // rather than the actual start or end of the resulting text selection.
        XFLOAT      lineHeight;
        XPOINTF     centerWorldCoordinate;

        IFC(TextSelectionManager::GetGripperCoordinatesForTextOffset(
            isStartGripper,                                                                 /*fBegin*/
            TRUE,                                                                           /*fAdjustForGripperPositioning*/
            pointerMoveOffset,                                                              /*offset*/
            isStartGripper ? LineForwardCharacterForward : LineBackwardCharacterBackward,   /*gravity*/
            &centerWorldCoordinate,                                                         /*pCenterWorldCoordinate*/
            nullptr,                                                                        /*pLineWorldCoordinate*/
            &lineHeight));                                                                  /*pLineHeight*/

        IFC(pGripper->UpdateCenterWorldCoordinate(centerWorldCoordinate, lineHeight));
    }

    IFC(NotifySelectionChanged(
        currentSelection.begin,
        currentSelection.end,
        FALSE /* fHideGrippers*/));

Cleanup:
    TraceNotifyGripperPositionChangedEnd();
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::NotifyGripperPressed
//
//  This method is called every time one gripper is pressed
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::NotifyGripperPressed(_In_ CTextSelectionGripper* pGripper)
{
    CTextPosition selectionStart;
    CTextPosition selectionEnd;
    SelectionRange<uint32_t> selectionRange;
    IFC_RETURN(EnsureGrippers());
    IFC_RETURN(EnsureSnappingCalculator());

    // Since user has grabbed a gripper, suppress showing the copy
    // button until SetActiveGripper is called when the griper is released.
    m_interacting = TRUE;

    IFC_RETURN(m_pTextSelection->GetStartTextPosition(&selectionStart));
    IFC_RETURN(m_pTextSelection->GetEndTextPosition(&selectionEnd));
    IFC_RETURN(selectionStart.GetOffset(&selectionRange.begin));
    IFC_RETURN(selectionEnd.GetOffset(&selectionRange.end));
    m_pSnappingCalculator->InitiateSelection(selectionRange, (pGripper == m_pGripperElementStart) ? GripperSide::Left : GripperSide::Right);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::EnsureSnappingCalculator
//
//  Synopsis: Creates the snapping calculator if it wasn't created yet
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::EnsureSnappingCalculator()
{
    HRESULT hr = S_OK;

    if (m_pSnappingCalculator == nullptr)
    {
        m_pSnappingCalculator = new CSnappingCalculator<uint32_t, TextSelectionManager>(*this,
            &TextSelectionManager::ArePositionsInSameWord);
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::SnapGrippersToSelection
//
//  Synopsis: Called by the gripper when touch is released to snap
//            both the grippers back to the corners of the selection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::SnapGrippersToSelection()
{
    IFC_RETURN(ShowGrippers(TRUE /* fAnimate */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::UpdateStartGripperPosition
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::UpdateStartGripperPosition()
{
    IFC_RETURN(UpdateGripperPosition(TRUE));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::UpdateEndGripperPosition
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::UpdateEndGripperPosition()
{
    IFC_RETURN(UpdateGripperPosition(FALSE));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::UpdateGripperPosition
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::UpdateGripperPosition(
    _In_  bool     isFirstGripper)
{
    XPOINTF centerWorldCoordinate;
    XPOINTF lineCenterWorldCoordinate;
    XFLOAT lineHeight;
    CTextSelectionGripper *pGripper = isFirstGripper ? m_pGripperElementStart : m_pGripperElementEnd;
    if ((pGripper->GetOwner() != nullptr) &&
        (pGripper->GetOwnerTextView() != nullptr))
    {
        IFC_RETURN(GetSelectionCoordinates(isFirstGripper, TRUE, &centerWorldCoordinate, &lineCenterWorldCoordinate, &lineHeight));
        IFC_RETURN(pGripper->UpdateCenterWorldCoordinate(centerWorldCoordinate, lineHeight));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetSelectionCoordinates
//
//  Synopsis: Retrieves the selection coordinates needed for the gripper
//            placement and shaping, and UDWM tether and safety zone
//            algorithm.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetSelectionCoordinates(
    _In_ bool         fBegin,
    _In_ bool         fAdjustForGripperPositioning,
    _Out_ XPOINTF     *pWorldCoordinate,
    _Out_opt_ XPOINTF *pLineWorldCoordinate,
    _Out_opt_ XFLOAT  *pLineHeight)
{
    uint32_t          offset = 0;
    TextGravity      gravity = LineForwardCharacterForward;
    CTextPosition    position;

    IFCEXPECT_ASSERT_RETURN(m_pTextSelection != nullptr);
    if (fBegin)
    {
        gravity = m_pTextSelection->GetStartGravity();
        IFC_RETURN(m_pTextSelection->GetStartTextPosition(&position));
        IFC_RETURN(position.GetOffset(&offset));
    }
    else
    {
        gravity = m_pTextSelection->GetEndGravity();
        IFC_RETURN(m_pTextSelection->GetEndTextPosition(&position));
        IFC_RETURN(position.GetOffset(&offset));
    }

    IFC_RETURN(TextSelectionManager::GetGripperCoordinatesForTextOffset(
        fBegin,
        fAdjustForGripperPositioning,
        offset,
        gravity,
        pWorldCoordinate,
        pLineWorldCoordinate,
        pLineHeight));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetGripperCoordinatesForTextOffset
//
//  Synopsis: Retrieves the gripper placement coordinates for placement, shaping,
//            UDWM tether and safety zone algorithm based on text offset
//            and gravity.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetGripperCoordinatesForTextOffset(
    _In_ bool         fBegin,                      // Gripper side
    _In_ bool         fAdjustForGripperPositioning,// Adjust the adjustment for gripper size (tether does not need it)
    _In_ uint32_t       offset,                      // Text offset
    _In_ TextGravity   gravity,                     // Text gravity
    _Out_ XPOINTF     *pWorldCoordinate,            // Resulting world coordinate of the text position
    _Out_opt_ XPOINTF *pLineWorldCoordinate,        // Resulting line coordinate
    _Out_opt_ XFLOAT  *pLineHeight)                 // Resulting line height (for gripper pole size)
{
    HRESULT          hr = S_OK;
    XFLOAT           ePixelOffset = 0.0f;     // Relative to origin of line
    XFLOAT           eCharacterTop = 0.0f;     // Relative to TextView top
    XFLOAT           eCharacterHeight = 0.0f;
    XFLOAT           eLineTop = 0.0f;     // Relative to TextView top
    XFLOAT           eLineHeight = 0.0f;
    XFLOAT           eLineBaseline = 0.0f;
    XFLOAT           eLineOffset = 0.0f;     // Padding and alignment offset
    CTextSelectionGripper *pGripper = nullptr;
    XPOINTF          localCoordinate;
    XPOINTF          worldCoordinate;
    XPOINTF          lineLocalCoordinate;
    XPOINTF          lineWorldCoordinate;
    ITransformer    *pTransformer = nullptr;

    if (fBegin)
    {
        pGripper = m_pGripperElementStart;
    }
    else
    {
        pGripper = m_pGripperElementEnd;

        bool contains = false;
        IFC(pGripper->GetOwnerTextView()->ContainsPosition(offset, gravity, &contains));
        if (!contains)
        {
            offset = pGripper->GetOwnerTextView()->GetContentStartPosition() + pGripper->GetOwnerTextView()->GetContentLength();
            gravity = static_cast<TextGravity>(gravity | CharacterBackward);
        }
    }

    ASSERT(pGripper != nullptr);
    ASSERT(pGripper->GetOwner() != nullptr);
    ASSERT(pGripper->GetOwnerTextView() != nullptr);

    IFC(pGripper->GetOwnerTextView()->TextPositionToPixelPosition(
        offset,
        gravity,
        &ePixelOffset,
        &eCharacterTop,
        &eCharacterHeight,
        &eLineTop,
        &eLineHeight,
        &eLineBaseline,
        &eLineOffset));

    localCoordinate.x = ePixelOffset + eLineOffset;
    localCoordinate.y = eLineTop + eLineHeight;

    if (fAdjustForGripperPositioning)
    {
        localCoordinate.y += pGripper->GetGripperCenterToLineEdgeOffset();
    }

    lineLocalCoordinate.x = localCoordinate.x;
    lineLocalCoordinate.y = eLineTop + (eLineHeight / 2);

    IFC(pGripper->GetOwner()->TransformToRoot(&pTransformer));

    IFC(pTransformer->Transform(&lineLocalCoordinate, &lineWorldCoordinate, 1));
    IFC(pTransformer->Transform(&localCoordinate, &worldCoordinate, 1));

    *pWorldCoordinate = worldCoordinate;
    if (pLineWorldCoordinate != nullptr)
    {
        *pLineWorldCoordinate = lineWorldCoordinate;
    }

    if (pLineHeight != nullptr)
    {
        *pLineHeight = eLineHeight;
    }

Cleanup:
    ReleaseInterface(pTransformer);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ShowGrippers
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::ShowGrippers(_In_ bool fAnimate)
{
    if (!IsSelectionVisible() || m_pTextSelection->IsEmpty())
    {
        return S_OK;
    }

    IFC_RETURN(EnsureGrippers());
    IFC_RETURN(UpdateGripperParents());

    // Make sure the grippers are assigned the proper semantic, as they could
    // have been swapped previously.
    m_pGripperElementStart->SetStartGripper(TRUE /*isStart*/);
    m_pGripperElementEnd->SetStartGripper(FALSE /*isStart*/);

    IFC_RETURN(UpdateStartGripperPosition());
    IFC_RETURN(m_pGripperElementStart->Show(fAnimate));
    IFC_RETURN(UpdateEndGripperPosition());
    IFC_RETURN(m_pGripperElementEnd->Show(fAnimate));

    m_endGripperLastMoved = m_pGripperActive == m_pGripperElementEnd;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::HideGrippers
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::HideGrippers(bool fAnimate)
{
    if (m_pGripperElementStart)
    {
        IFC_RETURN(m_pGripperElementStart->Hide(fAnimate));
        IFC_RETURN(m_pGripperElementEnd->Hide(fAnimate));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetTextViewForRichTextBlock
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::GetTextViewForRichTextBlock(
    _In_        uint32_t      iTextPosition,
    _In_        TextGravity  gravity,
    _Outptr_ ITextView  **ppView,
    _Outptr_ CUIElement **ppOwnerElement
)
{
    CRichTextBlock         *pRichTextBlock = nullptr;
    RichTextBlockView     *pView = nullptr;
    CRichTextBlockOverflow *pOverflow = nullptr;
    CUIElement            *pTargetUIElement = nullptr;
    bool contains = false;

    ASSERT(m_pOwnerUIElement != nullptr);
    ASSERT(m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>());
    pRichTextBlock = do_pointer_cast<CRichTextBlock>(m_pOwnerUIElement);

    if (pRichTextBlock)
    {
        pView = pRichTextBlock->GetSingleElementTextView();
        pTargetUIElement = pRichTextBlock;
        pOverflow = static_cast<CRichTextBlockOverflow *>(pRichTextBlock->GetNext());

        while (pView != nullptr)
        {
            IFC_RETURN(pView->ContainsPosition(
                iTextPosition,
                gravity,
                &contains));

            if (contains)
            {
                *ppView = pView;
                *ppOwnerElement = pTargetUIElement;
                break;
            }

            if (pOverflow != nullptr)
            {
                pView = pOverflow->GetSingleElementTextView();
                pTargetUIElement = pOverflow;
                pOverflow = static_cast<CRichTextBlockOverflow *>(pOverflow->GetNext());
            }
            else
            {
                *ppView = pView;
                *ppOwnerElement = pTargetUIElement;
                break;
            }
        }
    }
    else
    {
        ASSERT(FALSE);
        *ppView = nullptr;
        *ppOwnerElement = nullptr;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ShowContextMenu
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::ShowContextMenu(
    _In_ XPOINTF point,
    _In_ bool   isSelectionEmpty,
    _In_ bool   showGrippersOnDismiss)
{
    IFC_RETURN(HideGrippers(TRUE /* fAnimate */));

    IFC_RETURN(NotifyContextMenuOpening(point, isSelectionEmpty));
    m_showGrippersOnCMDismiss = showGrippersOnDismiss;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::OnContextMenuDismiss
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::OnContextMenuDismiss()
{
    if (m_showGrippersOnCMDismiss)
    {
        m_showGrippersOnCMDismiss = FALSE;
        IFC_RETURN(ShowGrippers(TRUE /* fAnimate */));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::SwapGrippers
//
//------------------------------------------------------------------------

void TextSelectionManager::SwapGrippers()
{
    CTextSelectionGripper *pTempGripper = m_pGripperElementStart;
    m_pGripperElementStart = m_pGripperElementEnd;
    m_pGripperElementEnd = pTempGripper;
    m_pGripperElementStart->SetStartGripper(TRUE  /*isStart*/);
    m_pGripperElementEnd->SetStartGripper(FALSE /*isStart*/);
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::IsSelectionVisible
//
//------------------------------------------------------------------------

bool TextSelectionManager::IsSelectionVisible() const
{
    ASSERT(m_pOwnerUIElement != nullptr);
    bool isSelectionVisible = m_pOwnerUIElement->IsFocused() || m_forceFocusedVisualState;

    if (!isSelectionVisible)
    {
        if (CRichTextBlock *pRichTextBlock = do_pointer_cast<CRichTextBlock>(m_pOwnerUIElement))
        {
            CRichTextBlockOverflow *pOverflowTarget = pRichTextBlock->m_pOverflowTarget;

            while (pOverflowTarget != nullptr)
            {
                if (pOverflowTarget->IsFocused())
                {
                    isSelectionVisible = TRUE;
                    break;
                }

                pOverflowTarget = pOverflowTarget->m_pOverflowTarget;
            }
        }
    }

    return isSelectionVisible;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ChangeSelection
//
//  Changes the selection range
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::ChangeSelection(
    _In_ const SelectionRange<uint32_t>& currentRange,
    _In_ GripperSide gripperSide,
    _In_ uint32_t newPosition)
{
    TraceChangeSelectionBegin();
    ASSERT(gripperSide != GripperSide::Undefined);
    HRESULT hr = S_OK;
    SelectionRange<uint32_t> newRange = currentRange;

    TraceComputeSnappingModeBegin();
    auto snappingMode = m_pSnappingCalculator->ComputeSnappingMode(gripperSide, newPosition);
    TraceComputeSnappingModeEnd();

    IFC(ExtendSelectionRange(snappingMode, gripperSide, newPosition, currentRange, &newRange));

    // If there is non-visible text in a text block, the wordbreaker will not go past the visible part
    // In this case, we may end up generating an empty selection because the word boundary may be in a non-visible part,
    // and the word boundary will cut to the last visible character
    // To prevent this empty selection, we'll bail from changing the selection in this case
    if (newRange.begin == newRange.end)
    {
        goto Cleanup;
    }

    IFC(m_pTextSelection->Select(newRange.begin, newRange.end, c_defaultGravity));

    // Updating gripper drawing positions if they have changed
    if ((gripperSide == GripperSide::Right) && (currentRange.begin != newRange.begin))
    {
        IFC(UpdateStartGripperPosition());
    }

    if ((gripperSide == GripperSide::Left) && (currentRange.end != newRange.end))
    {
        IFC(UpdateEndGripperPosition());
    }

Cleanup:
    TraceChangeSelectionEnd();
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ExtendSelectionRange
//
//  Changes the selection boundaries according to the given snapping mode
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::ExtendSelectionRange(
    _In_ SnappingMode snappingMode,
    _In_ GripperSide gripperSide,
    _In_ uint32_t newPosition,
    _In_ const SelectionRange<uint32_t>& currentRange,
    _Out_ SelectionRange<uint32_t>* newRange)
{
    TraceExtendSelectionRangeBegin();
    HRESULT hr = S_OK;
    *newRange = currentRange;
    if (gripperSide == GripperSide::Left)
    {
        newRange->begin = newPosition;
    }
    else if (gripperSide == GripperSide::Right)
    {
        newRange->end = newPosition;
    }

    switch (snappingMode)
    {
    case SnappingMode::Word:
        if (gripperSide == GripperSide::Left)
        {
            IFC(GetStartOfWordInPosition(newRange->begin, &newRange->begin));
        }
        else if (gripperSide == GripperSide::Right)
        {
            IFC(GetEndOfWordInPosition(newRange->end - 1, &newRange->end));
        }
        break;
    case SnappingMode::Char:
        // Selection already changed
        break;
    default:
        ASSERT(false);
        IFC(E_UNEXPECTED);
    }

Cleanup:
    TraceExtendSelectionRangeEnd();
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetStartOfWordInPosition
//
//  Returns the offset of the first word boundary, backward
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::GetStartOfWordInPosition(_In_ uint32_t position, _Out_ uint32_t* wordStart) const
{
    return GetBoundaryOfWordInPosition(position, FindBoundaryType::Backward, wordStart);
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetEndOfWordInPosition
//
//  Returns the offset of the first word boundary, forward
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::GetEndOfWordInPosition(_In_ uint32_t position, _Out_ uint32_t* wordEnd) const
{
    return GetBoundaryOfWordInPosition(position, FindBoundaryType::ForwardIncludeTrailingWhitespace, wordEnd);
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::GetBoundaryOfWordInPosition
//
//  Returns the offset of the first word boundary, in the given direction
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextSelectionManager::GetBoundaryOfWordInPosition(
    _In_ uint32_t position,
    _In_ FindBoundaryType findType,
    _Out_ uint32_t* boundary) const
{
    CTextPosition original(CPlainTextPosition(m_pContainer, position, c_defaultGravity));
    CTextPosition wordBoundary;
    IFC_RETURN(CTextBoxHelpers::GetAdjacentWordSelectionBoundaryPosition(m_pContainer, original, findType, TagConversion::Default, &wordBoundary, nullptr));
    IFC_RETURN(wordBoundary.GetOffset(boundary));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ArePositionsInSameWord
//
//  Returns whether two positions are in the same word
//  Note: This is a callback, called by CSnappingCalculator::ComputeSnappingMode
//
//------------------------------------------------------------------------

bool TextSelectionManager::ArePositionsInSameWord(_In_ uint32_t begin, _In_ uint32_t end)
{
    return ArePositionsInSameWordBoundary(begin, end, &TextSelectionManager::GetStartOfWordInPosition) &&
        ArePositionsInSameWordBoundary(begin, end, &TextSelectionManager::GetEndOfWordInPosition);
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::ArePositionsInSameWordBoundary
//
//  Returns whether two positions are in the same word boundary (boundary dependent on function passed)
//
//------------------------------------------------------------------------

bool TextSelectionManager::ArePositionsInSameWordBoundary(
    _In_ uint32_t first,
    _In_ uint32_t second,
    _In_ HRESULT(TextSelectionManager::*GetBoundaryOfWordInPosition)(_In_ uint32_t, _Out_ uint32_t*) const) const
{
    bool areInSameBoundary = true; // This will disable snapping on failure
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    uint32_t firstEnd;
    uint32_t secondEnd;
    IFC((this->*GetBoundaryOfWordInPosition)(first, &firstEnd));
    IFC((this->*GetBoundaryOfWordInPosition)(second, &secondEnd));
    areInSameBoundary = (firstEnd == secondEnd);

Cleanup:
    return areInSameBoundary;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::GetOtherGripper
//
//  Synopsis: Returns the pointer to the other gripper, i.e. if the given one
//            is start, then returns the end gripper and vice versa.
//
//------------------------------------------------------------------------
CTextSelectionGripper *TextSelectionManager::GetOtherGripper(_In_ const CTextSelectionGripper *pGripper) const
{
    if (pGripper == m_pGripperElementEnd)
    {
        return m_pGripperElementStart;
    }
    else
    {
        ASSERT(pGripper == m_pGripperElementStart);
        return m_pGripperElementEnd;
    }
}

//------------------------------------------------------------------------
//
//  Method:   AreCachedSelectionHighlightOffsetsEqual
//
//  Synopsis:
//      Compares two selection regions.
//      Returns true if both sets match exactly, including if both are empty.
//
//------------------------------------------------------------------------
/* static */
bool TextSelectionManager::AreSelectionHighlightOffsetsEqual(
    _In_ std::shared_ptr<HighlightRegion> oldSelection,
    _In_ std::shared_ptr<HighlightRegion> newSelection
)
{
    if (!oldSelection && !newSelection)
    {
        return true;
    }
    if (!oldSelection || !newSelection) {
        return false;
    }
    if (oldSelection->startIndex != newSelection->startIndex)
    {
        return false;
    }
    if (oldSelection->endIndex != newSelection->endIndex)
    {
        return false;
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::UpdateGripperPositions
//
//  Synopsis: Refreshes the gripper screen coordinates
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::UpdateGripperPositions()
{
    if ((m_pGripperElementStart != nullptr) &&
        (m_pGripperElementEnd != nullptr))
    {
        // Parent ITextView pointers saved in grippers may become stale
        // due to layout changes. Update them first.
        IFC_RETURN(UpdateGripperParents());

        IFC_RETURN(UpdateGripperPosition(TRUE  /*isStart*/));
        IFC_RETURN(UpdateGripperPosition(FALSE /*isStart*/));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::SetActiveGripper
//
//  Synopsis: Sets the active gripper allowing us to show copy icon in correct position.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::SetActiveGripper(_In_ CTextSelectionGripper *pActiveGripper)
{
    m_pGripperActive = pActiveGripper;

    // Active gripper is set when it is released, meaning it is
    // okay to show the copy button again.
    m_interacting = false;

    IFC_RETURN(QueueUpdateSelectionFlyoutVisibility());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionManager::ClearSelection
//
//  Synopsis: Clears the current selection and places the caret in the given position
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TextSelectionManager::ClearSelection(_In_ const CTextPosition& caretPosition)
{
    IFC_RETURN(m_pTextSelection->SetCaretPositionFromTextPosition(caretPosition));
    IFC_RETURN(HideGrippers(TRUE /* fAnimate */));

    return S_OK;
}

// <summary>
// Sets selection to the word at 'position'.
// </summary>
_Check_return_ HRESULT TextSelectionManager::SetSelectionToWord(_In_ const CTextPosition& position)
{
    CTextPosition closestNonWhitespace;

    IFC_RETURN(CTextBoxHelpers::GetClosestNonWhitespaceWordBoundary(
        m_pContainer,
        position,
        TagConversion::Default,
        &closestNonWhitespace));

    IFC_RETURN(CTextBoxHelpers::SelectWordFromTextPosition(
        m_pContainer,
        closestNonWhitespace,
        m_pTextSelection.get(),
        FindBoundaryType::ForwardExact,
        TagConversion::Default));

    IFC_RETURN(ShowGrippers(true /* fAnimate */));
    IFC_RETURN(QueueUpdateSelectionFlyoutVisibility());

    return S_OK;
}

CFlyoutBase* TextSelectionManager::GetSelectionFlyoutNoRef() const
{
    CUIElement* owner = m_pContainer->GetOwnerUIElement();

    if (auto textBlock = do_pointer_cast<CTextBlock>(owner))
    {
        return textBlock->GetSelectionFlyoutNoRef();
    }
    else if (auto richTextBlock = do_pointer_cast<CRichTextBlock>(owner))
    {
        return richTextBlock->GetSelectionFlyoutNoRef();
    }

    return nullptr;
}

_Check_return_ HRESULT TextSelectionManager::QueueUpdateSelectionFlyoutVisibility()
{
    CUIElement* owner = m_pContainer->GetOwnerUIElement();

    bool succeeded = false;
    IFC_RETURN(VisualTree::GetContentRootForElement(owner)->GetInputManager().TryGetPrimaryPointerLastPosition(&m_lastPointerPosition, &succeeded));

    if (!m_isSelectionFlyoutUpdateQueued)
    {
        m_isSelectionFlyoutUpdateQueued = true;

        if (owner->GetTypeIndex() == KnownTypeIndex::TextBlock)
        {
            IFC_RETURN(FxCallbacks::TextBlock_QueueUpdateSelectionFlyoutVisibility(owner));
        }
        else if (owner->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
        {
            IFC_RETURN(FxCallbacks::RichTextBlock_QueueUpdateSelectionFlyoutVisibility(owner));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::UpdateSelectionFlyoutVisibility()
{
    m_isSelectionFlyoutUpdateQueued = false;

    auto selectionFlyout = GetSelectionFlyoutNoRef();

    if (selectionFlyout)
    {
        auto ownerUIElement = m_pContainer->GetOwnerUIElement();

        // Get last pointer position
        XPOINTF lastPointerPosition = m_lastPointerPosition;

        // Transform point
        wrl::ComPtr<ITransformer> transformer;
        IFC_RETURN(ownerUIElement->TransformToRoot(transformer.ReleaseAndGetAddressOf()));

        IFC_RETURN(transformer->ReverseTransform(&lastPointerPosition, &lastPointerPosition, 1));

        wf::Point point{ lastPointerPosition.x, lastPointerPosition.y };

        // Make sure owner is focused before showing selectionFlyout, so it will restore focus back when selection Flyout is closed.
        if (!ownerUIElement->IsFocused())
        {
            bool wasFocusUpdated;
            IFC_RETURN(ownerUIElement->Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &wasFocusUpdated));
        }

        wf::Rect exclusionRect = DirectUI::RectUtil::CreateEmptyRect();
        IFC_RETURN(GetSelectionBoundingRect(&exclusionRect));

        // We want the point to appear at a consistent location regardless of whether it overlaps the selection,
        // so we'll set its vertical position to be the top of the exclusion rectangle.
        point.Y = exclusionRect.Y;

        IFC_RETURN(FxCallbacks::TextControlFlyout_ShowAt(selectionFlyout, static_cast<CFrameworkElement*>(ownerUIElement), point, exclusionRect, xaml_primitives::FlyoutShowMode_Transient));
    }

    return S_OK;
}

bool TextSelectionManager::ShouldForceFocusedVisualState()
{
    auto owner = static_cast<CFrameworkElement*>(m_pContainer->GetOwnerUIElement());

    return FxCallbacks::TextControlFlyout_IsGettingFocus(GetSelectionFlyoutNoRef(), owner)
        || FxCallbacks::TextControlFlyout_IsGettingFocus(owner->GetContextFlyout().get(), owner);
}

_Check_return_ HRESULT TextSelectionManager::ForceFocusLoss()
{
    m_forceFocusedVisualState = false;

    // LostFocus can't change the selection, only make it invisible, so only invalidate render if it's non-empty.
    if (m_pTextSelection != nullptr && !(m_pTextSelection->IsEmpty()))
    {
        IFC_RETURN(HideGrippers(false /* fAnimate */));

        // Selection went from visible to not, notify visibility changed.
        IFC_RETURN(NotifySelectionVisibilityChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT TextSelectionManager::DismissAllFlyouts()
{
    bool focusUpdated = false;
    CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(m_pOwnerUIElement);
    const DirectUI::FocusState focusState = pFocusManager->GetRealFocusStateForFocusedElement();
    IFC_RETURN(m_pOwnerUIElement->Focus(focusState, false /*animateIfBringIntoView*/, &focusUpdated));
    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));
    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(m_pOwnerUIElement->GetContextFlyout().get()));

    return S_OK;
}


void TextSelectionManager::ShowCaretElement()
{
    EnsureCaret();
    UpdateCaretElement();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::EnsureCaret
//
//---------------------------------------------------------------------------
void TextSelectionManager::EnsureCaret()
{
    if (nullptr == m_caretElement)
    {
        if (m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())  //find out where it bind to
        {
            XUINT32 startOffset;
            CTextPosition anchorPosition;
            ITextView         *textView = nullptr;
            CUIElement        *rTBTarget = nullptr;
            TextGravity        startGravity;
            startGravity = m_pTextSelection->GetStartGravity();
            IFCFAILFAST(m_pTextSelection->GetAnchorTextPosition(&anchorPosition));
            IFCFAILFAST(anchorPosition.GetOffset(&startOffset));
            IFCFAILFAST(GetTextViewForRichTextBlock(startOffset, startGravity, &textView, &rTBTarget));
            m_caretOwnerUIElementNoRef = rTBTarget;
        }
        else if (m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            m_caretOwnerUIElementNoRef = m_pOwnerUIElement;
        }

        CValue value;
        xref_ptr<CSolidColorBrush> spBrush;

        CREATEPARAMETERS cp(m_caretOwnerUIElementNoRef->GetContext());

        value.SetColor(DefaultCaretColor); // white.
        CREATEPARAMETERS brushCreateParams(m_caretOwnerUIElementNoRef->GetContext(), value);

        xref_ptr<CCaretBrowsingCaret> spCaretElement;
        IFCFAILFAST(CreateDO(spCaretElement.ReleaseAndGetAddressOf(), &cp));
        m_caretElement = std::move(spCaretElement);

        value.Set(DirectUI::HorizontalAlignment::Left);
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        value.Set(DirectUI::VerticalAlignment::Top);
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.SetBool(FALSE);
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, value));

        value.Set(DirectUI::ElementCompositeMode::DestInvert);
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_CompositeMode, value));

        value.Set(DirectUI::Visibility::Visible);
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, value));

        IFCFAILFAST(CreateDO(spBrush.ReleaseAndGetAddressOf(), &brushCreateParams));
        IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::Panel_Background, spBrush.get()));

        IFCFAILFAST(m_caretOwnerUIElementNoRef->AddChild(m_caretElement));
    }
    SetupCaretBlinkAnimation();
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::SetupCaretBlinkAnimation
//       Setup storyboard and timer
//
//---------------------------------------------------------------------------
void TextSelectionManager::SetupCaretBlinkAnimation()
{
    // Add a storyboard to make the caret blink.  Leave it paused.
    if (m_caretBlink == nullptr)
    {
        XFLOAT caretBlinkingPeriod = CTextBoxHelpers::GetCaretBlinkingPeriod();
        XFLOAT caretBlinkingTimeout = (static_cast<XINT32>(TextSelectionSettings::Get()->m_rCaretBlinkTimeout / caretBlinkingPeriod)) * caretBlinkingPeriod;
        auto core = m_caretOwnerUIElementNoRef->GetContext();
        CREATEPARAMETERS cp(core);
        CValue value;
        xref_ptr<CTimeSpan> timeSpan;
        int nWidth;
        IFCW32FAILFAST(SystemParametersInfo(SPI_GETCARETWIDTH, 0, &nWidth, 0));
        float caretOpacity = nWidth > 1 ? TextSelectionSettings::Get()->m_rCaretAnimationMaxBlockOpacity : 1.0f;

        IFCFAILFAST(CTextBoxHelpers::CreateCaretAnimationStoryboard(
            m_caretBlink.ReleaseAndGetAddressOf(),
            core,
            m_caretElement,
            m_caretElement->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_Opacity),
            caretOpacity,
            caretBlinkingPeriod));
        IFCFAILFAST(m_caretBlink->BeginPrivate(TRUE /* Top-level Storyboard */));
        IFCFAILFAST(m_caretBlink->PausePrivate());

        //The timer is used to timeout the caret blinking for energy saving.
        IFCFAILFAST(CreateDO(m_caretTimer.ReleaseAndGetAddressOf(), &cp));
        value.SetInternalHandler(OnCaretTimeout);
        IFCFAILFAST(m_caretTimer->AddEventListener(
            EventHandle(KnownEventIndex::DispatcherTimer_Tick),
            &value,
            EVENTLISTENER_INTERNAL, nullptr, FALSE));
        IFCFAILFAST(CreateDO(timeSpan.ReleaseAndGetAddressOf(), &cp));
        timeSpan->m_rTimeSpan = caretBlinkingTimeout;
        IFCFAILFAST(m_caretTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval, timeSpan.get()));
        IFCFAILFAST(m_caretTimer->SetTargetObject(m_caretBlink));  // Make sure we can get the m_caretBlink back from the event handler
    }
}

_Check_return_ HRESULT TextSelectionManager::OnCaretTimeout(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    )
{
    xref_ptr<CDispatcherTimer> timer(static_cast<CDispatcherTimer*>(pSender));
    IFCFAILFAST(timer->WorkComplete());
    IFCFAILFAST(timer->Stop());

    CStoryboard* caretBlink = (static_cast<CStoryboard*>(timer->GetTargetObjectNoRef()));
    caretBlink->PausePrivate();
    return S_OK;
}

void TextSelectionManager::ResetCaretBlink()
{
    xref_ptr<CTimeSpan> timeSpan;
    CValue value;
    value.SetFloat(0.0f);
    CREATEPARAMETERS createParams(m_caretOwnerUIElementNoRef -> GetContext(), value);

    IFCFAILFAST(CreateDO(timeSpan.ReleaseAndGetAddressOf(), &createParams));
    IFCFAILFAST(m_caretBlink->SeekPrivate(timeSpan)); // Moves the current time to a specified interval
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSelectionManager::Remove Caret from tree, also relase the memory of
//
//---------------------------------------------------------------------------
void TextSelectionManager::RemoveCaret()
{
    if (nullptr != m_caretElement)
    {
        IFCFAILFAST(m_caretOwnerUIElementNoRef->RemoveChild(m_caretElement));
        m_caretElement.reset();
        IFCFAILFAST(m_caretTimer->WorkComplete());
        IFCFAILFAST(m_caretTimer->Stop());
        IFCFAILFAST(m_caretBlink->Stop());
        m_caretTimer.reset();
        m_caretBlink.reset();
    }
}

// textOwnerObject should be Textblock, RichTextBlock and RichTextBlockOverflow
void TextSelectionManager::RemoveCaretFromTextObject(_In_ CDependencyObject* textOwnerObject)
{
    if (nullptr != m_caretElement && m_caretOwnerUIElementNoRef == textOwnerObject)
    {
        RemoveCaret();
        m_caretOwnerUIElementNoRef = nullptr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the size, position, and blink animation of the UIElement
//      used to render the caret.
//
//------------------------------------------------------------------------
void TextSelectionManager::UpdateCaretElement()
{
    if (nullptr != m_caretElement)
    {
        XUINT32 startOffset;
        XUINT32 endOffset;
        CTextPosition anchorPosition;
        CTextPosition movingPosition;

        IFCFAILFAST(m_pTextSelection->GetAnchorTextPosition(&anchorPosition));
        IFCFAILFAST(m_pTextSelection->GetMovingTextPosition(&movingPosition));
        anchorPosition.GetOffset(&startOffset);
        movingPosition.GetOffset(&endOffset);

        if (startOffset == endOffset)    //if start == end, the caret should be created
        {
            XRECTF *caretRect = nullptr;
            XUINT32 numRects = 0;

            auto extraCleanup = wil::scope_exit([&]{
                if (caretRect != nullptr)
                {
                    delete [] caretRect;
                }
            });

            // Use the gravity of the caret to determine how to get the correct caret location.
            // Backward = get the bounds of the previous character, using its right edge below.
            // Forward = get the bounds of the next character, using its left edge.
            TextGravity startGravity = m_pTextSelection->GetStartGravity();
            if (startGravity & CharacterBackward)
            {
                startOffset--;
            }
            else
            {
                endOffset++;
            }

            if (m_pOwnerUIElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
            {
                ITextView         *textView = nullptr;
                CUIElement        *rTBTarget = nullptr;
                IFCFAILFAST(GetTextViewForRichTextBlock(startOffset, startGravity, &textView, &rTBTarget));
                textView->TextRangeToTextBounds(startOffset, endOffset, &numRects, &caretRect);
                if (m_caretOwnerUIElementNoRef != rTBTarget)
                {
                    IFCFAILFAST(m_caretOwnerUIElementNoRef->RemoveChild(m_caretElement));
                    m_caretOwnerUIElementNoRef = rTBTarget;
                    IFCFAILFAST(m_caretOwnerUIElementNoRef->AddChild(m_caretElement));
                }
            }
            else if (auto textBlock = do_pointer_cast<CTextBlock>(m_pOwnerUIElement))
            {
                textBlock->GetTextView()->TextRangeToTextBounds(startOffset, endOffset, &numRects, &caretRect);
            }

            if (numRects == 1)
            {
                int nWidth = 0;
                CValue value;
                wfn::Vector3 layoutOffset;
                const float scale = RootScale::GetRasterizationScaleForElement(m_caretOwnerUIElementNoRef);

                // If the gravity is backward, take the right edge of the previous character we just got the bounds for.
                if (startGravity & CharacterBackward)
                {
                    caretRect->X += caretRect->Width;
                }

                // Update caret size.
                IFCW32FAILFAST(SystemParametersInfo(SPI_GETCARETWIDTH, 0, &nWidth, 0));
                caretRect->Width = (float)nWidth / scale;
                value.SetFloat(caretRect->Width);
                IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, value));
                value.SetFloat(caretRect->Height);
                IFCFAILFAST(m_caretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, value));

                // Update caret position.
                layoutOffset.X = caretRect->X;
                layoutOffset.Y = caretRect->Y;
                layoutOffset.Z = 0;
                m_caretElement->SetTranslation(layoutOffset);
                ResetCaretBlink();
                IFCFAILFAST(m_caretBlink->ResumePrivate());
                IFCFAILFAST(m_caretTimer->Start());
            }
            else    //empty caretRect
            {
                RemoveCaret();
            }
        }
        else     //if start != end, the caret should be removed
        {
            RemoveCaret();
        }
    }
}

void TextSelectionManager::OnSelectionChanged()
{
    UpdateCaretElement();
}

//------------------------------------------------------------------------
//
//  Method:   TextSelectionManager::CaretOnKeyDown
//  Description:   This function handle the keyboard input for CaretBrowsing mode
//                 moving caret or making selection
//
//------------------------------------------------------------------------
void TextSelectionManager::CaretOnKeyDown(
    _In_ CUIElement *pSender,
    _In_ CEventArgs* pEventArgs,
    _In_ ITextView *pTextView
)
{
    CKeyEventArgs *pKeyEventArgs = nullptr;
    uint32_t startOffset;
    uint32_t endOffset;
    CTextPosition anchorPosition;
    CTextPosition movingPosition;
    CTextPosition textPosition;
    CPlainTextPosition plainTextPosition;
    uint32_t previousSelectionStartOffset = 0;
    uint32_t previousSelectionEndOffset = 0;
    pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    IFCFAILFAST(GetSelectionStartEndOffsets(
        &previousSelectionStartOffset,
        &previousSelectionEndOffset));


    IFCFAILFAST(m_pTextSelection->GetAnchorTextPosition(&anchorPosition));
    IFCFAILFAST(m_pTextSelection->GetMovingTextPosition(&movingPosition));
    anchorPosition.GetOffset(&startOffset);
    movingPosition.GetOffset(&endOffset);
    plainTextPosition = movingPosition.GetPlainPosition();

    switch (pKeyEventArgs->m_platformKeyCode)
    {
        case wsy::VirtualKey::VirtualKey_Right:
        {
            if (pKeyEventArgs->IsCtrlPressed())                          //If control key pressed, move moving position to next word
            {
                IFCFAILFAST(TextNavigationHelper::MoveByWord(1, &plainTextPosition));
            }
            else if (startOffset != endOffset && !pKeyEventArgs->IsShiftPressed())    //If selection made and arrow key pressed, go to the end position of selection
            {
                textPosition = CTextPosition(plainTextPosition);
                m_pTextSelection->GetEndTextPosition(&textPosition);
                plainTextPosition = textPosition.GetPlainPosition();
            }
            else                                                                      //else move the moving position
            {
                IFCFAILFAST(TextNavigationHelper::MoveByCharacter(1, &plainTextPosition));
            }
            break;
        }
        case wsy::VirtualKey::VirtualKey_Left:
        {
            if (pKeyEventArgs->IsCtrlPressed())
            {
                IFCFAILFAST(TextNavigationHelper::MoveByWord(-1, &plainTextPosition));
            }
            else if (startOffset != endOffset && !pKeyEventArgs->IsShiftPressed())
            {
                textPosition = CTextPosition(plainTextPosition);
                m_pTextSelection->GetStartTextPosition(&textPosition);
                plainTextPosition = textPosition.GetPlainPosition();
            }
            else
            {
                IFCFAILFAST(TextNavigationHelper::MoveByCharacter(-1, &plainTextPosition));
            }
            break;
        }
        case wsy::VirtualKey::VirtualKey_Down:
        {
            IFCFAILFAST(TextNavigationHelper::MoveToLineUpOrDownPosition(true, m_caretOwnerUIElementNoRef, &plainTextPosition));
            break;
        }
        case wsy::VirtualKey::VirtualKey_Up:
        {
            IFCFAILFAST(TextNavigationHelper::MoveToLineUpOrDownPosition(false, m_caretOwnerUIElementNoRef, &plainTextPosition));
            break;
        }
        case wsy::VirtualKey::VirtualKey_End:
        {
            if (!pKeyEventArgs->IsCtrlPressed())
            {
                IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfLine(true, m_caretOwnerUIElementNoRef, &plainTextPosition));
            }
            else
            {
                IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfContent(true, m_caretOwnerUIElementNoRef, &plainTextPosition));
            }
            break;
        }
        case wsy::VirtualKey::VirtualKey_Home:
        {
            if (!pKeyEventArgs->IsCtrlPressed())
            {
                IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfLine(false, m_caretOwnerUIElementNoRef, &plainTextPosition));
            }
            else
            {
                IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfContent(false, m_caretOwnerUIElementNoRef, &plainTextPosition));
            }
            break;
        }
        // case wsy::VirtualKey::VirtualKey_PageUp:      // we don't support moving by page as we don't have this concept in XAML
        //     break;
        // case wsy::VirtualKey::VirtualKey_PageDown:
        //     break;
    }
    textPosition = CTextPosition(plainTextPosition);

    if (pKeyEventArgs->IsShiftPressed() && anchorPosition != textPosition)  //select
    {
        RemoveCaret();
        IFCFAILFAST(m_pTextSelection->Select(
            anchorPosition,
            textPosition,
            LineForwardCharacterBackward));
    }
    else   //moving caret
    {
        EnsureCaret();
        IFCFAILFAST(m_pTextSelection->SetCaretPositionFromTextPosition(textPosition));
    }

    //when hyperlink and contenlink have focus, moving caret or selection should move the focus back to textblock
    bool focusChanged = false;
    IFCFAILFAST(m_caretOwnerUIElementNoRef->Focus(DirectUI::FocusState::Programmatic, true, &focusChanged));    //move the caret or selection away from hypercontentlink



    IFCFAILFAST(NotifySelectionChanged(
    previousSelectionStartOffset,
    previousSelectionEndOffset,
    FALSE /* fHideGrippers */)); // Already hidden with the code above if necessary

}
