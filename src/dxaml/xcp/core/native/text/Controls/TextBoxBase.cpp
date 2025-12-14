// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputServices.h"
#include "TextBoxBase.h"
#include "TextBoxView.h"
#include "TextServicesHost.h"
#include "RichEditOleCallback.h"
#include "TextBoxUIManagerEventSink.h"
#include "TextContextMenu.h"
#include "PrivateTextInputSettings.h"
#include "XboxUtility.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <TextInput.h>
#include <XamlTraceLogging.h>
#include <KnownColors.h>
#include <CandidateWindowBoundsChangedEventArgs.h>
#include <DesktopUtility.h>
#include "TextCommon.h"
#include "VisualTree.h"
#include "xcpwindow.h"

#include <DoubleUtil.h>
#include "Storyboard.h"
#include "TimelineCollection.h"
#include "DoubleAnimation.h"
#include <CAutoSuggestBox.g.h>
#include "localizedResource.h"
#include "CValueUtil.h"
#include "DXamlServices.h"
#include "KeyboardAcceleratorUtility.h"

#include "RootScale.h"
#include "XamlOneCoreTransforms.h"
#include <GeneralTransformHelper.h>
#include <FeatureFlags.h>
#include <windows.ui.viewmanagement.h>
#include <CValueBoxer.h>
#include "CMenuFlyoutItem.g.h"
#include "CMenuFlyoutSeparator.g.h"
#include "LoadLibraryAbs.h"

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace RuntimeFeatureBehavior;
using namespace ::Windows::Internal;

namespace
{
    typedef HRESULT (*CreateTextServicesFunction)(
      _In_   IUnknown *punkOuter,
      _In_   ITextHost *pITextHost,
      _Out_  IUnknown **ppUnk
    );
}

// 8d33f741-cf58-11ce-a89d-00aa006cadc5
constexpr IID IID_ITextServices2 =  { 0x8d33f741, 0xcf58, 0x11ce, { 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 } };

// C241F5E0-7206-11D8-A2C7-00A0D1D6C6B3
constexpr IID IID_ITextDocument2 =  { 0xC241F5E0, 0x7206, 0x11D8, { 0xA2, 0xC7, 0x00, 0xA0, 0xD1, 0xD6, 0xC6, 0xB3 } };

// 983e572d-20cd-460b-9104-83111592dd10
constexpr IID IID_IRicheditWindowlessAccessibility = { 0x983e572d, 0x20cd, 0x460b, { 0x91, 0x4, 0x83, 0x11, 0x15, 0x92, 0xdd, 0x10 } };

#define ENM_HIDELINKTOOLTIP        0x80000000

#ifndef SES_EX_USEMOUSEWPARAM
#define SES_EX_USEMOUSEWPARAM        0x20000000    // Use wParam when handling WM_MOUSEMOVE message and do not call GetAsyncKeyState
#endif

#define MAKECSET(type, set) (set + (type << 16))
#define LANGIDFROMHKL(x) LANGID(LOWORD(HandleToLong(x)))

static void Clamp(_Out_ XFLOAT *val, _In_ XFLOAT min, _In_ XFLOAT max)
{
    *val = *val > max ? max : *val;
    *val = *val < min ? min : *val;
}

static VARIANT s_csetWhitespace;

static const XFLOAT c_rectPadding = 15;

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strActivityIdMaxCharacterCountReached, L"MaxCharacterCountReached");

namespace TextBoxBase_Internal{
//------------------------------------------------------------------------
//
//  Synopsis:
//      Wrapper for ITextServices::TxSendMessage, sends an instruction
//      to this control's ITextServices instance.
//  Notes:
//      Although this function is logically part of CTextBoxBase, it's
//      defined here to keep WPARAM & LPARAM windows type dependency out
//      of CTextBoxBase.h.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TxSendMessageHelper(
    _In_ CTextBoxBase *pTextBoxBase,
    _In_ XUINT32 message,
    _In_ XUINT32 originalMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _Out_ bool *pHandled,
    _Out_ XLONG_PTR *pResult
    )
{
    pTextBoxBase->SetLastMessage(originalMessage);

    LRESULT result;
    HRESULT hr = pTextBoxBase->GetTextServices()->TxSendMessage(message, wParam, lParam, &result);

    *pHandled = FALSE;
    *pResult = result;

    switch (hr)
    {
        case S_OK:
            // RichEdit took some action, handled the message.
            *pHandled = TRUE;
            break;

        case S_FALSE:
            // RichEdit didn't process the message.
            break;

        case S_MSG_KEY_IGNORED:
            // RichEdit processed a keystroke but took no action (e.g., right arrow at end of doc).
            // This is the point to hook up some extra state to allow smart tabbing, etc. if we
            // decide to do that.
            break;

        default:
            ASSERT(FAILED(hr));
            IFC_RETURN(hr);
            break;
    }

    return S_OK;
}

// Retrieves the first ScrollViewer ancestor of a Dependency Object
CScrollViewer* GetClosestScrollViewerAncestor( CDependencyObject *pChild)
{
    CDependencyObject *pParent = pChild;
    CScrollViewer *pParentAsScrollViewer;

    do
    {
        pParent = pParent->GetParent();
        pParentAsScrollViewer = do_pointer_cast<CScrollViewer>(pParent);

    } while (pParent && !pParentAsScrollViewer);

    return pParentAsScrollViewer;
}
} // namespace TextBoxBase_Internal

CTextBoxBase::ScopedSuppressChangeNotifications::ScopedSuppressChangeNotifications(
    _In_     CTextBoxBase*   textBoxBase,
    _In_     ITextDocument2* document) :
    m_textBoxBase(textBoxBase),
    m_document(document)
{
    if ((m_textBoxBase->m_suppressChangeNotificationsRequests == 0) && (m_document != nullptr))
    {
        m_document->Undo(tomSuspend, nullptr);
    }
    m_textBoxBase->m_suppressChangeNotificationsRequests++;
}

CTextBoxBase::ScopedSuppressChangeNotifications::ScopedSuppressChangeNotifications(ScopedSuppressChangeNotifications&& move) noexcept :
    m_textBoxBase(move.m_textBoxBase),
    m_document(move.m_document)
{
    move.m_textBoxBase = nullptr;
    move.m_document = nullptr;
}

CTextBoxBase::ScopedSuppressChangeNotifications::~ScopedSuppressChangeNotifications()
{
    Reset();
}

CTextBoxBase::ScopedSuppressChangeNotifications&
CTextBoxBase::ScopedSuppressChangeNotifications::operator=(ScopedSuppressChangeNotifications&& move) noexcept
{
    if (this != &move)
    {
        Reset();
        m_textBoxBase = move.m_textBoxBase;
        m_document = move.m_document;

        move.m_textBoxBase = nullptr;
        move.m_document = nullptr;
    }

    return *this;
}

void CTextBoxBase::ScopedSuppressChangeNotifications::Reset()
{
    if (m_textBoxBase != nullptr)
    {
        FAIL_FAST_IF(m_textBoxBase->m_suppressChangeNotificationsRequests == 0);
        m_textBoxBase->m_suppressChangeNotificationsRequests--;

        if ((m_textBoxBase->m_suppressChangeNotificationsRequests == 0) && (m_document != nullptr))
        {
            m_document->Undo(tomResume, nullptr);
        }

        m_textBoxBase = nullptr;
    }

    m_document = nullptr;
}

