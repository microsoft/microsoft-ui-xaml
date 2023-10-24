// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PasswordBox.h"
#include "Indexes.g.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <TextInput.h>
#include <XBoxUtility.h>
#include <DesktopUtility.h>
#include "shlwapi.h"
#include <Wincrypt.h>

#include "TextCommon.h"

using namespace RuntimeFeatureBehavior;

bool CPasswordBox::s_fRevealDisabledByRegKey = true; // Secure default
bool CPasswordBox::s_fRevealDisabledByRegKeyInitialized = false;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
CPasswordBox::CPasswordBox(_In_ CCoreServices *pCore)
    : CTextBoxBase(pCore)
    , m_strPasswordChar()
    , m_fRevealButtonEnabled(false)
    , m_fCanShowRevealButton(FALSE)
    , m_fHasSpaceForRevealButton(FALSE)
    , m_fAlwaysShowRevealButton(false)
    , m_fPasswordRevealed(FALSE)
    , m_fNumericPinInputScope(FALSE)
{
    m_isSpellCheckEnabled = false;
    m_isTextPredictionEnabled = false;
    m_spellCheckIsDefault = false;
    m_textPredictionIsDefault = false;
    m_passwordRevealMode = DirectUI::PasswordRevealMode::Peek;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
CPasswordBox::~CPasswordBox()
{
    Destroy();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Creates and initializes a new CPasswordBox.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::Create(
       _Outptr_ CDependencyObject **ppObject,
       _In_ CREATEPARAMETERS *pCreate
    )
{
    xref_ptr<CPasswordBox> pCPasswordBox;
    pCPasswordBox.attach(new CPasswordBox(pCreate->m_pCore));


    IFC_RETURN(pCPasswordBox->Initialize());

    *ppObject = pCPasswordBox.detach();

    return S_OK;
}

KnownEventIndex CPasswordBox::GetPastePropertyID() const { return KnownEventIndex::PasswordBox_Paste; }
KnownPropertyIndex CPasswordBox::GetSelectionHighlightColorPropertyID() const { return KnownPropertyIndex::PasswordBox_SelectionHighlightColor; }
KnownPropertyIndex CPasswordBox::GetSelectionHighlightColorWhenNotFocusedPropertyID() const { return KnownPropertyIndex::UnknownType_UnknownProperty; }

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the index of CPasswordBox class.
//
//---------------------------------------------------------------------------
KnownTypeIndex CPasswordBox::GetTypeIndex() const
{
    return DependencyObjectTraits<CPasswordBox>::Index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a DependencyProperty value.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;

    xstring_ptr strOriginalPassword;
    SecureZeroString secureZeroOriginalPassword(strOriginalPassword);

    EnableSetValueReentrancyGuard();

    IFC(ValidateSetValueArguments(args.m_pDP, args.m_value));

    // Store the old Password value. We later compare it to the new value to determine whether we
    // should update the value.
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::PasswordBox_Password)
    {
        IFC(CryptUnprotectPassword(strOriginalPassword));
    }

    IFC(CTextBoxBase::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::PasswordBox_Password:
            {
                xstring_ptr strNewPassword;
                SecureZeroString secureZeroNewPassword(strNewPassword);
                IFC(CryptUnprotectPassword(strNewPassword));
                if (!strNewPassword.Equals(strOriginalPassword))
                {
                    IFC(SetTextServicesBuffer(strNewPassword));
                }
            }
            break;

        case KnownPropertyIndex::PasswordBox_PasswordChar:
            // TXTBIT_USEPASSWORD is treated specially by OnTxPropertyBitsChange. Setting the flag more than
            // once means "the password char changed", so we do that here.
            IFC(GetTextServices()->OnTxPropertyBitsChange(TXTBIT_USEPASSWORD, TXTBIT_USEPASSWORD));
            break;

        case KnownPropertyIndex::PasswordBox_MaxLength:
            IFC(OnMaxLengthChanged());
            break;

        case KnownPropertyIndex::PasswordBox_IsPasswordRevealButtonEnabled:
            // if IsPasswordRevealButtonEnabled (deprecated field as of Threashold), forward mapping its value to PasswordRevealMode
            // Addressing legacy compatibility when the application sets the IsPasswordRevealButtonEnabled property.
            if (m_fRevealButtonEnabled)
            {
                m_passwordRevealMode = DirectUI::PasswordRevealMode::Peek;
            }
            else
            {
                m_passwordRevealMode = DirectUI::PasswordRevealMode::Hidden;
            }

            IFC(UpdateVisualState());
            break;

        case KnownPropertyIndex::PasswordBox_PasswordRevealMode:
            if (m_passwordRevealMode == DirectUI::PasswordRevealMode::Peek)
            {
                m_fRevealButtonEnabled = true;
            }
            else  // Hidden or Visible
            {
                m_fRevealButtonEnabled = false;
            }

            IFC(UpdateVisualState());
            break;

        case KnownPropertyIndex::PasswordBox_SelectionHighlightColor:
            OnSelectionHighlightColorChanged();
            break;

        case KnownPropertyIndex::PasswordBox_TextReadingOrder:
            IFC(OnBidiOptionsChanged());
            break;

        case KnownPropertyIndex::PasswordBox_InputScope:
            IFC(OnInputScopeChanged(m_pInputScope));
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
_Check_return_ HRESULT CPasswordBox::OnApplyTemplate()
{

    IFC_RETURN(CTextBoxBase::OnApplyTemplate());

    m_fAlwaysShowRevealButton = false;

    m_spRevealButton = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"RevealButton"));

    // ensure the password text is hidden
    IFC_RETURN(RevealPassword(FALSE));

    IFC_RETURN(FxCallbacks::PasswordBox_OnApplyTemplateHandler(this));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether the reveal button should be visible or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize
    )
{
    CValue value;
    XFLOAT minimumWidthToEnableButton;

    IFC_RETURN(CTextBoxBase::ArrangeOverride(finalSize, newFinalSize));

    // Minimum width for PasswordBox with RevealButton visible is 5em.
    IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::Control_FontSize), &value));
    minimumWidthToEnableButton = value.AsFloat() * 5;
    if (finalSize.width > minimumWidthToEnableButton && !m_fHasSpaceForRevealButton)
    {
        m_fHasSpaceForRevealButton = TRUE;
        IFC_RETURN(UpdateVisualState());
    }
    else if (finalSize.width <= minimumWidthToEnableButton && m_fHasSpaceForRevealButton)
    {
        m_fHasSpaceForRevealButton = FALSE;
        IFC_RETURN(UpdateVisualState());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by the framework when a PasswordBox's reveal button IsPressed or Checked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::RevealPassword(
    // If TRUE, display the password plain text.  Otherwise, display
    // the PasswordChar in place of plain text.
    _In_ bool revealed,
    _In_ bool syncToggleButtonCheckedState
    )
{
    bool allowRevealOrHide = true;
    if (revealed)
    {
        // It is possible to get Reveal Button invoked through UIA even if it is collapsed.
        // Block this scenario by checking if the button can be shown in UI and invoked.
        // Always reveal when reveal mode is visible
        // Also block reveal if current mode is hidden.
        if ((m_passwordRevealMode == DirectUI::PasswordRevealMode::Hidden) || (!CanInvokeRevealButton() && (m_passwordRevealMode != DirectUI::PasswordRevealMode::Visible)))
        {
            allowRevealOrHide = false;
        }
    }
    else
    {
        // In case if we want to hide password we should always allow it, except if current reveal mode is Visible
        if (m_passwordRevealMode == DirectUI::PasswordRevealMode::Visible)
        {
            allowRevealOrHide = false;
        }
    }

    if (allowRevealOrHide)
    {
        m_fPasswordRevealed = revealed;

        const CTextBoxView_ScrollData* pScrollData = GetView()->GetScrollData();

        IFC_RETURN(OnPasswordRevealedChanged(revealed));
        if (syncToggleButtonCheckedState)
        {
            if (m_spRevealButton && m_spRevealButton->OfTypeByIndex<KnownTypeIndex::ToggleButton>())
            {
                CValue cVal;
                cVal.SetBool(!!revealed);
                IFC_RETURN(m_spRevealButton->SetValueByIndex(KnownPropertyIndex::ToggleButton_IsChecked, cVal));
            }
        }

        // Extent may change when the reveal mode is toggled. Position the viewport at the
        // end of the extent so that the end of the password is visible.
        IFC_RETURN(GetView()->SetScrollOffsets(pScrollData->ExtentWidth, pScrollData->ExtentHeight));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets the password substitution character.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::TxGetPasswordChar(_Out_ WCHAR *pChar)
{
    *pChar = m_strPasswordChar.GetChar(0);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::Initialize()
{
    // Initialize m_strPasswordChar before calling base because TextServices initialization
    // from base depends on it.
    XStringBuilder passwordCharBuilder;

    IFC_RETURN(passwordCharBuilder.Initialize(1));
    IFC_RETURN(passwordCharBuilder.AppendChar(0x25CF));   // BlackCircle
    IFC_RETURN(passwordCharBuilder.DetachString(&m_strPasswordChar));

    IFC_RETURN(CTextBoxBase::Initialize());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Called after RichEdit modifies the backing store.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::OnContentChanged(_In_ const bool fTextChanged)
{
    xstring_ptr strPassword;
    xstring_ptr strCurrentPassword;
    SecureZeroString secureZeroPassword(strPassword);
    SecureZeroString secureZeroCurrentPassword(strCurrentPassword);

    IFC_RETURN(CryptUnprotectPassword(strCurrentPassword));
    const CDependencyProperty *pPasswordProperty = GetPropertyByIndexInline(KnownPropertyIndex::PasswordBox_Password);
    CValue newValue;
    bool updateVisualState = false;

    InvalidateView();

    // Update the status of the password reveal button.
    // Password reveal button can be only shown if new password has been entered. It means that
    // only allow password reveal button if transitioning from empty to non-empty state.
    if (!m_fCanShowRevealButton)
    {
        if (IsEmpty())
        {
            // Update the state of the password reveal button.
            m_fCanShowRevealButton = TRUE;
            updateVisualState = TRUE;
        }
    }

    // Update public properties.
    // If the property system didn't change the content, we need to make the new value
    // as a local value now.
    IFC_RETURN(GetTextServicesBuffer(&(strPassword)));

    // if the content has changed while password is revealed in peek mode (through UIA toggle), we should hide it
    if (m_passwordRevealMode == DirectUI::PasswordRevealMode::Peek)
    {
        if (!strPassword.Equals(strCurrentPassword) && IsPasswordRevealed())
        {
            CValue result;
            IFC_RETURN(m_spRevealButton->GetValueByIndex(KnownPropertyIndex::ButtonBase_IsPressed, &result));
            bool isPressed = result.AsBool();
            if (!isPressed) // if user is currently pressing the reveal button, we should not hide the password
            {
                IFC_RETURN(RevealPassword(FALSE));
            }
        }
    }

    IFC_RETURN(CoercePropertyValue(pPasswordProperty, strCurrentPassword, strPassword));
    IFC_RETURN(CryptProtectPassword(strCurrentPassword));

    // Raise PasswordChanging and PasswordChanged event.
    if (!ParserOwnsParent())
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            IFC_RETURN(FxCallbacks::PasswordBox_OnPasswordChangingHandler(this, fTextChanged));

            xref_ptr<CRoutedEventArgs> spArgs;
            spArgs.init(new CRoutedEventArgs());

            pEventManager->RaiseRoutedEvent(
                EventHandle(KnownEventIndex::PasswordBox_PasswordChanged), this, spArgs);
        }
    }

    // Notify about property value change.
    newValue.SetString(strCurrentPassword);
    IFC_RETURN(NotifyPropertyChanged(
        PropertyChangedParams(
            pPasswordProperty,
            CValue(),
            newValue)));

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::PasswordBox_ShowPlaceholderTextHandler(this, IsEmpty()));

    // Update the visual state of the control.
    if (IsEmpty())
    {
        m_fCanShowRevealButton = FALSE;
        updateVisualState = TRUE;
    }
    if (updateVisualState)
    {
        IFC_RETURN(UpdateVisualState());
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
bool CPasswordBox::IsPassword() const
{
    return true;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control holds secret content and shows the content
//      in reveal mode
//
//---------------------------------------------------------------------------
bool CPasswordBox::IsPasswordRevealed() const
{
    return m_fPasswordRevealed;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if this control has no content, FALSE otherwise.
//
//---------------------------------------------------------------------------
bool CPasswordBox::IsEmpty()
{
    return (m_passwordLength == 0);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Parameter validation for SetValue calls.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::ValidateSetValueArguments(
    _In_ const CDependencyProperty *pProperty,
    _In_ const CValue& value
    )
{
    switch (pProperty->GetIndex())
    {
        case KnownPropertyIndex::PasswordBox_MaxLength:
            if (!IsPositiveInteger(value))
            {
                IFC_RETURN(E_INVALIDARG);
            }
            break;

        case KnownPropertyIndex::PasswordBox_PasswordChar:
            xstring_ptr strPasswordChar;
            if (value.GetType() == valueString)
            {
                strPasswordChar = value.AsString();
            }
            else if (value.GetType() == valueObject)
            {
                CString* pPasswordCharObj = do_pointer_cast<CString>(value.AsObject());
                if (pPasswordCharObj)
                {
                    strPasswordChar = pPasswordCharObj->m_strString;
                }
            }

            if (   strPasswordChar.GetCount() != 1
                || strPasswordChar.GetChar(0) == '\0')
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
//      Updates the visual state of the control.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::UpdateVisualState()
{
    IFC_RETURN(CTextBoxBase::UpdateVisualState());

    // ButtonStates VisualStateGroup.
    switch (m_passwordRevealMode)
    {
        case DirectUI::PasswordRevealMode::Peek:
            if (CanInvokeRevealButton())
            {
                IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonVisible", nullptr, nullptr, true /* useTransitions */));
            }
            else
            {
                // Ensure the password text is hidden.
                IFC_RETURN(RevealPassword(FALSE));
                IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonCollapsed", nullptr, nullptr, true /* useTransitions */));
            }
            break;

        case DirectUI::PasswordRevealMode::Hidden:
            IFC_RETURN(RevealPassword(FALSE));
            IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonCollapsed", nullptr, nullptr, true /* useTransitions */));
            break;

        case DirectUI::PasswordRevealMode::Visible:
            IFC_RETURN(RevealPassword(TRUE));
            IFC_RETURN(CVisualStateManager::GoToState(this, L"ButtonCollapsed", nullptr, nullptr, true /* useTransitions */));
            break;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for GotFocus event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::OnGotFocus(_In_ CEventArgs* pEventArgs)
{
    // Password reveal button should not appear if focus got moved to the PasswordBox
    // control.
    m_fCanShowRevealButton = FALSE;

    IFC_RETURN(CTextBoxBase::OnGotFocus(pEventArgs));
    return S_OK;
}

void CPasswordBox::EnsureDisablePasswordRevealRegKeyChecked()
{
    if (!s_fRevealDisabledByRegKeyInitialized)
    {
        wil::unique_hkey key;

        // If we cannot read the registry, we'll leave the password reveal eye enabled.
        // Do not attempt to read again since we want consistent behavior across all PasswordBox elements
        // in a single application.
        s_fRevealDisabledByRegKey = false;

        // First try to read HKLM
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CredUI", 0, KEY_READ, &key) == ERROR_SUCCESS)
        {
            s_fRevealDisabledByRegKey = !!SHRegGetIntW(key.get(), L"DisablePasswordReveal", 0 /* if there's no such key, we leave the reveal button enabled */);
        }
        else
        {
            // If we couldn't read HKLM, fallback to HKCU
            if (RegOpenKeyEx(HKEY_CURRENT_USER, L"SOFTWARE\\Policies\\Microsoft\\Windows\\CredUI", 0, KEY_READ, &key) == ERROR_SUCCESS)
            {
                s_fRevealDisabledByRegKey = !!SHRegGetIntW(key.get(), L"DisablePasswordReveal", 0 /* if there's no such key, we leave the reveal button enabled */);
            }
        }

        s_fRevealDisabledByRegKeyInitialized = true;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether RevealButton is visible and can be invoked.
//
//------------------------------------------------------------------------
bool CPasswordBox::CanInvokeRevealButton()
{
    EnsureDisablePasswordRevealRegKeyChecked();

    return
        !s_fRevealDisabledByRegKey &&
        !XboxUtility::IsOnXbox() && // Gamepad can't reach the reveal button
        ((m_fCanShowRevealButton && m_fHasSpaceForRevealButton && IsFocused()) || m_fAlwaysShowRevealButton);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for CharacterReceived event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::OnCharacterReceived(_In_ CEventArgs* pEventArgs)
{
    CTextBoxBase::OnCharacterReceived(pEventArgs);

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::PasswordBox_ShowPlaceholderTextHandler(this, IsEmpty()));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the RichEdit buffer.
//
//  Notes:
//      This method resets the RichEdit state, clearing undo and the selection
//      state. If strText is null or empty, it removes all content.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::SetTextServicesBuffer(_In_ const xstring_ptr& strText)
{
    CTextBoxBase::SetTextServicesBuffer(strText);

    // TextBox in the XAML layer needs to be notified when pText is changed so it can
    // update Placeholder Text visiblity.
    IFC_RETURN(FxCallbacks::PasswordBox_ShowPlaceholderTextHandler(this, strText.IsNullOrEmpty()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the InputScopeNameValue associated with this passwordbox control. If no
//      InputScopeNameValue has been set, returns InputScopeNameValuePassword.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPasswordBox::GetInputScope(_Out_ ::InputScopeNameValue *pInputScope)
{
    ::InputScopeNameValue result = InputScopeNameValueDefault;

    IFCPTRRC_RETURN(pInputScope, E_INVALIDARG);
    *pInputScope = result;

    if (nullptr == m_pInputScope)
    {
        result = InputScopeNameValuePassword;
    }
    else
    {
        xref_ptr<CInputScopeName> spInputScopeName;
        spInputScopeName.attach(static_cast<CInputScopeName*>(m_pInputScope->m_pNames->GetItemWithAddRef(0)));
        result = static_cast<::InputScopeNameValue>(spInputScopeName->m_nameValue);
    }
    *pInputScope = result;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     Override CtextBoxBase::OnInputScopeChanged for desktop passwordbox control
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CPasswordBox::OnInputScopeChanged(_In_ CInputScope *pInputScope)
{
    if (pInputScope != NULL && pInputScope->m_pNames->GetCount() > 0)
    {
        xref_ptr<CInputScopeName> spInputScopeName;
        spInputScopeName.attach(static_cast<CInputScopeName *>(pInputScope->m_pNames->GetItemWithAddRef(0)));
        InputScopeNameValue nameValue = static_cast<::InputScopeNameValue>(spInputScopeName->m_nameValue);

        // only allow password or numeric pin input scope
        ASSERT((nameValue == InputScopeNameValuePassword) || (nameValue == InputScopeNameValueNumericPin));

        UINT inputScopeCount = 1;
        InputScope inputScopeList[2];
        if (nameValue == InputScopeNameValueNumericPin)
        {
            m_fNumericPinInputScope = TRUE;
            // a bit hacky, but this is to workaround issue in RichEdit CTextMsgFilter::OnGetInputScopes.
            // we have to append IS_PASSWORD in the list so RichEdit will not insert IS_PASSWORD as the first element, which happens to be what TSF3 is going to use
            inputScopeList[0] = IS_NUMERIC_PIN;
            inputScopeList[1] = IS_PASSWORD;
            inputScopeCount = 2;
        }
        else
        {
            m_fNumericPinInputScope = FALSE;
            inputScopeList[0] = IS_PASSWORD;
        }
        TraceSendInputScopeToRichEditInfo(static_cast<UINT32>(inputScopeList[0])); // in reality, only first input scope in the list is used
        IFC_RETURN(GetTextServices()->TxSendMessage(EM_SETCTFINPUTSCOPES, inputScopeCount, reinterpret_cast<LPARAM>(inputScopeList), nullptr));
        IFC_RETURN(UpdateIhmSkinToBitLocker(!!m_fNumericPinInputScope));
    }

    return S_OK;
}

_Check_return_ HRESULT CPasswordBox::UpdateIhmSkinToBitLocker(bool skinBitLocker)
{
    if (DesktopUtility::IsOnDesktop())
    {
        HWND hwnd = GetElementInputWindow();
        if (skinBitLocker)
        {
            IFC_RETURN(::TextInput_SetKeyboardSkin(::TextInput_KeyboardSkins::Bitlocker, hwnd));
            TraceSetIhmKeyboardSkinInfo(static_cast<UINT32>(::TextInput_KeyboardSkins::Bitlocker));
        }
        else
        {
            IFC_RETURN(::TextInput_SetKeyboardSkin(::TextInput_KeyboardSkins::Default, hwnd));
            TraceSetIhmKeyboardSkinInfo(static_cast<UINT32>(::TextInput_KeyboardSkins::Default));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CPasswordBox::UpdateFocusState(_In_ DirectUI::FocusState focusState)
{
    if (focusState == DirectUI::FocusState::Unfocused)
    {
        // reset BitLocker Ihm skin to default when leaving focus
        IFC_RETURN(UpdateIhmSkinToBitLocker(false /*skinBitLocker*/));
    }
    else
    {
        IFC_RETURN(UpdateIhmSkinToBitLocker(!!m_fNumericPinInputScope /*skinBitLocker*/));
    }

    IFC_RETURN(CTextBoxBase::UpdateFocusState(focusState));

    return S_OK;
}

_Check_return_
HRESULT
CPasswordBox::Password(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    CPasswordBox *pThis = do_pointer_cast<CPasswordBox>(pObject);

    IFC_RETURN(pThis && cArgs <= 1 ? S_OK : E_INVALIDARG);

    pResult->Unset();

    if (cArgs == 0) //getter
    {
        xstring_ptr strPassword; // can't use SecureZeroString otherwise the caller will get null string
        IFC_RETURN(pThis->CryptUnprotectPassword(strPassword));

        if (!strPassword.IsNull())
        {
            pResult->SetString(strPassword);
        }
        else
        {
            pResult->SetString(xstring_ptr::EmptyString());
        }
    }
    else  //setter
    {
        xstring_ptr strNewPassword;
        IFC_RETURN(pArgs->GetString(strNewPassword));
        IFC_RETURN(pThis->CryptProtectPassword(strNewPassword));
    }

    return S_OK;
}

_Check_return_ HRESULT CPasswordBox::CryptUnprotectPassword(_Out_ xstring_ptr& strPlainPassword) const
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    strPlainPassword = xstring_ptr::EmptyString();
    if (m_passwordLength == 0)
    {
        return S_OK;
    }
    DWORD dwPaswordLengthWithFill = m_passwordLength + m_passwordCryptFill;
    WCHAR *strPasswordTmp = new WCHAR[dwPaswordLengthWithFill];
    memcpy_s(strPasswordTmp, dwPaswordLengthWithFill * sizeof(WCHAR), m_pPasswordEncrypted.get(), dwPaswordLengthWithFill * sizeof(WCHAR));
    IFCW32(CryptUnprotectMemory(strPasswordTmp, dwPaswordLengthWithFill * sizeof(WCHAR), CRYPTPROTECTMEMORY_SAME_PROCESS));
    IFC(xstring_ptr::CloneBuffer(strPasswordTmp, m_passwordLength, &strPlainPassword));

Cleanup:
    SecureZeroMemory(strPasswordTmp, dwPaswordLengthWithFill * sizeof(WCHAR));
    SAFE_DELETE_ARRAY(strPasswordTmp);
    return S_OK;
}

_Check_return_ HRESULT CPasswordBox::CryptProtectPassword(_In_ const xstring_ptr& strPlainPassword)
{
    //Cleanup Current encrpted password field and length varaibles
    m_passwordLength = 0;
    m_passwordCryptFill = 0;

    DWORD dwPasswordLength = strPlainPassword.GetCount();
    if (dwPasswordLength == 0) // nothing to encrypt
    {
        return S_OK;
    }

    // Memory to be encrypted must be a multiple of CRYPTPROTECTMEMORY_BLOCK_SIZE
    DWORD dwFill = dwPasswordLength % (CRYPTPROTECTMEMORY_BLOCK_SIZE / sizeof(WCHAR));
    if (dwFill)
    {
        dwFill = CRYPTPROTECTMEMORY_BLOCK_SIZE / sizeof(WCHAR) - dwFill;
    }

    DWORD dwPasswordLengthWithFill = dwPasswordLength + dwFill;
    m_pPasswordEncrypted.reset(new BYTE[(dwPasswordLengthWithFill + 1) * sizeof(WCHAR)]); // +1 for null terminator in case fill is zero
    memcpy_s(m_pPasswordEncrypted.get(), (dwPasswordLengthWithFill + 1) * sizeof(WCHAR), strPlainPassword.GetBuffer(), (dwPasswordLength + 1) * sizeof(WCHAR));

    IFCW32_RETURN(CryptProtectMemory(m_pPasswordEncrypted.get(), dwPasswordLengthWithFill * sizeof(WCHAR), CRYPTPROTECTMEMORY_SAME_PROCESS));
    m_passwordLength = dwPasswordLength;
    m_passwordCryptFill = dwFill;

    return S_OK;
}

/* static */
_Check_return_ HRESULT CPasswordBox::CanPasteClipboardContent(
    _In_ CDependencyObject* pObject,
    _In_ uint32_t cArgs,
    _Inout_updates_(cArgs) CValue* ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    auto passwordBox = do_pointer_cast<CPasswordBox>(pObject);

    if (!passwordBox || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->SetBool(do_pointer_cast<CTextBoxBase>(pObject)->CanPasteClipboardContent());
    return S_OK;
}