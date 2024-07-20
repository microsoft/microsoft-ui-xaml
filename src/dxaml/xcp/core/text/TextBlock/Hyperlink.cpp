// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Theme.h"
#include "ColorUtil.h"
#include <CaretBrowsingGlobal.h>
#include <FrameworkTheming.h>

std::optional<bool> CHyperlink::s_underlineVisibleResourceDirective;

//------------------------------------------------------------------------
//
//      Creates a Hyperlink element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_     CREATEPARAMETERS   *pCreate
)
{
    HRESULT        hr           = S_OK;
    CHyperlink    *pHyperlink   = nullptr;

    // For initializing & registering default values & event handlers.
    CValue               cValue;

    // From generic.xaml:
    // HyperlinkForegroundThemeBrush - "normal" color
    // HyperlinkPointerOverForegroundThemeBrush - hover color
    // HyperlinkPressedForegroundThemeBrush - pressed color
    // The default Foreground property is set to the "normal" color

    CDependencyObject* pForegroundBrushDO = nullptr;

    // Start the creation process.
    pHyperlink = new CHyperlink(pCreate->m_pCore);

    IFC(pHyperlink->UpdateForegroundColor(HYPERLINK_NORMAL));
    IFC(pHyperlink->UpdateUnderline());

    // NOTE: AddEventListener() will allocate m_pEventList, which needs to be freed in destructor.
    cValue.SetInternalHandler(&CHyperlink::GotFocusEventListener);
    IFC(CEventManager::AddEventListener(
        pHyperlink,
        &(pHyperlink->m_pEventList),
        EventHandle(KnownEventIndex::UIElement_GotFocus),
        &cValue,
        EVENTLISTENER_INTERNAL,
        nullptr));

    cValue.SetInternalHandler(&CHyperlink::LostFocusEventListener);
    IFC(CEventManager::AddEventListener(
        pHyperlink,
        &(pHyperlink->m_pEventList),
        EventHandle(KnownEventIndex::UIElement_LostFocus),
        &cValue,
        EVENTLISTENER_INTERNAL,
        nullptr));

    cValue.SetInternalHandler(&CHyperlink::KeyUpEventListener);
    IFC(CEventManager::AddEventListener(
        pHyperlink,
        &(pHyperlink->m_pEventList),
        EventHandle(KnownEventIndex::UIElement_KeyUp),
        &cValue,
        EVENTLISTENER_INTERNAL,
        nullptr));

    cValue.SetInternalHandler(&CHyperlink::KeyDownEventListener);
    IFC(CEventManager::AddEventListener(
        pHyperlink,
        &(pHyperlink->m_pEventList),
        EventHandle(KnownEventIndex::UIElement_KeyDown),
        &cValue,
        EVENTLISTENER_INTERNAL,
        nullptr));

    *ppObject = pHyperlink;
    pHyperlink = nullptr;

Cleanup:
    ReleaseInterface(pHyperlink);
    ReleaseInterface(pForegroundBrushDO);
    return hr;
}

//------------------------------------------------------------------------
//
//  CHyperlink Constructor
//
//------------------------------------------------------------------------
CHyperlink::CHyperlink(_In_ CCoreServices *pCore)
    : CSpan(pCore)
    , m_pEventList(nullptr)
    , m_strNavigateUri()
    , m_pAP(nullptr)
    , m_state(HYPERLINK_NORMAL)
    , m_isLinkNavigationKeyDown(false)
{
    m_focusableHelper = new CFocusableHelper(this);
}

CHyperlink::~CHyperlink()
{
    // Clear event list
    if (m_pEventList)
    {
        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = nullptr;
    }

    if (m_pAP)
    {
        m_pAP->InvalidateOwner();   // The automation peer can outlive this element, so detach it.
        ReleaseInterface(m_pAP);
    }

    if (m_focusableHelper)
    {
        delete m_focusableHelper;
    }
}

// The "HyperlinkUnderlineVisible" theme resource value is used to evaluate the presence of the underline.
// For performance reasons, the directive is only evaluated once. TAEF tests can invalidate the cache by calling TestServices::Utilities::DeleteResourceDictionaryCaches().
bool
CHyperlink::UnderlineVisibleResourceDirective() const
{
    if (!s_underlineVisibleResourceDirective.has_value())
    {
        s_underlineVisibleResourceDirective = UnderlineVisibleResourceDirective(GetContext());
    }

    return s_underlineVisibleResourceDirective.value();
}

