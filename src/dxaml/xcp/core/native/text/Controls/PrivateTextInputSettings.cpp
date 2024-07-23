// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace Microsoft::WRL;

TextInput_InputSettings operator&(const TextInput_InputSettings& firstValue, const TextInput_InputSettings& secondValue)
{
    DWORD finalValue = static_cast<DWORD>(firstValue) & static_cast<DWORD>(secondValue);
    return static_cast<TextInput_InputSettings>(finalValue);
}

TextInput_InputSettings& operator&=(TextInput_InputSettings& firstValue, const TextInput_InputSettings& secondValue)
{
    DWORD finalValue = static_cast<DWORD>(firstValue) & static_cast<DWORD>(secondValue);
    return firstValue = static_cast<TextInput_InputSettings>(finalValue);
}

TextInput_InputSettings operator|(const TextInput_InputSettings& firstValue, const TextInput_InputSettings& secondValue)
{
    DWORD finalValue = static_cast<DWORD>(firstValue) | static_cast<DWORD>(secondValue);
    return static_cast<TextInput_InputSettings>(finalValue);
}

TextInput_InputSettings& operator|=(TextInput_InputSettings& firstValue, const TextInput_InputSettings& secondValue)
{
    DWORD finalValue = static_cast<DWORD>(firstValue) | static_cast<DWORD>(secondValue);
    return firstValue = static_cast<TextInput_InputSettings>(finalValue);
}

TextInput_InputSettings operator~(const TextInput_InputSettings& value)
{
    DWORD finalValue = ~static_cast<DWORD>(value);
    return static_cast<TextInput_InputSettings>(finalValue);
}

CTextInputPrivateSettings::CTextInputPrivateSettings() :
    m_pTextBox(nullptr)
{
}

CTextInputPrivateSettings::~CTextInputPrivateSettings()
{
    m_pTextBox = nullptr;
}


void CTextInputPrivateSettings::Create(
    CTextBoxBase *pTextBox,
    CTextInputPrivateSettings **ppPrivateTextInputSettings)
{
    Microsoft::WRL::ComPtr<CTextInputPrivateSettings> spPrivateTextInputSettings;

    *ppPrivateTextInputSettings = nullptr;
    spPrivateTextInputSettings = Microsoft::WRL::Make<CTextInputPrivateSettings>();
    spPrivateTextInputSettings->m_pTextBox = pTextBox;

    // caller gets ownership of the new object - we'll release the reference when owner calls Teardown
    *ppPrivateTextInputSettings = spPrivateTextInputSettings.Detach();
}

