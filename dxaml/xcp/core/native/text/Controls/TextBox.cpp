// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBox.h"
#include "Indexes.g.h"
#include "XboxUtility.h"
#include "ValueBuffer.h"
#include <XamlOneCoreTransforms.h>

using namespace DirectUI;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
CTextBox::CTextBox(_In_ CCoreServices *pCore)
    : CTextBoxBase(pCore)
    , m_strText()
    , m_strSelectedText()
    , m_isDesktopPopupMenuEnabled(false)
    , m_fHasSpaceForDeleteButton(FALSE)
    , m_isTextChangingFiring(FALSE)
{
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
CTextBox::~CTextBox()
{
    Destroy();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates a new CTextBox.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::Create(
       _Outptr_ CDependencyObject **ppObject,
       _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CTextBox> pTextBox;
    pTextBox.attach(new CTextBox(pCreate->m_pCore));


    IFC_RETURN(pTextBox->Initialize());

    *ppObject = pTextBox.detach();

    return S_OK;
}

KnownEventIndex CTextBox::GetPastePropertyID() const { return KnownEventIndex::TextBox_Paste; }
KnownEventIndex CTextBox::GetCopyPropertyID() const { return KnownEventIndex::TextBox_CopyingToClipboard; }
KnownEventIndex CTextBox::GetCutPropertyID() const { return KnownEventIndex::TextBox_CuttingToClipboard; }
KnownPropertyIndex CTextBox::GetSelectionHighlightColorPropertyID() const { return KnownPropertyIndex::TextBox_SelectionHighlightColor; }
KnownPropertyIndex CTextBox::GetSelectionHighlightColorWhenNotFocusedPropertyID() const { return KnownPropertyIndex::TextBox_SelectionHighlightColorWhenNotFocused; }
KnownEventIndex CTextBox::GetTextCompositionEventPropertyID(TextCompositionStage compositionStage) const
{
    KnownEventIndex index = KnownEventIndex::UnknownType_UnknownEvent;
    switch (compositionStage)
    {
        case TextCompositionStage::CompositionStarted:
            index = KnownEventIndex::TextBox_TextCompositionStarted;
            break;
        case TextCompositionStage::CompositionChanged:
            index = KnownEventIndex::TextBox_TextCompositionChanged;
            break;
        case TextCompositionStage::CompositionEnded:
            index = KnownEventIndex::TextBox_TextCompositionEnded;
            break;
    }
    return index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      RichEdit fires EN_STARTCOMPOSITION before EN_CHANGE, we can't wait EN_CHANGE to refresh
//      Text property on TextBox.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::UpdateTextForCompositionStartedEvent()
{
    // Update TextBox.Text property and send PropertyChanged notification only, leave everything else
    // such as TextChanging/TextChanged to subsequent EN_CHANGE message.

    IFC_RETURN(UpdateTextProperty(true /*notifyPropertyChangeOnly*/, false /*fTextChanged*/));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the index of CTextBox class.
//
//---------------------------------------------------------------------------
KnownTypeIndex CTextBox::GetTypeIndex() const
{
    return DependencyObjectTraits<CTextBox>::Index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a DependencyProperty value.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;
    xstring_ptr oldString;
    xstring_ptr newString;
    CValue tempValue;

    const bool shouldSetValue = args.m_pDP->GetIndex() != KnownPropertyIndex::TextBox_Text ||
        (m_isTextChangingFiring == TRUE && args.m_pDP->GetIndex() == KnownPropertyIndex::TextBox_Text);

    EnableSetValueReentrancyGuard();

    IFC(ValidateSetValueArguments(args.m_pDP, args.m_value));

    // Store the incoming text value.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::TextBox_Text)
    {
        ASSERT(args.m_pDP->GetStorageType() == valueString);

        oldString = m_strText;
        newString = args.m_value.As<valueString>();

        // There is a possibility that there is still a string value, it just can't be arbitrarily retrieved
        // through GetString. Instead, we need to first do some repackaging.
        if (newString.IsNull())
        {
            ValueBuffer valueBuffer(GetContext());
            CValue* temp = nullptr;
            IFC(valueBuffer.RepackageValueAndSetPtr(args.m_pDP, &args.m_value, &temp));

            if (temp != nullptr)
            {
                newString = temp->As<valueString>();
            }
        }
    }

    // We need to ensure that m_strText is not changed until after we fire BeforeTextChanging. m_strText will then
    // appropriately be set after the EN_CHANGE message. The only exception to this is when set text during a
    // EN_CHANGE message. We do not receive additional EN_CHANGE events after the first one has fired, meaning we'd want
    // to call SetValue in this situation
    if(shouldSetValue)
    {
        IFC(CTextBoxBase::SetValue(args));
    }

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextBox_Text:
            if (!oldString.Equals(newString))
            {
                IFC(SetTextServicesBuffer(newString));
            }
            break;

        case KnownPropertyIndex::TextBox_SelectedText:
            IFC(SetSelectedText(m_strSelectedText));
            break;

        case KnownPropertyIndex::TextBox_SelectionStart:
        case KnownPropertyIndex::TextBox_SelectionLength:
            IFC(Select(m_iSelectionStart, m_iSelectionLength));
            break;

        case KnownPropertyIndex::TextBox_TextWrapping:
            IFC(OnTextWrappingChanged());
            InvalidateView();
            break;

        case KnownPropertyIndex::TextBox_IsReadOnly:
            IFC(OnIsReadOnlyChanged());
            break;

        case KnownPropertyIndex::TextBox_TextAlignment:
            IFC(OnTextAlignmentChanged());
            break;

        case KnownPropertyIndex::TextBox_AcceptsReturn:
            IFC(OnAcceptsReturnChanged());
            break;

        case KnownPropertyIndex::TextBox_MaxLength:
            IFC(OnMaxLengthChanged());
            break;

        case KnownPropertyIndex::TextBox_IsSpellCheckEnabled:
            m_spellCheckIsDefault = false;
            IFC(OnIsSpellCheckEnabledChanged(m_isSpellCheckEnabled));
            break;

        case KnownPropertyIndex::TextBox_IsTextPredictionEnabled:
            m_textPredictionIsDefault = false;
            IFC(OnIsTextPredictionEnabledChanged(m_isTextPredictionEnabled));
            break;

        case KnownPropertyIndex::TextBox_DesiredCandidateWindowAlignment:
            IFC(GetValue(args.m_pDP, &tempValue));
            IFC(OnDesiredCandidateWindowAlignmentChanged(static_cast<DirectUI::CandidateWindowAlignment>(tempValue.AsEnum())));
            break;

        case KnownPropertyIndex::TextBox_InputScope:
            IFC(OnInputScopeChanged(m_pInputScope));
            break;

        case KnownPropertyIndex::TextBox_IsColorFontEnabled:
            InvalidateViewAndForceRedraw();
            break;

        case KnownPropertyIndex::TextBox_SelectionHighlightColor:
            OnSelectionHighlightColorChanged();
            break;
        case KnownPropertyIndex::TextBox_SelectionHighlightColorWhenNotFocused:
            IFC(OnSelectionHighlightColorWhenNotFocusedChanged());
            break;

        case KnownPropertyIndex::TextBox_TextReadingOrder:
            IFC(OnBidiOptionsChanged());
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

//------------------------------------------------------------------------
//
//  Synopsis:
//      Performs late template-dependent initialization. Attaches the
//      control to the container element from the template. Initializes
//      visual states.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::OnApplyTemplate()
{
    // Delay updating richedit for spell checking until OnApplyTemplate, this is to optimize perf (avoid start/stop if spellchecking is turned off in markup)
    IFC_RETURN(OnIsSpellCheckEnabledChanged(m_isSpellCheckEnabled));

    IFC_RETURN(CTextBoxBase::OnApplyTemplate());

    IFC_RETURN(FxCallbacks::TextBox_OnApplyTemplateHandler(this));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether the delete button should be visible or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize
    )
{
    CValue value;
    XFLOAT minimumWidthToEnableButton;

    IFC_RETURN(CTextBoxBase::ArrangeOverride(finalSize, newFinalSize));

    // Minimum width for TextBox with DeleteButton visible is 5em.
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::Control_FontSize), &value));
    minimumWidthToEnableButton = value.AsFloat() * 5;
    if (finalSize.width > minimumWidthToEnableButton && !m_fHasSpaceForDeleteButton)
    {
        m_fHasSpaceForDeleteButton = TRUE;
        IFC_RETURN(UpdateVisualState());
    }
    else if (finalSize.width <= minimumWidthToEnableButton && m_fHasSpaceForDeleteButton)
    {
        m_fHasSpaceForDeleteButton = FALSE;
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Synopsis:
//      Framework pinvoke, called when the templated DeleteButton is clicked.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::OnDeleteButtonClick()
{
    // It is possible to get Clear Button invoked through UIA even if it is collapsed.
    // Block this scenario by checking if the button can be shown in UI and invoked.
    if (CanInvokeDeleteButton())
    {
        IFC_RETURN(SetTextServicesBuffer(xstring_ptr::NullString()));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::Initialize()
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
_Check_return_ HRESULT CTextBox::OnContentChanged(_In_ const bool fTextChanged)
{
    InvalidateView();
    IFC_RETURN(UpdateTextProperty(false /*notifyPropertyChangeOnly*/, fTextChanged));

    return S_OK;
}

_Check_return_ HRESULT CTextBox::UpdateTextProperty(_In_ const bool notifyPropertyChangeOnly, _In_ const bool fTextChanged)
{
    xstring_ptr strText;
    xstring_ptr oldText;
    const CDependencyProperty *pTextProperty = GetPropertyByIndexInline(KnownPropertyIndex::TextBox_Text);
    CValue newValue;
    CValue oldValue;
    bool contentPreviouslyEmpty = m_strText.IsNullOrEmpty();
    bool propertyChangeListening = false;
    bool textChangeListening = false;
    auto core = GetContext();
    CEventManager* pEventManager = core->GetEventManager();

    if (!m_strText.IsNull())
    {
        oldText = m_strText;
        oldValue.SetString(oldText);
    }
    else
    {
        oldValue.SetNull();
    }

    IFC_RETURN(GetTextServicesBuffer(&strText));

    // Fire BeforeTextChanging. We do not want to fire this event during a TextComposition. In this scenario, we should wait
    // until the composition has ended. Once the composition has ended, we will get a final EN_CHANGE message that represents
    // the composed string. We should then fire the event for this composed string
    const bool shouldRaiseBeforeTextChanging = !m_compositionInProgress;

    if (shouldRaiseBeforeTextChanging)
    {
        BOOLEAN beforeTextChangingCanceled = FALSE;
        IFC_RETURN(FxCallbacks::TextBox_OnBeforeTextChangingHandler(this, &strText, &beforeTextChangingCanceled));

        if(beforeTextChangingCanceled)
        {
            // Any text change we receive here has already changed in richedit. As a result, we need
            // to communicate to richedit the fact that we've canceled the incoming text. To achieve this, we will
            // call SetText with the old text. In addition, we need to clear the undo stack and restore the
            // selection status

            // Cache the last selection start, since it will be changed after calling SetTextServicesBuffer
            const XINT32 lastSelectionStart = m_iLastSelectionStart;
            const XINT32 lastLength = m_iLastSelectionLength;

            IFC_RETURN(SetTextServicesBuffer(m_strText));

            ITextSelection2* pSelection = nullptr;
            IFC_RETURN(GetSelection(&pSelection));
            IFC_RETURN(pSelection->SetRange(lastSelectionStart, lastSelectionStart + lastLength));

            return S_OK;
        }
    }

    // Update public properties.
    // If the property system didn't change the content, we need to make the new value
    // as a local value now.
    IFC_RETURN(CoercePropertyValue(pTextProperty, m_strText, strText));
    newValue.SetString(m_strText);

    // Raise TextChanged event.
    if (!ParserOwnsParent() && !notifyPropertyChangeOnly)
    {
        // We only want to raise TextChanging and TextChanged events when we have not canceled them in the
        // BeforeTextChanging event
        if (pEventManager)
        {
            m_isTextChangingFiring = true;
            // We cannot use EventManager here due to possibility of reentrancy so raising through the peer directly
            IFC_RETURN(FxCallbacks::TextBox_OnTextChangingHandler(this, fTextChanged));
            m_isTextChangingFiring = false;

            xref_ptr<CTextChangedEventArgs> asyncArgs;
            asyncArgs.attach(new CTextChangedEventArgs());
            pEventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::TextBox_TextChanged), this, asyncArgs);

            propertyChangeListening = core->UIAClientsAreListening(UIAXcp::AEPropertyChanged) == S_OK;
            textChangeListening = core->UIAClientsAreListening(UIAXcp::AETextPatternOnTextChanged) == S_OK;
            if (propertyChangeListening || textChangeListening)
            {
                if (!m_pAP)
                {
                    OnCreateAutomationPeer();
                }

                if (m_pAP)
                {
                    if (textChangeListening)
                    {
                        m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextChanged);
                    }
                    if (propertyChangeListening)
                    {
                        m_pAP->RaisePropertyChangedEvent(UIAXcp::APValueValueProperty, oldValue, newValue);
                    }
                }
            }
        }
    }

    if (oldValue != newValue)
    {
        // Notify about property value change.
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                pTextProperty,
                oldValue,
                newValue)));
    }

    if (!notifyPropertyChangeOnly)
    {
        // Notify the change in content to the dxaml layer.
        IFC_RETURN(FxCallbacks::TextBox_ShowPlaceholderTextHandler(this, IsEmpty()));

        // Update the visual state of the control.
        if (contentPreviouslyEmpty || m_strText.IsNullOrEmpty())
        {
            IFC_RETURN(UpdateVisualState());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the selection is changed. Fires a selection
//      changed event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::OnSelectionChanged()
{
    wrl::ComPtr<ITextSelection2> spSelection = nullptr;
    long selectionStart;
    long selectionEnd;
    long selectionLength;

    IFC_RETURN(GetSelection(&spSelection));

    IFC_RETURN(spSelection->GetStart(&selectionStart));
    IFC_RETURN(spSelection->GetEnd(&selectionEnd));
    selectionLength = selectionEnd - selectionStart;
    ASSERT(selectionLength >= 0);
    BOOLEAN selectionChangingCanceled;
    IFC_RETURN(ProcessSelectionChangingEvent(selectionStart, selectionLength, selectionChangingCanceled));
    if (selectionChangingCanceled)
    {
        // double check to make sure selection change has been cancelled and original selection
        // is restored successfully, otherwise we still need to fire SelectionChanged event and
        // update SelectionStart, SelectionLength and SelectedText properties.
        long selectionStartAfterCancel;
        long selectionEndAfterCancel;
        IFC_RETURN(spSelection->GetStart(&selectionStartAfterCancel));
        IFC_RETURN(spSelection->GetEnd(&selectionEndAfterCancel));
        if (m_iSelectionStart == selectionStartAfterCancel && m_iSelectionStart + m_iSelectionLength == selectionEndAfterCancel)
        {
            wil::unique_bstr selectedTextAfterCancel;
            IFC_RETURN(spSelection->GetText(&selectedTextAfterCancel));
            if (m_strSelectedText.Equals(selectedTextAfterCancel.get()))
            {
                return S_OK;
            }
        }
    }

    CEventManager *pEventManager = nullptr;
    const CDependencyProperty *pSelectionStartProperty = GetPropertyByIndexInline(KnownPropertyIndex::TextBox_SelectionStart);
    const CDependencyProperty *pSelectionLengthProperty = GetPropertyByIndexInline(KnownPropertyIndex::TextBox_SelectionLength);
    const CDependencyProperty *pSelectedTextProperty = GetPropertyByIndexInline(KnownPropertyIndex::TextBox_SelectedText);
    wil::unique_bstr selectedText;
    xstring_ptr strSelectedText;
    CValue newValue;
    auto core = GetContext();

    IFC_RETURN(CTextBoxBase::OnSelectionChanged());

    // Update public properties.
    if (selectionLength > 0)
    {
        IFC_RETURN(spSelection->GetText(&selectedText));
        if (selectedText != nullptr)
        {
            IFC_RETURN(xstring_ptr::CloneBuffer(selectedText.get(), SysStringLen(selectedText.get()), &strSelectedText));
        }
    }

    // Selection changed. Proofing menu is no longer valid.
    if (m_iLastSelectionStart != m_iSelectionStart || m_iLastSelectionLength != m_iSelectionLength)
    {
        m_proofingMenuIsValid = false;
    }

    m_iLastSelectionStart = m_iSelectionStart;
    m_iLastSelectionLength = m_iSelectionLength;

    // If the property system didn't change the content, we need to make the new value
    // as a local value now.
    IFC_RETURN(CoercePropertyValue(pSelectedTextProperty, m_strSelectedText, strSelectedText));
    IFC_RETURN(CoercePropertyValue(pSelectionStartProperty, &m_iSelectionStart, (XINT32)selectionStart));
    IFC_RETURN(CoercePropertyValue(pSelectionLengthProperty, &m_iSelectionLength, (XINT32)selectionLength));

    //
    // Raise SelectionChanged event.
    //

    pEventManager = core->GetEventManager();

    if (pEventManager)
    {
        xref_ptr<CRoutedEventArgs> spArgs;
        spArgs.init(new CRoutedEventArgs());

        pEventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::TextBox_SelectionChanged), this, spArgs);

        if (core->UIAClientsAreListening(UIAXcp::AETextPatternOnTextSelectionChanged) == S_OK)
        {
            if(!m_pAP)
            {
                OnCreateAutomationPeer();
            }

            if (m_pAP)
            {
                m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextSelectionChanged);
            }
        }
    }

    newValue.SetSigned(m_iSelectionStart);
    IFC_RETURN(NotifyPropertyChanged(
        PropertyChangedParams(
        pSelectionStartProperty,
        CValue(),
        newValue)));

    newValue.SetSigned(m_iSelectionLength);
    IFC_RETURN(NotifyPropertyChanged(
        PropertyChangedParams(
        pSelectionLengthProperty,
        CValue(),
        newValue)));

    newValue.SetString(m_strSelectedText);
    IFC_RETURN(NotifyPropertyChanged(
        PropertyChangedParams(
        pSelectedTextProperty,
        CValue(),
        newValue)));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control processes cr/lf from user input.
//
//---------------------------------------------------------------------------
bool CTextBox::AcceptsReturn() const
{
    return m_bAcceptsReturn;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the AcceptsReturn property changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::OnAcceptsReturnChanged()
{
    IFC_RETURN(CTextBoxBase::OnAcceptsReturnChanged());

    IFC_RETURN(UpdateVisualState());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the current TextAlignment.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::GetAlignment(_Out_ TextAlignment *pAlignment)
{
    CValue value;

    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::TextBox_TextAlignment), &value));

    *pAlignment = static_cast<TextAlignment>(value.AsEnum());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control has no content, FALSE otherwise.
//
//---------------------------------------------------------------------------
bool CTextBox::IsEmpty()
{
    return !!(m_strText.IsNullOrEmpty());
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control cannot currently be modified by the user.
//
//---------------------------------------------------------------------------
bool CTextBox::IsReadOnly() const
{
    return m_bIsReadOnly;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Selects a run of text.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::Select(_In_ XINT32 start, _In_ XINT32 length)
{

    if (start < 0 || length < 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    wrl::ComPtr<ITextSelection2> pSelection;
    IFC_RETURN(GetSelection(&pSelection));

    IFC_RETURN(pSelection->SetRange(start, start + length));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces selected text with new text.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::SetSelectedText(_In_ const xstring_ptr& strString)
{
    HRESULT hr = S_OK;
    BSTR text = NULL;
    ITextSelection2 *pSelection = NULL;

    // SelectedText is a public property, so it is possible that it can be NULL.
    if (!strString.IsNull())
    {
        text = SysAllocStringLen(strString.GetBuffer(), strString.GetCount());
        IFCOOMFAILFAST(text);
    }

    IFC(GetSelection(&pSelection));
    IFC(pSelection->SetText(text));

Cleanup:
    SysFreeString(text);
    ReleaseInterface(pSelection);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Parameter validation for SetValue calls.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::ValidateSetValueArguments(
    _In_ const CDependencyProperty *pProperty,
    _In_ const CValue& value
    )
{
    switch (pProperty->GetIndex())
    {
        case KnownPropertyIndex::TextBox_SelectionStart:
        case KnownPropertyIndex::TextBox_SelectionLength:
        case KnownPropertyIndex::TextBox_MaxLength:
            if (!IsPositiveInteger(value))
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;
        case KnownPropertyIndex::TextBox_TextWrapping:
            // TextWrapping enum is shared between *Block and *Box. As specified,
            // WrapWholeWords is an invalid argument.
            if (IsWrapWholeWords(value))
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets rect for leading or trailing edge of a character
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::GetRectFromCharacterIndex(
    _In_ XINT32 charIndex,
    _In_ BOOLEAN trailingEdge,
    _Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<ITextRange> spTextRange;
    long left, top, bottom, flag = 0;

    if (charIndex < 0 || m_strText.IsNull() || (XUINT32)charIndex >= m_strText.GetCount())
    {
        IFC(E_INVALIDARG);
    }

    IFC(EnsureDocument());
    IFC(m_spDocument->Range(charIndex, charIndex, &spTextRange)); // Get the range from a single character
    // tomStart and tomEnd are the same for a single character range.
    // Use TA_LEFT and TA_RIGHT for leading and trailing edges.
    flag = trailingEdge ? TA_RIGHT : TA_LEFT;
    IFC(spTextRange->GetPoint(flag | tomStart | TA_TOP, &left, &top));    // Get the top-left point of the edge
    IFC(spTextRange->GetPoint(flag | tomStart | TA_BOTTOM, &left, &bottom)); // Get the bottom-left point of the edge

    if (left < XFLOAT_MAX
        && top < XFLOAT_MAX
        && (bottom-top) < XFLOAT_MAX)
    {
        XPOINT topLeft = {left, top};
        if (trailingEdge)
        {
            // Subtract character spacing from the trailing edge X cord.
            const TextFormatting* pTextFormatting = NULL;
            IFC(GetView()->GetTextFormatting(&pTextFormatting));
            XFLOAT characterSpacingInPx = (pTextFormatting->m_nCharacterSpacing / 1000.0f) * pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale());
            topLeft.x -= (XINT32)characterSpacingInPx;
        }

        if (XamlOneCoreTransforms::IsEnabled())
        {
            XPOINTF topLeftF = { static_cast<float>(topLeft.x), static_cast<float>(topLeft.y) };
            IFC(ClientToTextBox(&topLeftF)); //  Convert from client coordinates to local space
            topLeft = { static_cast<LONG>(topLeftF.x), static_cast<LONG>(topLeftF.y) };
        }
        else
        {
            IFC(ScreenToTextBox(&topLeft)); // Convert from screen coordinates to local space
        }

        returnValue->X = (XFLOAT)(topLeft.x);
        returnValue->Y = (XFLOAT)(topLeft.y);
        returnValue->Width = 0;
        returnValue->Height = (XFLOAT)(bottom - top);
    }
    else
    {
        memset(returnValue, 0, sizeof(XRECTF));
    }

Cleanup:
    ReleaseDocumentIfNotFocused();
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the visual state of the control.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::UpdateVisualState()
{
    IFC_RETURN(CTextBoxBase::UpdateVisualState());

    // ButtonStates VisualStateGroup.
    if (CanInvokeDeleteButton())
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonVisible", nullptr, nullptr, true /* useTransitions */));
    }
    else
    {
        IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonCollapsed", nullptr, nullptr, true /* useTransitions */));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether Delete Button is visible and can be invoked.
//
//------------------------------------------------------------------------
bool CTextBox::CanInvokeDeleteButton()
{
    return
        m_fHasSpaceForDeleteButton &&
        IsFocused() &&
        !IsEmpty() &&
        !IsReadOnly() &&
        (GetTextWrapping() == DirectUI::TextWrapping::NoWrap) &&
        !AcceptsReturn() &&
        !XboxUtility::IsOnXbox(); // Gamepad can't reach the delete button
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for CharacterReceived event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBox::OnCharacterReceived(_In_ CEventArgs* pEventArgs)
{
    CTextBoxBase::OnCharacterReceived(pEventArgs);

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::TextBox_ShowPlaceholderTextHandler(this, IsEmpty()));

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
_Check_return_ HRESULT CTextBox::SetTextServicesBuffer(
    _In_ const xstring_ptr& strText
        // If NULL, removes all content.
    )
{
    IFC_RETURN(CTextBoxBase::SetTextServicesBuffer(strText));


    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::TextBox_ShowPlaceholderTextHandler(this, strText.IsNull()));

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
CTextBox::GetInputScope(_Out_ ::InputScopeNameValue *pInputScope)
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

/* static */
_Check_return_ HRESULT CTextBox::CanPasteClipboardContent(
    _In_ CDependencyObject* pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue* ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    auto textBox = do_pointer_cast<CTextBox>(pObject);

    if (!textBox || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(do_pointer_cast<CTextBoxBase>(pObject)->CanPasteClipboardContent());
    return S_OK;
}

/* static */
_Check_return_ HRESULT CTextBox::CanUndo(
    _In_ CDependencyObject* pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue* ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    auto textBox = do_pointer_cast<CTextBox>(pObject);

    if (!textBox || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(do_pointer_cast<CTextBoxBase>(pObject)->CanUndo());
    return S_OK;
}

/* static */
_Check_return_ HRESULT CTextBox::CanRedo(
    _In_ CDependencyObject* pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue* ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    auto textBox = do_pointer_cast<CTextBox>(pObject);

    if (!textBox || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(do_pointer_cast<CTextBoxBase>(pObject)->CanRedo());
    return S_OK;
}