// Invoked when a TAEF test calls TestServices::Utilities::DeleteResourceDictionaryCaches().
/*static*/
void
CHyperlink::ClearUnderlineVisibleResourceDirectiveCache()
{
    s_underlineVisibleResourceDirective.reset();
}

/*static*/
bool
CHyperlink::UnderlineVisibleResourceDirective(_In_ CCoreServices* core)
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_hyperlinkUnderlineVisible, L"HyperlinkUnderlineVisible");
    bool underlineVisibleResourceDirective = false;
    bool underlineVisibleResourceDirectiveExists = false;

    IFCFAILFAST(CDependencyProperty::GetBooleanThemeResourceValue(core, c_hyperlinkUnderlineVisible, &underlineVisibleResourceDirective, &underlineVisibleResourceDirectiveExists));

    if (!underlineVisibleResourceDirectiveExists)
    {
        // When the HyperlinkUnderlineVisible resource is missing (on older OS versions), the fallback is to render the underline for compatibility.
        underlineVisibleResourceDirective = true;
    }

    return underlineVisibleResourceDirective;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base AddEventListener
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}

//------------------------------------------------------------------------
//
//  Static method to be called for when we receive focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::GotFocusEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    )
{
    CHyperlink *pThis = do_pointer_cast<CHyperlink>(pSender);
    IFC_RETURN(pThis->FocusChangedEventListener(pEventArgs, TRUE));

    bool hasToolTip = false;
    IFC_RETURN(pThis->HasToolTip(&hasToolTip));

    if (hasToolTip)
    {
        IFC_RETURN(FxCallbacks::ToolTipService_OnOwnerGotFocus(pThis, static_cast<CRoutedEventArgs*>(pEventArgs)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  LostFocusEventListener
//
//  Summary: Static method to be called for when we lose focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::LostFocusEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CHyperlink *pThis = do_pointer_cast<CHyperlink>(pSender);
    IFC_RETURN(pThis->FocusChangedEventListener(pEventArgs, FALSE));

    bool hasToolTip = false;
    IFC_RETURN(pThis->HasToolTip(&hasToolTip));

    if (hasToolTip)
    {
        IFC_RETURN(FxCallbacks::ToolTipService_OnOwnerLostFocus(pThis, static_cast<CRoutedEventArgs*>(pEventArgs)));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  FocusChangedEventListener
//
//  Remarks: When focus changes on a Hyperlink - either when it receives or
//  loses focus, it must invalidate render on its host FrameworkElement
//  to ensure that focus rects are correctly rendered/ not rendered.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::FocusChangedEventListener(
    _In_ CEventArgs* pEventArgs,
    _In_ bool       bGotFocus)
{
    xref_ptr<CTextPointerWrapper> pContentStart;
    xref_ptr<CTextPointerWrapper> pContentEnd;
    CFrameworkElement *pContentStartVisualParent;
    CFrameworkElement *pContentEndVisualParent;
    IFC_RETURN(GetContentStart(pContentStart.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetContentStart(pContentEnd.ReleaseAndGetAddressOf()));
    IFC_RETURN(pContentStart->GetVisualParent(&pContentStartVisualParent));
    IFC_RETURN(pContentEnd->GetVisualParent(&pContentEndVisualParent));

    // Invalidate rendering on all associated containers.
    if (pContentStartVisualParent != nullptr)
    {
        if (pContentStartVisualParent == pContentEndVisualParent)
        {
            // TextBlock or single RichTextBlock.
            CUIElement::NWSetContentDirty(pContentStartVisualParent, DirtyFlags::Render);
        }
        else
        {
            //TODO: this else seems like never going to run because the if statement is always true.
            RichTextServices::ILinkedTextContainer *pLinkedContainer = reinterpret_cast<RichTextServices::ILinkedTextContainer*>(pContentStartVisualParent);
            RichTextServices::ILinkedTextContainer *pLastLinkedContainer = reinterpret_cast<RichTextServices::ILinkedTextContainer*>(pContentEndVisualParent);
            while (pLinkedContainer != nullptr)
            {
                CUIElement::NWSetContentDirty(reinterpret_cast<CUIElement*>(pLinkedContainer), DirtyFlags::Render);
                if (pLinkedContainer == pLastLinkedContainer)
                {
                    break;
                }
                pLinkedContainer = pLinkedContainer->GetNext();
            }
        }
        if (GetCaretBrowsingModeEnable() && bGotFocus)   //moving the selection Caret
        {
            xref_ptr<CTextPointerWrapper> selectionStart;
            xref_ptr<CTextPointerWrapper> selectionEnd;
            XINT32 contentStartOffset;
            XINT32 contentEndOffset;
            XINT32 selectionStartOffset;
            XINT32 selectionEndOffset;
            IFC_RETURN(GetContentEnd(pContentEnd.ReleaseAndGetAddressOf()));
            IFC_RETURN(pContentStart->GetOffset(&contentStartOffset));
            IFC_RETURN(pContentEnd->GetOffset(&contentEndOffset));
            if (pContentStartVisualParent->GetTypeIndex() == KnownTypeIndex::TextBlock)
            {
                CTextBlock* textObject = do_pointer_cast<CTextBlock>(pContentStartVisualParent);
                IFC_RETURN(textObject->GetSelectionStart(selectionStart.ReleaseAndGetAddressOf()));
                IFC_RETURN(textObject->GetSelectionEnd(selectionEnd.ReleaseAndGetAddressOf()));
                IFC_RETURN(selectionStart->GetOffset(&selectionStartOffset));
                IFC_RETURN(selectionEnd->GetOffset(&selectionEndOffset));

                //only move if the selection is outside the range of HyperLink characters
                if (selectionStartOffset < contentStartOffset || selectionEndOffset < contentStartOffset || selectionStartOffset > contentEndOffset || selectionEndOffset > contentEndOffset)
                {
                    IFC_RETURN(textObject->Select(pContentStart, pContentStart));  //set the anchor and moving position same
                }
            }
            else if (pContentStartVisualParent->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
            {
                CRichTextBlock* textObject = do_pointer_cast<CRichTextBlock>(pContentStartVisualParent);
                IFC_RETURN(textObject->GetSelectionStart(selectionStart.ReleaseAndGetAddressOf()));
                IFC_RETURN(textObject->GetSelectionEnd(selectionEnd.ReleaseAndGetAddressOf()));
                IFC_RETURN(selectionStart->GetOffset(&selectionStartOffset));
                IFC_RETURN(selectionEnd->GetOffset(&selectionEndOffset));

                if (selectionStartOffset < contentStartOffset || selectionEndOffset < contentStartOffset || selectionStartOffset > contentEndOffset || selectionEndOffset > contentEndOffset)
                {
                    IFC_RETURN(textObject->Select(pContentStart, pContentStart));
                }
            }
            else if (pContentStartVisualParent->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
            {
                CRichTextBlockOverflow* textObject = do_pointer_cast<CRichTextBlockOverflow>(pContentStartVisualParent);
                CRichTextBlock* richTextBlock = textObject->m_pMaster;
                IFCFAILFAST(richTextBlock->GetSelectionStart(selectionStart.ReleaseAndGetAddressOf()));
                IFCFAILFAST(richTextBlock->GetSelectionEnd(selectionEnd.ReleaseAndGetAddressOf()));
                IFC_RETURN(selectionStart->GetOffset(&selectionStartOffset));
                IFC_RETURN(selectionEnd->GetOffset(&selectionEndOffset));

                if (selectionStartOffset < contentStartOffset || selectionEndOffset < contentStartOffset || selectionStartOffset > contentEndOffset || selectionEndOffset > contentEndOffset)
                {
                    IFC_RETURN(richTextBlock->Select(pContentStart, pContentStart));
                }
            }
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Static method to be called for KeyUp is received.
//  We only get keyboard input when we have focus, so if we see
//  the <Enter> key pressed, go navigate!
//
//  Remarks: We're using KeyUp instead of KeyDown because KeyDown is synchronous, which
//           introduces the possibility of reentrancy in certain scenarios.
//------------------------------------------------------------------------

_Check_return_ HRESULT CHyperlink::KeyUpEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CHyperlink *pThis = nullptr;
    CKeyEventArgs *pKeyEventArgs = nullptr;

    IFC_RETURN(DoPointerCast(pThis, pSender));
    pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    //We need to make sure that we saw KeyDown on NavigationKeys before we try to Navigate!
    if (pThis->m_isLinkNavigationKeyDown && IsLinkNavigationKey(pKeyEventArgs->m_platformKeyCode))
    {
        IFC_RETURN(pThis->UpdateForegroundColor(HYPERLINK_NORMAL));
        IFC_RETURN(pThis->Navigate());

        pThis->m_isLinkNavigationKeyDown = false;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Static method to be called for KeyDown is received.
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::KeyDownEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    CHyperlink *pThis = nullptr;
    CKeyEventArgs *pKeyEventArgs = nullptr;

    IFC_RETURN(DoPointerCast(pThis, pSender));
    pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    if (IsLinkNavigationKey(pKeyEventArgs->m_platformKeyCode))
    {
        pThis->m_isLinkNavigationKeyDown = true;
        IFC_RETURN(pThis->UpdateForegroundColor(HYPERLINK_PRESSED));
    }

    return S_OK;
}

_Check_return_ HRESULT CHyperlink::OnPointerEntered(_In_ CPointerEventArgs* pEventArgs)
{
    bool hasToolTip = false;
    IFC_RETURN(HasToolTip(&hasToolTip));

    if (hasToolTip)
    {
        IFC_RETURN(FxCallbacks::ToolTipService_OnOwnerPointerEntered(this, static_cast<CPointerEventArgs*>(pEventArgs)));
    }

    return S_OK;
}

_Check_return_ HRESULT CHyperlink::OnPointerExited(_In_ CPointerEventArgs* pEventArgs)
{
    IFC_RETURN(UpdateForegroundColor(HYPERLINK_NORMAL));

    bool hasToolTip = false;
    IFC_RETURN(HasToolTip(&hasToolTip));

    if (hasToolTip)
    {
        IFC_RETURN(FxCallbacks::ToolTipService_OnOwnerPointerExitedOrLostOrCanceled(this, static_cast<CPointerEventArgs*>(pEventArgs)));
    }

    return S_OK;
}

bool CHyperlink::IsLinkNavigationKey(const UINT32 key)
{
    return (key == VK_RETURN || key == VK_SPACE);
}

//------------------------------------------------------------------------
//
//  Add our events to the global event request list.
//  Need to do this ourselves since we don't derive from CUIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params
    )
{
    CEventManager *pEventManager = nullptr;

    IFC_RETURN(CSpan::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        auto core = GetContext();

        // If there are events registered on this element, ask the
        // EventManager to extract them and create a request for every event.
        if (m_pEventList)
        {
            // Get the event manager.
            IFCEXPECT_ASSERT_RETURN(core);
            pEventManager = core->GetEventManager();
            IFCEXPECT_ASSERT_RETURN(pEventManager);
            IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
        }

        const auto contentRoot = VisualTree::GetContentRootForElement(this);
        const auto& akExport = contentRoot->GetAKExport();

        if (akExport.IsActive())
        {
            IFC_RETURN(akExport.AddElementToAKMode(this));
        }

        IFC_RETURN(RegisterToolTip());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Clear our events from the global event request list.
//  Need to do this ourselves since we don't derive from CUIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params
    )
{
    CEventManager *pEventManager = nullptr;

    IFC_RETURN(CSpan::LeaveImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // Reverse of the Add() operation in EnterImpl().
        if (m_pEventList)
        {
            auto core = GetContext();
            IFCEXPECT_ASSERT_RETURN(core);
            pEventManager = core->GetEventManager();
            IFCEXPECT_ASSERT_RETURN(pEventManager);
            IFC_RETURN(pEventManager->RemoveRequests(this, m_pEventList));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Hyperlink is not a general span, it only supports Run as child.
//  Enforce that limitation here.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CHyperlink::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Span_Inlines)
    {
        CInlineCollection* pInlines = nullptr;
        XUINT32            cPositions = 0;

        if (FAILED(DoPointerCast(pInlines, args.m_value.AsObject())) || !pInlines)
        {
            IFC_RETURN(static_cast<HRESULT>(E_DO_INVALID_CONTENT));
        }

        // Only empty inline collections are allowed to be set directly.
        pInlines->GetPositionCount(&cPositions);
        if( cPositions > 0 )
        {
            IFC_RETURN(static_cast<HRESULT>(E_DO_INVALID_CONTENT));
        }
    }

    IFC_RETURN(CInline::SetValue(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::TextElement_Foreground)
    {
        if (!IsAnimatedProperty(args.m_pDP) && !IsPropertyDefault(args.m_pDP) && m_state != HYPERLINK_NORMAL)
        {
            IFC_RETURN(UpdateForegroundColor(GetTheme(), m_state));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Initiates the process to navigate to the specified URI.
//  Calls into DXAML layer for URI processing, following precedence of
//  HyperlinkButton class.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::Navigate()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CHyperlinkClickEventArgs *pArgs = nullptr;
    auto core = GetContext();

    // Raise Click event.
    CEventManager *pEventManager = core->GetEventManager();

    if (pEventManager)
    {
        pArgs = new CHyperlinkClickEventArgs();

        pEventManager->Raise(
            EventHandle(KnownEventIndex::Hyperlink_Click),
            TRUE,
            this,
            pArgs,
            TRUE);//Raise Sync
    }

    IFC(FxCallbacks::FrameworkCallbacks_Hyperlink_OnClick(this));

Cleanup:
    // Ignore navigation failure, as it is not crucial to continue execution.
    ReleaseInterfaceNoNULL(pArgs);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  OnCreateAutomationPeer
//
//  Summary: Return the hyperlink automation peer object
//
//------------------------------------------------------------------------
CAutomationPeer* CHyperlink::OnCreateAutomationPeer()
{
    if (m_pAP == nullptr)
    {
        m_pAP = CTextElement::OnCreateAutomationPeerInternal();
    }

    return m_pAP;
}

// Update the hyperlink's foreground color for the given theme and state.
_Check_return_ HRESULT CHyperlink::UpdateForegroundColor(_In_ Theming::Theme theme, _In_ HyperlinkStates state)
{
    CValue value;
    xref_ptr<CDependencyObject> foregroundBrush;
    const CDependencyProperty* foregroundProperty = GetPropertyByIndexInline(KnownPropertyIndex::TextElement_Foreground);

    if (m_state != state)
    {
        m_state = state;

        IFC_RETURN(UpdateUnderline());
    }

    // Clear the animated value so that the foreground color doesn't get "stuck" in the hover or pressed state.
    IFC_RETURN(ClearAnimatedValue(foregroundProperty, CValue::Empty()));

    // Threshold foreground depends on colors
    switch (m_state)
    {
        // Prioritize the use of resources named HyperlinkForeground, HyperlinkForegroundPointerOver & HyperlinkForegroundPressed starting in the Sun Valley release.

        case HYPERLINK_NORMAL:
            if (!IsPropertyDefault(foregroundProperty))
            {
                IFC_RETURN(GetValue(foregroundProperty, &value));
            }
            else
            {
                IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"HyperlinkForeground"), foregroundBrush.ReleaseAndGetAddressOf()));
                if (!foregroundBrush)
                {
                    IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"SystemControlHyperlinkTextBrush"), foregroundBrush.ReleaseAndGetAddressOf()));
                }
            }
            break;

        case HYPERLINK_POINTOVER:
            IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"HyperlinkForegroundPointerOver"), foregroundBrush.ReleaseAndGetAddressOf()));
            if (!foregroundBrush)
            {
                IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"SystemControlHyperlinkBaseMediumBrush"), foregroundBrush.ReleaseAndGetAddressOf()));
            }
            break;

        case HYPERLINK_PRESSED:
            IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"HyperlinkForegroundPressed"), foregroundBrush.ReleaseAndGetAddressOf()));
            if (!foregroundBrush)
            {
                IFC_RETURN(GetContext()->LookupThemeResource(theme, XSTRING_PTR_EPHEMERAL(L"SystemControlHighlightBaseMediumLowBrush"), foregroundBrush.ReleaseAndGetAddressOf()));
            }
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    // Wrap the theme resource DO if we got it above.
    if (foregroundBrush)
    {
        value.WrapObjectNoRef(foregroundBrush.get());
    }

    if (!value.IsUnset())
    {
        // Set the animated value, this will not affect the Foreground property.
        IFC_RETURN(SetAnimatedValue(foregroundProperty, value));
    }

    return S_OK;
}

// Update the Hyperlink underline's presence based on UnderlineStyle, theme and visual state.
_Check_return_ HRESULT CHyperlink::UpdateUnderline()
{
    CValue decorationsUnderline;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::TextElement_TextDecorations, &decorationsUnderline));

    // Starting with 21H1, when the HyperlinkUnderlineVisible resource is set to False, the underline is not rendered in the PointerOver and Pressed states, unless a HighContrast theme is in use.
    bool oldUnderline = static_cast<DirectUI::TextDecorations>(decorationsUnderline.AsEnum()) == DirectUI::TextDecorations::Underline;
    bool newUnderline = ShouldDrawUnderline() && (UnderlineVisibleResourceDirective() || GetContext()->GetFrameworkTheming()->HasHighContrastTheme() || m_state == HYPERLINK_NORMAL);

    if (oldUnderline != newUnderline)
    {
        decorationsUnderline.Set(newUnderline ? DirectUI::TextDecorations::Underline : DirectUI::TextDecorations::None);

        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TextElement_TextDecorations, decorationsUnderline));
        
        //
        // Subtle but important behavior - when tapping on a Hyperlink, we get OnPointerPressed,
        // OnPointerReleased, and OnTapped on the CTextBlock. If Hyperlink's UnderlineVisibleResource
        // is false, then we'll remove the underline during OnPointerPressed and restore it in OnPointerReleased.
        // That dirties m_pPageNode's layout. OnTapped then comes in and does a hit test against
        // the Hyperlinks, but that hit test will fail in TextBlockView::PixelPositionToTextPosition
        // because we don't hit test a page node that's dirty for layout. OnTapped then fails to
        // find the Hyperlink that was tapped, and we never call Navigate on it.
        //
        // The problem is that in between OnPointerReleased and OnTapped there's no time for Xaml
        // to run a layout pass, so the page node stays dirty for layout. We do an explicit layout
        // pass here to clean the page node so that OnTapped can successfully hit test against it.
        //
        // This is not a problem with mouse clicks, because that updates the hyperlink for pointer
        // over. The underline still isn't shown in the pointer over state, so we don't put it back
        // just yet and the page node doesn't get its layout invalidated.
        //
        if (auto containingFrameworkElement = GetContainingFrameworkElement())
        {
            IFC_RETURN(containingFrameworkElement->UpdateLayout());
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//
//  Update hyperlink color to new theme
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CHyperlink::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    return UpdateForegroundColor(theme, m_state);
}