CTextBoxBase::ScopedSuppressChangeNotifications CTextBoxBase::SuppressChangeNotifications()
{
    return ScopedSuppressChangeNotifications(this, m_spDocument.Get());
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
CTextBoxBase::CTextBoxBase(_In_ CCoreServices *pCore)
: CControl(pCore),
    m_lcid(MAKELCID(GetThreadUILanguage(), SORT_DEFAULT)),
    m_ensureRectVisiblePending(false),
    m_ensureRectVisibleWithPadding(false),
    m_isPointerOver(false),
    m_leftButtonPressed(false),
    m_middleButtonPressed(false),
    m_rightButtonPressed(false),
    m_barrelButtonPressed(false),
    m_handleRightTappedEvent(true),
    m_viewHasPeerRef(false),
    m_ignoreScrollIntoView(false),
    m_isViewRegisteredForGripperUpdates(false),
    m_gripperBeingManipulated(false),
    m_ignoreGripperTouchEvents(false),
    m_canEnableManualInputPane(false),
    m_isKeyboardRTL(false),
    m_ignoreKillFocus(false),
    m_manualInputPaneEnabled(false),
    m_gamepadEngaged(false),
    m_hideSelection(true),
    m_shouldScrollHeaderIntoView(false),
    m_previouslyGeneratedAlternatives(false),
    m_firedCandidateWindowEventAfterFocus(false),
    m_forceNotifyFocusEnter(false),
    m_compositionInProgress(false),
    m_spellCheckIsDefault(true),
    m_textPredictionIsDefault(true),
    m_handlingSelectionChangingEvent(false),
    m_forceFocusedVisualState(false),
    m_proofingMenuIsValid(false),
    m_updatingExtent(false),
    m_useLegacyContextMenu(true),
    m_isSelectionFlyoutUpdateQueued(false)
{
    XCP_STRONG(&m_pHost);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
CTextBoxBase::~CTextBoxBase()
{
    // Note that, during destruction, some operations in the text services object require
    // a callback back into us.  If those callbacks occur inside the destructor for CTextBoxBase,
    // then that will already be too late - the vtable for the subclass will have already been
    // torn down, and any calls to virtual methods will erroneously call into the CTextBoxBase implementation
    // instead of the implementation of the subclass.  This is bad, as we still want the subclass functionality
    // at this point in time. In short:
    //
    // DO NOT ADD SHARED DESTRUCTOR CODE HERE.
    //
    // Add it to the Destroy() method, which is called by the destructor method of each subclass of CTextBoxBase.
    // Calling it in this fashion allows us to still have an intact vtable during those callbacks.
    //
    // TODO 323104: Consider whether what the text services object is doing during destruction can be either
    // done without or done at an earlier time, in order to allow us to move the shared destructor code
    // back into this method.
}

_Check_return_ HRESULT CTextBoxBase::GetValue(
    _In_  const CDependencyProperty *pProperty,
    _Out_ CValue *pValue
    )
{
    if (pProperty->GetIndex() == GetSelectionHighlightColorPropertyID())
    {
        if (m_pSelectionHighlightColor == nullptr)
        {
            IFC_RETURN(CreateSelectionHighlightColor());
        }
    }

    if (pProperty->GetIndex() == GetSelectionHighlightColorWhenNotFocusedPropertyID())
    {
        if (m_pSelectionHighlightColorWhenNotFocused == nullptr)
        {
            IFC_RETURN(CreateSelectionHighlightColorWhenNotFocused());
        }
    }

    IFC_RETURN(CControl::GetValue(pProperty, pValue));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, requests or releases mouse capture.
//
//---------------------------------------------------------------------------
void CTextBoxBase::TxSetCapture(_In_ BOOL takeCapture)
{
    if (m_pLastPointer)
    {
        if (takeCapture)
        {
            bool result;
            IGNOREHR(CapturePointer(m_pLastPointer, &result));
        }
        else
        {
            IGNOREHR(ReleasePointerCapture(m_pLastPointer));
        }
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, requests input focus.
//
//---------------------------------------------------------------------------
void CTextBoxBase::TxSetFocus()
{
    // It's safe to pass DirectUI::FocusState::Pointer here because TxSetFocus() only gets called via mouse and touch.
    //
    // We have to pass DirectUI::FocusState::Pointer explicitly instead of plumbing it through because TxSetFocus()
    // is called by the richedit stack, not directly by the Jupiter stack.
    bool focusChanged;
    VERIFYHR(Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusChanged));
}

// Convert from TextBox space (where the origin is top-left of TextBox) to screen space
_Check_return_ HRESULT CTextBoxBase::TextBoxToScreen(_Inout_ XPOINT* point)
{
    if (ShouldUseVisualPixels())
    {
        XAML_FAIL_FAST();
    }

    wrl::ComPtr<ixp::IContentCoordinateConverter> coordinateConverter = GetCoordinateConverter();

    if (coordinateConverter)
    {
        // The coordinate converter takes rasterization scale into account, so we shouldn't apply it here.
        IFC_RETURN(TextBoxToClient(point, false /* applyRasterizationScale */));

        wf::Point inPoint = {static_cast<FLOAT>(point->x), static_cast<FLOAT>(point->y)};
        wgr::PointInt32 resultPoint;

        IFC_RETURN(coordinateConverter->ConvertLocalToScreenWithPoint(inPoint, &resultPoint));

        point->x = resultPoint.X;
        point->y = resultPoint.Y;
    }
    else
    {
        // This code path is still used by our tests which run as UWPs. It's not used in the product
        ASSERT(GetContext()->GetInitializationType() == InitializationType::MainView);

        IFC_RETURN(TextBoxToClient(point, true /* applyRasterizationScale */));

        HWND uwpHwnd = GetElementPositioningWindow();

        if (!::ClientToScreen(uwpHwnd, reinterpret_cast<POINT *>(point)))
        {
            // Note: ClientToScreen will fail when TextBoxToScreen is called after the window
            // has been destroyed. For example, assume the window is destroyed and then a CTextBox
            // is destroyed. The text box destructor will call into msftedit to deactivate the richedit
            // control (ITextServices2::OnTxInPlaceDeactivate). During deactivation the control may
            // call back into xaml to perform client-to-screen transformations.
            //
            // msftedit needs to handle a failure of this function. Note that some code in msftedit
            // just ignores the return value (e.g. CTouchHandlerImpl::IsSkewedText). In any case,
            // we need to NOTRACE here to avoid creating error noise, since the failure will be handled.
            IFC_NOTRACE_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

// Convert from TextBox space (where the origin is top-left of TextBox) to
// client space (where the origin is the top-left of the XAML root)
_Check_return_ HRESULT CTextBoxBase::TextBoxToClient(_Inout_ XPOINT* point, bool applyRasterizationScale)
{
    XPOINTF pointf = { static_cast<float>(point->x), static_cast<float>(point->y) };

    //TransformToVisual(nullptr) walks to the public root, not the CRootVisual.
    //Unlike TransformToRoot(NULL), it skips scale on the CoreWindow
    xref_ptr<CGeneralTransform> transform;
    IFC_RETURN(m_pView->TransformToVisual(nullptr, &transform));
    IFC_RETURN(transform->TransformPoints(&pointf, &pointf, 1));

    if (applyRasterizationScale)
    {
        const float scale = RootScale::GetRasterizationScaleForElement(this);
        pointf = pointf * scale;
    }

    *point = { static_cast<XINT32>(pointf.x), static_cast<XINT32>(pointf.y) };

    return S_OK;
}

// Convert from screen space to TextBox space (where the origin is top-left of TextBox)
_Check_return_ HRESULT CTextBoxBase::ScreenToTextBox(_Inout_ XPOINT* point, bool roundUp)
{
    if (ShouldUseVisualPixels())
    {
        XAML_FAIL_FAST();
    }

    wrl::ComPtr<ixp::IContentCoordinateConverter> coordinateConverter = GetCoordinateConverter();

    XPOINTF pointf;

    if (coordinateConverter)
    {
        wgr::PointInt32 inPoint = {point->x, point->y};
        wf::Point resultPoint;

        // The coordinate converter takes rasterization scale into account, so there is no need to
        // apply it here.
        IFC_RETURN(coordinateConverter->ConvertScreenToLocalWithPoint(inPoint, &resultPoint));

        pointf = { resultPoint.X, resultPoint.Y };
    }
    else
    {
        // This code path is still used by our tests which run as UWPs. It's not used in the product
        ASSERT(GetContext()->GetInitializationType() == InitializationType::MainView);

        const float scale = RootScale::GetRasterizationScaleForElement(this);

        HWND uwpHwnd = GetElementPositioningWindow();

        if (!::ScreenToClient(uwpHwnd, reinterpret_cast<POINT *>(point)))
        {
            IFC_RETURN(E_FAIL);
        }

        pointf = { static_cast<float>(point->x), static_cast<float>(point->y) };
        pointf = pointf / scale;
    }

    IFC_RETURN(ClientToTextBox(&pointf));

    if (roundUp)
    {
        *point = { XcpCeiling(pointf.x), XcpCeiling(pointf.y) };
    }
    else
    {
        *point = { static_cast<int>(pointf.x), static_cast<int>(pointf.y) };
    }

    return S_OK;
}

// Convert from client space (where the origin is the top-left of the XAML root) to
// TextBox space (where the origin is top-left of TextBox)
_Check_return_ HRESULT CTextBoxBase::ClientToTextBox(_Inout_ XPOINTF* point)
{
    xref_ptr<CGeneralTransform> transform;
    IFC_RETURN(m_pView->TransformToVisual(nullptr, &transform));
    xref_ptr<CGeneralTransform> inverse(GetInverseTransform(transform));
    if (inverse != nullptr)
    {
        IFC_RETURN(inverse->TransformPoints(point, point, 1));
    }

    return S_OK;
}

Microsoft::WRL::ComPtr<ixp::IContentCoordinateConverter> CTextBoxBase::GetCoordinateConverter()
{
    xref_ptr<CPopup> spPopup;
    CUIElement *pUIElementNoRef = do_pointer_cast<CUIElement>(this);

    Microsoft::WRL::ComPtr<ixp::IContentCoordinateConverter> converter;

    // If the element is in an open popup, get that popup.
    IGNOREHR(CPopupRoot::GetOpenPopupForElement(
        pUIElementNoRef,
        spPopup.ReleaseAndGetAddressOf()));

    // If the popup is windowed, return the popup island's coordinate converter
    if (spPopup && spPopup->IsWindowed())
    {
        spPopup->GetContentIslandNoRef()->get_CoordinateConverter(&converter);

        return converter;
    }

    if (GetContext()->HasXamlIslandRoots())
    {
        // If this element is in a XamlIslandRoot tree, return the CoordinateConverter for that island.
        auto root = GetContext()->GetRootForElement(this);
        auto xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(root);
        if (xamlIslandRoot)
        {
            xamlIslandRoot->GetContentIsland()->get_CoordinateConverter(&converter);

            return converter;
        }
    }

    // If we can't return a converter, we should instead check for an HWND.
    return nullptr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets the maximum number of characters allowed
//      in the control.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::TxGetMaxLength(_Out_ XUINT32 *pLength)
{
    *pLength = m_iMaxLength > 0 ? m_iMaxLength : INFINITE;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets the password substitution character.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::TxGetPasswordChar(_Out_ WCHAR *pChar)
{
    ASSERT(FALSE); // PasswordBox overrides this method, and otherwise we should never get here.
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets status flags.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::TxGetPropertyBits(
    _In_ XUINT32 mask,
    _Out_ XUINT32 *pFlags
    )
{
    XUINT32 flags = TXTBIT_SAVESELECTION |
        TXTBIT_D2DDWRITE |
        TXTBIT_D2DSUBPIXELLINES |
        TXTBIT_DISABLEDRAG;

    if (m_hideSelection)
    {
        flags |= TXTBIT_HIDESELECTION;
    }

    if (GetContext()->IsTSF3Enabled())
    {
        flags |= TXTBIT_ADVANCEDINPUT; //enable TSF3.0
    }

    flags |= IsPassword() ? TXTBIT_USEPASSWORD : 0;
    flags |= UseMultiLineMode() ? TXTBIT_MULTILINE : 0;
    flags |= AcceptsRichText() ? TXTBIT_RICHTEXT : 0;
    flags |= IsReadOnly() ? TXTBIT_READONLY : 0;
    flags |= ((GetTextWrapping() == DirectUI::TextWrapping::Wrap) ? TXTBIT_WORDWRAP : 0);

    if (GetView())
    {
        flags |= (GetView()->IsPixelSnapped() ? TXTBIT_D2DPIXELSNAPPED : 0);
    }

    *pFlags = flags & mask;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by TextServicesHost::ShowGrippers to set where last grippers were set.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetGripperRects(XRECT_WH rectBegin, XRECT_WH rectEnd)
{
    m_rectBeginGripperLast = rectBegin;
    m_rectEndGripperLast = rectEnd;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called by RichEditGripper to let this class know if a manipulation is in progress.
//      If fBeingManipulated is true, the copy icon will be hidden, if it is false, it will
//      be shown.  For the caret gripper, the blink animation will be paused for true
//      and resumed for false.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetGripperBeingManipulated(_In_ bool isBeingManipulated)
{
    ASSERT(m_gripperBeingManipulated != isBeingManipulated);
    m_gripperBeingManipulated = isBeingManipulated;
    m_ignoreGripperTouchEvents = false;

    if (isBeingManipulated)
    {
        m_rectBeginGripperManipStart = m_rectBeginGripperLast;
        m_rectEndGripperManipStart = m_rectEndGripperLast;

        m_pView->GripperPauseCaretBlink();
    }
    else
    {
        // Reset cached selection values (these are updated in TxNotify while a
        // gripper is being manipulated).
        m_cpLastSelBegin = -1;
        m_cpLastSelEnd = -1;

        m_pView->GripperResumeCaretBlink();

        IFC_RETURN(QueueUpdateSelectionFlyoutVisibility());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, notifies the host about content or selection
//      changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::TxNotify(
    _In_ UINT32 notification,
    _In_ void *pData
    )
{
    switch (notification)
    {
        //case EN_UPDATE:
    case EN_CHANGE:
    {
        CHANGENOTIFY* changeNotify = static_cast<CHANGENOTIFY*>(pData);

        if ((changeNotify->dwChangeType & CN_TEXTCHANGED))
        {
            IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));
        }

        if (ShouldGenerateLinguisticAlternatives())
        {
            ResetLinguisticAlternativeState();
        }

        if (m_suppressChangeNotificationsRequests == 0)
        {
            IFC_RETURN(OnContentChanged((changeNotify->dwChangeType & CN_TEXTCHANGED) != 0 /* fTextChanged */));
            if (m_compositionInProgress && changeNotify->dwChangeType & CN_TEXTCHANGED)
            {
                IFC_RETURN(SendTextCompositionEvent(TextCompositionStage::CompositionChanged));
            }
        }
    }
    break;

    case EN_STARTCOMPOSITION:
        if (!IsPassword())
        {
            // EN_CHANGE arrives later than EN_STARTCOMPOSITION, so we need to update text property for user to consume
            m_compositionInProgress = true;
            IFC_RETURN(UpdateTextForCompositionStartedEvent());
            m_previouslyGeneratedAlternatives = false;
            IFC_RETURN(SendTextCompositionEvent(TextCompositionStage::CompositionStarted));
        }
        break;

    case EN_ENDCOMPOSITION:
        if (m_compositionInProgress)
        {
            ENDCOMPOSITIONNOTIFY* pEndCompositionNotify = reinterpret_cast<ENDCOMPOSITIONNOTIFY*>(pData);
            // if ECN_ENDCOMPOSITION is set, end the composition for sure
            // If only bit set is ECN_NEWTEXT, there will be another EN_ENDCOMPOSITION later on, so treat it as composition change (subsequent EN_CHANGE will follow).
            if (pEndCompositionNotify->dwCode & ECN_ENDCOMPOSITION)
            {
                m_compositionInProgress = false;
                m_previouslyGeneratedAlternatives = true;
                IFC_RETURN(SendTextCompositionEvent(TextCompositionStage::CompositionEnded));
            }
        }
        break;

    case EN_SELCHANGE:
        if ((m_suppressChangeNotificationsRequests == 0) && !IsHandlingSelectionChangingEvent())
        {
            IFC_RETURN(OnSelectionChanged());
        }
        break;

    case EN_KILLFOCUS:
        if (!m_ignoreKillFocus)
        {
            IFC_RETURN(ForceRemoveFocus());
        }
        break;

    case EN_REQUESTRESIZE:
    {
        RECT rc = ((REQRESIZE*)pData)->rc;

        const bool isInitialLayout = (m_requestResize.right == 0 && m_requestResize.bottom == 0);

        m_requestResize.left = rc.left;
        m_requestResize.right = rc.right;
        m_requestResize.top = rc.top;
        m_requestResize.bottom = rc.bottom;

        // In the past, we have always invalidated the view, causing us to run Measure on every resize. If we have not exceeded the viewport
        // of the textboxview, there is no reason to do a measure. So instead, we check if the width/height of the text has exceeded the
        // width/height of the viewport before invalidating the view.

        // We return the scroll extent as the client rect to RichEdit.
        // If we do not increase the scroll extent here, RichEdit assumes
        // there is not enough space and tries to create a new line.
        if (!m_updatingExtent || isInitialLayout)
        {
            VERIFYHR(m_pView->InvalidateView());
        }
    }
    break;

    case EN_UIASELECT:
        // UIA Select (double tap action) from Narrator will be treated as OnTapped(), it will bring up SIP if PreventKeyboardFromProgrammaticFocus
        // is true and textbox was focused programmatically.
        IFC_RETURN(UpdateSIPSettings(DirectUI::FocusState::Pointer));
        IFC_RETURN(ForceNotifyFocusEnterIfNeeded());
        break;

    case EN_CUTREQUESTED:
    case EN_COPYREQUESTED:
    case EN_PASTEREQUESTED:
    {
        bool* handled = reinterpret_cast<bool*>(pData);

        if (notification == EN_CUTREQUESTED)
        {
            *handled = RaiseCutEventAndCheckIfHandled();
        }
        else if (notification == EN_COPYREQUESTED)
        {
            *handled = RaiseCopyEventAndCheckIfHandled();
        }
        else
        {
            ASSERT(notification == EN_PASTEREQUESTED);
            *handled = RaisePasteEventAndCheckIfHandled();
        }
        break;
    }
    case EN_MAXTEXT:
    {
        // Processing this notification for automation tools like Narrator.
        // It is raised when attempting to add a character while the maximum number MaxLength was already reached.
        // By the time this notification is raised though, the keystroke was already announced by Narrator.
        // The phrasing of the raised message takes this in to account, so for example when typing k, Narrator says:
        // "k not added because the maximum character count was reached."
        CAutomationPeer* automationPeer = GetAutomationPeer();

        if (automationPeer)
        {
            auto browserHost = GetContext()->GetBrowserHost();

            if (browserHost)
            {
                xstring_ptr messageString;

                IFC_RETURN(browserHost->GetLocalizedResourceString(UIA_TEXT_MAX_CHARACTER_COUNT_REACHED, &messageString));

                automationPeer->RaiseNotificationEvent(
                    UIAXcp::AutomationNotificationKind_ActionAborted,
                    UIAXcp::AutomationNotificationProcessing_MostRecent,
                    messageString,
                    XSTRING_PTR_FROM_STORAGE(c_strActivityIdMaxCharacterCountReached));
            }
        }
        break;
    }
    }

    return S_OK;
}

bool CTextBoxBase::HasSelection()
{
    ctl::ComPtr<ITextSelection2> spSelection;
    VERIFYHR(GetSelection(&spSelection));

    LONG cchSel = 0;
    if (spSelection)
    {
        VERIFYHR(spSelection->GetCch(&cchSel));
    }

    return cchSel != 0;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, requests the width of the margin used for
//      handling double of triple click activated selects.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::TxGetSelectionBarWidth(_Out_ XINT32 *pSelectionBarWidth)
{
    *pSelectionBarWidth = 0;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetVisibleRect(XRECT_RB* pRect)
{
    XRECTF_RB viewportRect;
    XRECTF_RB convertedVisibleRect;
    XPOINT ptUpperLeft;
    XPOINT ptLowerRight;

    viewportRect = m_pView->GetViewportGlobalRect();

    if (m_rectVisibleRegion.Width == -1 && m_rectVisibleRegion.Height == -1)
    {
        convertedVisibleRect = viewportRect;
    }
    else
    {
        convertedVisibleRect.left = m_rectVisibleRegion.X;
        convertedVisibleRect.top = m_rectVisibleRegion.Y;
        convertedVisibleRect.right = m_rectVisibleRegion.X + m_rectVisibleRegion.Width;
        convertedVisibleRect.bottom = m_rectVisibleRegion.Y + m_rectVisibleRegion.Height;

        IntersectRect(&convertedVisibleRect, &viewportRect);
    }

    // We round the global coordinates and then take the ceiling of the bottom-right client coordinates for the following reasons:
    //
    // 1. For the global visible rect, our concern is with regards to slight rounding errors that may propagate themselves
    // if we don't round - e.g., viewportRect.right might be 469.99996 instead of 470, or viewportRect.bottom might be
    // 130.00003 instead of 130.  We want to eliminate those rounding errors, so we round the values to the nearest integer.
    //
    // 2. For the client rect, we take the ceiling of the extent width and height in CTextBoxView::TxGetClientRect(), and pass in (0, 0)
    // for the top-left corner.  Since RichEdit makes comparisons between the positions of the visible rect and the client rect,
    // we need to report the same bounds so they're comparable, so we'll err on the side of a lower value for the top-left corner
    // and on the side of a higher value for the bottom-right corner.
    ptUpperLeft.x = XcpRound(convertedVisibleRect.left);
    ptUpperLeft.y = XcpRound(convertedVisibleRect.top);

    ptLowerRight.x = XcpRound(convertedVisibleRect.right);
    ptLowerRight.y = XcpRound(convertedVisibleRect.bottom);

    IFC_RETURN(ScreenToTextBox(&ptUpperLeft, false /* roundUp */));
    IFC_RETURN(ScreenToTextBox(&ptLowerRight, true /* roundUp */));

    pRect->left = ptUpperLeft.x;
    pRect->top = ptUpperLeft.y;
    pRect->right = ptLowerRight.x;
    pRect->bottom = ptLowerRight.y;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::EnsureRectVisible(_In_ const RECT &rect)
{
    m_rectLastVisibleRectInView = rect;
    m_ensureRectVisibleWithPadding = false;
    if (!m_pView->IsRendering())
    {
        IFC_RETURN(BringLastVisibleRectIntoView(true /* scrollTextBoxIntoView */));
    }
    else
    {
        IFC_RETURN(PostAsyncEnsureRectVisibleCall());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::EnsureRectVisibleWithPadding(_In_ const RECT &rect)
{
    m_rectLastVisibleRectInView = rect;
    m_ensureRectVisibleWithPadding = true;

    if (!m_pView->IsRendering())
    {
        IFC_RETURN(BringLastVisibleRectIntoView(true /* scrollTextBoxIntoView */));
    }
    else
    {
        IFC_RETURN(PostAsyncEnsureRectVisibleCall());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::PostAsyncEnsureRectVisibleCall()
{
    if (m_spEnsureRectVisibleTimer == nullptr)
    {
        auto core = GetContext();
        CREATEPARAMETERS cp(core);
        CValue value;
        xref_ptr<CTimeSpan> pTimeSpan;

        IFC_RETURN(CreateDO(m_spEnsureRectVisibleTimer.ReleaseAndGetAddressOf(), &cp));
        value.SetInternalHandler(OnDelayEnsureRectVisible);
        IFC_RETURN(m_spEnsureRectVisibleTimer->AddEventListener(
            EventHandle(KnownEventIndex::DispatcherTimer_Tick),
            &value,
            EVENTLISTENER_INTERNAL, nullptr));

        IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));
        pTimeSpan->m_rTimeSpan = 0.0f;

        IFC_RETURN(m_spEnsureRectVisibleTimer->SetValueByKnownIndex(
            KnownPropertyIndex::DispatcherTimer_Interval,
            pTimeSpan.get()));
        IFC_RETURN(m_spEnsureRectVisibleTimer->SetTargetObject(this));
    }

    IFC_RETURN(m_spEnsureRectVisibleTimer->Start());

    return S_OK;
}

void CTextBoxBase::EnableEnsureRectVisible()
{
    m_ensureRectVisibleEnabled = true;
}

void CTextBoxBase::DisableEnsureRectVisible()
{
    m_ensureRectVisibleEnabled = false;
}

_Check_return_ HRESULT CTextBoxBase::BringLastVisibleRectIntoView(bool scrollTextBoxIntoView)
{
    if (m_ensureRectVisibleEnabled)
    {
        // If the last-visible rect has never been set,
        // then we'll call EnsureRectVisible and pass in the caret rect
        // in order to initialize its value.
        if (m_rectLastVisibleRectInView.left == -1 &&
            m_rectLastVisibleRectInView.top == -1 &&
            m_rectLastVisibleRectInView.right == -1 &&
            m_rectLastVisibleRectInView.bottom == -1)
        {
            XRECTF caretRectF;
            RECT caretRect;

            caretRectF = GetView()->GetCaretRect();

            caretRect.left = static_cast<LONG>(caretRectF.X);
            caretRect.top = static_cast<LONG>(caretRectF.Y);
            caretRect.right = static_cast<LONG>(caretRectF.X + caretRectF.Width);
            caretRect.bottom = static_cast<LONG>(caretRectF.Y + caretRectF.Height);

            IFC_RETURN(EnsureRectVisible(caretRect));
        }
        else
        {
            XRECTF viewRect = {0, 0, 0, 0};
            XRECTF_RB viewRectRB = {0, 0, 0, 0};
            XRECTF_RB rectOuter = {0, 0, 0, 0};
            XRECTF visibleRect = {0, 0, 0, 0};
            XRECTF_RB boundingRect = {0, 0, 0, 0};

            viewRect.X = static_cast<XFLOAT>(m_rectLastVisibleRectInView.left);
            viewRect.Y = static_cast<XFLOAT>(m_rectLastVisibleRectInView.top);
            viewRect.Width = static_cast<XFLOAT>(m_rectLastVisibleRectInView.right - m_rectLastVisibleRectInView.left);
            viewRect.Height = static_cast<XFLOAT>(m_rectLastVisibleRectInView.bottom - m_rectLastVisibleRectInView.top);

            // Scroll text content into view within textbox
            IFC_RETURN(m_pView->MakeVisible(
                viewRect,
                DirectUI::DoubleUtil::NaN /*horizontalAlignmentRatio*/,
                DirectUI::DoubleUtil::NaN /*verticalAlignmentRatio*/,
                0.0 /*offsetX*/,
                0.0 /*offsetY*/,
                nullptr /*visibleViewRect*/,
                nullptr /*appliedOffsetX*/,
                nullptr /*appliedOffsetY*/));

            // If true, scroll textbox control itself into view: calculate the rect and hand it to ScrollViewer up the tree
            if (scrollTextBoxIntoView)
            {
                viewRectRB.left = viewRect.X;
                viewRectRB.top = viewRect.Y;
                viewRectRB.right = viewRect.X + viewRect.Width;
                viewRectRB.bottom = viewRect.Y + viewRect.Height;

                IFC_RETURN(TransformInnerToOuterChain(m_pView, this, &viewRectRB, &rectOuter));

                if (m_ensureRectVisibleWithPadding)
                {
                    rectOuter.left -= c_rectPadding;
                    rectOuter.right += c_rectPadding;
                    rectOuter.bottom += c_rectPadding;
                }

                // TransformInnerToOuterChain already applied current offset for the transform, so undo it.
                // The reason we need to undo the current offset: because we are only interested in where the rect will eventually be
                // in the future (stored in view's ScrollData as calculated earlier in m_pView->MakeVisible).
                // The current offset can be same or different as the just calculated future ScrollData. If it is different, it means TextBox's
                // internal ScrollViewer will scroll the text content later, so we need to undo the current offset and apply future offset to the RECT.

                BOOL fTransformSet = FALSE;
                XFLOAT dmTranslateX = 0.0f;
                XFLOAT dmTranslateY = 0.0f;
                FLOAT uncompressedZoomFactor = 1.0f;
                XFLOAT dmZoomFactorX = 0.0f;
                XFLOAT dmZoomFactorY = 0.0f;

                //Get current scroll offset directly from DManip
                IFC_RETURN(m_pView->GetDirectManipulationCompositorTransform(
                    TransformRetrievalOptions::None,
                    fTransformSet,
                    dmTranslateX,
                    dmTranslateY,
                    uncompressedZoomFactor,
                    dmZoomFactorX,
                    dmZoomFactorY));

                //Undo the current scroll offset
                rectOuter.left -= dmTranslateX;
                rectOuter.top -= dmTranslateY;
                rectOuter.right -= dmTranslateX;
                rectOuter.bottom -= dmTranslateY;

                //Apply future scroll offset to get the future rect position for BringIntoView
                rectOuter.left -= static_cast<float>(m_pView->GetScrollData()->HorizontalOffset);
                rectOuter.top -= static_cast<float>(m_pView->GetScrollData()->VerticalOffset);
                rectOuter.right -= static_cast<float>(m_pView->GetScrollData()->HorizontalOffset);
                rectOuter.bottom -= static_cast<float>(m_pView->GetScrollData()->VerticalOffset);

                boundingRect.bottom = static_cast<XFLOAT>(GetActualHeight());
                boundingRect.top = 0.0f;
                boundingRect.right = static_cast<XFLOAT>(GetActualWidth());
                boundingRect.left = 0.0f;

                // We want to clamp the visible rect to the bounds of the TextBox.
                Clamp(&rectOuter.left, boundingRect.left - 10, boundingRect.right + 10);
                Clamp(&rectOuter.right, boundingRect.left - 10, boundingRect.right + 10);
                Clamp(&rectOuter.top, boundingRect.top - 10, boundingRect.bottom + 10);
                Clamp(&rectOuter.bottom, boundingRect.top - 10, boundingRect.bottom + 10);

                visibleRect.X = rectOuter.left;
                visibleRect.Y = rectOuter.top;
                visibleRect.Width = rectOuter.right - rectOuter.left;
                visibleRect.Height = rectOuter.bottom - rectOuter.top;

                CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);

                // translate bring into view rect to global
                IFC_RETURN(m_pView->TransformToWorldSpace(&rectOuter, &rectOuter, true /* ignoreClipping */, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));
                contentRoot->GetInputManager().AdjustBringIntoViewRecHeight(rectOuter.top, rectOuter.bottom, visibleRect.Height);

                if (m_shouldScrollHeaderIntoView)
                {
                    // If the size of the ScrollViewer viewport can accommodate the TextBox
                    //header and view, then scroll the header into view
                    const CScrollViewer *pAncestorAsScrollViewer = TextBoxBase_Internal::GetClosestScrollViewerAncestor(this);
                    const float heightWithHeader = visibleRect.Height + visibleRect.Y;
                    if (pAncestorAsScrollViewer && (pAncestorAsScrollViewer->m_viewportHeight > heightWithHeader))
                    {
                        //Scroll the top of the TextBox into view
                        visibleRect.Y = 0;
                        visibleRect.Height = heightWithHeader;
                    }
                }

                BringIntoView(visibleRect, true /*forceIntoView*/, false /*useAnimation*/, true /*skipDuringManipulation*/);
            }
        }
    }
    else
    {
        m_ensureRectVisiblePending = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::RaisePendingBringLastVisibleRectIntoView(_In_ bool forceIntoView, _In_ bool focusChanged)
{
    // Calling this method implies that we now want to allow EnsureRectVisible() to be called,
    // so we'll unconditionally enable it.
    EnableEnsureRectVisible();

    // Only try to scroll the textbox header AND cursor into view if we scroll due
    // to a focus change. If we scrolled the header into view everytime, we would not be
    // able to navigate
    m_shouldScrollHeaderIntoView = focusChanged;

    if (m_ensureRectVisiblePending || forceIntoView)
    {
        if (focusChanged && forceIntoView)
        {
            IFC_RETURN(FxCallbacks::TextBox_GetBringIntoViewOnFocusChange(this, &forceIntoView));
        }

        IFC_RETURN(BringLastVisibleRectIntoView(forceIntoView /* scrollTextBoxIntoView */));

        m_ensureRectVisiblePending = false;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Copies the selection's content into the clipboard and then removes
//      the content from the control.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Cut()
{
    ctl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));

    // Ignore Richedit Clipboard errors to avoid crashing.
    IGNOREHR(spSelection->Cut(nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Inserts a copy of the selection's content into the clipboard.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Copy()
{
    ctl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));

    // Ignore Richedit Clipboard errors to avoid crashing.
    IGNOREHR(spSelection->Copy(nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Inserts the current clipboard content into the selection.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Paste()
{
    ctl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));

    // Ignore Richedit Clipboard errors to avoid crashing.
    IGNOREHR(spSelection->Paste(nullptr, 0));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Restores the control to its state before the last edit.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Undo()
{
    auto scopeGuard = wil::scope_exit([&]
    {
        ReleaseDocumentIfNotFocused();
    });

    IFC_RETURN(EnsureDocument());
    IFC_RETURN(m_spDocument->Undo(1, nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Restores the control to its state before the last undo.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Redo()
{
    auto scopeGuard = wil::scope_exit([&]
    {
        ReleaseDocumentIfNotFocused();
    });

    IFC_RETURN(EnsureDocument());
    IFC_RETURN(m_spDocument->Redo(1, nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Selects the entire content.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SelectAll()
{
    ctl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));
    IFC_RETURN(spSelection->Expand(tomStory, nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the undo/redo buffer
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::ClearUndoRedoHistory()
{
    if (m_pTextServices)
    {
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_EMPTYUNDOBUFFER, 0, 0, nullptr));
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Whether there is content in the Clipboard that can be pasted into the TextBox.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::CanPasteClipboardContent() const
{
    if (m_pTextServices)
    {
        LRESULT lRes = 0;
        return (SUCCEEDED(m_pTextServices->TxSendMessage(EM_CANPASTE, 0, 0, &lRes)) && (lRes != 0));
    }

    return false;
}

bool CTextBoxBase::CanUndo() const
{
    if (m_pTextServices)
    {
        LRESULT lRes = 0;
        return (SUCCEEDED(m_pTextServices->TxSendMessage(EM_CANUNDO, 0, 0, &lRes)) && (lRes != 0));
    }

    return false;
}

bool CTextBoxBase::CanRedo() const
{
    if (m_pTextServices)
    {
        LRESULT lRes = 0;
        return (SUCCEEDED(m_pTextServices->TxSendMessage(EM_CANREDO, 0, 0, &lRes)) && (lRes != 0));
    }

    return false;
}

bool CTextBoxBase::RaiseCutEventAndCheckIfHandled()
{
    const KnownEventIndex cutEventIndex = GetCutPropertyID();
    CEventManager *pEventManager = GetContext()->GetEventManager();

    if (cutEventIndex != KnownEventIndex::UnknownType_UnknownEvent && pEventManager)
    {
        xref_ptr<CTextControlCuttingToClipboardEventArgs> spArgs;
        spArgs.init(new CTextControlCuttingToClipboardEventArgs());

        spArgs->m_bHandled = false;
        pEventManager->Raise(
            EventHandle(cutEventIndex),
            true /*bRefire*/,
            this,
            spArgs.get(),
            true /*fRaiseSync*/);

        return !!spArgs->m_bHandled;
    }

    return false;
}

bool CTextBoxBase::RaiseCopyEventAndCheckIfHandled()
{
    const KnownEventIndex copyEventIndex = GetCopyPropertyID();
    CEventManager *pEventManager = GetContext()->GetEventManager();

    if (copyEventIndex != KnownEventIndex::UnknownType_UnknownEvent && pEventManager)
    {
        xref_ptr<CTextControlCopyingToClipboardEventArgs> spArgs;
        spArgs.init(new CTextControlCopyingToClipboardEventArgs());

        spArgs->m_bHandled = false;
        pEventManager->Raise(
            EventHandle(copyEventIndex),
            true /*bRefire*/,
            this,
            spArgs.get(),
            true /*fRaiseSync*/);

        return !!spArgs->m_bHandled;
    }

    return false;
}

bool CTextBoxBase::RaisePasteEventAndCheckIfHandled()
{
    CEventManager *pEventManager = GetContext()->GetEventManager();
    if (pEventManager)
    {
        xref_ptr<CTextControlPasteEventArgs> spArgs;
        spArgs.init(new CTextControlPasteEventArgs());

        spArgs->m_bHandled = false;
        pEventManager->Raise(
            EventHandle(GetPastePropertyID()),
            true,
            this,
            spArgs.get(),
            true); // Raise Sync

        return !!spArgs->m_bHandled;
    }

    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the measure and render of the contained view.
//
//---------------------------------------------------------------------------
void CTextBoxBase::InvalidateView()
{
    m_pView->InvalidateMeasure();
    CUIElement::NWSetContentAndBoundsDirty(m_pView, DirtyFlags::None);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Forces the view (and RichEdit control) to redraw itself.
//      This is more than just what InvalidateView does since that just
//      invalidates the UI tree properties but the cached drawing results
//      RichEdit are still used.  This also causes RichEdit to redraw itself.
//
//---------------------------------------------------------------------------
void CTextBoxBase::InvalidateViewAndForceRedraw()
{
    m_pView->TxInvalidateRect(NULL, FALSE);
    InvalidateView();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Framework pinvoke, gets the content baseline.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset)
{
    // Default answer
    *pBaselineOffset = 0;

    // The baseline offset returned is in TBV-space. Transform it into TBB-space.
    XFLOAT textViewBaselineOffset = 0;
    IFC_RETURN(m_pView->GetBaselineOffset(&textViewBaselineOffset));

    XPOINTF textViewBaselinePoint = { 0.0f, textViewBaselineOffset };

    xref_ptr<ITransformer> spTransformer;
    if (SUCCEEDED(m_pView->TransformToAncestor(this, spTransformer.ReleaseAndGetAddressOf())))
    {
        XPOINTF baselinePoint = { 0.0f, 0.0f };
        IFC_RETURN(spTransformer->Transform(&textViewBaselinePoint, &baselinePoint, 1));
        *pBaselineOffset = baselinePoint.y;
    }
    else
    {
        *pBaselineOffset = textViewBaselineOffset;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Register with TextInput to receive callback when input language changes
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::InitializeTextServiceManager()
{
    // If we're in the context of XAML islands, then we don't want to use GetForCurrentView -
    // that requires CoreWindow, which is not supported in islands.
    if (GetContext()->GetInitializationType() != InitializationType::IslandsOnly)
    {
        ComPtr<wut::Core::ICoreTextServicesManagerStatics> spTextServicesManagerStatics;

        IFC_NOTRACE_RETURN(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Text_Core_CoreTextServicesManager).Get(), &spTextServicesManagerStatics));


        IFC_NOTRACE_RETURN(spTextServicesManagerStatics->GetForCurrentView(&m_spTextServiceManager));
        IFC_NOTRACE_RETURN(m_spTextServiceManager->add_InputLanguageChanged(
            Callback<wf::ITypedEventHandler<wut::Core::CoreTextServicesManager*, IInspectable*>>(
            this, &CTextBoxBase::OnCurrentInputLanguageChanged).Get(), &m_inputLanguageChangedToken));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Callback from TextInput, XAML notifies RichEdit about the change
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnCurrentInputLanguageChanged(
    _In_ wut::Core::ICoreTextServicesManager *,
    _In_ IInspectable *)
{
    HString languageTag;
    LCID lcid;

    ComPtr<wg::ILanguage> spLanguage;

    // E_FAIL will be returned when there is no input language to be returned.
    // This usually happens when input service is restarting or down.
    if (m_spTextServiceManager && m_spTextServiceManager->get_InputLanguage(&spLanguage) == E_FAIL)
    {
        return S_OK;
    }

    IFC_RETURN(spLanguage->get_LanguageTag(languageTag.GetAddressOf()));

    UINT32 uiInputScope;
    if (IsPassword())
    {
        uiInputScope = IS_PASSWORD;
    }
    else
    {
        InputScopeNameValue isNameValue;
        IFC_RETURN(GetInputScope(&isNameValue));
        uiInputScope = static_cast<UINT32>(isNameValue);
    }

    bool isLTRInputScope =
           (uiInputScope == IS_DIGITS)
        || (uiInputScope == IS_TELEPHONE_FULLTELEPHONENUMBER)
        || (uiInputScope == IS_NUMERIC_PIN)
        || (uiInputScope == IS_NUMERIC_PASSWORD);

    if (isLTRInputScope)
    {
        lcid = 0x0409; // Always use en-us for numeric SIPs
    }
    else
    {
        lcid = LocaleNameToLCID(languageTag.GetRawBuffer(nullptr), LOCALE_ALLOW_NEUTRAL_NAMES);
    }

    IFC_RETURN(UpdateKeyboardLCID(lcid));

    return S_OK;
}

// Processes WM_INPUTLANGCHANGE notification from input manager
_Check_return_ HRESULT CTextBoxBase::OnInputLanguageChange(_In_ HKL inputLanguage)
{
    LCID lcid = MAKELCID(LANGIDFROMHKL(inputLanguage), SORT_DEFAULT);
    IFC_RETURN(UpdateKeyboardLCID(lcid));

    return S_OK;
}

// Processes WM_MOVE notification from input manager
_Check_return_ HRESULT CTextBoxBase::OnWindowMoved()
{
    if (m_pView)
    {
        // Force RichEdit to update grippers when window moved
        if (m_pView->AreGrippersVisible())
        {
            IFC_RETURN(m_pView->ShowSelectionGrippers());
        }
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTextBoxBase::UpdateKeyboardLCID
//
// Called when the keyboard locale changes.
//
///////////////////////////////////////////////////////////////////////////////

_Check_return_ HRESULT CTextBoxBase::UpdateKeyboardLCID(LCID lcid)
{
    m_lcid = lcid;
    // Update RichEdit with input language if currently focused and its Text Alignment is set to DTC
    if (IsFocused())
    {
        if (m_textAlignment == DirectUI::TextAlignment::DetectFromContent)
        {

            DWORD dwResult = 0;
            ::GetLocaleInfoW(
                m_lcid,
                LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
                reinterpret_cast<LPWSTR>(&dwResult),
                sizeof(dwResult) / sizeof(wchar_t));

            m_isKeyboardRTL = (dwResult == 1);

            // Check if the current paragraph is empty.  If it is, then set the LTR/RTL
            // direction of the paragraph according to the keyboard locale.
            IFC_RETURN(UpdateCaretParagraphDirection());
        }

        // Update RichEdit with current keyboard input language, this is especially important on phone as RichEdit does not call CW32System::RefreshKeyboardLayout for phone.
        IFC_RETURN(m_pTextServices->TxSendMessage(
            WM_INPUTLANGCHANGE,
            (WPARAM)DEFAULT_CHARSET,
            (LPARAM)m_lcid,
            nullptr));
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTextBoxBase::UpdateCaretParagraphDirection
//
// Check if the caret is empty and, if it is then check if it's in an empty
// paragraph and set the direction accordingly.
//
///////////////////////////////////////////////////////////////////////////////

_Check_return_ HRESULT CTextBoxBase::UpdateCaretParagraphDirection()
{
    auto scopeGuard = wil::scope_exit([&]
    {
        ReleaseDocumentIfNotFocused();
    });

    IFC_RETURN(EnsureDocument());

    // Get the current selection.  If it's start == end then it means it's just
    // the caret.  In that case we try to update the paragraph direction.
    Microsoft::WRL::ComPtr<ITextSelection2> spCurrentPosition;
    IFC_RETURN(m_spDocument->GetSelection2(&spCurrentPosition));

    if (spCurrentPosition) // Null selection is returned when the control is not yet activated
    {
        long selectionStart, selectionEnd;
        IFC_RETURN(spCurrentPosition->GetStart(&selectionStart));
        IFC_RETURN(spCurrentPosition->GetEnd(&selectionEnd));

        if (selectionStart == selectionEnd)
        {
            IFC_RETURN(SetParagraphDirectionIfEmpty(selectionStart));
        }
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// CTextBoxBase::SetParagraphDirectionIfEmpty
//
// Sets the direction of the paragraph at the provided position based on the
// provided LCID if the paragraph is empty.
//
//
///////////////////////////////////////////////////////////////////////////////

_Check_return_ HRESULT CTextBoxBase::SetParagraphDirectionIfEmpty(_In_ long position)
{
    auto scopeGuard = wil::scope_exit([&]
    {
        ReleaseDocumentIfNotFocused();
    });

    IFC_RETURN(EnsureDocument());

    Microsoft::WRL::ComPtr<ITextRange2> spParagraphRange;
    IFC_RETURN(m_spDocument->Range2(position, position, &spParagraphRange));

    BOOL isRichText = AcceptsRichText();
    IFC_RETURN(spParagraphRange->Expand(isRichText ? tomParagraph : tomStory, nullptr));

    long paraStart, paraEnd;
    IFC_RETURN(spParagraphRange->GetStart(&paraStart));
    IFC_RETURN(spParagraphRange->GetEnd(&paraEnd));

    // Check if the paragraph is empty.  This is the case if the start/end are
    // the same or if the paragraph contains one new line character.
    bool isParagraphEmpty = false;
    if (paraStart == paraEnd)
    {
        isParagraphEmpty = true;
    }
    else if (paraStart == paraEnd - 1)
    {
        long ch;
        IFC_RETURN(spParagraphRange->GetChar(&ch));

        if (ch == '\r' || ch == '\n')
        {
            isParagraphEmpty = true;
        }
    }

    // If the paragraph is empty, figure out whether the provided LCID is
    // RTL or LTR.  Setting PFE_RTLPARA makes the paragraph RTL reading and
    // flips the alignment.  Clearing the bit puts things back to LTR reading
    // and normal alignment.
    if (isParagraphEmpty)
    {
        Microsoft::WRL::ComPtr<ITextPara2> spTextPara;
        IFC_RETURN(spParagraphRange->GetPara2(&spTextPara));

        long oldEffects, oldEffectsMask;
        long newEffects = m_isKeyboardRTL ? PFE_RTLPARA : 0;
        IFC_RETURN(spTextPara->GetEffects(&oldEffects, &oldEffectsMask));

        if ((oldEffects & PFE_RTLPARA) != newEffects)
        {
            if (isRichText)
            {
                // Try to change the paragraph direction, but don't fail if we can't.  This can
                // happen if the control is readonly.
                if (SUCCEEDED(spTextPara->SetEffects(newEffects, PFE_RTLPARA)))
                {
                    // If the direction of the paragraph containing the caret changed, we need
                    // RichEdit to update the caret.  There appears to be no API to explicitly
                    // have them do this, but notifying them of a default paragraph format change
                    // (which is close to what we changed) does cause them to call UpdateCaret.
                    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE));
                }
            }
            else
            {
                // In plain text controls there's only the default paragraph format available
                // for us to change, so change it only when there's nothing in the text box
                // and just set the overall paragraph format.
                PARAFORMAT2 pfDelta;
                pfDelta.cbSize = sizeof(pfDelta);
                pfDelta.dwMask = PFM_RTLPARA;
                pfDelta.wEffects = static_cast<WORD>(newEffects);
                LRESULT lret;
                m_pTextServices->TxSendMessage(EM_SETPARAFORMAT, 0, (LPARAM)&pfDelta, &lret);
            }
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::Initialize()
{
    auto scopeGuard = wil::scope_exit([&]
    {
        TraceInitializeTextStop();
    });

    TraceInitializeTextStart();

    IFC_RETURN(InitializeView());

    GetContext()->EnsureOleIntialized();

    // Calling InitializeTextServices() makes COM calls that can lead to bad
    // re-entrancy. We use PauseNewDispatch here to turn off CoreMessaging
    // while we call InitializeTextServices().
    {
        PauseNewDispatch deferReentrancy(GetContext());
        IFC_RETURN(InitializeTextServices());
    }

    // Activate the control.  This means it is visually active, but does not yet have focus.
    // There is no RichEdit selection object until we do this, so it has to happen immediately to
    // support programmatic edits.
    XRECT_RB clientRect;
    IFC_RETURN(m_pView->TxGetClientRect(&clientRect));
    IFC_RETURN(m_pTextServices->OnTxInPlaceActivate(reinterpret_cast<RECT *>(&clientRect)));
    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(InitializeTextServiceManager());
    }

    VariantInit(&s_csetWhitespace);
    s_csetWhitespace.vt = VT_I4;
    s_csetWhitespace.lVal = MAKECSET(CT_CTYPE2, C2_WHITESPACE);

    {
        CREATEPARAMETERS cp(GetContext());
        xref_ptr<CTimeSpan> pTimeSpan;
        CValue value;

        IFC_RETURN(CreateDO(m_spShowGrippersTimer.ReleaseAndGetAddressOf(), &cp));
        value.SetInternalHandler(OnShowGrippersTimeout);
        IFC_RETURN(m_spShowGrippersTimer->AddEventListener(
            EventHandle(KnownEventIndex::DispatcherTimer_Tick),
            &value,
            EVENTLISTENER_INTERNAL, nullptr, false));

        IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));
        pTimeSpan->m_rTimeSpan = 0.2f;
        IFC_RETURN(m_spShowGrippersTimer->SetValueByKnownIndex(
            KnownPropertyIndex::DispatcherTimer_Interval,
            pTimeSpan.get()));
        IFC_RETURN(m_spShowGrippersTimer->SetTargetObject(this));
        IFC_RETURN(m_spShowGrippersTimer->Stop());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Destroys this instance.
//
//---------------------------------------------------------------------------
void CTextBoxBase::Destroy()
{

    if (m_uiManagerEventSink)
    {
        m_uiManagerEventSink->UnadviseControl();
    }

    if (m_tfSource)
    {
        m_tfSource->UnadviseSink(m_uiManagerSinkCookie);
    }

    if (m_pPrivateTextInputSettings)
    {
        m_pPrivateTextInputSettings->Teardown();
        m_pPrivateTextInputSettings = nullptr;
    }

    if (m_inputLanguageChangedToken.value != 0)
    {
        m_spTextServiceManager->remove_InputLanguageChanged(m_inputLanguageChangedToken);
    }

    if (m_pTextServices)
    {
        VERIFYHR(m_pTextServices->TxSendMessage(EM_SETOLECALLBACK, 0, NULL, NULL)); // TODO: TxMessage here and elsewhere.

        VERIFYHR(m_pTextServices->OnTxInPlaceDeactivate());
    }

    if (m_proofingMenu)
    {
        m_proofingMenu->UnpegManagedPeer();
        m_proofingMenu->RemoveParent(this);
        m_proofingMenu.reset();
    }

    ReleaseInterface(m_pRichEditOleCallback);

    if (m_pView)
    {
        if (m_isViewRegisteredForGripperUpdates)
        {
            VERIFYHR(GetContext()->UnregisterGripper(m_pView));
            m_isViewRegisteredForGripperUpdates = false;
        }

        m_pView->ClearOwnerTextControl();
        VERIFYHR(RemovePeerReferenceToView());
    }

    ReleaseInterface(m_pView);
    m_pHostRef.reset();
    ReleaseInterface(m_pLastPointer);

    ReleaseInterface(m_pSelectionHighlightColor);
    ReleaseInterface(m_pSelectionHighlightColorWhenNotFocused);

    // The below call to Destroy must happen before releasing m_pTextServices as
    // RichEdit releases all of it's states leaving dangling pointers.
    if (m_pWindowlessHost)
    {
        IRicheditWindowlessAccessibility* pRichEditWindowlessAcc = nullptr;
        IGNOREHR(GetTextServices()->QueryInterface(IID_IRicheditWindowlessAccessibility, reinterpret_cast<void **>(&pRichEditWindowlessAcc)));
        m_pWindowlessHost->Destroy(reinterpret_cast<void *>(pRichEditWindowlessAcc));
        ReleaseInterface(pRichEditWindowlessAcc);
    }

    // Calling ShutdownTextServices() makes COM calls that can lead to bad
    // re-entrancy. We use PauseNewDispatch here to turn off CoreMessaging
    // while we call ShutdownTextServices().
    {
        PauseNewDispatch deferReentrancy(GetContext());
        if (m_pFnShutdownTextServices && SUCCEEDED(m_pFnShutdownTextServices(m_pTextServices)))
        {
            m_pTextServices = nullptr;
        }
    }

    ReleaseInterface(m_pTextServices);
    ReleaseInterface(m_pInputScope);
    ReleaseInterface(m_pHost);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerEntered event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerEntered(_In_ CEventArgs* pEventArgs)
{
    m_isPointerOver = true;
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerExited event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerExited(_In_ CEventArgs* pEventArgs)
{
    m_isPointerOver = false;
    IFC_RETURN(UpdateVisualState());
    // Need to send a message to stop the interaction context if we leave a text box
    IFC_RETURN(SendPointerMessage(WM_POINTERLEAVE, pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerPressed event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerPressed(_In_ CEventArgs* pEventArgs)
{
    auto scopeGuard = wil::scope_exit([&] {
        m_ignoreScrollIntoView = false;
    });

    bool sendPointerDown = true;
    CPointerEventArgs *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);

    if (!FocusOnPointerReleased(pPointerEventArgs) && !IsFocused())
    {
        IFC_RETURN(SetFocusFromPointer(pPointerEventArgs, sendPointerDown));
    }

    if (sendPointerDown)
    {
        IFC_RETURN(SendPointerMessage(WM_POINTERDOWN, pEventArgs));
        pPointerEventArgs->m_bHandled = TRUE;
    }

    const bool inputIsTouch = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch;

    if (inputIsTouch && DesktopUtility::IsOnDesktop() && IsFocused() && GetContext()->IsTSF3Enabled())
    {
        // On desktop, Text Input Service evaluates the pointer up against the control layout rect provided by XAML to decide if SIP should be shown.
        // Unfortunately, the layout rect is in screen coordinates, which means the rect value may have already changed without TextBox knowing it happened.
        // For example, bring into view when SIP is shown. It would not have been an issue if TSF3 layout API was not screen coordinates based in the first place.
        // Easiest and most straight forward solution/workaround is to force TSF3 getting the latest screen coordinates for every touch pointer down.
        IFC_RETURN(EnsureTextInputSettings());
        IFC_RETURN(m_pPrivateTextInputSettings->NotifyLayoutChanged());
    }

    return S_OK;
}

bool CTextBoxBase::ShouldFowardMoveMessage(_In_ const CPointerEventArgs* pPointerEventArgs) const
{
    const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    // Only foward when input is pen and barrel button is pressed
    // or when the input is touch or mouse.
    return ((inputIsPen && m_barrelButtonPressed) || !inputIsPen);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerMoved event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerMoved(_In_ CEventArgs* pEventArgs)
{
    const CPointerEventArgs *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    // Only foward when input is pen and barrel button is pressed
    // or when the input is touch or mouse.
    if (ShouldFowardMoveMessage(pPointerEventArgs))
    {
        IFC_RETURN(SendPointerMessage(WM_POINTERUPDATE, pEventArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnShowGrippersTimeout(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CDispatcherTimer *pTimer = nullptr;
    IFC_RETURN(DoPointerCast(pTimer, pSender));

    CTextBoxBase *pThis = nullptr;
    IFC_RETURN(DoPointerCast(pThis, pTimer->GetTargetObject()));

    IFCPTR_RETURN(pThis);
    IFC_RETURN(pThis->OnShowGrippersTimeout());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnShowGrippersTimeout()
{
    IFC_RETURN(m_spShowGrippersTimer->WorkComplete());
    IFC_RETURN(m_spShowGrippersTimer->Stop());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnDelayEnsureRectVisible(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CDispatcherTimer *pTimer = nullptr;
    IFC_RETURN(DoPointerCast(pTimer, pSender));

    // Timer can fire after object (weak ref) is released
    xref_ptr<CDependencyObject> pThisAsDO(pTimer->GetTargetObject());
    if (auto pThis = do_pointer_cast<CTextBoxBase>(pThisAsDO))
    {
      IFC_RETURN(pThis->OnDelayEnsureRectVisible());
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnDelayEnsureRectVisible()
{
    IFC_RETURN(m_spEnsureRectVisibleTimer->WorkComplete());
    IFC_RETURN(m_spEnsureRectVisibleTimer->Stop());

    IFC_RETURN(BringLastVisibleRectIntoView(true /* scrollTextBoxIntoView */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerReleased event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerReleased(_In_ CEventArgs* pEventArgs)
{
    bool sendPointerUp = true;
    CPointerEventArgs *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);

    const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;

    if (FocusOnPointerReleased(pPointerEventArgs) &&
        !IsFocused() &&
        !FxCallbacks::TextControlFlyout_IsOpen(GetContextFlyout().get()))
    {
        IFC_RETURN(SetFocusFromPointer(pPointerEventArgs, sendPointerUp));
    }

    const bool rightClickWasPressed = m_rightButtonPressed;

    if (sendPointerUp)
    {
        const bool barrelWasPressed = m_barrelButtonPressed;
        const bool leftClickWasPressed = m_leftButtonPressed;

        IFC_RETURN(SendPointerMessage(WM_POINTERUP, pEventArgs));

        if (inputIsPen)
        {
            const bool barrelButtonReleased = barrelWasPressed && !m_barrelButtonPressed;
            const bool leftButtonReleased = leftClickWasPressed && !m_leftButtonPressed;
            if (barrelButtonReleased || leftButtonReleased)
            {
                IFC_RETURN(m_spShowGrippersTimer->Start());
            }
        }

        // Show gripper again when OnGotFocus is called. For Focus on PointerReleased scenario and user switching from one
        // TextBox to another, gripper could be hidden by OnLostFocus call from other TextBox, so bring it back at OnGotFocus.
        if (m_pView->AreGrippersVisible())
        {
            m_pView->ShowGripperOnGotFocus();
        }
    }

    // If the pointer released was the right mouse pointer, then that'll trigger a context flyout.
    // We shouldn't be showing the selection flyout in that circumstance.
    if (IsFocused() &&
        !rightClickWasPressed)
    {
        IFC_RETURN(QueueUpdateSelectionFlyoutVisibility());
    }

    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for PointerCaptureLost event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPointerCaptureLost(_In_ CEventArgs* pEventArgs)
{
    m_isPointerOver = false;
    IFC_RETURN(UpdateVisualState());
    IFC_RETURN(SendPointerMessage(WM_POINTERCAPTURECHANGED, pEventArgs));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      DoubleTap reported by gesture engine. Convert to mouse left button double click
//      if device type is mouse. Don't do anything for other device types, RichEdit can
//      handle those.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnDoubleTapped(_In_ CEventArgs* pEventArgs)
{
    CPointerEventArgs *pDoubleTappedEventArgs = NULL;
    XLONG_PTR result = 0;

    pDoubleTappedEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    if (pDoubleTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch)
    {
        if (IsFocused())
        {
            IFC_RETURN(SendGripperHostTapMessage(pDoubleTappedEventArgs, 2));
        }
    }
    else
    {
        // RichEdit expect mouse messages from mouse devices,
        // so do the conversion.
        RE_MOUSEINPUT reMouseInput;
        IFC_RETURN(PackageMouseEventParams(WM_LBUTTONDBLCLK, pDoubleTappedEventArgs, DirectUI::VirtualKeyModifiers::None, pDoubleTappedEventArgs->m_pointerDeviceType, &reMouseInput));
        IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_MOUSEINPUT, WM_LBUTTONDBLCLK, 0, reinterpret_cast<LPARAM>(&reMouseInput), &pDoubleTappedEventArgs->m_bHandled, &result));

        const bool inputIsPen = pDoubleTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
        if (inputIsPen)
        {
            IFC_RETURN(OnShowGrippersTimeout());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      RightTap reported by gesture engine. Mark as handled so controls
//      up the tree ignores it. RichEdit handles this using OnPointerReleased.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnRightTapped(_In_ CEventArgs* pEventArgs)
{
    CRightTappedEventArgs* pRightTappedEventArgs = static_cast<CRightTappedEventArgs*>(pEventArgs);

    if (m_handleRightTappedEvent)
    {
        const bool inputIsPen = pRightTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
        if (inputIsPen)
        {
            // Convert RightTapped to Right Mouse Button Up in order to show the context menu.
            RE_MOUSEINPUT reMouseInput;
            IFC_RETURN(PackageMouseEventParams(WM_RBUTTONUP, pRightTappedEventArgs, DirectUI::VirtualKeyModifiers::None, pRightTappedEventArgs->m_pointerDeviceType, &reMouseInput));
            XLONG_PTR result = NULL;
            IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_MOUSEINPUT, WM_RBUTTONUP, 0, reinterpret_cast<LPARAM>(&reMouseInput), &pRightTappedEventArgs->m_bHandled, &result));
        }
        else
        {
            pRightTappedEventArgs->m_bHandled = TRUE;
        }
    }

    m_handleRightTappedEvent = true;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Tap reported by gesture engine. Mark as handled so controls
//      up the tree ignores it. RichEdit handles this using
//      OnPointerPressed and OnPointerReleased.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnTapped(_In_ CEventArgs* pEventArgs)
{
    if (IsFocused())
    {
        CPointerEventArgs *pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);


        if (m_bPreventKeyboardDisplayOnProgrammaticFocus)
        {
            IFC_RETURN(UpdateSIPSettings(DirectUI::FocusState::Pointer));
        }
        IFC_RETURN(ForceNotifyFocusEnterIfNeeded());

        const bool inputIsTouch = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch;
        if (inputIsTouch)
        {
            IFC_RETURN(SendGripperHostTapMessage(pPointerEventArgs));
        }
    }

    CTappedEventArgs* pTappedEventArgs = static_cast<CTappedEventArgs*>(pEventArgs);
    pTappedEventArgs->m_bHandled = TRUE;

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnGripperDoubleTapped(_In_ CEventArgs* pEventArgs)
{
    CInputPointEventArgs* pTappedEventArgs = static_cast<CInputPointEventArgs*>(pEventArgs);
    const bool inputIsPen = pTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    const bool enablePenDoubleTap = inputIsPen;

    if (enablePenDoubleTap)
    {
        IFC_RETURN(OnGripperTapped(pEventArgs, true /* doubleTap*/));
    }
    else
    {
        IFC_RETURN(OnGripperTapped(pEventArgs));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::SendMouseInput(
    _In_ XUINT32 message,
    _In_ CInputPointEventArgs *pInputPointEventArgs)
{
    RE_MOUSEINPUT reMouseInput = {};
    IFC_RETURN(PackageMouseEventParams(
        message,
        pInputPointEventArgs,
        DirectUI::VirtualKeyModifiers::None,
        pInputPointEventArgs->m_pointerDeviceType,
        &reMouseInput));

    XLONG_PTR result = NULL;
    IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(
        this,
        EM_MOUSEINPUT,
        message,
        0,
        reinterpret_cast<LPARAM>(&reMouseInput),
        &pInputPointEventArgs->m_bHandled, &result));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Handles taps reported through through a gripper (since a tap on a
//      gripper is just passed through to the owning text box).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnGripperTapped(_In_ CEventArgs* pEventArgs, bool doubleTap)
{
    CPointerEventArgs* pTappedEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    pTappedEventArgs->m_bHandled = TRUE;

    if (!m_ignoreGripperTouchEvents)
    {
        const bool inputIsPen = pTappedEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
        const bool enablePenTap = inputIsPen;

        if (enablePenTap && !doubleTap)
        {
            IFC_RETURN(SendMouseInput(WM_LBUTTONDOWN, pTappedEventArgs));
            IFC_RETURN(SendMouseInput(WM_LBUTTONUP, pTappedEventArgs));
            IFC_RETURN(m_pView->ShowSelectionGrippers());
        }
        else if (IsPassword() && !IsPasswordRevealed())
        {
            // Selection will be toggled if user taps inside a password box and
            // the password is hidden.
            bool hasSelection = false;

            ctl::ComPtr<ITextSelection2> spSelection;
            IFC_RETURN(GetSelection(&spSelection));
            if (spSelection != nullptr)
            {
                LONG selectionStart;
                LONG selectionEnd;

                IFC_RETURN(spSelection->GetStart(&selectionStart));
                IFC_RETURN(spSelection->GetEnd(&selectionEnd));

                hasSelection = (selectionStart != selectionEnd);
            }

            if (hasSelection)
            {
                // Place caret at the end
                IFC_RETURN(spSelection->Collapse(false));
            }
            else
            {
                IFC_RETURN(SelectAll());
            }
        }
        else
        {
            IFC_RETURN(SendGripperHostTapMessage(pTappedEventArgs));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::SendGripperHostTapMessage(_In_ CPointerEventArgs* pPointerEventArgs, int tapCount)
{
    CANVASTOUCHPARAMS params;
    LRESULT lres = 0;
    INT32 cpTapped = -1;

    wf::Point position;
    POINT ptTouch;

    // Get the pointer position relative to the inner CTextBoxView.
    IFC_RETURN(pPointerEventArgs->GetPosition(m_pView, &position));

    ptTouch.x = static_cast<int>(position.X);
    ptTouch.y = static_cast<int>(position.Y);

    params.ptTouch = ptTouch;
    params.tapCnt = tapCount;
    params.pcpResult = &cpTapped;

    IFC_RETURN(m_pTextServices->TxSendMessage(EM_ONCANVASTOUCHTAP, (WPARAM)&params, NULL, &lres));

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnGripperPressedWithBarrelButtonDown(_In_ CEventArgs* pEventArgs)
{
    // Clear selection and forward the pen pointer down to textbox to start a new selection
    ctl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));
    if (spSelection != nullptr)
    {
        IFC_RETURN(spSelection->Collapse(false));
    }

    IFC_RETURN(OnPointerPressed(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnGripperRightTapped(_In_ CEventArgs* pEventArgs)
{
    const CInputPointEventArgs* pPointerEventArgs = static_cast<CInputPointEventArgs*>(pEventArgs);
    const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    if (inputIsPen)
    {
        IFC_RETURN(OnRightTapped(pEventArgs));
        IFC_RETURN(m_pView->ShowSelectionGrippers());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for KeyUp event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnKeyUp(_In_ CEventArgs* pEventArgs)
{
    if (ShouldProcessKeyMessage(pEventArgs))
    {
        CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
        IFC_RETURN(SendKeyMessage(WM_KEYUP, pEventArgs));
        if (pKeyEventArgs->m_platformKeyCode == TextContextMenu::ContextMenuKeyCode
            && !m_handleRightTappedEvent)
        {
            // Do not handle the KeyUp event to let the RightTapped fire.
            // If m_handleRightTappedEvent is set to false this means the CM
            // did not show because it had no options to display. In this case we
            // want the RightTapped event to bubble up in order for the parent control
            // to get a chance to handle it.
            pKeyEventArgs->m_bHandled = FALSE;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for KeyDown event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnKeyDown(_In_ CEventArgs* pEventArgs)
{
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    if (ShouldProcessKeyMessage(pEventArgs))
    {
        auto scopeGuard = wil::scope_exit([&] {
            XUINT32 modifierKeys = 0;
            IFCFAILFAST(gps->GetKeyboardModifiersState(&modifierKeys));

            if (!pKeyEventArgs->m_bHandled
            && KeyboardAcceleratorUtility::IsKeyValidForAccelerators(pKeyEventArgs->m_platformKeyCode, modifierKeys))
            {
                bool shouldNotImpedeTextInput = KeyboardAcceleratorUtility::TextInputHasPriorityForKey(pKeyEventArgs->m_platformKeyCode,
                    pKeyEventArgs->IsCtrlPressed(),
                    pKeyEventArgs->IsAltPressed());
                pKeyEventArgs->put_HandledShouldNotImpedeTextInput(shouldNotImpedeTextInput);
            }
        });

        IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));

        if (XboxUtility::IsGamepadNavigationAccept(pKeyEventArgs->m_originalKeyCode))
        {
            if (!GetContext()->IsTSF3Enabled())
            {
                pKeyEventArgs->m_bHandled = TRUE;
                return S_OK;
            }
            IFC_RETURN(EnsureTextInputSettings());
            m_gamepadEngaged = true;

            IFC_RETURN(m_pPrivateTextInputSettings->UpdateSIPSettings(false));
            m_manualInputPaneEnabled = false;

            pKeyEventArgs->m_bHandled = TRUE;
        }
        else if (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_PageUp || pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_PageDown)
        {
            XRECTF caretRectBefore = GetView()->GetCaretRect();
            CTextBoxView_ScrollCommand scrollCommand = (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_PageUp) ? CTextBoxView_ScrollCommand::PageUp : CTextBoxView_ScrollCommand::PageDown;

            // We still need to detect if the caret moved and not use the scrolled flag
            // since a pagedown can move the caret and not cause a scroll.
            IFC_RETURN(GetView()->Scroll(scrollCommand, TRUE /*moveCaret*/, pKeyEventArgs->IsShiftPressed() /*expandSelection*/, 0 /*mouseWheelDelta*/, NULL /*scrolled*/));

            XRECTF caretRectAfter = GetView()->GetCaretRect();
            if (caretRectBefore.X != caretRectAfter.X || caretRectBefore.Y != caretRectAfter.Y)
            {
                pKeyEventArgs->m_bHandled = TRUE;
            }
        }
        else
        {
            // Intercept paste event for keyboard paste. (control +v and shift + insert)
            if (pKeyEventArgs->m_xEditKey == XEDITKEY_PASTE)
            {
                if (!IsReadOnly())
                {
                    IFC_RETURN(Paste());
                }
            }
            else if (pKeyEventArgs->m_xEditKey == XEDITKEY_COPY)
            {
                IFC_RETURN(Copy());
            }
            else if (pKeyEventArgs->m_xEditKey == XEDITKEY_CUT)
            {
                if (!IsReadOnly())
                {
                    IFC_RETURN(Cut());
                }
            }
            else if ((pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_F10 && pKeyEventArgs->IsShiftPressed()) ||
                     (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_GamepadMenu))
            {
                // If the ContextFlyout property is set,
                // don't fake a context menu key up event.
                if (HasContextFlyout()) { return S_OK; }

                // Store the original platform key code.
                wsy::VirtualKey platformKeyCode = pKeyEventArgs->m_platformKeyCode;

                // Fake a context menu key up event and send to RichEdit, so it will open the context menu.
                pKeyEventArgs->m_platformKeyCode = TextContextMenu::ContextMenuKeyCode;
                IFC_RETURN(SendKeyMessage(WM_KEYUP, pEventArgs));

                // In case RichEdit does not handle this event, we still need to restore the platform key code
                // to its original state.
                pKeyEventArgs->m_platformKeyCode = platformKeyCode;
            }
            else if (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Back && pKeyEventArgs->IsAltPressed())
            {
                IFC_RETURN(SendKeyMessage(WM_SYSKEYDOWN, pEventArgs));
            }
            else
            {
                IFC_RETURN(SendKeyMessage(WM_KEYDOWN, pEventArgs));
            }
        }
    }

    return S_OK;
}


_Check_return_ HRESULT CTextBoxBase::NotifyEditFocusLost()
{
    m_gamepadEngaged = false;
    IFC_RETURN(EnsureTextInputSettings());
    IFC_RETURN(m_pPrivateTextInputSettings->UpdateSIPSettings(true));
    m_manualInputPaneEnabled = true;

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::NotifyEditControlInputPaneHiding()
{
    // Request a call to NotifyFocusEnter for Tapped or UIA SELECT next time,
    // in order to bring up SIP in case it was manually dismissed.
    m_forceNotifyFocusEnter = true;
    return S_OK;
}


_Check_return_ HRESULT CTextBoxBase::ForceNotifyFocusEnterIfNeeded()
{
    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(EnsureTextInputSettings());
        if (m_forceNotifyFocusEnter)
        {
            m_forceNotifyFocusEnter = false;
            IFC_RETURN(m_pPrivateTextInputSettings->NotifyFocusEnter());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::ForceEditFocusLoss()
{
    IFC_RETURN(GetTextServices()->TxSendMessage(WM_KILLFOCUS, 0, 0, nullptr));
    IFC_RETURN(GetTextServices()->OnTxUIDeactivate());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::ForceFocusLoss()
{
    m_forceFocusedVisualState = false;

    IFC_RETURN(GetView()->OnLostFocus());
    IFC_RETURN(UpdateVisualState());
    IFC_RETURN(ForceEditFocusLoss());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::DismissAllFlyouts()
{
    bool focusUpdated = false;
    CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
    const DirectUI::FocusState focusState = pFocusManager->GetRealFocusStateForFocusedElement();
    IFC_RETURN(Focus(focusState, false /*animateIfBringIntoView*/, &focusUpdated));
    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(do_pointer_cast<CFlyoutBase>(m_proofingMenu.get())));
    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));
    IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetContextFlyout().get()));

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for GotFocus event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnGotFocus(_In_ CEventArgs* pEventArgs)
{
    if (!IsEnabled() && AllowFocusWhenDisabled())
    {
        IFC_RETURN(UpdateVisualState());
        return S_OK;
    }

    // Verify that this textbox is still the focused element.
    // This is to account for a scenario where a new element gains focus
    // between focus manager queueing the GotFocus event and it being raised.
    auto const focusManager = VisualTree::GetFocusManagerForElement(this);
    if (focusManager && pEventArgs)
    {
        auto const focusedElement = focusManager->GetFocusedElementNoRef();
        auto const pRoutedEventArgs = static_cast<CRoutedEventArgs*>(pEventArgs);
        auto const originalSource = pRoutedEventArgs->m_pSource;
        auto const textBox = static_cast<CDependencyObject*>(this);
        // Only return if this TextBox is the original source of the OnGotFocus event.
        // Other controls may directly call this OnGotFocus function and we do not want
        // to block those calls.
        if(originalSource == textBox && focusedElement != textBox)
        {
            return S_OK;
        }
    }

    if (m_forceFocusedVisualState)
    {
        m_forceFocusedVisualState = false;

        if (m_shouldHideGrippersOnFlyoutOpening)
        {
            IFC_RETURN(GetView()->OnContextMenuDismiss());
        }
    }
    else
    {
        IFC_RETURN(SendRichEditFocus());
        IFC_RETURN(GetView()->OnGotFocus());
    }

    IFC_RETURN(UpdateVisualState());

    // We call UpdateLastSelectedTextElement here to make sure our state is updated
    // in case the focus was gained as a result on mouse or pen input.
    IFC_RETURN(UpdateLastSelectedTextElement());

    // On Phone, we do not want to scroll headers into view if the input pane is not showing, since we
    // would not try to bring the last rect into view.
    // However, on Desktop, bring in to view logic does not depend on the input pane.
    // As a result, we always want to attempt to scroll headers into view.
    m_shouldScrollHeaderIntoView = true;

    // We intentionally did not send WM_KILLFOCUS to RichEdit at OnLostFocus, and it will skip notifying TSF3 of
    // focus enter in WM_SETFOCUS call, so we notify TSF3 directly here.
    IFC_RETURN(ForceNotifyFocusEnterIfNeeded());

    if (m_spTextServiceManager)
    {
        IFC_RETURN(OnCurrentInputLanguageChanged(nullptr, nullptr)); // get the current input language now
    }
    else
    {
        if (CInputServices* inputServices = GetContext()->GetInputServices())
        {
            LCID lcid = MAKELCID(LANGIDFROMHKL(inputServices->GetInputLanguage()), SORT_DEFAULT);
            UpdateKeyboardLCID(lcid);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for LostFocus event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnLostFocus(_In_ CEventArgs* pEventArgs)
{
    auto scopeGuard = wil::scope_exit([&] {
        m_ignoreKillFocus = false;
        m_spDocument = nullptr; // release richedit document object when focus is gone
    });

    if (m_forceFocusedVisualState && m_shouldHideGrippersOnFlyoutOpening)
    {
        auto lastInputDeviceType = VisualTree::GetContentRootForElement(this)->GetInputManager().GetLastInputDeviceType();
        IFC_RETURN(GetView()->OnContextMenuOpen(lastInputDeviceType == DirectUI::InputDeviceType::Touch));
    }


    // RichEdit will send back EN_KILLFOCUS which we should ignore, because we
    // don't need/want to force remove focus from whichever control now has focus.
    m_ignoreKillFocus = true;
    m_shouldScrollHeaderIntoView = false;

    if (!m_forceFocusedVisualState)
    {
        IFC_RETURN(GetView()->OnLostFocus());
        IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));
    }

    IFC_RETURN(UpdateVisualState());

    m_canEnableManualInputPane = false;

    if (GetContext()->IsTSF3Enabled())
    {
        // If the SIP is currently up we should keep it up when we are in manual SIP mode
        if (m_gamepadEngaged && m_preventEditFocusLoss)
        {
            IFC_RETURN(EnsureTextInputSettings());

            IFC_RETURN(m_pPrivateTextInputSettings->UpdateSIPSettings(!!m_preventEditFocusLoss));
            m_manualInputPaneEnabled = m_preventEditFocusLoss;
        }

        m_gamepadEngaged = false;
    }

    // In most cases, we should sync with TSF about the focus state, but special app dependencies have developed for years which
    // requires hiding the focus lost state from TSF, scenarios below are very important to not regress if this code is changed:
    // For Cortana:
    //     - Press Window key to activate the Cortana's search box, quickly follow by typing the search string (i.e. type Win + "notepad" quickly):
    //       make sure all key strokes show up in the Cortana's search box (i.e. does not show incomplete string such as "epad").
    //       This is special Cortana scenario that we don't notify TSF if focus lost due to WM_DEACTIVATE, and we want the text edit focus
    //       stays in the Cortana's search box.
    //     - After results are shown, expand the web search results which will put focus into edgehtml, type some keys and make sure the
    //       text input does not go to the Cortana's search box.
    // For Microsoft Edge:
    //      - Type bing.com in AddressBox and navigate, focus should move to the textbox in the webpage. Ctrl-f to bring up the "Find" box, click into AddressBox again and typing should still work.
    //      - Japanese IME, switch back and forth between Microsoft Edge (focus on AddressBox) and Notepad, make sure Japanese IME is not disabled in AddressBox.

    bool fNotifyTSF = true; // for most cases we should notify TSF about focus leave by sending WM_KILLFOCUS to RichEdit.

    // special consideration when plugin is not focused and m_preventEditFocusLoss is not true.
    auto focusManager = VisualTree::GetFocusManagerForElement(this);
    if (!focusManager->IsPluginFocused() && !m_preventEditFocusLoss)
    {
        // if TSF3 is not enabled, we should skip
        if (!GetContext()->IsTSF3Enabled())
        {
            fNotifyTSF = false;
        }
    }

    if (fNotifyTSF && !m_forceFocusedVisualState)
    {
        IFC_RETURN(GetTextServices()->TxSendMessage(WM_KILLFOCUS, 0, 0, nullptr));
        IFC_RETURN(GetTextServices()->OnTxUIDeactivate());
    }

    // if there was candidate windown bound changed event fired earlier, we should always fire a zero sized candidate window event when focus lost
    if (m_firedCandidateWindowEventAfterFocus)
    {
        XRECTF empty {};
        m_firedCandidateWindowEventAfterFocus = false;
        IFC_RETURN(RaiseCandidateWindowBoundsChangedEvent(empty));
    }

    // Lost focus does not necessary trigger redrawing of contents, sometimes it only needs to redraw border
    // Force redraw if SelectionHighlightColorWhenNotFocused is set by app
    if (m_pSelectionHighlightColorWhenNotFocused != nullptr)
    {
        InvalidateViewAndForceRedraw();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for IsEnabledChanged event.
//
//  Notes:
//      Although we don't do anything here, the default implementation
//      returns E_NOTIMPL which creates a lot of debug spew and short circuits
//      caller logic.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnIsEnabledChanged(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(UpdateVisualState());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for CharacterReceived event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnCharacterReceived(_In_ CEventArgs* pEventArgs)
{
    if (!IsEnabled()) // Disabled TextBox can receive this call because AllowFocusWhenDisabled is true
    {
        return S_OK;
    }

    if (GetContext()->GetInputServices()->IsUsingInternalTextInputProducer())
    {
        // For UWP text input, character insertion occurs via the TSF3 document interfaces
        return S_OK;
    }

    CCharacterReceivedRoutedEventArgs *pCharacterReceivedArgs;
    XUINT32 wParam;
    XUINT32 lParam;
    XLONG_PTR result;

    pCharacterReceivedArgs = static_cast<CCharacterReceivedRoutedEventArgs*>(pEventArgs);
    ASSERT(!pCharacterReceivedArgs->m_bHandled);

    if (pCharacterReceivedArgs->m_platformKeyCode)
    {
        XUINT32 message = WM_NULL;
        wParam = pCharacterReceivedArgs->m_platformKeyCode;
        lParam = 1; // TODO: ignoring some bits.

        switch (pCharacterReceivedArgs->m_msgID)
        {
            case XCP_CHAR:
                message = WM_CHAR;
                break;
            case XCP_DEADCHAR:
                message = WM_DEADCHAR;
                break;
            default:
                IFC_RETURN(E_FAIL);
                break;
        }

        bool handled = false;
        IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, message, message, wParam, lParam, &handled, &result));

        // pCharacterReceivedArgs->m_bHandled is not set to the 'handled' value so that app code does receive the UIElement.CharacterReceived
        // event like in the UWP branch above, even for handlers with handledEventsToo==False.
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for Holding event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnHolding(_In_ CEventArgs* pEventArgs)
{
    CHoldingEventArgs *pHoldingEventArgs = static_cast<CHoldingEventArgs*>(pEventArgs);
    const bool inputIsPen = pHoldingEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    const bool inputIsTouch = pHoldingEventArgs->m_holdingState == DirectUI::HoldingState::Started && pHoldingEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch;
    const bool holdStarted = pHoldingEventArgs->m_holdingState == DirectUI::HoldingState::Started;
    const bool holdCompleted = pHoldingEventArgs->m_holdingState == DirectUI::HoldingState::Completed;

    if (holdCompleted && inputIsPen)
    {
        // RichEdit does not support Pen input directly.
        // Convert Holding Completed to Right Mouse Button Up in order to show the context menu.
        RE_MOUSEINPUT reMouseInput;
        IFC_RETURN(PackageMouseEventParams(WM_RBUTTONUP, pHoldingEventArgs, DirectUI::VirtualKeyModifiers::None, pHoldingEventArgs->m_pointerDeviceType, &reMouseInput));
        XLONG_PTR result = NULL;
        IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_MOUSEINPUT, WM_RBUTTONUP, 0, reinterpret_cast<LPARAM>(&reMouseInput), &pHoldingEventArgs->m_bHandled, &result));
    }
    else if (holdStarted && inputIsTouch)
    {
        // Since TextServicesHost exposes IGripperHost, we are working with the immersive version of msftedit touch
        // code. We place the caret in response to a press-and-hold on the RichEdit canvas.
        wf::Point position;
        // Get the pointer position relative to the inner CTextBoxView.
        IFC_RETURN(pHoldingEventArgs->GetPosition(m_pView, &position));

        POINT ptTouch;
        ptTouch.x = static_cast<int>(position.X);
        ptTouch.y = static_cast<int>(position.Y);

        INT32 cpResult = -1;
        CANVASTOUCHPARAMS params = {};
        params.ptTouch = ptTouch;
        params.pcpResult = &cpResult;

        LRESULT lres = 0;
        // Forward to msftedit (touch.cpp) to handle caret placement.
        m_pTextServices->TxSendMessage(EM_ONCANVASTOUCHHOLD, (WPARAM) &params, NULL, &lres);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Performs late template-dependent initialization. Attaches the
//      control to the container element from the template. Initializes
//      visual states.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnApplyTemplate()
{
    IFC_RETURN(DetachFromHost());

    xref_ptr<CDependencyObject> spContentHost = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"ContentElement"));
    if (!spContentHost || !spContentHost->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
    {
        return S_OK;
    }

    if (FAILED(AttachToHost(do_pointer_cast<CFrameworkElement>(spContentHost.get()))))
    {
        // If we fail to attach the TBV, we fail silently
        return S_OK;
    }

    IFC_RETURN(AddPeerReferenceToView());
    IFC_RETURN(UpdateVisualState());

    IFC_RETURN(__super::OnApplyTemplate());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notifies elements that the theme has changed, and updates the gripper color
//      based on the new theme.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    IFC_RETURN(CFrameworkElement::NotifyThemeChangedCore(theme, fForceRefresh));

    if (m_pHost)
    {
        IFC_RETURN(m_pHost->UpdateGripperColors());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a flag noting that the caller is setting property values locally
//      inside SetValue.
//
//---------------------------------------------------------------------------
void CTextBoxBase::EnableSetValueReentrancyGuard()
{
    m_setValueReentrancyCount++;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Clears a flag noting that the caller is setting property values locally
//      inside SetValue.
//
//---------------------------------------------------------------------------
void CTextBoxBase::ClearSetValueReentrancyGuard()
{
    ASSERT(m_setValueReentrancyCount > 0);
    m_setValueReentrancyCount--;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the caller is setting property values locally
//      inside SetValue somewhere higher up the stack.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::IsInSetValue() const
{
    return m_setValueReentrancyCount > 0;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a DependencyProperty value.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetValue(_In_ const SetValueParams& args)
{
    const bool wasFocused = !!IsFocused();
    const bool wasEnabled = !!IsEnabled();

    IFC_RETURN(CControl::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::UIElement_FocusState:
        if (m_pView)
        {
            if (wasFocused != !!IsFocused())
            {
                // If focus was lost, unregister the view for gripper updates.
                if (wasFocused)
                {
                    ASSERT(m_isViewRegisteredForGripperUpdates);

                    IFC_RETURN(GetContext()->UnregisterGripper(m_pView));
                    m_isViewRegisteredForGripperUpdates = false;
                }
                // If focus was gained, register the view.
                else
                {
                    ASSERT(!m_isViewRegisteredForGripperUpdates);

                    IFC_RETURN(GetContext()->RegisterGripper(m_pView));
                    m_isViewRegisteredForGripperUpdates = true;
                }
            }
        }
        break;

    case KnownPropertyIndex::Control_IsEnabled:
        // switching from disabled and focused state (AllowFocusWhenDisabled had to be true) to enabled
        if (!wasEnabled && wasFocused && (wasEnabled != !!IsEnabled()))
        {
            OnGotFocus(nullptr); // call OnGotFocus, this step was skipped earlier due to disabled and AllowFocusWhenDisabled was true
        }
        break;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the last text element that has the selection.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::UpdateLastSelectedTextElement()
{
    // If UpdateLastSelectedTextElement() was called as a result of
    // programatically setting the selection then we will not update
    // the last selected text element to this one since the selection
    // will not be visible if the control was not in focus.
    if (IsFocused())
    {
        CTextCore* pTextCore = nullptr;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));

        uint32_t selectionLength = 0;
        IFC_RETURN(GetSelectionLength(selectionLength));

        if (selectionLength > 0)
        {
            pTextCore->SetLastSelectedTextElement(this);
        }
        else
        {
            pTextCore->ClearLastSelectedTextElement();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the selection is changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnSelectionChanged()
{
    // Let the view update its scroll offset.
    IFC_RETURN(m_pView->UpdateCaretElement());

    // Invalidate rendering to redraw the selection.
    // TODO: Theoretically we don't need this, but in practice the selection does not render
    // completely without it.  Why?
    NWSetContentDirty(this, DirtyFlags::Render);
    IFC_RETURN(UpdateLastSelectedTextElement());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::ProcessSelectionChangingEvent(_Inout_ long& selectionStart, _Inout_ long& selectionLength, _Out_ BOOLEAN& selectionChangingCanceled)
// SelectionChanging Synchronous event handling
{
    m_handlingSelectionChangingEvent = true;
    auto scopeGuard = wil::scope_exit([&]
    {
        m_handlingSelectionChangingEvent = false;
    });

    wrl::ComPtr<ITextSelection2> spSelection;
    IFC_RETURN(GetSelection(&spSelection));

    selectionChangingCanceled = FALSE;
    if (GetTypeIndex() == KnownTypeIndex::TextBox)
    {
        IFC_RETURN(FxCallbacks::TextBox_OnSelectionChangingHandler(this, selectionStart, selectionLength, &selectionChangingCanceled));
    }
    else if (GetTypeIndex() == KnownTypeIndex::RichEditBox)
    {
        IFC_RETURN(FxCallbacks::RichEditBox_OnSelectionChangingHandler(this, selectionStart, selectionLength, &selectionChangingCanceled));
    }
    else
    {
        ASSERT(FALSE);
    }

    long selectionStartAfterChangingEvent;
    long selectionEndAfterChangingEvent;
    IFC_RETURN(spSelection->GetStart(&selectionStartAfterChangingEvent));
    IFC_RETURN(spSelection->GetEnd(&selectionEndAfterChangingEvent));

    const bool selectionChangedByApp = (selectionStartAfterChangingEvent != selectionStart || selectionEndAfterChangingEvent != selectionStart + selectionLength);

    if (selectionChangedByApp)
    {
        selectionStart = selectionStartAfterChangingEvent;
        selectionLength = selectionEndAfterChangingEvent - selectionStartAfterChangingEvent;
        selectionChangingCanceled = FALSE; // ignore cancel set by app if selection has been changed in handling event
    }

    if (selectionChangingCanceled)
    {
        // restore the original text selection if cancelled
        IFC_RETURN(spSelection->SetRange(m_iSelectionStart, m_iSelectionLength + m_iSelectionStart));
    }

    // Snap the grippers to the current selection if it has been modified by app or because of cancellation
    if (m_pView->AreGrippersVisible() && (selectionChangedByApp || selectionChangingCanceled))
    {
        // Gripper dragging condition may be in progress, ignore that
        IFC_RETURN(m_pView->ShowSelectionGrippers(true /*fIgnoreDrag */));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control holds secret content.
//
//  Notes:
//      When IsPassword returns TRUE, we run RichEdit in password mode.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::IsPassword() const
{
    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control processes cr/lf from user input.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::AcceptsReturn() const
{
    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control holds secret content and shows the content
//      in reveal mode
//
//---------------------------------------------------------------------------
bool CTextBoxBase::IsPasswordRevealed() const
{
    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control contains formatted text, otherwise the
//      control only contains plaintext Unicode.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::AcceptsRichText() const
{
    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the current TextAlignment.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetAlignment(_Out_ DirectUI::TextAlignment *pAlignment)
{
    *pAlignment = DirectUI::TextAlignment::Left;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control cannot currently be modified by the user.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::IsReadOnly() const
{
    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the AcceptsReturn property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnAcceptsReturnChanged()
{
    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_MULTILINE /* mask */, UseMultiLineMode() ? TXTBIT_MULTILINE : 0));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the TextWrapping property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnTextWrappingChanged()
{
    IFC_RETURN(OnAcceptsReturnChanged()); //Need to coerce TextWrapping with AcceptsReturn to compute the MultiLineMode value for RichEdit.
    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_WORDWRAP /* mask */, GetTextWrapping() == DirectUI::TextWrapping::Wrap ? TXTBIT_WORDWRAP : 0));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the IsReadOnly property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnIsReadOnlyChanged()
{
    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_READONLY /* mask */, IsReadOnly() ? TXTBIT_READONLY : 0));
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETTOUCHOPTIONS, RTO_READINGMODE, IsReadOnly(), NULL));
    IFC_RETURN(GetView()->ShowOrHideCaret());
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the TextAlignment property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnTextAlignmentChanged()
{
    IFC_RETURN(m_pView->UpdateDefaultParagraphFormat(PFM_ALIGNMENT));
    IFC_RETURN(m_pView->OnBidiOptionsChanged());
    IFC_RETURN(UpdateKeyboardLCID(m_lcid));

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnBidiOptionsChanged()
{
    IFC_RETURN(m_pView->OnBidiOptionsChanged());
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the MaxLength property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnMaxLengthChanged()
{
    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_MAXLENGTHCHANGE /* mask */, TXTBIT_MAXLENGTHCHANGE));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the IsSpellCheckEnabled property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnIsSpellCheckEnabledChanged(_Inout_ bool& isSpellCheckEnabled)
{
    if (m_spellCheckIsDefault)
    {
        InputScopeNameValue isNameValue;
        IFC_RETURN(GetInputScope(&isNameValue));
        if (isNameValue == InputScopeNameValuePersonalFullName) // default to be consistent with mobile
        {
            isSpellCheckEnabled = false;
        }
    }

    IFC_RETURN(SetLanguageOption(IMF_SPELLCHECKING, isSpellCheckEnabled));

    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(EnsureTextInputSettings());
        IFC_RETURN(m_pPrivateTextInputSettings->OnSpellCheckEnabledChanged(static_cast<BOOL>(isSpellCheckEnabled)));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the IsTextPredictionEnabled property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnIsTextPredictionEnabledChanged(_Inout_ bool& isTextPredictionEnabled)
{
    if (DesktopUtility::IsOnDesktop())
    {
        HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(GetElementIslandInputSite().Get());
        if (nullptr != inputHwnd)
        {
            IFC_RETURN(::TextInput_SetTextPrediction(inputHwnd));
        }
    }

    if (m_textPredictionIsDefault)
    {
        InputScopeNameValue isNameValue;
        IFC_RETURN(GetInputScope(&isNameValue));
        if (isNameValue == InputScopeNameValuePersonalFullName) // default to be consistent with mobile
        {
            isTextPredictionEnabled = false;
        }
    }

    IFC_RETURN(SetLanguageOption(IMF_TKBPREDICTION, isTextPredictionEnabled));

    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(EnsureTextInputSettings());
        IFC_RETURN(m_pPrivateTextInputSettings->OnTextPredictionEnabledChanged(static_cast<BOOL>(isTextPredictionEnabled)));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnDesiredCandidateWindowAlignmentChanged(_In_ DirectUI::CandidateWindowAlignment alignment)
{
    const BOOL isCandidateWindowAlignAtBottom = (DirectUI::CandidateWindowAlignment::BottomEdge == alignment);
    IFC_RETURN(SetLanguageOption(IMF_IMEUIINTEGRATION, isCandidateWindowAlignAtBottom));

    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(EnsureTextInputSettings());
        IFC_RETURN(m_pPrivateTextInputSettings->OnCandidateWindowAlignmentChanged(isCandidateWindowAlignAtBottom));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the InputScope property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnInputScopeChanged(_In_ CInputScope *pInputScope)
{
    HRESULT hr = S_OK;
    InputScope* pInputScopeList = nullptr;
    XUINT32 inputScopeCount = 0;
    CInputScopeName* pInputScopeName = nullptr;

    if (pInputScope && pInputScope->m_pNames->GetCount() > 0)
    {
        inputScopeCount = pInputScope->m_pNames->GetCount();
        pInputScopeList = new InputScope[inputScopeCount];

        for (XUINT32 i = 0; i < inputScopeCount; i++)
        {
            pInputScopeName = static_cast<CInputScopeName *>(pInputScope->m_pNames->GetItemWithAddRef(i));

            switch (pInputScopeName->m_nameValue)
            {
                case DirectUI::InputScopeNameValue::Default:
                    pInputScopeList[i] = IS_DEFAULT;
                    break;
                case DirectUI::InputScopeNameValue::Url:
                    pInputScopeList[i] = IS_URL;
                    break;
                case DirectUI::InputScopeNameValue::EmailSmtpAddress:
                    pInputScopeList[i] = IS_EMAIL_SMTPEMAILADDRESS;
                    break;
                case DirectUI::InputScopeNameValue::PersonalFullName:
                    pInputScopeList[i] = IS_PERSONALNAME_FULLNAME;
                    break;
                case DirectUI::InputScopeNameValue::CurrencyAmountAndSymbol:
                    pInputScopeList[i] = IS_CURRENCY_AMOUNTANDSYMBOL;
                    break;
                case DirectUI::InputScopeNameValue::CurrencyAmount:
                    pInputScopeList[i] = IS_CURRENCY_AMOUNT;
                    break;
                case DirectUI::InputScopeNameValue::DateMonthNumber:
                    pInputScopeList[i] = IS_DATE_MONTH;
                    break;
                case DirectUI::InputScopeNameValue::DateDayNumber:
                    pInputScopeList[i] = IS_DATE_DAY;
                    break;
                case DirectUI::InputScopeNameValue::DateYear:
                    pInputScopeList[i] = IS_DATE_YEAR;
                    break;
                case DirectUI::InputScopeNameValue::Digits:
                    pInputScopeList[i] = IS_DIGITS;
                    break;
                case DirectUI::InputScopeNameValue::Number:
                    pInputScopeList[i] = IS_NUMBER;
                    break;
                case DirectUI::InputScopeNameValue::Password:
                    pInputScopeList[i] = IS_PASSWORD;
                    break;
                case DirectUI::InputScopeNameValue::TelephoneNumber:
                    pInputScopeList[i] = IS_TELEPHONE_FULLTELEPHONENUMBER;
                    break;
                case DirectUI::InputScopeNameValue::TelephoneCountryCode:
                    pInputScopeList[i] = IS_TELEPHONE_COUNTRYCODE;
                    break;
                case DirectUI::InputScopeNameValue::TelephoneAreaCode:
                    pInputScopeList[i] = IS_TELEPHONE_AREACODE;
                    break;
                case DirectUI::InputScopeNameValue::TelephoneLocalNumber:
                    pInputScopeList[i] = IS_TELEPHONE_LOCALNUMBER;
                    break;
                case DirectUI::InputScopeNameValue::TimeHour:
                    pInputScopeList[i] = IS_TIME_HOUR;
                    break;
                case DirectUI::InputScopeNameValue::TimeMinutesOrSeconds:
                    pInputScopeList[i] = IS_TIME_MINORSEC;
                    break;
                case DirectUI::InputScopeNameValue::NumberFullWidth:
                    pInputScopeList[i] = IS_NUMBER_FULLWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::AlphanumericHalfWidth:
                    pInputScopeList[i] = IS_ALPHANUMERIC_HALFWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::AlphanumericFullWidth:
                    pInputScopeList[i] = IS_ALPHANUMERIC_FULLWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::Hiragana:
                    pInputScopeList[i] = IS_HIRAGANA;
                    break;
                case DirectUI::InputScopeNameValue::KatakanaHalfWidth:
                    pInputScopeList[i] = IS_KATAKANA_HALFWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::KatakanaFullWidth:
                    pInputScopeList[i] = IS_KATAKANA_FULLWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::Hanja:
                    pInputScopeList[i] = IS_HANJA;
                    break;
                case DirectUI::InputScopeNameValue::HangulHalfWidth:
                    pInputScopeList[i] = IS_HANGUL_HALFWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::HangulFullWidth:
                    pInputScopeList[i] = IS_HANGUL_FULLWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::Search:
                    pInputScopeList[i] = IS_SEARCH;
                    break;
                case DirectUI::InputScopeNameValue::Formula:
                    pInputScopeList[i] = IS_FORMULA;
                    break;
                case DirectUI::InputScopeNameValue::SearchIncremental:
                    pInputScopeList[i] = IS_SEARCH_INCREMENTAL;
                    break;
                case DirectUI::InputScopeNameValue::ChineseHalfWidth:
                    pInputScopeList[i] = IS_CHINESE_HALFWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::ChineseFullWidth:
                    pInputScopeList[i] = IS_CHINESE_FULLWIDTH;
                    break;
                case DirectUI::InputScopeNameValue::NativeScript:
                    pInputScopeList[i] = IS_NATIVE_SCRIPT;
                    break;
                case DirectUI::InputScopeNameValue::Text:
                    pInputScopeList[i] = IS_TEXT;
                    break;
                case DirectUI::InputScopeNameValue::Chat:
                    pInputScopeList[i] = IS_CHAT;
                    break;
                case DirectUI::InputScopeNameValue::NameOrPhoneNumber:
                    pInputScopeList[i] = IS_NAME_OR_PHONENUMBER;
                    break;
                case DirectUI::InputScopeNameValue::EmailNameOrAddress:
                    pInputScopeList[i] = IS_EMAILNAME_OR_ADDRESS;
                    break;
                case DirectUI::InputScopeNameValue::Maps:
                    pInputScopeList[i] = IS_MAPS;
                    break;
                case DirectUI::InputScopeNameValue::NumericPassword:
                    pInputScopeList[i] = IS_NUMERIC_PASSWORD;
                    break;
                case DirectUI::InputScopeNameValue::NumericPin:
                    pInputScopeList[i] = IS_NUMERIC_PIN;
                    break;
                case DirectUI::InputScopeNameValue::AlphanumericPin:
                    pInputScopeList[i] = IS_ALPHANUMERIC_PIN;
                    break;
                case DirectUI::InputScopeNameValue::FormulaNumber:
                    pInputScopeList[i] = IS_FORMULA_NUMBER;
                    break;
                case DirectUI::InputScopeNameValue::ChatWithoutEmoji:
                    pInputScopeList[i] = IS_CHAT_WITHOUT_EMOJI;
                    break;
                default:
                    IFC(E_FAIL);
                    break;
            }

            ReleaseInterface(pInputScopeName);
        }

        TraceSendInputScopeToRichEditInfo(static_cast<UINT32>(pInputScopeList[0])); // in reality, only first input scope in the list is used
    }

    LRESULT result;
    IFC(m_pTextServices->TxSendMessage(EM_GETEVENTMASK, 0, 0, &result));
    if (pInputScopeList && pInputScopeList[0] == IS_URL)
    {
        // Disable auto formatting but enable URL detection for URL inputscope
        IFC(m_pTextServices->TxSendMessage(EM_AUTOURLDETECT, AURL_DISABLEAUTOFORMAT | AURL_ENABLEURL | AURL_ENABLEEAURLS, 0, nullptr));
        // Disable URL link notification
        IFC(m_pTextServices->TxSendMessage(EM_SETEVENTMASK, 0, result & (~ENM_LINK), nullptr));
    }
    else
    {
        // Clear any prior AutoURL detection settings.
        IFC(m_pTextServices->TxSendMessage(EM_AUTOURLDETECT, 0, 0, nullptr));
        // enable URL link notification
        IFC(m_pTextServices->TxSendMessage(EM_SETEVENTMASK, 0, result | ENM_LINK, nullptr));
    }

    // Note: When WPARAM is 0, EM_SETCTFINPUTSCOPES resets the current input scope to the default value.
    IFC(m_pTextServices->TxSendMessage(EM_SETCTFINPUTSCOPES, inputScopeCount, reinterpret_cast<LPARAM>(pInputScopeList), NULL));



Cleanup:
    ReleaseInterface(pInputScopeName);
    delete[] pInputScopeList;
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when the visual state of the control is modified (eg. focus
//      change). UpdateVisualState performs the visual state transition
//      for the new state based on managed or native control status. Use
//      the managed VisualStateManager to perform the transition.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::UpdateVisualState()
{
    auto focusManager = VisualTree::GetFocusManagerForElement(this);
    // CommonStates & FocusStates are combined
    //
    // NOTES: Pressed state is the same as Focused
    //        PointerFocused state is the same as Focused
    if (!IsEnabled())
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"Disabled", nullptr, nullptr, true));
    }
    else if (m_forceFocusedVisualState || (GetFocusState() != DirectUI::FocusState::Unfocused && focusManager->IsPluginFocused()))
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"Focused", nullptr, nullptr, true));
    }
    else if (m_isPointerOver)
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"PointerOver", nullptr, nullptr, true));
    }
    else
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"Normal", nullptr, nullptr, TRUE));
    }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    // HeaderStates VisualStateGroup.
    if (GetHeaderPlacementPropertyID() != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        switch (GetHeaderPlacement())
        {
            case DirectUI::ControlHeaderPlacement::Top:
                IFC_RETURN(CVisualStateManager::GoToState(this, L"TopHeader", nullptr, nullptr, true /* useTransitions */));
                break;

            case DirectUI::ControlHeaderPlacement::Left:
                IFC_RETURN(CVisualStateManager::GoToState(this, L"LeftHeader", nullptr, nullptr, true /* useTransitions */));
                break;
        }
    }
#endif

    if (GetTargetValidationCommandProperty() != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        EnsureValidationVisuals();
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether the given value is an integer greater than or
//      equal to zero.
//
//------------------------------------------------------------------------
bool CTextBoxBase::IsPositiveInteger(_In_ const CValue& value)
{
    if (value.GetType() == valueSigned && value.AsSigned() >= 0)
    {
        return true;
    }
    else if (value.AsObject())
    {
        const CInt32* pInt = nullptr;
        if (SUCCEEDED(DoPointerCast(pInt, value)) && pInt->m_iValue >= 0)
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the RichEdit buffer as an xstring_ptr.
//
//  Notes:
//      Caller must delete the returned string.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetTextServicesBuffer(_Out_ xstring_ptr* pstrText)
{
    auto scopeGuard = wil::scope_exit([&]
    {
        ReleaseDocumentIfNotFocused();
    });

    wil::unique_bstr text;

    if (IsPassword())
    {
        IFC_RETURN(m_pTextServices->TxGetText(&text));
    }
    else
    {
        IFC_RETURN(EnsureDocument());
        Microsoft::WRL::ComPtr<ITextRange2> spRange;
        IFC_RETURN(m_spDocument->Range2(tomForward, 0, &spRange));
        IFC_RETURN(spRange->GetText2(tomAllowFinalEOP, &text));
    }

    // RichEdit will return NULL when the backing store is empty, but CloneBuffer doesn't
    // like that.  We'll return NULL as well, as that matches the default value of m_pText.
    if (text)
    {
        IFC_RETURN(xstring_ptr::CloneBuffer(text.get(), SysStringLen(text.get()), pstrText));
    }
    else
    {
        pstrText->Reset();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the RichEdit buffer.
//
//  Notes:
//      This method resets the RichEdit state, clearing undo and the selection
//      state.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetTextServicesBuffer(
    _In_ const xstring_ptr& strText
        // If empty, removes all content.
    )
{
    const WCHAR* pNullTermintatedText = nullptr;

    auto scopeGuard = wil::scope_exit([&] {
        delete[] pNullTermintatedText;
    });

    if (!strText.IsNullOrEmpty())
    {
        pNullTermintatedText = strText.MakeBufferCopy();
    }

    // In single line mode if the text begins with a newline RichEdit will return E_FAIL
    // which will cause us to throw a random COM Exception.
    IGNOREHR(m_pTextServices->TxSetText(pNullTermintatedText));
    InvalidateView();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates and initializes a new TextServices instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::InitializeTextServices()
{
    HMODULE winUIEdit = LoadLibraryExWAbs(L"WinUIEdit.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

    if (!winUIEdit)
    {
        IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

#if DBG
    // The IID values defined at the top of this file are available as exports from WinUIEdit.dll.
    // They are hardcoded for performance and convenience. Validate on debug builds that the IID
    // values are correct. (They shouldn't change, but this will catch if they ever do change.)
    static bool s_verifiedIIDs = false;
    if (!s_verifiedIIDs)
    {
        s_verifiedIIDs = true;

        auto pIID_ITextServices2 = (IID*)::GetProcAddress(winUIEdit, "IID_ITextServices2");
        ASSERT(pIID_ITextServices2);
        ASSERT(*pIID_ITextServices2 == IID_ITextServices2);

        auto pIID_ITextDocument2 = (IID*)::GetProcAddress(winUIEdit, "IID_ITextDocument2");
        ASSERT(pIID_ITextDocument2);
        ASSERT(*pIID_ITextDocument2 == IID_ITextDocument2);

        auto pIID_IRicheditWindowlessAccessibility = (IID*)::GetProcAddress(winUIEdit, "IID_IRicheditWindowlessAccessibility");
        ASSERT(pIID_IRicheditWindowlessAccessibility);
        ASSERT(*pIID_IRicheditWindowlessAccessibility == IID_IRicheditWindowlessAccessibility);
    }
#endif

    CreateTextServicesFunction pFnCreateTextServices = reinterpret_cast<CreateTextServicesFunction>(GetProcAddress(winUIEdit, "CreateTextServices"));
    IFCPTR_RETURN(pFnCreateTextServices);

    m_pFnShutdownTextServices = reinterpret_cast<ShutdownTextServicesFunction>(GetProcAddress(winUIEdit, "ShutdownTextServices"));
    IFCPTR_RETURN(m_pFnShutdownTextServices);

    m_pHost = new TextServicesHost(this);

    m_pRichEditOleCallback = new RichEditOleCallback(this);

    ComPtr<IUnknown> spUnknown;
    // NB: contrary to standard COM rules, CreateTextServices will not AddRef pHost, even though it
    // does cache the pointer.
    IFC_RETURN(pFnCreateTextServices(nullptr, m_pHost, &spUnknown));

    IFC_RETURN(spUnknown->QueryInterface(IID_ITextServices2, reinterpret_cast<void **>(&m_pTextServices)));

    if (!GetContext()->IsTSF3Enabled())
    {
        // Enable Cicero based IME handling.
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETEDITSTYLE, SES_USECTF, SES_USECTF, nullptr));
    }

    IFC_RETURN(OnBidiOptionsChanged());

    // Enable touch input.
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETEDITSTYLEEX, SES_EX_MULTITOUCH | SES_EX_USEMOUSEWPARAM | SES_EX_NOACETATESELECTION, SES_EX_MULTITOUCH | SES_EX_USEMOUSEWPARAM | SES_EX_NOACETATESELECTION, NULL));

    // Enable content and selection change notifications.
    LRESULT result;
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETEVENTMASK, 0, 0, &result));
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETEVENTMASK, 0, result | ENM_CHANGE | ENM_SELCHANGE | ENM_REQUESTRESIZE | ENM_LINK | ENM_HIDELINKTOOLTIP | ENM_STARTCOMPOSITION | ENM_ENDCOMPOSITION, &result));

    // Enable change notifications during IME composition update, for all languages.
    IFC_RETURN(SetLanguageOption(IMF_IMEALWAYSSENDNOTIFY, true));

    // Turn off dual font support, which is not needed and undesirable with modern EA fonts.
    IFC_RETURN(SetLanguageOption(IMF_DUALFONT, false));

    // Register our listener for context menu events.
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETOLECALLBACK, 0, reinterpret_cast<LPARAM>(m_pRichEditOleCallback), NULL));

    // Explicitly set DPI on RE to 96.0 - plateau scaling is handled by the XAML rendering layer
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETDPI, 96, 96, &result));

    if (!result)
    {
        IFC_RETURN(E_FAIL);
    }

    if (IsPassword())
    {
        // Set password input scope when we run RichEdit in password mode.
        const InputScope passwordInputScope = IS_PASSWORD;
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETCTFINPUTSCOPES, 1, reinterpret_cast<LPARAM>(&passwordInputScope), NULL));
    }

    const XFLOAT rScale = 0.8f;
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETTOUCHSCALE, 0, reinterpret_cast<LPARAM>(&rScale), NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates and initializes a new TextBoxView.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::InitializeView()
{
    CREATEPARAMETERS cp(GetContext());

    xref_ptr<CTextBoxView> spView;
    IFC_RETURN(CreateDO(spView.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(spView->SetOwnerTextControl(this));
    m_pView = spView.detach();

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::RemovePeerReferenceToView()
{
    if (m_viewHasPeerRef)
    {
        IFC_RETURN(RemovePeerReferenceToItem(m_pView));
    }

    m_viewHasPeerRef = false;
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::AddPeerReferenceToView()
{
    if (!m_viewHasPeerRef)
    {
        // We're going to create a peer for the view soon. Make sure that we protect this view for the
        // remainder of the lifetime of our TextBox.
        IFC_RETURN(AddPeerReferenceToItem(m_pView));
        m_viewHasPeerRef = true;
    }

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Synopsis:
//      Attaches this control to the new template host.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::AttachToHost(_In_ CFrameworkElement* pContentHost)
{
    CValue valueView;
    const CDependencyProperty *pContentProperty = pContentHost->GetContentProperty();

    valueView.WrapObjectNoRef(m_pView);

    ASSERT(m_pHostRef.expired());

    // Set the content property of the host element to view object.
    // Attempt to set the native content property first.
    // If that fails, attempts to set the managed content property.
    if (pContentProperty)
    {
        IFC_RETURN(pContentHost->SetValue(SetValueParams(pContentProperty, valueView)));
    }
    else
    {
        IFC_RETURN(E_FAIL);
    }

    m_pHostRef = xref::get_weakref(pContentHost);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Detaches this control from the old template host.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::DetachFromHost()
{
    auto pContentHost = m_pHostRef.lock();

    auto hostGuard = wil::scope_exit([&] {
        m_pHostRef.reset();
    });

    ASSERT(m_pView != nullptr);

    if (pContentHost)
    {
        const CDependencyProperty *pContentProperty = pContentHost->GetContentProperty();
        if (pContentProperty)
        {
            // We perform the inverse list operation described in CUIElement::SetValue()
            // for detaching the TextBoxView if the ContentProperty is KnownPropertyIndex::Panel_Children
            // of a CUIElement.
            if (pContentProperty->GetIndex() == KnownPropertyIndex::Panel_Children)
            {

                IFC_RETURN(pContentHost->RemoveChild(m_pView));
            }
            else
            {
                IFC_RETURN(pContentHost->ClearValue(pContentProperty));
            }
        }
        else
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a core pointer event into an ITextServices message and
//      forwards it to ITextServices::SendMessage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SendPointerMessage(
    _In_ XUINT32 message,
    _In_ CEventArgs* pEventArgs
    )
{
    CPointerEventArgs* pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    ASSERT(!pPointerEventArgs->m_bHandled);

    CPointer* pPointer = pPointerEventArgs->m_pPointer;
    ReplaceInterface(m_pLastPointer, pPointer);

    const bool inputIsPen = pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    const bool inputIsMouse = pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse;
    const bool barrelButtonPressed = m_barrelButtonPressed || pPointer->m_bBarrelButtonPressed;
    const bool leftButtonPressed = m_leftButtonPressed || pPointer->m_bLeftButtonPressed;
    const bool penButtonPressed = inputIsPen && (barrelButtonPressed || leftButtonPressed);
    const bool penAsMouse = inputIsPen && penButtonPressed;
    const bool interationAsMouse = penAsMouse || inputIsMouse;

    XLONG_PTR result;

    if (interationAsMouse)
    {
        // RichEdit expect mouse messages for mouse devices, so do the conversion.
        XUINT32 mouseMessage = PointerMessageToMouseMessage(message, pPointer);
        if (mouseMessage != WM_NULL)
        {
            if (mouseMessage == WM_LBUTTONDOWN)
            {
                bool handled = false;
                IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_TERMINATECOMPOSITION, EM_TERMINATECOMPOSITION, 0L, 0L, &handled, &result));
            }

            RE_MOUSEINPUT reMouseInput;
            IFC_RETURN(PackageMouseEventParams(mouseMessage, pPointerEventArgs, pPointerEventArgs->m_keyModifiers, pPointer->m_pointerDeviceType, &reMouseInput));
            IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_MOUSEINPUT, mouseMessage, 0, reinterpret_cast<LPARAM>(&reMouseInput), &pPointerEventArgs->m_bHandled, &result));
        }
    }
    else // Interation as Touch or Pen
    {
        // RichEdit does not use the HIWORD(wParam)/flags, so we don't bother to try to
        // reconstruct them here.  Similarly, RichEdit does not use lParam.
        XUINT32 wParam = pPointer->m_uiPointerId;

        IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, message, message, wParam, 0 /* lParam */, &pPointerEventArgs->m_bHandled, &result));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a pointer message into the equivalent mouse message.
//
//  Notes:
//      Returns WM_NULL if no conversion is made.
//
//------------------------------------------------------------------------
XUINT32 CTextBoxBase::PointerMessageToMouseMessage(
    _In_ XUINT32 pointerMessage,
    _In_ CPointer* pPointer)
{
    XUINT32 mouseMessage = WM_NULL;

    switch (pointerMessage)
    {
        case WM_POINTERUPDATE:
            mouseMessage = WM_MOUSEMOVE;
            break;

        case WM_POINTERDOWN:
            // For Pen Device first check the status of the Right Button, since
            // both m_bRightButtonPressed and m_bLeftButtonPressed are set when
            // the Right Button is pressed.
            // For Mouse Device this order does not matter.
            if (pPointer->m_bRightButtonPressed && !m_rightButtonPressed)
            {
                m_rightButtonPressed = true;
                mouseMessage = WM_RBUTTONDOWN;
            }
            else if (pPointer->m_bLeftButtonPressed && !m_leftButtonPressed)
            {
                m_leftButtonPressed = true;
                mouseMessage = WM_LBUTTONDOWN;
            }
            else if (pPointer->m_bMiddleButtonPressed && !m_middleButtonPressed)
            {
                m_middleButtonPressed = true;
                mouseMessage = WM_MBUTTONDOWN;
            }
            else if (pPointer->m_bBarrelButtonPressed && !m_barrelButtonPressed)
            {
                m_barrelButtonPressed = true;
                mouseMessage = WM_LBUTTONDOWN;
            }

            break;

        case WM_POINTERUP:
            // Do not process pointer up without a corresponding pointer down. This makes sure
            // that we do not process pointer up after pointer is pressed outside the text control
            // and then dragged inside.
            //
            if (!pPointer->m_bRightButtonPressed && m_rightButtonPressed)
            {
                m_rightButtonPressed = false;
                mouseMessage = WM_RBUTTONUP;
            }
            else if (!pPointer->m_bLeftButtonPressed && m_leftButtonPressed)
            {
                m_leftButtonPressed = false;
                mouseMessage = WM_LBUTTONUP;
            }
            else if (!pPointer->m_bMiddleButtonPressed && m_middleButtonPressed)
            {
                m_middleButtonPressed = false;
                mouseMessage = WM_MBUTTONUP;
            }
            else if (!pPointer->m_bBarrelButtonPressed && m_barrelButtonPressed)
            {
                m_barrelButtonPressed = false;
                mouseMessage = WM_LBUTTONUP;
            }
            break;
    }

    return mouseMessage;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a core pointer event into an ITextServices mouse message.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::PackageMouseEventParams(
    _In_ XUINT32 mouseMessage,
    _In_ CInputPointEventArgs *pInputPointEventArgs,
    _In_ DirectUI::VirtualKeyModifiers keyModifiers,
    _In_ DirectUI::PointerDeviceType pointerDeviceType,
    _Out_ RE_MOUSEINPUT *pReMouseInput
    ) const
{
    XUINT32 wparam = 0;

    // Get the current keyboard state.
    if ((keyModifiers & DirectUI::VirtualKeyModifiers::Shift) != DirectUI::VirtualKeyModifiers::None)
    {
        wparam |= MK_SHIFT;
    }
    if ((keyModifiers & DirectUI::VirtualKeyModifiers::Control) != DirectUI::VirtualKeyModifiers::None)
    {
        wparam |= MK_CONTROL;
    }
    if (m_leftButtonPressed || m_barrelButtonPressed)
    {
        wparam |= MK_LBUTTON;
    }
    if (m_middleButtonPressed)
    {
        wparam |= MK_MBUTTON;
    }
    if (m_rightButtonPressed)
    {
        wparam |= MK_RBUTTON;
    }

    // Get the pointer position relative to the inner CTextBoxView.
    wf::Point mousePosition{};
    IFC_RETURN(pInputPointEventArgs->GetPosition(m_pView, &mousePosition));

    pReMouseInput->msg = mouseMessage;
    pReMouseInput->wparam = wparam;

    pReMouseInput->pt.x = static_cast<XINT32>(mousePosition.X);
    pReMouseInput->pt.y = static_cast<XINT32>(mousePosition.Y);

    pReMouseInput->screenPtInRawPixels.x = static_cast<XINT32>(pInputPointEventArgs->GetGlobalPoint().x);
    pReMouseInput->screenPtInRawPixels.y = static_cast<XINT32>(pInputPointEventArgs->GetGlobalPoint().y);

    const bool inputIsPen = pointerDeviceType == DirectUI::PointerDeviceType::Pen;

    if (inputIsPen && !m_barrelButtonPressed)
    {
        const bool shouldClearSelection = mouseMessage == WM_LBUTTONUP &&
            static_cast<CPointerEventArgs*>(pInputPointEventArgs)->m_uiGestureFollowing == DirectUI::GestureModes::Tapped;

        // This property, isPenInput, tells RichEdit whether this message is from a pen. Richedit behavior for pen is to never clear
        // selection when clicking inside the selection range. However, when we receive a tap gesture, we'd like
        // to override this behavior. We accomplish this by telling richedit to treat this message (during left button up) as a
        // normal mouse message instead of pen message. Any other gesture should not affect selection.
        pReMouseInput->isPenInput = shouldClearSelection == false;
    }
    else
    {
        pReMouseInput->isPenInput = inputIsPen;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a core keyboard event into an ITextServices message and
//      forwards it to ITextServices::SendMessage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SendKeyMessage(
    _In_ XUINT32 message,
    _In_ CEventArgs* pEventArgs
    )
{
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    ASSERT(!pKeyEventArgs->m_bHandled);

    XUINT32 wParam = pKeyEventArgs->m_platformKeyCode;
    XUINT32 lParam =
        (static_cast<XUINT32>(pKeyEventArgs->m_physicalKeyStatus.m_bIsExtendedKey) << 24 & 0x01000000) |
        (pKeyEventArgs->m_physicalKeyStatus.m_uiScanCode << 16 & 0x00FF0000) |
        (pKeyEventArgs->m_physicalKeyStatus.m_uiRepeatCount & 0x0000FFFF);

    if (message == WM_SYSKEYDOWN)
    {
        lParam |= KF_ALTDOWN << 16;
    }

    XLONG_PTR result;
    IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, message, message, wParam, lParam, &pKeyEventArgs->m_bHandled, &result));

    // RichEdit returns S_OK for backspace but still expects a WM_CHAR for this key.
    // Set handled to FALSE otherwise InputManager will not generate a WM_CHAR
    // for this key.
    if (wParam == wsy::VirtualKey::VirtualKey_Back)
    {
        pKeyEventArgs->m_bHandled = FALSE;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the RichEdit document.
//
//  Notes:
//      Caller must Release the returned document.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetDocument(_Outptr_ ITextDocument2 **ppDocument) const
{
    IFC_RETURN(m_pTextServices->QueryInterface(IID_ITextDocument2, reinterpret_cast<void **>(ppDocument)));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the RichEdit selection.
//
//  Notes:
//      Caller must Release the returned selection.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetSelection(_Outptr_ ITextSelection2 **ppSelection)
{
    auto scopeGuard = wil::scope_exit([this]()
    {
        ReleaseDocumentIfNotFocused();
    });

    IFC_RETURN(EnsureDocument());
    IFC_RETURN(m_spDocument->GetSelection2(ppSelection));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     This is to communicate with RichEdit for retrieval of IRawElementProviderSimple by utilizing IRicheditWindowlessAccessibility.
//     the direct communication is not possible from Core layer to windows layer hence we will be using abstraction
//     layer through Plat/Win IPALWindowlessHost* m_pWindowlessHost.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetRichEditRawElementProviderSimple(_Out_ void** ppProvider)
{
    IFC_RETURN(EnsureWindowlessHost());

    ComPtr<IRicheditWindowlessAccessibility> spRichEditWindowlessAcc;
    IFC_RETURN(GetTextServices()->QueryInterface(IID_IRicheditWindowlessAccessibility, reinterpret_cast<void **>(spRichEditWindowlessAcc.GetAddressOf())));
    IFC_RETURN(m_pWindowlessHost->GetChildRawElementProviderSimple(reinterpret_cast<void *>(spRichEditWindowlessAcc.Get()), ppProvider));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      This uses IRawElementProviderSimple for Rich Edit windowless as the adapter to serve for the
//      patterns it supports. By doing it we reflect the Text patterns supported by windowless RichEdit
//      on our own controls AutomationPeer.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::GetUnwrappedPattern(_In_ XINT32 patternID, _In_ bool isRichEdit, _Out_ void** ppPattern)
{
    IFC_RETURN(EnsureWindowlessHost());

    ComPtr<IRicheditWindowlessAccessibility> spRichEditWindowlessAcc;
    IFC_RETURN(GetTextServices()->QueryInterface(IID_IRicheditWindowlessAccessibility, reinterpret_cast<void **>(spRichEditWindowlessAcc.GetAddressOf())));
    IFC_RETURN(m_pWindowlessHost->GetUnwrappedPattern(reinterpret_cast<void *>(spRichEditWindowlessAcc.Get()), patternID, isRichEdit, ppPattern));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Demand creates a host for RichEdit's UI Accessibility exports.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::EnsureWindowlessHost()
{
    if (!m_pWindowlessHost)
    {
        auto core = GetContext();
        IXcpBrowserHost* pBrowserHost = core->GetBrowserHost();
        IFCEXPECT_RETURN(pBrowserHost);

        m_pWindowlessHost = pBrowserHost->CreateWindowlessHost(core->GetHostSite(), this, core->GetNextRuntimeId());
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Notify RichEdit that the SelectionHighlightColor property has been changed.
//
//---------------------------------------------------------------------------
void CTextBoxBase::OnSelectionHighlightColorChanged()
{
    InvalidateViewAndForceRedraw();
}

_Check_return_ HRESULT CTextBoxBase::OnSelectionHighlightColorWhenNotFocusedChanged()
{
    UINT32 SelectionHighlightColorWhenNotFocused = static_cast<UINT32>(KnownColors::Transparent);

    // Set to the current highlight color
    if (m_pSelectionHighlightColorWhenNotFocused)
    {
        SelectionHighlightColorWhenNotFocused = m_pSelectionHighlightColorWhenNotFocused->m_rgb;
    }

    IFC_RETURN(HideSelection(SelectionHighlightColorWhenNotFocused == static_cast<UINT32>(KnownColors::Transparent)));

    InvalidateViewAndForceRedraw();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the ShowPassword property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnPasswordRevealedChanged(_In_ bool passwordRevealed)
{
    IFC_RETURN(m_pTextServices->OnTxPropertyBitsChange(TXTBIT_SHOWPASSWORD, passwordRevealed ? TXTBIT_SHOWPASSWORD : 0));

    return S_OK;
}

CSolidColorBrush* CTextBoxBase::GetSelectionHighlightColorNoRef()
{
    if (IsFocused() || m_forceFocusedVisualState)
    {
        if (!m_pSelectionHighlightColor)
        {
            IGNOREHR(CreateSelectionHighlightColor());
        }

        return m_pSelectionHighlightColor;
    }

    return m_pSelectionHighlightColorWhenNotFocused;
}

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
DirectUI::ControlHeaderPlacement CTextBoxBase::GetHeaderPlacement() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(GetHeaderPlacementPropertyID(), &result));
    return static_cast<DirectUI::ControlHeaderPlacement>(result.AsEnum());
}
#endif

//------------------------------------------------------------------------
//
//
//  Synopsis: Creates the SelectionHighlightColor property, and set to the default color.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::CreateSelectionHighlightColor()
{
    CValue value;

    IFCEXPECT_RETURN(m_pSelectionHighlightColor == nullptr);

    value.SetColor(GetDefaultSelectionHighlightColor());

    IFC_RETURN(SetValueByKnownIndex(GetSelectionHighlightColorPropertyID(), value));

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::CreateSelectionHighlightColorWhenNotFocused()
{
    CValue value;

    IFCEXPECT_RETURN(m_pSelectionHighlightColorWhenNotFocused == nullptr);

    value.SetColor(static_cast<UINT32>(KnownColors::Transparent));

    IFC_RETURN(SetValueByKnownIndex(GetSelectionHighlightColorWhenNotFocusedPropertyID(), value));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a specific RichEdit language option.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetLanguageOption(
    _In_ XUINT32 option,
            // One of the IMF_* constants defined by RichEdit.
    _In_ bool value
    )
{
    LRESULT options;
    XUINT32 optionValue = value ? option : 0;

    IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETLANGOPTIONS, 0, 0, &options));

    options = (options & ~static_cast<XUINT64>(option)) | optionValue;

    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETLANGOPTIONS, 0, options, nullptr));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the SIP display policy to manual or automatic for TSF1 controls
//      If manual, InputPane's TryShow() and TryHide() are now required to show and hide the SIP.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetInputPaneDisplayPolicyForTSF1(
    _In_ bool isManualDisplayPolicy
    )
{
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETSIPDISPLAYPOLICY, 0, isManualDisplayPolicy, nullptr));
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the property if the value has changed.
//      If the value is the same and we are not in SetValue,
//      then just update the property value source to Local.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::CoercePropertyValue(
    _In_ const CDependencyProperty* pProperty,
    _Inout_ xstring_ptr& strSource,
    _Inout_ xstring_ptr& strTarget)
{
    if (!strTarget.Equals(strSource))
    {
        strSource = std::move(strTarget);
        ASSERT(strTarget.IsNull()); // ...make sure move semantics kicked in
        IFC_RETURN(SetBaseValueSource(pProperty, BaseValueSourceLocal));
    }
    else if (!IsInSetValue())
    {
        IFC_RETURN(SetBaseValueSource(pProperty, BaseValueSourceLocal));
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the property if the value has changed.
//      If the value is the same and we are not in SetValue,
//      then just update the property value source to Local.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::CoercePropertyValue(
    _In_ const CDependencyProperty* pProperty,
    _In_ XINT32* pSource,
    _In_ XINT32 target)
{
    if (*pSource != target)
    {
        *pSource = target;
        IFC_RETURN(SetBaseValueSource(pProperty, BaseValueSourceLocal));
    }
    else if (!IsInSetValue())
    {
        IFC_RETURN(SetBaseValueSource(pProperty, BaseValueSourceLocal));
    }

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//       Check if textbox should focus on pointer released or pressed
// PointerType
// Touch        PointerReleased
// Mouse button PointerPressed
// Pen          PointerPressed
//---------------------------------------------------------------------------
bool CTextBoxBase::FocusOnPointerReleased(_In_ CPointerEventArgs* pPointerEventArgs)
{
    // Focus on pointer pressed for mouse button and pen, on pointer released for touch
    CPointer* pPointer = pPointerEventArgs->m_pPointer;

    if (pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch)
    {
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Computes whether to set RichEdit to multi-line mode based on the
//      values for AcceptsReturn and TextWrapping properties.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::UseMultiLineMode() const
{
    // Set RichEdit in multi-line mode is word wrap is on.
    // If AcceptsReturn is FALSE, we will explicitly block Enter
    // key to simulate the correct behavior.
    return (AcceptsReturn() || GetTextWrapping() == DirectUI::TextWrapping::Wrap);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     Filters out specific key strokes.
//
//---------------------------------------------------------------------------
bool CTextBoxBase::ShouldProcessKeyMessage(_In_ CEventArgs* pEventArgs)
{
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
    if (pKeyEventArgs && AcceptsReturn() == FALSE && GetTextWrapping() == DirectUI::TextWrapping::Wrap)
    {
        // Special case when AcceptsReturn is FALSE and word wrap
        // is turned on. In this case, we want RichEdit in multi-line mode
        // but do not want it to process Enter. RichEdit does not support this
        // mode, so we block Enter key explicitly.
        if (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Enter)
        {
            return false;
        }
    }

    if (XboxUtility::IsGamepadNavigationDirection(pKeyEventArgs->m_originalKeyCode))
    {
        return false;
    }

    // RichEdit invokes legacy context menu on key up, disable processing context menu key if we have a context flyout
    if (pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Application && HasContextFlyout())
    {
        return false;
    }

    return true;
}

_Check_return_ HRESULT CTextBoxBase::ArrangeOverride(
    _In_ XSIZEF finalSize,
    _Out_ XSIZEF &newFinalSize
)
{
    IFC_RETURN(CControl::ArrangeOverride(finalSize, newFinalSize));
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Async event callback indicating we have pending inherited property changes
//      to propagate to RichEdit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::OnInheritedPropertyChanged(
    _In_ CEventArgs* pEventArgs
    )
{
    IFC_RETURN(m_pView->OnInheritedPropertyChanged());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CControl::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::UIElement_Visibility:
        {
            if (IsVisible())
            {
                IFC_RETURN(UpdateVisualState());
            }
            else
            {
                m_isPointerOver = false;
            }
            break;
        }
        case KnownPropertyIndex::TextBox_CharacterCasing:
        case KnownPropertyIndex::RichEditBox_CharacterCasing:
        {
            LRESULT lRes = 0;
            WPARAM editStyle = 0;
            IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETEDITSTYLE, 0, 0, &lRes));
            switch (static_cast<DirectUI::CharacterCasing>(args.m_pNewValue->AsEnum()))
            {
                case DirectUI::CharacterCasing::Normal:
                    editStyle = lRes & (~SES_UPPERCASE) & (~SES_LOWERCASE);
                    break;

                case DirectUI::CharacterCasing::Lower:
                    editStyle = (lRes | SES_LOWERCASE); //Flip lowercase bit
                    editStyle = editStyle & (~SES_UPPERCASE); //Ensure uppercase bit is zero
                    break;

                case DirectUI::CharacterCasing::Upper:
                    editStyle = lRes | SES_UPPERCASE; //Flip uppercase bit
                    editStyle = editStyle & (~SES_LOWERCASE); //Ensure lowercase bit is zero
                    break;

                default:
                    IFC_RETURN(E_INVALIDARG);
            }

            IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETEDITSTYLE, editStyle, lRes | SES_LOWERCASE | SES_UPPERCASE, &lRes));
            break;
        }
        case KnownPropertyIndex::RichEditBox_IsTelemetryCollectionEnabled:
        case KnownPropertyIndex::TextBox_IsTelemetryCollectionEnabled:
        {
            IFC_RETURN(m_pPrivateTextInputSettings->SetIsTelemetryCollectionEnabled(static_cast<bool>(args.m_pNewValue->AsBool())));
            break;
        }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
        case KnownPropertyIndex::PasswordBox_HeaderPlacement:
        case KnownPropertyIndex::RichEditBox_HeaderPlacement:
        case KnownPropertyIndex::TextBox_HeaderPlacement:
        {
            IFC_RETURN(UpdateVisualState());
            break;
        }
#endif

        case KnownPropertyIndex::UIElement_ContextFlyout:
        {
            auto oldContextFlyout = static_cast<CFlyoutBase*>(args.m_pOldValue->AsObject());
            auto newContextFlyout = static_cast<CFlyoutBase*>(args.m_pNewValue->AsObject());

            // Enable/Disable RichEdit's PopupMenu context menu
            if (!oldContextFlyout && newContextFlyout)
            {
                m_useLegacyContextMenu = false;
                UpdateRichEditContextMenu(false /*useLegacyContextMenu*/);
            }
            else if (oldContextFlyout && !newContextFlyout)
            {
                m_useLegacyContextMenu = true;
                UpdateRichEditContextMenu(true /*useLegacyContextMenu*/);
            }
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateRichEditContextMenu( bool useLegacyContextMenu)
{
    if (useLegacyContextMenu)
    {
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETOLECALLBACK, 0, reinterpret_cast<LPARAM>(m_pRichEditOleCallback), nullptr));
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETTOUCHOPTIONS, RTO_DISABLECONTEXTMENU, false, nullptr));
    }
    else
    {
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETOLECALLBACK, 0, 0, nullptr));
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SETTOUCHOPTIONS, RTO_DISABLECONTEXTMENU, true, nullptr));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Queries RichEdit for its frozen state by retrieving the display frozen
//      flag using EM_GETFREEZECOUNT.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::IsFrozen(_Out_ bool& isFrozen)
{
    LRESULT result = 0;
    isFrozen = false;
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETFREEZECOUNT, NULL, (LPARAM)&isFrozen, &result));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether the properties set allows for the TextBox to autogrow
//      as text is input.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::IsAutoGrowing(_Out_ bool& isAutoGrowing)
{
    if (IsDefaultWidth())
    {
        // Optimize by not doing the dp resolution unless width is default
        CValue value;
        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_HorizontalAlignment), &value));
        isAutoGrowing = static_cast<DirectUI::HorizontalAlignment>(value.AsEnum()) != DirectUI::HorizontalAlignment::Stretch;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks whether the given value is an enum with value of WrapWholeWords
//
//------------------------------------------------------------------------
bool CTextBoxBase::IsWrapWholeWords(_In_ const CValue& value)
{
    if (value.IsEnum())
    {
        if (static_cast<DirectUI::TextWrapping>(value.AsEnum()) == DirectUI::TextWrapping::WrapWholeWords)
        {
            return true;
        }
    }
    else if (value.AsObject())
    {
        const CEnumerated* pEnum = nullptr;
        if (SUCCEEDED(DoPointerCast(pEnum, value)) &&
            static_cast<DirectUI::TextWrapping>(pEnum->m_nValue) == DirectUI::TextWrapping::WrapWholeWords)
        {
            return true;
        }
    }

    return false;
}

_Check_return_ HRESULT CTextBoxBase::SendRichEditFocus()
{
    bool handled;
    XLONG_PTR result;

    IFC_RETURN(GetTextServices()->OnTxUIActivate());
    IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, WM_SETFOCUS, WM_SETFOCUS, NULL, NULL, &handled, &result));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets focus in response to a pointer message.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxBase::SetFocusFromPointer(_In_ CPointerEventArgs *pPointerEventArgs, _Out_ bool& sendMessage)
{
    sendMessage = true;

    CPointer* pPointer = nullptr;
    IFCPTR_RETURN(pPointer = pPointerEventArgs->m_pPointer);
    CTextCore* pTextCore =nullptr;
    IFC_RETURN(GetContext()->GetTextCore(&pTextCore));

    const bool evIsMouse = pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse;
    const bool evIsPen = pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    const bool evIsTouch = pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Touch;
    if (evIsMouse || evIsPen || evIsTouch)
    {
        // RichEdit will ignore the pointer down if it doesn't have focus.
        // If the device type is mouse or touch, we should take focus before passing
        // the message to RichEdit so that it can begin mouse selection on this click.
        // If control is not focusable, do not focus RichEdit.
        bool focusUpdated = false;
        IFC_RETURN(Focus(DirectUI::FocusState::Pointer, false /*animateIfBringIntoView*/, &focusUpdated))
        if (focusUpdated)
        {
            IFC_RETURN(SendRichEditFocus());
        }
    }

    // Enforce "light dismiss text selection":  If we are gaining focus via touch, and we are read-only,
    // and another control currently has a selection, then switch focus, but do not perform a selection
    // on this touch.  We implement this policy by not sending RichEdit the WM_POINTERDOWN when we detect
    // this condition.  This way RichEdit does get focus (above), but will not perform a selection on the
    // first touch.  The user must touch again to select.
    if (evIsTouch && IsReadOnly() && !pTextCore->CanSelectText(this))
    {
        sendMessage = false;
    }

    // Clear the previous selection.
    // If the TextBox had a previous selection and we do not clear
    // it, then in the Readonly mode we set focus (code above)
    // and so if we tap on that previously selected word by chance we will show the CM.
    // In Editable mode we will simply bring the control in focus
    // and show the highlight on the last selected word while the desired
    // behavior is to clear the selection. Thus in those cases we want to clear
    // the selection first.
    if (evIsTouch ||
        evIsPen)
    {
        ctl::ComPtr<ITextSelection2> spSelection;
        IFC_RETURN(GetSelection(&spSelection));
        XLONG selectionStart = 0;
        IFC_RETURN(spSelection->GetStart(&selectionStart));
        XLONG selectionEnd = 0;
        IFC_RETURN(spSelection->GetEnd(&selectionEnd));
        if (selectionEnd != selectionStart)
        {
            // We just need to clear the selection without causing any scrolling to the view.
            m_ignoreScrollIntoView = true;
            IFC_RETURN(spSelection->SetEnd(selectionStart));
            m_ignoreScrollIntoView = false;
        }

        if (evIsPen)
        {
            // Hide grippers immediately to allow another selection to start
            IFC_RETURN(m_pView->HideSelectionGrippers());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateTextForCompositionStartedEvent()
{
    return S_OK;
}

KnownEventIndex CTextBoxBase::GetTextCompositionEventPropertyID(TextCompositionStage compositionStage) const
{
    return KnownEventIndex::UnknownType_UnknownEvent;
}

_Check_return_ HRESULT CTextBoxBase::GetCompositionString(_Outptr_ HSTRING* composition)
{
    xruntime_string_ptr strSubstring;
    xstring_ptr strText;

    IFC_RETURN(GetTextServicesBuffer(&strText));
    if (m_compositionLength <= 0 || strText.GetCount() > 500)
    {
        *composition = nullptr;
    }
    else
    {
        IFC_RETURN(strText.SubString(m_compositionStartIndex, m_compositionStartIndex + m_compositionLength, &strSubstring));
        WindowsDuplicateString(strSubstring.GetHSTRING(), composition);
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::GetCompositionPrefixAndPostfixStrings(_Outptr_ HSTRING* prefix, _Outptr_ HSTRING* postfix)
{
    xstring_ptr strText;
    xruntime_string_ptr strPrefix;
    xruntime_string_ptr strPostfix;

    // Get full text in the box
    IFC_RETURN(GetTextServicesBuffer(&strText));

    // Get the prefix
    IFC_RETURN(strText.SubString(0, m_compositionStartIndex, &strPrefix));

    // Get the postfix
    IFC_RETURN(strText.SubString(m_compositionStartIndex + m_compositionLength, strText.GetCount(), &strPostfix));

    WindowsDuplicateString(strPrefix.GetHSTRING(), prefix);
    WindowsDuplicateString(strPostfix.GetHSTRING(), postfix);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     TextBox and RichEditBox share same implementation for sending composition event
//    (only differs on event property ID)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBoxBase::SendTextCompositionEvent(TextCompositionStage compositionStage)
{
    CHARRANGE compositionRange= { 0 };
    LRESULT lres = 0;
    IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETCOMPOSITIONRANGE, (WPARAM) &compositionRange, NULL, &lres));
    ASSERT(compositionRange.cpMax >= compositionRange.cpMin);

    INT startIndex = static_cast<INT>(compositionRange.cpMin);
    INT length = static_cast<INT>(compositionRange.cpMax - compositionRange.cpMin);

    if (m_compositionInProgress)
    {
        m_compositionStartIndex = startIndex;
        m_compositionLength = length;
    }

    CEventManager *pEventManager = GetContext()->GetEventManager();
    if (pEventManager)
    {
        xref_ptr<CTextCompositionEventArgsBase> spArgs;

        if (compositionStage == TextCompositionStage::CompositionStarted)
        {
            spArgs.init(new CTextCompositionStartedEventArgs());
        }
        else if (compositionStage == TextCompositionStage::CompositionEnded)
        {
            spArgs.init(new CTextCompositionEndedEventArgs());
        }
        else if (compositionStage == TextCompositionStage::CompositionChanged)
        {
            spArgs.init(new CTextCompositionChangedEventArgs());
        }

        // tomForward(0x3fffffff) is the default value of the composition start index set in RichEdit
        // This can happen when composition just started and there is no text yet, we should change  composition
        // startIndex and length to 0 (effectively null composition string), to avoid access error in App's event handler
        if (startIndex == tomForward)
        {
            spArgs->m_startIndex = 0;
            spArgs->m_length = 0;
        }
        else
        {
            spArgs->m_startIndex = startIndex;
            spArgs->m_length = length;
        }

        // Sometimes App can cause composition events reentrancy issue, for example:
        // App can change text in the composition event handler which causes another
        // composition changed event to fire. In that case, we should fire event async
        // to avoid renentrancy crash.
        const bool raiseSync = (m_compositionEventReentrancyCount == 0);
        m_compositionEventReentrancyCount++;
        pEventManager->Raise(
            EventHandle(GetTextCompositionEventPropertyID(compositionStage)),
            TRUE,   /*bRefire*/
            this,
            spArgs,
            raiseSync /*raiseSync*/);
        m_compositionEventReentrancyCount--;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::EnableCandidateWindowBoundsTracking(EventHandle event)
{
    m_CandidateWindowevent = event;
    if (GetContext()->IsTSF3Enabled())
    {
        if (CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this))
        {
            IFC_RETURN(contentRoot->GetInputManager().RegisterFrameworkInputView());
        }
    }
    else // TODO: remove the block below and TextBoxUIManagerEventSink.h/.cpp once we enable TSF3 completely
    {
        if (!m_uiManagerEventSink)
        {
            m_uiManagerEventSink.Attach(new TextBoxUIManagerEventSink(this));

            // Note: 0x0435446 is a magic number not defined in any of the RichEdit headers (see cmsgflt.cpp).
            // It is the ascii digits for 'CTF', and will get the TSF (Cicero) Context object
            // when used as the WPARAM for the EM_GETOLEINTERFACE message.
            const WPARAM RICHEDIT_GETOLEINTERFACE_CTF = 0x0435446;

            // Get the TSF (Cicero) Context object for the control; does AddRef, we need to Release.
            Microsoft::WRL::ComPtr<ITfContext> tfContext;
            BOOL success = FALSE;
            IFC_RETURN(GetTextServices()->TxSendMessage(EM_GETOLEINTERFACE, RICHEDIT_GETOLEINTERFACE_CTF,
                (LPARAM)tfContext.ReleaseAndGetAddressOf(), (LRESULT*)&success));
            IFCCHECK_RETURN(tfContext && success);

            Microsoft::WRL::ComPtr<ITfDocumentMgr> editDocMgr;
            IFC_RETURN(tfContext->GetDocumentMgr(&editDocMgr)); // if not available, returns S_FALSE and nullptr
            IFCPTR_RETURN(editDocMgr);

            Microsoft::WRL::ComPtr<ITfSource> tfSource;
            IFC_RETURN(editDocMgr.As(&tfSource));
            IFCPTR_RETURN(tfSource);

            IFC_RETURN(tfSource->AdviseSink(IID_IUIManagerEventSink, m_uiManagerEventSink.Get(), &m_uiManagerSinkCookie));

            // Only keep source if AdviseSink succeeded to not attempt to UnadviseSink in the destructor otherwise
            m_tfSource = tfSource;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateFocusState(_In_ DirectUI::FocusState focusState)
{
    IFC_RETURN(__super::UpdateFocusState(focusState));
    if (focusState != DirectUI::FocusState::Unfocused) // only check and update SIP settings when textbox has focus
    {
        CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
        focusState = pFocusManager->GetRealFocusStateForFocusedElement();
        IFC_RETURN(UpdateSIPSettings(focusState));
    }

    if (focusState == DirectUI::FocusState::Unfocused)
    {
        // We want to force the focused visual state when either the Context or Selection flyouts are getting focus.
        // This needs to happen before the async call to OnLostFocus so that GetSelectionHighlightColorNoRef returns the correct color.
        m_forceFocusedVisualState = ShouldForceFocusedVisualState();

        // We only want to hide the grippers when the context flyout is getting focus.  The selection flyout is intended
        // to be transient in nature, and the user is still intended to be able to interact with the rest of the app
        // while it's being shown.
        m_shouldHideGrippersOnFlyoutOpening = ShouldHideGrippersOnFlyoutOpening();
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateSIPSettings(_In_ DirectUI::FocusState state)
{
    if (GetContext()->IsTSF3Enabled())
    {
        IFC_RETURN(EnsureTextInputSettings());

        // If we get focus through SetFocus (an app implementing their own autofocus algorithm) we won't receive an IncomingFocusFromGamepad call.
        // we'll check here if the last input was from a gamepad, and use the gamepad behavior if thats the case.
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);

        if (contentRoot->GetInputManager().GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote)
        {
            const bool inputFromSip = contentRoot->GetInputManager().IsSipOpen();
            //If the SIP is still up, we know that we are navigating into a text control with the SIP still up.
            //In keeping with SIP semantics, we shouldn't dismiss the SIP.
            m_canEnableManualInputPane = !inputFromSip;
        }

        const bool enableManualInputPane =
            (state == DirectUI::FocusState::Programmatic && m_bPreventKeyboardDisplayOnProgrammaticFocus) ||
            (state != DirectUI::FocusState::Programmatic && m_canEnableManualInputPane);

        if (m_manualInputPaneEnabled != enableManualInputPane)
        {
            IFC_RETURN(m_pPrivateTextInputSettings->UpdateSIPSettings(!!enableManualInputPane));

            if (IsFocused() && m_manualInputPaneEnabled)
            {
                // Switching m_manualInputPaneEnabled from true to false, this happens when user taps on a programmatically focused textbox,
                // so we need to manually notify TSF3 to bring up SIP
                IFC_RETURN(m_pPrivateTextInputSettings->NotifyFocusEnter());
            }

            m_manualInputPaneEnabled = enableManualInputPane;
        }
    }
    else
    {
        const bool enableManualInputPane = (state == DirectUI::FocusState::Programmatic && m_bPreventKeyboardDisplayOnProgrammaticFocus);

        if (m_manualInputPaneEnabled != enableManualInputPane)
        {
            IFC_RETURN(SetInputPaneDisplayPolicyForTSF1(!!enableManualInputPane));
            m_manualInputPaneEnabled = enableManualInputPane;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::EnsureTextInputSettings()
{
    if (m_pPrivateTextInputSettings == nullptr)
    {
        CTextInputPrivateSettings::Create(this, &m_pPrivateTextInputSettings);
        IFC_RETURN(m_pPrivateTextInputSettings->Initialize(m_pTextServices));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::EnsureDocument()
{
    if (m_spDocument == nullptr)
    {
        IFC_RETURN(GetDocument(&m_spDocument));
    }

    return S_OK;
}

void CTextBoxBase::ReleaseDocumentIfNotFocused()
{
    if (!IsFocused())
    {
        m_spDocument = nullptr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a value indicating whether spell check should be enabled for the
//      text control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextBoxBase::GetSpellCheckEnabled(_Out_ bool *pIsSpellCheckEnabled, _Out_ bool *pIsDefault) const
{
    *pIsSpellCheckEnabled = m_isSpellCheckEnabled;
    *pIsDefault = m_spellCheckIsDefault;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a value indicating whether text predictions should be enabled for
//      the text control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextBoxBase::GetTextPredictionEnabled(_Out_ bool *pIsTextPredictionEnabled, _Out_ bool *pIsDefault) const
{
    *pIsTextPredictionEnabled = m_isTextPredictionEnabled;
    *pIsDefault = m_textPredictionIsDefault;
    return S_OK;
}

// Returns true if the composition string exists.
bool CTextBoxBase::ShouldGenerateLinguisticAlternatives()
{
    return m_compositionLength > 0;
}

// Resets the cached composition start index and length.
void CTextBoxBase::ResetLinguisticAlternativeState()
{
    if (!m_compositionInProgress)
    {
        if (!m_previouslyGeneratedAlternatives)
        {
            // Text change without composition, clear alternatives
            m_compositionStartIndex = -1;
            m_compositionLength = -1;
        }

        // Always reset.
        m_previouslyGeneratedAlternatives = false;
    }
}

_Check_return_ HRESULT CTextBoxBase::RaiseCandidateWindowBoundsChangedEventForRoot(_In_ const RECT* rect)
{
    XRECTF resultRect;

    // Convert from view to text box outer frame
    XPOINTF outerPoints[2] = { 0 };
    xref_ptr<CGeneralTransform> transform;

    ASSERT(GetView());
    IFC_RETURN(GetView()->TransformToVisual(this, &transform));

    ASSERT(transform);
    XPOINTF tmpPoints[2] = {
        { static_cast<float>(rect->left), static_cast<float>(rect->top) },
        { static_cast<float>(rect->right), static_cast<float>(rect->bottom) }
    };

    IFC_RETURN(transform->TransformPoints(tmpPoints, outerPoints, 2));

    resultRect.X = outerPoints[0].x;
    resultRect.Y = outerPoints[0].y;
    resultRect.Width = outerPoints[1].x - outerPoints[0].x;
    resultRect.Height = outerPoints[1].y - outerPoints[0].y;

    IFC_RETURN(RaiseCandidateWindowBoundsChangedEvent(resultRect));

    m_firedCandidateWindowEventAfterFocus = true;
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::RaiseCandidateWindowBoundsChangedEventForDIPs(_In_ const wf::Rect& rect)
{
    XPOINTF ltPoint = { rect.X, rect.Y };
    XPOINTF rbPoint = { rect.X + rect.Width, rect.Y + rect.Height };

    IFC_RETURN(ClientToTextBox(&ltPoint));
    IFC_RETURN(ClientToTextBox(&rbPoint));

    const RECT rectConvert = { static_cast<LONG>(ltPoint.x),
        static_cast<LONG>(ltPoint.y),
        static_cast<LONG>(rbPoint.x),
        static_cast<LONG>(rbPoint.y) };

    IFC_RETURN(RaiseCandidateWindowBoundsChangedEventForRoot(&rectConvert));

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::RaiseCandidateWindowBoundsChangedEvent(_In_ XRECTF &xRect)
{
    // Current focused textbox has not registered for candidate window event
    if (m_CandidateWindowevent.index == KnownEventIndex::UnknownType_UnknownEvent)
    {
        return S_OK;
    }

    if (CEventManager *eventManager = GetContext()->GetEventManager())
    {
        auto args = make_xref<CCandidateWindowBoundsChangedEventArgs>();
        args->m_bounds = xRect;
        eventManager->Raise(m_CandidateWindowevent, TRUE /* refire */, this, args.get());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::OnGripperHeld(_In_ CEventArgs* pEventArgs)
{
    CHoldingEventArgs *pHoldingEventArgs = static_cast<CHoldingEventArgs*>(pEventArgs);
    const bool holdCompleted = pHoldingEventArgs->m_holdingState == DirectUI::HoldingState::Completed;

    if (holdCompleted)
    {
        // RichEdit does not support Context menu on grippers.
        if (HasContextFlyout())
        {
            OnContextRequested(this, pEventArgs);
        }
        else
        {
            // Convert Holding Completed to Right Mouse Button Up in order to show the context menu.
            RE_MOUSEINPUT reMouseInput;
            IFC_RETURN(PackageMouseEventParams(WM_RBUTTONUP, pHoldingEventArgs, DirectUI::VirtualKeyModifiers::None, pHoldingEventArgs->m_pointerDeviceType, &reMouseInput));

            XLONG_PTR result = 0;
            IFC_RETURN(TextBoxBase_Internal::TxSendMessageHelper(this, EM_MOUSEINPUT, WM_RBUTTONUP, 0, reinterpret_cast<LPARAM>(&reMouseInput), &pHoldingEventArgs->m_bHandled, &result));
        }
    }

    return S_OK;
}

void CTextBoxBase::GetActualSize(_Out_ float& actualWidth, _Out_ float& actualHeight)
{
    actualWidth = GetActualWidth();
    actualHeight = GetActualHeight();
}

void CTextBoxBase::GetOriginalSize(_Out_ float& originalWidth, _Out_ float& originalHeight)
{
    originalWidth = GetActualWidth();
    originalHeight = GetActualHeight();
}

bool CTextBoxBase::IsInXamlIsland()
{
    auto pContentRootForRoot = VisualTree::GetContentRootForElement(this);
    return (pContentRootForRoot->GetType() == CContentRoot::Type::XamlIslandRoot) &&
        (pContentRootForRoot->GetIslandType() == CContentRoot::IslandType::DesktopWindowContentBridge);
}

bool CTextBoxBase::IsSecondaryInputScopeDefault() const
{
    if (nullptr == m_pInputScope)
    {
        return false;
    }

    if (m_pInputScope->m_pNames->GetCount() < 2)
    {
        return false;
    }

    xref_ptr<CInputScopeName> spInputScopeName;
    spInputScopeName.attach(static_cast<CInputScopeName*>(m_pInputScope->m_pNames->GetItemDOWithAddRef(1)));
    ::InputScopeNameValue secondaryInputScope = static_cast<::InputScopeNameValue>(spInputScopeName->m_nameValue);

    return (secondaryInputScope == InputScopeNameValueDefault);
}

_Check_return_ HRESULT CTextBoxBase::HideSelection(_In_ bool hideSelection)
{
    m_hideSelection = hideSelection;
    IFC_RETURN(GetTextServices()->OnTxPropertyBitsChange(TXTBIT_HIDESELECTION, hideSelection ? TXTBIT_HIDESELECTION : 0));

    return S_OK;
}

bool CTextBoxBase::ShouldHideGrippersOnFlyoutOpening()
{
    return FxCallbacks::TextControlFlyout_IsGettingFocus(GetContextFlyout().get(), this);
}

bool CTextBoxBase::ShouldForceFocusedVisualState()
{
    return FxCallbacks::TextControlFlyout_IsGettingFocus(GetSelectionFlyoutNoRef(), this)
        || FxCallbacks::TextControlFlyout_IsGettingFocus(GetContextFlyout().get(), this);
}

bool CTextBoxBase::HasSelectionFlyout() const
{
    auto selectionFlyout = GetSelectionFlyoutNoRef();
    return selectionFlyout != nullptr;
}

bool CTextBoxBase::HasContextFlyout() const
{
    auto contextFlyout = GetContextFlyout();
    return contextFlyout != nullptr;
}

_Check_return_ HRESULT CTextBoxBase::FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const  wf::Point& point)
{
    handled = false;

    XPOINTF pointerPosition = { point.X, point.Y};

    xref_ptr<ITransformer> spTransformer;
    IFC_RETURN(m_pView->TransformToRoot(spTransformer.ReleaseAndGetAddressOf()));
    IFC_RETURN(spTransformer->Transform(&pointerPosition, &pointerPosition, 1 /* count */));

    // Convert to DIPS by adjusting for scale
    const float zoomScale = RootScale::GetRasterizationScaleForElement(this);
    pointerPosition /= zoomScale;

    // We cannot use EventManager here due to reentrancy so raising through the peer directly
    KnownTypeIndex index = GetTypeIndex();
    if (index == KnownTypeIndex::TextBox)
    {
        IFC_RETURN(FxCallbacks::TextBox_OnContextMenuOpeningHandler(static_cast<CDependencyObject*>(this), pointerPosition.x, pointerPosition.y, handled));
    }
    else if (index == KnownTypeIndex::RichEditBox)
    {
        IFC_RETURN(FxCallbacks::RichEditBox_OnContextMenuOpeningHandler(static_cast<CDependencyObject*>(this), pointerPosition.x, pointerPosition.y, handled));
    }
    else if (index == KnownTypeIndex::PasswordBox)
    {
        IFC_RETURN(FxCallbacks::PasswordBox_OnContextMenuOpeningHandler(static_cast<CDependencyObject*>(this), pointerPosition.x, pointerPosition.y, handled));
    }

    return S_OK;
}

CFlyoutBase* CTextBoxBase::GetSelectionFlyoutNoRef() const
{
    auto selectionFlyoutProperty = GetSelectionFlyoutPropertyID();
    if (selectionFlyoutProperty == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        return nullptr;
    }

    CValue result;
    VERIFYHR(GetValueByIndex(selectionFlyoutProperty, &result));
    return do_pointer_cast<CFlyoutBase>(result.AsObject());
}

_Check_return_ HRESULT CTextBoxBase::QueueUpdateSelectionFlyoutVisibility()
{
    CInputManager& inputManager = VisualTree::GetContentRootForElement(this)->GetInputManager();

    m_lastInputDeviceType = inputManager.GetLastInputDeviceType();

    bool succeeded = false;
    IFC_RETURN(inputManager.TryGetPrimaryPointerLastPosition(&m_lastPointerPosition, &succeeded));

    if (succeeded && !m_isSelectionFlyoutUpdateQueued)
    {
        m_isSelectionFlyoutUpdateQueued = true;

        if (GetTypeIndex() == KnownTypeIndex::TextBox)
        {
            IFC_RETURN(FxCallbacks::TextBox_QueueUpdateSelectionFlyoutVisibility(this));
        }
        else if (GetTypeIndex() == KnownTypeIndex::RichEditBox)
        {
            IFC_RETURN(FxCallbacks::RichEditBox_QueueUpdateSelectionFlyoutVisibility(this));
        }
        else if (GetTypeIndex() == KnownTypeIndex::PasswordBox)
        {
            IFC_RETURN(FxCallbacks::PasswordBox_QueueUpdateSelectionFlyoutVisibility(this));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateSelectionFlyoutVisibility()
{
    m_isSelectionFlyoutUpdateQueued = false;

    if (HasSelectionFlyout() && !FxCallbacks::TextControlFlyout_IsOpen(GetContextFlyout().get()))
    {
        bool showSelectionFlyout = false;
        xaml_primitives::FlyoutShowMode showMode = xaml_primitives::FlyoutShowMode_Transient;
        switch (m_lastInputDeviceType)
        {
            case DirectUI::InputDeviceType::Mouse:
            {
                // For mouse, we only want to show the selection flyout if we're a RichEditBox -
                // otherwise, we have no selection flyout UI to display.
                // We additionally want to hide the flyout if the pointer moves away from it.
                showMode = xaml_primitives::FlyoutShowMode_TransientWithDismissOnPointerMoveAway;
                if (GetTypeIndex() == KnownTypeIndex::RichEditBox)
                {
                    uint32_t selectionLength = 0;
                    IFC_RETURN(GetSelectionLength(selectionLength));
                    showSelectionFlyout = selectionLength > 0;
                }
                break;
            }
            case DirectUI::InputDeviceType::Pen:
            case DirectUI::InputDeviceType::Touch:
            {
                // For pen and touch, we want to show the selection flyout anytime we have a selection.
                // If we've just set the caret without a selection, we won't show the flyout.
                uint32_t selectionLength = 0;
                IFC_RETURN(GetSelectionLength(selectionLength));
                showSelectionFlyout = selectionLength > 0;
                break;
            }
            default:
                // For all other forms of input (e.g., keyboard), we never show the selection flyout.
                break;
        }

        if (showSelectionFlyout)
        {
            auto selectionFlyout = GetSelectionFlyoutNoRef();

            if (selectionFlyout)
            {
                XPOINTF lastPointerPosition = m_lastPointerPosition;

                // Transform point
                wrl::ComPtr<ITransformer> transformer;
                IFC_RETURN(TransformToRoot(transformer.ReleaseAndGetAddressOf()));

                IFC_RETURN(transformer->ReverseTransform(&lastPointerPosition, &lastPointerPosition, 1));

                wf::Point point = { lastPointerPosition.x, lastPointerPosition.y };

                wf::Rect exclusionRect{};
                IFC_RETURN(GetSelectionBoundingRect(&exclusionRect));

                // We want the point to appear at a consistent location regardless of whether it overlaps the selection,
                // so we'll set its vertical position to be the top of the exclusion rectangle.
                point.Y = exclusionRect.Y;

                IFC_RETURN(FxCallbacks::TextControlFlyout_ShowAt(selectionFlyout, this, point, exclusionRect, showMode));
            }
        }
        else
        {
            IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));
        }
    }

    m_lastInputDeviceType = DirectUI::InputDeviceType::None;
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::GetSelectionLength(_Out_ uint32_t& selectionLength)
{
    ctl::ComPtr<ITextSelection2> selection;
    IFC_RETURN(GetSelection(&selection));

    LONG selectionStart;
    IFC_RETURN(selection->GetStart(&selectionStart));

    LONG selectionEnd;
    IFC_RETURN(selection->GetEnd(&selectionEnd));

    selectionLength = selectionEnd - selectionStart;

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::EnsureProofingMenu()
{
    // Proofing MenuFlyout
    if (!m_proofingMenu)
    {
        CREATEPARAMETERS cp(GetContext());
        CValue value;

        IFC_RETURN(CMenuFlyout::Create(reinterpret_cast<CDependencyObject**>(m_proofingMenu.ReleaseAndGetAddressOf()), &cp));
        IFC_RETURN(m_proofingMenu->PegManagedPeer());
        IFC_RETURN(m_proofingMenu->AddParent(this));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::GetProofingMenuFlyout(
    _In_ CDependencyObject* pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue* ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    auto textBoxBase = do_pointer_cast<CTextBoxBase>(pObject);

    if (!textBoxBase || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CFlyoutBase* proofingMenu = nullptr;
    IFC_RETURN(textBoxBase->GetProofingMenuFlyoutNoRef(proofingMenu));
    pResult->SetObjectAddRef(proofingMenu);
    IFC_RETURN(FxCallbacks::TextControlFlyout_AddProofingFlyout(proofingMenu, textBoxBase));
    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::GetProofingMenuFlyoutNoRef(_Out_ CFlyoutBase*& proofingMenu)
{
    IFC_RETURN(EnsureProofingMenu());

    if (m_isSpellCheckEnabled && IsFocused())
    {
        IFC_RETURN(UpdateProofingMenu());
        m_proofingMenuIsValid = true;
    }

    proofingMenu = do_pointer_cast<CFlyoutBase>(m_proofingMenu.get());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::GetSpellingInfoForCP(_Out_ GETCONTEXTMENUEX& gcmex)
{
    ctl::ComPtr<ITextSelection2> selection;
    IFC_RETURN(GetSelection(&selection));
    ASSERT(selection);

    LONG currentPosition;
    IFC_RETURN(selection->GetStart(&currentPosition));

    IFC_RETURN(m_pTextServices->TxSendMessage(EM_GETSPELLINGINFOFORCP, static_cast<WPARAM>(currentPosition), reinterpret_cast<LPARAM>(&gcmex), nullptr));

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::UpdateProofingMenu()
{
    auto core = GetContext();
    CREATEPARAMETERS cp(core);
    CValue value;

    // Get and clear the items
    IFC_RETURN(m_proofingMenu->GetValueByIndex(KnownPropertyIndex::MenuFlyout_Items, &value));
    CMenuFlyoutItemBaseCollection* items = do_pointer_cast<CMenuFlyoutItemBaseCollection>(value);
    IFC_RETURN(items->Clear());

    // Get the spelling info for the current position from RichEidt
    GETCONTEXTMENUEX gcmex { };
    IFC_RETURN(GetSpellingInfoForCP(gcmex));

    // Add spelling suggestions
    IFC_RETURN(AddSpellingSuggestions(gcmex, items));

    // Add separator if we have spelling suggestions
    if (items->GetCount() > 0)
    {
        xref_ptr<CMenuFlyoutSeparator> menuFlyoutSeparator;
        IFC_RETURN(CMenuFlyoutSeparator::Create(reinterpret_cast<CDependencyObject**>(menuFlyoutSeparator.ReleaseAndGetAddressOf()), &cp));
        IFC_RETURN(menuFlyoutSeparator->EnsurePeerDuringCreate());
        value.SetObjectAddRef(menuFlyoutSeparator);
        IFC_RETURN(items->Append(value));
    }

    // Add spell check buttons
    auto browserHost = GetContext()->GetBrowserHost();

    // Misspelled word
    if (gcmex.dwFlags & GCMF_SPELLING)
    {
        // Add to dictionary
        xstring_ptr addToDictionaryText;
        IFC_RETURN(browserHost->GetLocalizedResourceString(TEXT_SPELLCHECK_ADD_TO_DICTIONARY, &addToDictionaryText));
        IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnAddToDictionaryClicked, addToDictionaryText, items));

        // Ignore all
        xstring_ptr ignoreAllText;
        IFC_RETURN(browserHost->GetLocalizedResourceString(TEXT_SPELLCHECK_IGNORE, &ignoreAllText));
        IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnIgnoreAllClicked, ignoreAllText, items));
    }

    // Repeated word
    if (gcmex.dwFlags & GCMF_SPELLING_REPEATEDWORD)
    {
        // Delete repeated word
        xstring_ptr deleteRepeatedText;
        IFC_RETURN(browserHost->GetLocalizedResourceString(TEXT_SPELLCHECK_DELETE_REPEATED_WORD, &deleteRepeatedText));
        IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnDeleteRepeatedClicked, deleteRepeatedText, items));

        // Ignore once
        xstring_ptr ignoreOnceText;
        IFC_RETURN(browserHost->GetLocalizedResourceString(TEXT_SPELLCHECK_IGNORE, &ignoreOnceText));
        IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnIgnoreOnceClicked, ignoreOnceText, items));
    }

    // Autocorrected word
    if (gcmex.dwFlags & GCMF_SPELLING_AUTOCORRECTED)
    {
        // Stop correcting
        wil::unique_bstr bstrCorrectedWord;
        bstrCorrectedWord.reset(gcmex.bstrAutoCorrectedWord);

        xstring_ptr stopCorrectingFormat;
        IFC_RETURN(browserHost->GetLocalizedResourceString(TEXT_SPELLCHECK_STOP_CORRECTING, &stopCorrectingFormat));

        XStringBuilder strBuilder;
        const uint32_t strCount = 256;
        PWSTR pstrBuffer = nullptr;

        IFC_RETURN(strBuilder.InitializeAndGetFixedBuffer(strCount, &pstrBuffer));

        IFC_RETURN(StringCchPrintf(
            pstrBuffer,
            strCount,
            stopCorrectingFormat.GetBuffer(),
            bstrCorrectedWord.get()));

        strBuilder.SetFixedBufferCount(static_cast<uint32_t>(wcslen(pstrBuffer)));

        xstring_ptr stopCorrectingText;
        IFC_RETURN(strBuilder.DetachString(&stopCorrectingText));

        IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnStopCorrectingClicked, stopCorrectingText, items));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::AddSpellingSuggestions(const GETCONTEXTMENUEX& gcmex, _Out_ CMenuFlyoutItemBaseCollection*& items)
{
    Microsoft::WRL::ComPtr<IEnumString> suggestions = gcmex.pSpellingSuggestions;

    if (suggestions && (gcmex.dwFlags & GCMF_SPELLING))
    {
        ULONG nFetched = 0;
        uint8_t suggestionCount = 0;
        wil::unique_cotaskmem_string suggestionText;
        const uint8_t maxSuggestionCount = 3;

        while (SUCCEEDED(suggestions->Next(1, &suggestionText, &nFetched)) && nFetched == 1 && suggestionCount++ < maxSuggestionCount)
        {
            auto core = GetContext();
            CREATEPARAMETERS cp(core);

            xstring_ptr spellingSuggestionText;
            IFC_RETURN(xstring_ptr::CloneBuffer(suggestionText.get(), &spellingSuggestionText));

            IFC_RETURN(CreateProofingMenuItem(TextContextMenu::OnSpellingSuggestionClicked, spellingSuggestionText, items));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxBase::CreateProofingMenuItem(_In_ INTERNAL_EVENT_HANDLER eventHandler, const xstring_ptr& itemText, _Out_ CMenuFlyoutItemBaseCollection*& items)
{
    auto core = GetContext();
    CREATEPARAMETERS cp(core);

    // Create menu item
    xref_ptr<CMenuFlyoutItem> menuItem;
    IFC_RETURN(CMenuFlyoutItem::Create(reinterpret_cast<CDependencyObject**>(menuItem.ReleaseAndGetAddressOf()), &cp));
    IFC_RETURN(menuItem->EnsurePeerDuringCreate());

    // Set menu item's text
    CValue value;
    value.SetString(itemText);
    IFC_RETURN(menuItem->SetValueByIndex(KnownPropertyIndex::MenuFlyoutItem_Text, value));

    // Attach click event handler
    IFC_RETURN(FxCallbacks::TextBox_AddMenuFlyoutItemClickHandler(menuItem.get(), eventHandler));

    // Set item and append to list
    value.SetObjectAddRef(menuItem);
    IFC_RETURN(items->Append(value));

    return S_OK;
}

 _Check_return_ HRESULT CTextBoxBase::GetContextMenuShowPosition(_Out_ XPOINTF *position)
{
    XRECT_RB beginRect;
    XRECT_RB endRect;
    IFC_RETURN(GetView()->GetSelectionEdgeRects(&beginRect, &endRect));

    xref_ptr<CGeneralTransform> transformFromViewToTextBox;
    IFC_RETURN(GetView()->TransformToVisual(this, &transformFromViewToTextBox));

    XPOINTF contextMenuShowPosition{};

    if (IsRightToLeft())
    {
        contextMenuShowPosition.x = static_cast<float>(beginRect.left);
        contextMenuShowPosition.y = static_cast<float>(beginRect.bottom);
    }
    else
    {
        contextMenuShowPosition.x = static_cast<float>(endRect.right);
        contextMenuShowPosition.y = static_cast<float>(endRect.bottom);
    }

    IFC_RETURN(transformFromViewToTextBox->TransformPoints(&contextMenuShowPosition, &contextMenuShowPosition));

    *position = contextMenuShowPosition;
    return S_OK;
}

 _Check_return_ HRESULT CTextBoxBase::GetSelectionBoundingRect(_Out_ wf::Rect *selectionBoundingRect)
 {
     std::vector<RECT> selectionRects;
     IFC_RETURN(GetView()->GetSelectionRects(selectionRects));

     XRECT_RB boundingRectRB = { INT_MAX, INT_MAX, 0, 0 };

     for (RECT selectionRect : selectionRects)
     {
         boundingRectRB.left = std::min(boundingRectRB.left, static_cast<int>(selectionRect.left));
         boundingRectRB.top = std::min(boundingRectRB.top, static_cast<int>(selectionRect.top));
         boundingRectRB.right = std::max(boundingRectRB.right, static_cast<int>(selectionRect.right));
         boundingRectRB.bottom = std::max(boundingRectRB.bottom, static_cast<int>(selectionRect.bottom));
     }

     wf::Rect boundingRect;
     boundingRect.X = static_cast<float>(boundingRectRB.left);
     boundingRect.Y = static_cast<float>(boundingRectRB.top);
     boundingRect.Width = static_cast<float>(boundingRectRB.right - boundingRectRB.left);
     boundingRect.Height = static_cast<float>(boundingRectRB.bottom - boundingRectRB.top);

     *selectionBoundingRect = boundingRect;
     return S_OK;
 }


_Check_return_ HRESULT
CTextBoxBase::ForceRemoveFocus()
{
    CFocusManager *pFocusManager;

    if (IsFocused())
    {
        pFocusManager = VisualTree::GetFocusManagerForElement(this);

        IFCPTR_RETURN(pFocusManager);

        pFocusManager->ClearFocus();
    }

    return S_OK;
}

bool CTextBoxBase::ShouldUseVisualPixels() // TSF3 and UIA coordinates are based on visual relative pixel
{
   // We should be calling contentRoot->ShouldUseVisualRelativePixels but there is a AppWindow on desktop bug for IME candidate window
   // For now, only apply visual pixels for TSF3 on WCOS.
    return XamlOneCoreTransforms::IsEnabled();
}

_Check_return_ HRESULT CTextBoxBase::EnterImpl(_In_ CDependencyObject *namescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(namescopeOwner, params));

    if (params.fIsLive && GetContext()->HasXamlIslandRoots())
    {
        // Text Services is initialized when element is not in the tree yet, causing it to attach to the wrong HWND
        // Options considered for fixing this:
        // 1. create Text Services in EnterImpl, but places needed to access it before entering the tree (eg: setting text programmatically)
        // 2. create a new text services in EnterImpl and transfer state from the old one, which is no good because we can't read out the old state perfectly
        // 3. send it a EM_RESETHWND message, resetting HWND of text services when element enters a new window
        IFC_RETURN(m_pTextServices->TxSendMessage(EM_RESETHWND, 0, 0, NULL));
    }

    return S_OK;
}

// in some cases, we don't want to raise a character received event but still want to send
// XCP_CHAR events to Text Services Framework (TSF). This function gets called in such cases
_Check_return_ HRESULT CTextBoxBase::InjectCharaterReceivedTSF(_In_ CEventArgs* pEventArgs)
{
    return OnCharacterReceived(pEventArgs);
}
