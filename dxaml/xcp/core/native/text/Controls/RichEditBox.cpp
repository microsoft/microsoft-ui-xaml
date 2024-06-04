// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditBox.h"
#include "Indexes.g.h"
#include <XamlTraceLogging.h>
#include <windows.ui.text.h>

using namespace DirectUI;

#define EN_HIDELINKTOOLTIP        0x0716

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
CRichEditBox::CRichEditBox(_In_ CCoreServices *pCore) : CTextBoxBase(pCore)
{
    m_bAcceptsReturn = true;
    m_linkHoverTarget = LinkMouseHoverTarget::None;
    m_eNonHyperlinkCursor = MouseCursorIBeam;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
CRichEditBox::~CRichEditBox()
{
    Destroy();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates a new CRichEditBox.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::Create(
       _Outptr_ CDependencyObject **ppObject,
       _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CRichEditBox> pRichEditBox;
    pRichEditBox.attach(new CRichEditBox(pCreate->m_pCore));


    IFC_RETURN(pRichEditBox->Initialize());

    *ppObject = pRichEditBox.detach();

    return S_OK;
}

KnownEventIndex CRichEditBox::GetCopyPropertyID() const { return KnownEventIndex::RichEditBox_CopyingToClipboard; }
KnownEventIndex CRichEditBox::GetPastePropertyID() const { return KnownEventIndex::RichEditBox_Paste; }
KnownEventIndex CRichEditBox::GetCutPropertyID() const { return KnownEventIndex::RichEditBox_CuttingToClipboard; }
KnownPropertyIndex CRichEditBox::GetSelectionHighlightColorPropertyID() const { return KnownPropertyIndex::RichEditBox_SelectionHighlightColor; }
KnownPropertyIndex CRichEditBox::GetSelectionHighlightColorWhenNotFocusedPropertyID() const { return KnownPropertyIndex::RichEditBox_SelectionHighlightColorWhenNotFocused; }

KnownEventIndex CRichEditBox::GetTextCompositionEventPropertyID(TextCompositionStage compositionStage) const
{
    KnownEventIndex index = KnownEventIndex::UnknownType_UnknownEvent;
    switch (compositionStage)
    {
        case TextCompositionStage::CompositionStarted:
            index = KnownEventIndex::RichEditBox_TextCompositionStarted;
            break;
        case TextCompositionStage::CompositionChanged:
            index = KnownEventIndex::RichEditBox_TextCompositionChanged;
            break;
        case TextCompositionStage::CompositionEnded:
            index = KnownEventIndex::RichEditBox_TextCompositionEnded;
            break;
    }
    return index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a DependencyProperty value.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    CValue tempValue;

    EnableSetValueReentrancyGuard();

    IFC(ValidateSetValueArguments(args.m_pDP, args.m_value));

    IFC(CTextBoxBase::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::RichEditBox_ClipboardCopyFormat:
        IFC(GetValue(args.m_pDP, &tempValue));
        IFC(SetLanguageOption(IMF_COPYPLAINTEXTONLY,
            static_cast<DirectUI::RichEditClipboardFormat>(tempValue.AsEnum()) == DirectUI::RichEditClipboardFormat::PlainText));
        break;

    case KnownPropertyIndex::RichEditBox_DesiredCandidateWindowAlignment:
        IFC(GetValue(args.m_pDP, &tempValue));
        IFC(OnDesiredCandidateWindowAlignmentChanged(static_cast<DirectUI::CandidateWindowAlignment>(tempValue.AsEnum())));
        break;

    case KnownPropertyIndex::RichEditBox_TextWrapping:
        IFC(OnTextWrappingChanged());
        InvalidateView();
        break;

    case KnownPropertyIndex::RichEditBox_IsReadOnly:
        IFC(OnIsReadOnlyChanged());
        break;

    case KnownPropertyIndex::RichEditBox_AcceptsReturn:
        IFC(OnAcceptsReturnChanged());
        break;

    case KnownPropertyIndex::RichEditBox_TextAlignment:
        IFC(OnTextAlignmentChanged());
        break;

    case KnownPropertyIndex::RichEditBox_IsSpellCheckEnabled:
        m_spellCheckIsDefault = false;
        IFC(OnIsSpellCheckEnabledChanged(m_isSpellCheckEnabled));
        break;

    case KnownPropertyIndex::RichEditBox_IsTextPredictionEnabled:
        m_textPredictionIsDefault = false;
        IFC(OnIsTextPredictionEnabledChanged(m_isTextPredictionEnabled));
        break;

    case KnownPropertyIndex::RichEditBox_InputScope:
        IFC(OnInputScopeChanged(m_pInputScope));
        break;

    case KnownPropertyIndex::RichEditBox_IsColorFontEnabled:
        InvalidateViewAndForceRedraw();
        break;

    case KnownPropertyIndex::RichEditBox_SelectionHighlightColor:
        OnSelectionHighlightColorChanged();
        break;

    case KnownPropertyIndex::RichEditBox_SelectionHighlightColorWhenNotFocused:
        IFC(OnSelectionHighlightColorWhenNotFocusedChanged());
        break;

    case KnownPropertyIndex::RichEditBox_TextReadingOrder:
        IFC(OnBidiOptionsChanged());
        break;

    case KnownPropertyIndex::RichEditBox_MaxLength:
        IFC(OnMaxLengthChanged());
        break;

    case KnownPropertyIndex::RichEditBox_DisabledFormattingAccelerators:
        IFC(OnFormattingAcceleratorsChanged());
        break;
    }

Cleanup:
    ClearSetValueReentrancyGuard();

    if (hr == E_INVALIDARG)
    {
        IGNOREHR(SetAndOriginateError(hr, RuntimeError, AG_E_RUNTIME_SETVALUE));
    }

    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the inner RichEdit document.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::GetDocument(
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppDocument
    )
{
    return GetTextServices()->QueryInterface(iid, reinterpret_cast<void **>(ppDocument));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::Initialize()
{
    IFC_RETURN(CTextBoxBase::Initialize());

    // IsTextPredictionEnabled defaults to true, so init that here.
    IFC_RETURN(OnIsTextPredictionEnabledChanged(m_isTextPredictionEnabled));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called after RichEdit modifies the backing store.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::OnContentChanged(_In_ const bool fTextChanged)
{
    InvalidateView();

    //
    // Raise TextChanging and TextChanged events.
    //

    auto core = GetContext();

    if (!ParserOwnsParent())
    {
        CEventManager *eventManager = core->GetEventManager();

        if (eventManager)
        {
            // We cannot use EventManager here due to MSFT:1993154 so raising through the peer directly as a workaround
            IFC_RETURN(FxCallbacks::RichEditBox_OnTextChangingHandler(this, fTextChanged));

            xref_ptr<CRoutedEventArgs> args;
            args.attach(new CRoutedEventArgs());
            eventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::RichEditBox_TextChanged), this, args);

            if (core->UIAClientsAreListening(UIAXcp::AETextPatternOnTextChanged) == S_OK)
            {
                if (!m_pAP)
                {
                    OnCreateAutomationPeer();
                }

                if (m_pAP)
                {
                    m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextChanged);
                }
            }
        }
    }

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::RichEditBox_ShowPlaceholderTextHandler(this, IsEmpty()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the selection is changed. Fires a selection
//      changed event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::OnSelectionChanged()
{
    CEventManager *pEventManager = NULL;

    wrl::ComPtr<ITextSelection2> spSelection;
    long selectionStart;
    long selectionEnd;
    long selectionLength;

    IFC_RETURN(GetSelection(&spSelection));

    IFC_RETURN(spSelection->GetStart(&selectionStart));
    IFC_RETURN(spSelection->GetEnd(&selectionEnd));
    selectionLength = selectionEnd - selectionStart;

    // An empty selection with non-zero length is the EOP and we don't want to select it.
    if (IsEmpty())
    {
        long nLength;
        IFC_RETURN(spSelection->GetCch(&nLength));

        if (nLength != 0)
        {
            // SetRange causes re-entrancy back into the function.
            // Re-entrancy will occur and the goto Cleanup avoids having us run all the code below the goto twice.
            IFC_RETURN(spSelection->SetRange(selectionStart, selectionStart));
            return S_OK;
        }
    }
    BOOLEAN SelectionChangingCanceled;
    IFC_RETURN(ProcessSelectionChangingEvent(selectionStart, selectionLength, SelectionChangingCanceled));
    if (SelectionChangingCanceled)
    {
        return S_OK;
    }

    IFC_RETURN(CTextBoxBase::OnSelectionChanged());

    // Selection changed. Proofing menu is no longer valid.
    if (m_iSelectionStart != selectionStart || m_iSelectionLength != selectionLength)
    {
        m_proofingMenuIsValid = false;
    }

    m_iSelectionStart = selectionStart;
    m_iSelectionLength = selectionLength;

    // Raise SelectionChanged event.
    pEventManager = GetContext()->GetEventManager();

    if (pEventManager)
    {
        xref_ptr<CRoutedEventArgs> spArgs;
        spArgs.init(new CRoutedEventArgs());

        pEventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::RichEditBox_SelectionChanged), this, spArgs);

        if (m_pAP)
        {
            m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextSelectionChanged);
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control processes cr/lf from user input.
//
//---------------------------------------------------------------------------
bool CRichEditBox::AcceptsReturn() const
{
    return m_bAcceptsReturn;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control contains formatted text, otherwise the
//      control only contains plaintext Unicode.
//
//---------------------------------------------------------------------------
bool CRichEditBox::AcceptsRichText() const
{
    return true;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the current TextAlignment.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::GetAlignment(_Out_ TextAlignment *pAlignment)
{
    CValue value;

    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::RichEditBox_TextAlignment), &value));

    *pAlignment = static_cast<TextAlignment>(value.AsEnum());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control cannot be modified by the user.
//
//---------------------------------------------------------------------------
bool CRichEditBox::IsReadOnly() const
{
    return m_bIsReadOnly;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control has no content, FALSE otherwise.
//
//---------------------------------------------------------------------------
bool CRichEditBox::IsEmpty()
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<ITextRange>  spTextRange;
    LONG storyLength = 0;
    bool isEmpty = false;

    IFC(EnsureDocument());
    IFC(m_spDocument->Range(0, 2, &spTextRange));
    IFC(spTextRange->GetStoryLength(&storyLength));

    if (storyLength <= 1)
    {
        isEmpty = true;
    }

Cleanup:
    if (FAILED(hr))
    {
        isEmpty = true;
    }

    ReleaseDocumentIfNotFocused();
    return isEmpty;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, notifies the host about content or selection
//      changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::TxNotify(
    _In_ UINT32 notification,
    _In_ void *pData
    )
{
    IFC_RETURN(CTextBoxBase::TxNotify(notification, pData));

    switch (notification)
    {
        case EN_LINK:
            IFC_RETURN(HandleLinkNavigation(notification, static_cast<ENLINK*>(pData)->msg, pData));
            break;
        case EN_HIDELINKTOOLTIP:
            // Invoked when mouse leaves the HyperLink
            m_linkHoverTarget = LinkMouseHoverTarget::None;
            if (GetView())
            {
                IFC_RETURN(GetView()->SetCursor(m_eNonHyperlinkCursor));
            }
            break;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, notifies the host about an input action over a link
//      This method inspects the input action and performs a navigation if necessary
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::HandleLinkNavigation(_In_ UINT32 linkType, _In_ UINT msg, _In_ void* pData)
{
    bool fNavigate = false;

    switch (msg)
    {
    case WM_LBUTTONUP:
    case WM_POINTERUP:
    case EM_HANDLELINKNOTIFICATION:
        if ((msg == WM_LBUTTONUP && m_lastMessage == WM_LBUTTONUP)
            || (msg == WM_POINTERUP && m_lastMessage == WM_POINTERUP))
        {
            IFC_RETURN(ShouldNavigateLinkOnMouseClick(&fNavigate));
        }
        else if ((msg == WM_LBUTTONUP && (m_lastMessage == WM_KEYDOWN || m_lastMessage == WM_KEYUP)) || msg == EM_HANDLELINKNOTIFICATION)
        {
            // Enter key is sent to TxNotify as mouse left button down. Navigate
            // on keyboard Enter key. Also, it supports the case when call is coming from UIA client
            // in which case there is no Mouse/Keyboard related message.
            fNavigate = true;
        }

        if (fNavigate && linkType == EN_LINK)
        {
            ENLINK* pEnLink = static_cast<ENLINK*>(pData);

            wrl::ComPtr<ITextDocument2> spTextDocument;
            IFC_RETURN(CTextBoxBase::GetDocument(spTextDocument.GetAddressOf()));

            Microsoft::WRL::ComPtr<ITextRange> spTextRange;
            IFC_RETURN(spTextDocument->Range(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, spTextRange.GetAddressOf()));

            wil::unique_bstr spLinkText;
            IFC_RETURN(spTextRange->GetText(&spLinkText));
            IFC_RETURN(FxCallbacks::RichEditBox_HandleHyperlinkNavigation(spLinkText.get(), SysStringLen(spLinkText.get())));
        }
        else
        {
            return EN_LINK_DO_DEFAULT;
        }
        break;
    case WM_MOUSEMOVE:
        CTextBoxView* pView = GetView();

        if (pView)
        {
            if (m_linkHoverTarget == LinkMouseHoverTarget::None)
            {
                m_eNonHyperlinkCursor = pView->m_eMouseCursor;
            }

            MouseCursor navigateCursor = MouseCursorArrow;

            switch (linkType)
            {
            case EN_LINK:
                m_linkHoverTarget = LinkMouseHoverTarget::HyperLink;

                IFC_RETURN(ShouldNavigateLinkOnMouseClick(&fNavigate));
                navigateCursor = fNavigate ? MouseCursorArrow : MouseCursorIBeam;
                break;
            default:
                m_linkHoverTarget = LinkMouseHoverTarget::None;
                break;
            }

            if (pView->m_eMouseCursor != navigateCursor)
            {
                IFC_RETURN(GetView()->SetCursor(navigateCursor));
            }
        }
        break;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if current input state should cause a link navigation.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::ShouldNavigateLinkOnMouseClick(_Out_ bool *pfNavigate)
{
    // For mouse input, hyperlink navigated with click in readonly mode and
    // Ctrl+Click in editable mode.
    if (IsReadOnly())
    {
        *pfNavigate = true;
    }
    else
    {
        XUINT32 modifierKeys = 0;
        IFC_RETURN(gps->GetKeyboardModifiersState(&modifierKeys));
        *pfNavigate = modifierKeys & KEY_MODIFIER_CTRL ? true : false;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//       Event handler for KeyDown event.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::OnKeyDown(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(CTextBoxBase::OnKeyDown(pEventArgs));

    // If the mouse is over a hyperlink and Ctrl key is now pressed, change the cursor
    // to hyperlink cursor
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
    CTextBoxView* pView = GetView();

    if (pKeyEventArgs
        && m_linkHoverTarget == LinkMouseHoverTarget::HyperLink
        && !IsReadOnly()
        && pView
        && pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Control)
    {
        IFC_RETURN(pView->SetCursor(MouseCursorArrow));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//       Event handler for KeyUp event.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::OnKeyUp(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(CTextBoxBase::OnKeyUp(pEventArgs));

    // If the mouse is over a hyperlink and the Ctrl key is released, revert the cursor
    // back to non-hyperlink state.
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);
    CTextBoxView* pView = GetView();

    if (pKeyEventArgs
        && m_linkHoverTarget == LinkMouseHoverTarget::HyperLink
        && !IsReadOnly()
        && pView
        && pKeyEventArgs->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Control)
    {
        IFC_RETURN(GetView()->SetCursor(m_eNonHyperlinkCursor));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Parameter validation for SetValue calls.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::ValidateSetValueArguments(
    _In_ const CDependencyProperty *pProperty,
    _In_ const CValue& value
    )
{
    switch (pProperty->GetIndex())
    {
        case KnownPropertyIndex::TextBox_TextWrapping:
            // TextWrapping enum is shared between *Block and *Box. As specified,
            // WrapWithOverflow is an invalid argument.
            if (IsWrapWholeWords(value))
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;
        case KnownPropertyIndex::RichEditBox_MaxLength:
            if (!IsPositiveInteger(value))
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;
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
_Check_return_ HRESULT CRichEditBox::OnApplyTemplate()
{
    // Delay updating richedit for spell checking until OnApplyTemplate, this is to optimize perf (avoid start/stop if spellchecking is turned off in markup)
    IFC_RETURN(OnIsSpellCheckEnabledChanged(m_isSpellCheckEnabled));
    IFC_RETURN(OnFormattingAcceleratorsChanged());
    IFC_RETURN(CTextBoxBase::OnApplyTemplate());

    IFC_RETURN(FxCallbacks::RichEditBox_OnApplyTemplateHandler(this));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for CharacterReceived event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditBox::OnCharacterReceived(_In_ CEventArgs* pEventArgs)
{
    CTextBoxBase::OnCharacterReceived(pEventArgs);

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::RichEditBox_ShowPlaceholderTextHandler(this, IsEmpty()));

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
_Check_return_ HRESULT CRichEditBox::SetTextServicesBuffer(
    _In_ const xstring_ptr& strText
        // If NULL, removes all content.
    )
{
    CTextBoxBase::SetTextServicesBuffer(strText);


    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::RichEditBox_ShowPlaceholderTextHandler(this, strText.IsNull()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the InputScopeNameValue associated with this text control. If no
//      InputScopeNameValue has been set, returns InputScopeNameValueDefault.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRichEditBox::GetInputScope(_Out_ ::InputScopeNameValue *pInputScope)
{
    ::InputScopeNameValue result = InputScopeNameValueDefault;

    IFCPTRRC_RETURN(pInputScope, E_INVALIDARG);
    *pInputScope = result;

    if (nullptr == m_pInputScope)
    {
        result = InputScopeNameValueDefault;
    }
    else
    {
        xref_ptr<CInputScopeName> pCInputScopeName;
        pCInputScopeName.attach(static_cast<CInputScopeName*>(m_pInputScope->m_pNames->GetItemDOWithAddRef(0)));
        result = static_cast<::InputScopeNameValue>(pCInputScopeName->m_nameValue);
    }
    *pInputScope = result;

    return S_OK;
}

_Check_return_ HRESULT CRichEditBox::OnFormattingAcceleratorsChanged()
{
    DirectUI::DisabledFormattingAccelerators disabledFormattingAccelerators;
    CValue value;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::RichEditBox_DisabledFormattingAccelerators, &value));
    disabledFormattingAccelerators = static_cast<DirectUI::DisabledFormattingAccelerators>(value.AsEnum());

    LPARAM lparam = CFE_BOLD | CFE_ITALIC | CFE_UNDERLINE;
    if (static_cast<int>(disabledFormattingAccelerators & DirectUI::DisabledFormattingAccelerators::Bold) != 0)
    {
        lparam &= ~CFE_BOLD;
    }
    if (static_cast<int>(disabledFormattingAccelerators & DirectUI::DisabledFormattingAccelerators::Italic) != 0)
    {
        lparam &= ~CFE_ITALIC;
    }
    if (static_cast<int>(disabledFormattingAccelerators & DirectUI::DisabledFormattingAccelerators::Underline) != 0)
    {
        lparam &= ~CFE_UNDERLINE;
    }

    IFC_RETURN(GetTextServices()->TxSendMessage(EM_SETFORMATACCELERATORS, 0, lparam, nullptr));

    return S_OK;
}

_Check_return_ HRESULT CRichEditBox::UpdateVisualState()
{
    IFC_RETURN(CTextBoxBase::UpdateVisualState());
    return S_OK;
}