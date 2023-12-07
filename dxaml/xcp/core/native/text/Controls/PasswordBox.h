// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef PASSWORD_BOX_H
#define PASSWORD_BOX_H

#include "TextBoxBase.h"

//---------------------------------------------------------------------------
//
//  The DirectUI password text control.
//
//---------------------------------------------------------------------------

// Use this class to make sure string content is zeroed out upon destruction
class SecureZeroString
{
public:
    SecureZeroString(xstring_ptr& string) : m_string(string)
    {
    }

    ~SecureZeroString()
    {
        UINT32 count = m_string.GetCount();
        if (count > 0)
        {
            SecureZeroMemory((PVOID)(m_string.GetBuffer()), count*sizeof(WCHAR));
        }
    }

private:
    xstring_ptr& m_string;
};

class CPasswordBox final : public CTextBoxBase
{
public:
    // Public properties.
    xstring_ptr m_strPassword;          // Password plaintext.
    xstring_ptr m_strPasswordChar;      // Substitute character used in place of password plaintext during display.
    DirectUI::PasswordRevealMode m_passwordRevealMode;
    bool m_fAlwaysShowRevealButton;     // field isn't exposed as public property due to API lock.
                                        // We set it to true when the checkbox is default element for the reveal button templated part.
    bool m_fRevealButtonEnabled;

    _Check_return_ static HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ static HRESULT CanPasteClipboardContent(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult
    );

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    KnownTypeIndex GetTypeIndex() const final;
    _Check_return_ HRESULT OnApplyTemplate() final;
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    // TextBoxBase overrides.
    _Check_return_ HRESULT TxGetPasswordChar(_Out_ WCHAR *pChar) final;
    bool IsPassword() const final;
    bool IsPasswordRevealed() const final;

    // DXaml framework pinvokes.
    _Check_return_ HRESULT RevealPassword(_In_ bool revealed, _In_ bool syncToggleButtonCheckedState = true);

    bool IsEmpty() final;

    _Check_return_ HRESULT GetInputScope(_Out_ ::InputScopeNameValue *inputScope) final;

    _Check_return_ HRESULT UpdateFocusState(_In_ DirectUI::FocusState focusState) final;

    static _Check_return_ HRESULT Password(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

protected:

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    _Check_return_ HRESULT OnGotFocus(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;
    _Check_return_ HRESULT OnCharacterReceived(_In_ CEventArgs* pEventArgs) final;

    // TextBoxBase overrides.
    _Check_return_ HRESULT Initialize() final;
    _Check_return_ HRESULT OnContentChanged(_In_ const bool fTextChanged) final;
    _Check_return_ HRESULT UpdateVisualState() final;

    KnownEventIndex GetPastePropertyID() const final;

    KnownPropertyIndex GetSelectionHighlightColorPropertyID() const final;
    KnownPropertyIndex GetSelectionHighlightColorWhenNotFocusedPropertyID() const final;
    KnownPropertyIndex GetSelectionFlyoutPropertyID() const final { return KnownPropertyIndex::PasswordBox_SelectionFlyout; }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    KnownPropertyIndex GetHeaderPlacementPropertyID() const final { return KnownPropertyIndex::PasswordBox_HeaderPlacement; }
#endif

    _Check_return_ HRESULT SetTextServicesBuffer(_In_ const xstring_ptr& strText) final;
    _Check_return_ HRESULT OnInputScopeChanged(_In_ CInputScope *pInputScope);

private:
    bool m_fCanShowRevealButton        : 1;
    bool m_fHasSpaceForRevealButton    : 1;
    bool m_fPasswordRevealed           : 1;
    bool m_fNumericPinInputScope       : 1;

    xref_ptr<CDependencyObject> m_spRevealButton;

    static bool s_fRevealDisabledByRegKey;
    static bool s_fRevealDisabledByRegKeyInitialized;
    void EnsureDisablePasswordRevealRegKeyChecked();

    CPasswordBox(_In_ CCoreServices *pCore);
    ~CPasswordBox() override;

    static _Check_return_ HRESULT ValidateSetValueArguments(
        _In_ const CDependencyProperty *pProperty,
        _In_ const CValue& value
        );

    bool CanInvokeRevealButton();
    _Check_return_ HRESULT UpdateIhmSkinToBitLocker(bool skinBitLocker);

    _Check_return_ HRESULT CryptUnprotectPassword(_Out_ xstring_ptr& strPlainPassword) const;
    _Check_return_ HRESULT CryptProtectPassword(_In_ const xstring_ptr& strPlainPassword);

    std::unique_ptr<BYTE[]> m_pPasswordEncrypted = nullptr;   // Password encrypted.
    DWORD m_passwordLength = 0;
    DWORD m_passwordCryptFill = 0;


};

#endif // PASSWORD_BOX_H