_Check_return_ HRESULT CTextInputPrivateSettings::Initialize(ITextServices2 *pTextServices)
{
    bool bIsEnabled = false;
    bool bIsDefault = false;

    m_spTextServices = pTextServices;
    m_inputSettings = TextInput_InputSettings::None;

    m_inputScope = IS_DEFAULT;
    if (m_pTextBox->IsPassword())
    {
        m_inputScope = IS_PASSWORD;
        m_inputSettings |= TextInput_InputSettings::PasswordObfuscated;
    }
    else
    {
        InputScopeNameValue isNameValue;
        IFC_RETURN(m_pTextBox->GetInputScope(&isNameValue));
        m_inputScope = static_cast<UINT32>(isNameValue);
    }

    IFC_RETURN(m_pTextBox->GetSpellCheckEnabled(&bIsEnabled, &bIsDefault));

    if (IsPureNumberIS(m_inputScope))
    {
        // All typing intelligence is always disabled for the pure number InputScopes
        bIsEnabled = FALSE;
    }
    else if (bIsDefault && IsIntelligenceOffByDefaultForIS(m_inputScope))
    {
        // For input scope  such as URL, spell checking is turned off by default, but can be overridden by setting property to true.
        bIsEnabled = FALSE;
    }

    UpdateSpellCheckingSettingBits(bIsEnabled);

    IFC_RETURN(m_pTextBox->GetTextPredictionEnabled(&bIsEnabled, &bIsDefault));

    if (IsPureNumberIS(m_inputScope))
    {
        // All typing intelligence is always disabled for the pure number InputScopes
        bIsEnabled = FALSE;
    }
    else if (bIsDefault && IsIntelligenceOffByDefaultForIS(m_inputScope))
    {
        // For input scope  such as URL, text prediction is turned off by default, but can be overridden by setting property to true.
        bIsEnabled = FALSE;
    }

    UpdateTextPredictionSettingBits(bIsEnabled);

    if ((m_inputScope == IS_PERSONALNAME_FULLNAME) && bIsDefault)
    {
        // For PFN input scope, we need to support special default behavior: enable swipe typing and disable text prediction on keyboard typing
        m_inputSettings &= ~TextInput_InputSettings::DisableAutoSuggest;
        m_inputSettings &= ~TextInput_InputSettings::DisableHaveTrailer;
        m_inputSettings |= TextInput_InputSettings::EnableHaveTrailer;
    }

    IFC_RETURN(UpdateEditSettings());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by TextBoxBase when the associated edit control's lifetime is ending.
//
//------------------------------------------------------------------------
void CTextInputPrivateSettings::Teardown()
{
    m_spTextServices = nullptr;
    // Release the initial reference that was "held" by the TextBoxBase
    // that is calling Teardown().
    Release();
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Lets the Input Service know that the OnSpellCheckEnabledChanged setting has
//      changed and that it should change behavior accordingly.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextInputPrivateSettings::OnSpellCheckEnabledChanged(
    BOOL bIsSpellCheckEnabled)
{
    if (IsPureNumberIS(m_inputScope))
    {
        return S_OK;
    }

    UpdateSpellCheckingSettingBits(bIsSpellCheckEnabled);
    IFC_RETURN(UpdateEditSettings());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Lets the Input Service know that the TextPredictionEnabled setting has
//     changed and that it should change behavior accordingly.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextInputPrivateSettings::OnTextPredictionEnabledChanged(
BOOL bIsTextPredictionEnabled)
{
    if (IsPureNumberIS(m_inputScope))
    {
        return S_OK;
    }

    UpdateTextPredictionSettingBits(bIsTextPredictionEnabled);
    IFC_RETURN(UpdateEditSettings());

    return S_OK;
}

_Check_return_ HRESULT CTextInputPrivateSettings::OnCandidateWindowAlignmentChanged(
    _In_ BOOL bIsCandidateWindowAlignAtBottom)
{

    if (bIsCandidateWindowAlignAtBottom)
    {
        m_inputSettings |= TextInput_InputSettings::BottomEdgeCandidateWindowAlignment;
    }
    else
    {
        m_inputSettings &= ~TextInput_InputSettings::BottomEdgeCandidateWindowAlignment;
    }

    IFC_RETURN(UpdateEditSettings());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Static method that tells if the specified InputScope uses the Number layout
//     and should, consequently, have typing intelligence disabled.
//
//------------------------------------------------------------------------
BOOL CTextInputPrivateSettings::IsPureNumberIS(
    _In_ UINT32 uiInputScope)
{
    BOOL bResult = FALSE;
    if (IS_NUMBER == uiInputScope ||
        IS_NUMBER_FULLWIDTH == uiInputScope ||
        IS_TELEPHONE_FULLTELEPHONENUMBER == uiInputScope)
    {
        bResult = TRUE;
    }
    return bResult;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//     Static method that tells if typing intelligence should be disabled by
//     default for the specified InputScope.
//
//------------------------------------------------------------------------
BOOL CTextInputPrivateSettings::IsIntelligenceOffByDefaultForIS(
    _In_ UINT32 uiInputScope)
{
    BOOL bResult = FALSE;
    if (IS_URL == uiInputScope ||
        IS_NAME_OR_PHONENUMBER == uiInputScope ||
        IS_EMAIL_SMTPEMAILADDRESS == uiInputScope ||
        IS_PERSONALNAME_FULLNAME == uiInputScope)
    {
        bResult = TRUE;
    }
    return bResult;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//     Static method that tells if auto-cap should be disabled
//     for the specified InputScope.
//
//------------------------------------------------------------------------
BOOL CTextInputPrivateSettings::NoAutoCapForIS(
    _In_ UINT32 uiInputScope)
{
    BOOL bResult = FALSE;
    if (IS_SEARCH == uiInputScope ||
        IS_CURRENCY_AMOUNTANDSYMBOL == uiInputScope ||
        IS_FORMULA == uiInputScope)
    {
        bResult = TRUE;
    }
    return bResult;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Static method that tells if auto-cap should be enabled
//     for the specified InputScope, even if the spell checking is turned off.
//
//------------------------------------------------------------------------
BOOL CTextInputPrivateSettings::AlwaysAutoCapForIS(
    _In_ UINT32 uiInputScope)
{
    BOOL bResult = FALSE;
    if (IS_PERSONALNAME_FULLNAME == uiInputScope)
    {
        bResult = TRUE;
    }
    return bResult;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static method that tells if auto-space (trailer) should be disabled
//      for the specified InputScope.
//
//------------------------------------------------------------------------

BOOL CTextInputPrivateSettings::NoAutoSpaceForIS(
    _In_ UINT32 uiInputScope)
{
    BOOL bResult = FALSE;
    if (IS_FORMULA == uiInputScope)
    {
        bResult = TRUE;
    }
    return bResult;
}

void CTextInputPrivateSettings::UpdateSpellCheckingSettingBits(BOOL bIsSpellCheckEnabled)
{
    if (bIsSpellCheckEnabled)
    {
        m_inputSettings |=
            TextInput_InputSettings::EnableSpellCheck |
            TextInput_InputSettings::EnableAutoCorrect |
            TextInput_InputSettings::EnableAutoCapitalization |
            TextInput_InputSettings::EnableCandidatesOnDemand;

        // exceptions to enable everything
        if (NoAutoCapForIS(m_inputScope))
        {
            // for search, currency (amount and symbol) and formula, auto-cap is disabled
            m_inputSettings &= ~TextInput_InputSettings::EnableAutoCapitalization;
            m_inputSettings |= TextInput_InputSettings::DisableAutoCapitalization;
        }
    }
    else
    {
        m_inputSettings &= ~(TextInput_InputSettings::EnableSpellCheck |
            TextInput_InputSettings::EnableAutoCorrect |
            TextInput_InputSettings::EnableAutoCapitalization |
            TextInput_InputSettings::EnableCandidatesOnDemand);

        m_inputSettings |=
            TextInput_InputSettings::DisableSpellCheck |
            TextInput_InputSettings::DisableAutoCorrect |
            TextInput_InputSettings::DisableAutoCapitalization |
            TextInput_InputSettings::DisableCandidatesOnDemand;

        if (AlwaysAutoCapForIS(m_inputScope))
        {
            m_inputSettings &= ~TextInput_InputSettings::DisableAutoCapitalization;
            m_inputSettings |= TextInput_InputSettings::EnableAutoCapitalization;
        }
    }
}

_Check_return_ HRESULT CTextInputPrivateSettings::UpdateSIPSettings(_In_ bool disable)
{
    if (disable)
    {
        m_inputSettings |= TextInput_InputSettings::EnableManualInputPane;
    }
    else
    {
        m_inputSettings &= ~TextInput_InputSettings::EnableManualInputPane;
        m_inputSettings |= TextInput_InputSettings::DisableManualInputPane;
    }

    IFC_RETURN(UpdateEditSettings());

    return S_OK;
}

_Check_return_ HRESULT CTextInputPrivateSettings::NotifyFocusEnter()
{
    ComPtr<wut::Core::ICoreTextEditContext> spTSF3EditContext;

    // Get Private TSF3 EditContext object
    LRESULT result = 0;
    IFC_RETURN(m_spTextServices->TxSendMessage(EM_GETTSF, 0, reinterpret_cast<LPARAM>(spTSF3EditContext.GetAddressOf()), &result));

    // We can be in a scenario where _pTextInput was unsuccessfully initialized. One way this can happen is when
    // attempting to create _pTextInput on a different thread. To protect ourselves from crashing,
    // we should make sure that spTSF3EditContext is not null.
    if (spTSF3EditContext == nullptr) { return S_OK; }

    IFC_RETURN(spTSF3EditContext->NotifyFocusEnter());

    return S_OK;
}

_Check_return_ HRESULT CTextInputPrivateSettings::NotifyLayoutChanged()
{
    ComPtr<wut::Core::ICoreTextEditContext> spTSF3EditContext;

    // Get Private TSF3 EditContext object
    LRESULT result = 0;
    IFC_RETURN(m_spTextServices->TxSendMessage(EM_GETTSF, 0, reinterpret_cast<LPARAM>(spTSF3EditContext.GetAddressOf()), &result));

    // We can be in a scenario where _pTextInput was unsuccessfully initialized. One way this can happen is when
    // attempting to create _pTextInput on a different thread. To protect ourselves from crashing,
    // we should make sure that spTSF3EditContext is not null.
    if (spTSF3EditContext == nullptr) { return S_OK; }

    IFC_RETURN(spTSF3EditContext->NotifyLayoutChanged());

    return S_OK;
}

_Check_return_ HRESULT CTextInputPrivateSettings::SetIsTelemetryCollectionEnabled(bool isEnabled)
{
    // When TextInput_InputSettings::EnablePrivateInputSetting is set, it means "don't collect telemetry"
    if (isEnabled)
    {
        m_inputSettings &= ~TextInput_InputSettings::EnablePrivateInputSetting;
        m_inputSettings |= TextInput_InputSettings::DisablePrivateInputSetting;
    }
    else
    {
        // TextInput_InputSettings::EnablePrivateInputSetting has two bits set which include the one that TextInput_InputSettings::DisablePrivateInputSetting has set
        // So we don't have to clear the disabled bit before setting the enabled bits
        m_inputSettings |= TextInput_InputSettings::EnablePrivateInputSetting;
    }

    IFC_RETURN(UpdateEditSettings());
    return S_OK;
}

void CTextInputPrivateSettings::UpdateTextPredictionSettingBits(BOOL bIsPredictionEnabled)
{
    if (bIsPredictionEnabled)
    {
        m_inputSettings |= TextInput_InputSettings::EnableAutoSuggest;
        m_inputSettings |= TextInput_InputSettings::EnableHaveTrailer;

        // exception to enable everything
        if (NoAutoSpaceForIS(m_inputScope))
        {
            m_inputSettings &= ~TextInput_InputSettings::EnableHaveTrailer;
            m_inputSettings |= TextInput_InputSettings::DisableHaveTrailer;
        }
    }
    else
    {
        m_inputSettings &= ~TextInput_InputSettings::EnableAutoSuggest;
        m_inputSettings |= TextInput_InputSettings::DisableAutoSuggest;
        m_inputSettings &= ~TextInput_InputSettings::EnableHaveTrailer;
        m_inputSettings |= TextInput_InputSettings::DisableHaveTrailer;
    }
}

_Check_return_ HRESULT CTextInputPrivateSettings::UpdateEditSettings()
{
    ComPtr<wut::Core::ICoreTextEditContext> spCoreTextEditContext;
    IFC_RETURN(GetCoreTextEditContext(&spCoreTextEditContext));

    if (spCoreTextEditContext != nullptr)
    {
        IFC_RETURN(TextInputProxy_putInputSettings(spCoreTextEditContext.Get(), m_inputSettings));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get TSF3 edit context object and QI the private context for updating spellchecking
//      textPrediction properties.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextInputPrivateSettings::GetCoreTextEditContext(_COM_Outptr_result_maybenull_ wut::Core::ICoreTextEditContext** privateEditContext)
{
    *privateEditContext = nullptr;

    // Get TSF3 EditContext object
    LRESULT result = 0;
    ComPtr<wut::Core::ICoreTextEditContext> spTSF3EditContext;
    IFC_RETURN(m_spTextServices->TxSendMessage(EM_GETTSF, 0, reinterpret_cast<LPARAM>(spTSF3EditContext.GetAddressOf()), &result));

    // We can be in a scenario where _pTextInput was unsuccessfully initialized. One way this can happen is when
    // attempting to create _pTextInput on a different thread. To protect ourselves from crashing,
    // we should make sure that spTSF3EditContext is not null.
    if (spTSF3EditContext == nullptr) { return S_OK; }

    *privateEditContext = spTSF3EditContext.Detach();

    return S_OK;
}