DirectUI::UnderlineStyle CHyperlink::GetUnderlineStyle() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::Hyperlink_UnderlineStyle, &result));
    return static_cast<DirectUI::UnderlineStyle>(result.AsEnum());
}

_Check_return_ HRESULT CHyperlink::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Hyperlink_UnderlineStyle:
        {
            IFC_RETURN(UpdateUnderline());
        }
        break;
    }

    IFC_RETURN(__super::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ToolTipService_ToolTip:
        {
            // If we previously had a tool tip, we need to unregister it before we register the new one.
            if (!args.m_pOldValue->IsNullOrUnset())
            {
                IFC_RETURN(UnregisterToolTip());
            }

            // Now if we have a new tool tip, we can register it at this time.
            if (!args.m_pNewValue->IsNullOrUnset())
            {
                IFC_RETURN(RegisterToolTip());
            }
        }
        break;
    }

    return S_OK;
}

bool CHyperlink::ShouldDrawUnderline() const
{
    bool shouldDraw = false;

    if (GetUnderlineStyle() != DirectUI::UnderlineStyle::None)
    {
        // Post-TH, both phone and desktop have underline.
        shouldDraw = true;
    }
    return shouldDraw;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns Content of the Hyperlink as a string.
//
//---------------------------------------------------------------------------

_Check_return_ HRESULT CHyperlink::GetContentText(_Outptr_ HSTRING *output)
{
    *output = nullptr;

    xref_ptr<CString> stringValue;
    IFC_RETURN(m_pInlines->GetText(false, stringValue.ReleaseAndGetAddressOf()));

    IFC_RETURN(WindowsCreateString(stringValue->m_strString.GetBuffer(), stringValue->m_strString.GetCount(), output));

    return S_OK;
}

_Check_return_ HRESULT CHyperlink::HasToolTip(_Out_ bool *hasToolTip)
{
    CValue value;
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::ToolTipService_ToolTip), &value));

    *hasToolTip = !value.IsNullOrUnset();

    return S_OK;
}

