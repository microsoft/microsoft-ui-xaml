// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Apply input settings through private text input setting API
//      Public TSF3 API only supports changing input scope, XAML needs to be able to toggle
//      text prediction and spell check enabled properties.
//      To get hold of private text input settings API, we need to
//      ASK RichEdit to give us the TSF3 edit context object through sending EM_GETTSF message
//      QI on the edit context object to get the private setting context

#pragma once

#include <textserv.h>
#include <TextInput.h>

class CTextInputPrivateSettings :
    public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::ClassicCom>,
        IUnknown>
{
public:
    CTextInputPrivateSettings();

private:
    ~CTextInputPrivateSettings(); // no plan to derive from this class

public:
    static void Create(
        _In_ CTextBoxBase *pTextBox,
        _Outptr_ CTextInputPrivateSettings **ppTextBoxProxy);

    _Check_return_ HRESULT Initialize(
        _In_ ITextServices2 *pTextServices);

    void Teardown();

    _Check_return_ HRESULT OnSpellCheckEnabledChanged(
        _In_ BOOL isSpellCheckEnabled);

    _Check_return_ HRESULT OnTextPredictionEnabledChanged(
        _In_ BOOL bIsTextPredictionEnabled);

    _Check_return_ HRESULT OnCandidateWindowAlignmentChanged(
        _In_ BOOL bIsCandidateWindowAlignAtBottom);

    _Check_return_ HRESULT UpdateSIPSettings(_In_ bool disable);
    _Check_return_ HRESULT NotifyFocusEnter();
    _Check_return_ HRESULT NotifyLayoutChanged();

    _Check_return_ HRESULT SetIsTelemetryCollectionEnabled(bool isEnabled);
    
protected:
    // helper methods
    static BOOL IsPureNumberIS(_In_ UINT32 uiInputScope);

    static BOOL IsIntelligenceOffByDefaultForIS(_In_ UINT32 uiInputScope);

    static BOOL NoAutoCapForIS(_In_ UINT32 uiInputScope);

    static BOOL NoAutoSpaceForIS(_In_ UINT32 uiInputScope);

    static BOOL AlwaysAutoCapForIS(_In_ UINT32 uiInputScope);

    _Check_return_ HRESULT UpdateEditSettings();

    _Check_return_ HRESULT GetCoreTextEditContext(_COM_Outptr_result_maybenull_ wut::Core::ICoreTextEditContext** privateEditContext);

private:

    void UpdateSpellCheckingSettingBits(BOOL bIsSpellCheckEnabled);
    void UpdateTextPredictionSettingBits(BOOL bIsPredictionEnabled);

    CTextBoxBase *m_pTextBox;
    UINT32 m_inputScope;
    TextInput_InputSettings m_inputSettings;
    Microsoft::WRL::ComPtr<ITextServices2> m_spTextServices;
  };