_Check_return_ HRESULT CHyperlink::RegisterToolTip()
{
    bool hasToolTip = false;
    IFC_RETURN(HasToolTip(&hasToolTip));

    CFrameworkElement *containingFrameworkElementNoRef = GetContainingFrameworkElement();

    if (hasToolTip &&
        containingFrameworkElementNoRef != nullptr)
    {
        // We need to guarantee that our containing framework element has a managed peer since the tooltip uses it,
        // so we'll create its managed peer now if we haven't already.
        IFC_RETURN(containingFrameworkElementNoRef->SetParticipatesInManagedTreeDefault());
        IFC_RETURN(containingFrameworkElementNoRef->EnsurePeer());

        IFC_RETURN(FxCallbacks::ToolTipService_RegisterToolTip(this, containingFrameworkElementNoRef));
    }

    return S_OK;
}

_Check_return_ HRESULT CHyperlink::UnregisterToolTip()
{
    CFrameworkElement *containingFrameworkElementNoRef = GetContainingFrameworkElement();

    if (containingFrameworkElementNoRef != nullptr)
    {
        IFC_RETURN(FxCallbacks::ToolTipService_UnregisterToolTip(this, containingFrameworkElementNoRef));
    }

    return S_OK;
}

// Returns the start of the Hyperlink's text content, tightly bound to not include any hidden positions.
// Fr example, in the following case, would return offsets of GetTextContentStart = 3, GetTextContentEnd = 6
// <Hyperlink><Run>abc</Run></Hyperlink><whatever comes next>
//  0          12  345       6           7
// This is different than GetContentStart and End, which wouild return 2 and 7, respectively.
// Will return null if Hyperlink has no display text.
_Check_return_ HRESULT CHyperlink::GetTextContentStart(_Outptr_result_maybenull_ CTextPointerWrapper **ppTextPointerWrapper)
{
    *ppTextPointerWrapper = nullptr;

    CInline *pTempAsIn = static_cast<CInline*>(this);
    int offset = 0;
    while (pTempAsIn
        && pTempAsIn->OfTypeByIndex<KnownTypeIndex::Span>())
    {
        // Drill into any nested spans to find the start of text after any hidden positions
        offset++;
        CInlineCollection *pCollection = static_cast<CSpan*>(pTempAsIn)->m_pInlines;
        if (pCollection == nullptr || pCollection->empty())
        {
            break;
        }
        pTempAsIn = static_cast<CInline*>((*pCollection)[0]);
    }
    if (pTempAsIn &&
        pTempAsIn->GetTypeIndex() == KnownTypeIndex::Run)
    {
        pTempAsIn->GetContentStart(ppTextPointerWrapper);
    }
    return S_OK;
}

_Check_return_ HRESULT CHyperlink::GetTextContentEnd(_Outptr_result_maybenull_ CTextPointerWrapper **ppTextPointerWrapper)
{
    *ppTextPointerWrapper = nullptr;

    CInline *pTempAsIn = static_cast<CInline*>(this);
    int offset = 0;
    while (pTempAsIn
        && pTempAsIn->OfTypeByIndex<KnownTypeIndex::Span>())
    {
        // Drill into any nested spans to find the end of text before any hidden positions
        offset++;
        CInlineCollection *pCollection = static_cast<CSpan*>(pTempAsIn)->m_pInlines;
        if (pCollection == nullptr || pCollection->empty())
        {
            break;
        }
        pTempAsIn = static_cast<CInline*>((*pCollection)[(*pCollection).size() - 1]);
    }
    if (pTempAsIn &&
        pTempAsIn->GetTypeIndex() == KnownTypeIndex::Run)
    {
        pTempAsIn->GetContentEnd(ppTextPointerWrapper);
    }
    return S_OK;
}
