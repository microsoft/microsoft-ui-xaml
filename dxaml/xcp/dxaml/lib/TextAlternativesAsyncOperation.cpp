// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextAlternativesAsyncOperation.h"
#include "TextBoxBase.h"

#include <windowscollections.h>
#include <TrackerCollections.h>

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

ULONG TextAlternativesOperation::z_ulUniqueAsyncActionId = 1;

TextAlternativesOperation::TextAlternativesOperation()
{ }

// Caches the composition, pre- and postfix strings, and language from the TextBox.
_Check_return_ HRESULT TextAlternativesOperation::Init(_In_ CTextBoxBase* pOwner)
{
    HRESULT hr = S_OK;

    // Get the composition string, or nullptr if we shouldn't generate alternatives
    IFC(pOwner->GetCompositionString(m_compositionString.GetAddressOf()));

    // Get the prefix string and the postfix string
    IFC(pOwner->GetCompositionPrefixAndPostfixStrings(m_prefixString.GetAddressOf(), m_postfixString.GetAddressOf()));

    // Get the input language
    IFC(GetInputLanguage(m_currentInputMethodLanguageTag.GetAddressOf()));

Cleanup:
    return hr;
}

_Check_return_ HRESULT TextAlternativesOperation::GetInputLanguage(_Outptr_ HSTRING* pLanguage)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<wg::ILanguageStatics> spLanguageStatics; // Used for getting current input method language tag
    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Language).Get(), &spLanguageStatics));
    IFC(spLanguageStatics->get_CurrentInputMethodLanguageTag(pLanguage));

Cleanup:
    return hr;
}

// Concatenates the string prefix, alternate, and postfix strings.
_Check_return_ HRESULT TextAlternativesOperation::InjectAlternateIntoFullText(_In_ HSTRING alternate, _Out_ HSTRING* concatString)
{
    wrl_wrappers::HString local;

    IFC_RETURN(WindowsConcatString(m_prefixString.Get(), alternate, local.GetAddressOf()));
    IFC_RETURN(WindowsConcatString(local.Get(), m_postfixString.Get(), concatString));

    return S_OK;
}

_Check_return_ HRESULT TextAlternativesOperation::GetLinguisticAlternativesAsyncImpl(
    _Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** returnValue
)
{
    HRESULT hr = S_OK;

    if (m_compositionString != nullptr)
    {
        wrl::ComPtr<wda::Text::ITextPredictionGeneratorFactory> predGenFactory;
        wrl::ComPtr<wda::Text::ITextPredictionGenerator> predGen;

        wrl::ComPtr<wda::Text::ITextConversionGeneratorFactory> convGenFactory;
        wrl::ComPtr<wda::Text::ITextConversionGenerator> convGen;

        wrl_wrappers::HString resolvedLanguage;
        boolean isLanguageAvailableButNotInstalled;

        // Create a PredictionGenerator. In TH, t works specifically for keyboard = "ja"
        IFC(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_Data_Text_TextPredictionGenerator).Get(), &predGenFactory));
        IFC(predGenFactory->Create(m_currentInputMethodLanguageTag.Get(), &predGen));

        predGen->get_ResolvedLanguage(resolvedLanguage.GetAddressOf());

        // Use conversion generator if
        // 1. Prediction generator does not support current input language.
        // 2. Always prefer conversion generator for Simplified Chinese input.
        bool useConverionGenerator = (wcscmp(resolvedLanguage.GetRawBuffer(nullptr), L"und") == 0);
        if (!useConverionGenerator)
        {
            LCID localeId = LocaleNameToLCID(resolvedLanguage.GetRawBuffer(nullptr), 0);
            LANGID langId = LANGIDFROMLCID(localeId);
            if (langId == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
            {
                useConverionGenerator = true;
            }
        }

        if (useConverionGenerator)
        {
            IFC(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_Data_Text_TextConversionGenerator).Get(), &convGenFactory));
            IFC(convGenFactory->Create(m_currentInputMethodLanguageTag.Get(), &convGen));
            convGen->get_ResolvedLanguage(resolvedLanguage.GetAddressOf());
            convGen->get_LanguageAvailableButNotInstalled(&isLanguageAvailableButNotInstalled);

            // If keyboard is not "ja" or "chs", this API has nothing useful to return.
            if (wcscmp(resolvedLanguage.GetRawBuffer(nullptr), L"und") == 0 || isLanguageAvailableButNotInstalled)
            {
                returnValue = nullptr;
            }
            else
            {
                // Keyboard is a language that doesn't support prediction, like "chs" in TH, so return the conversion results
                IFC(convGen->GetCandidatesAsync(m_compositionString.Get(), returnValue));
            }
        }
        else
        {
            predGen->get_LanguageAvailableButNotInstalled(&isLanguageAvailableButNotInstalled);

            if (isLanguageAvailableButNotInstalled)
            {
                returnValue = nullptr;
            }
            else
            {
                // Keyboard is a language that supports prediction, like "ja" in TH, return the prediction results.
                // Prediction results are generally a superset of conversion results, so don't attempt conversion if prediction fails to return anything.
                IFC(predGen->GetCandidatesAsync(m_compositionString.Get(), returnValue));
            }
        }
    }
    else
    {
        returnValue = nullptr;
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT TextAlternativesOperation::OnStart()
{
    ctl::ComPtr<wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>> spAsyncOperation;

    // CompletedHandler overrides.
    ctl::ComPtr<TextAlternativesAsyncCompleteHandler> spLoadCompletedHandler;
    ctl::ComPtr<wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*>> spOperationCompletedHandler;

    // Make the completed handler
    // TODO: might not be strictly necessary to have a TextAlternativesAsyncCompleteHandler object?
    IFC_RETURN(ctl::make<TextAlternativesAsyncCompleteHandler>(&spLoadCompletedHandler));
    IFC_RETURN(spLoadCompletedHandler.As<wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*>>(&spOperationCompletedHandler));
    spLoadCompletedHandler->SetLinguisticCallback(this);

    IFC_RETURN(GetLinguisticAlternativesAsyncImpl(spAsyncOperation.ReleaseAndGetAddressOf()));

    if (spAsyncOperation) // The API returned something
    {
        // Set the operation's completed event to the overridden event
        IFC_RETURN(spAsyncOperation->put_Completed(spOperationCompletedHandler.Get()));
    }
    else // The API didn't return anything
    {
        IFC_RETURN(SetEmptyResults());
    }

    return S_OK;
}

_Check_return_ HRESULT TextAlternativesOperation::SetEmptyResults()
{
    // Create an empty IVector.
    wrl::ComPtr<wfci_::Vector<HSTRING>> emptyVector;
    wfci_::Vector<HSTRING>::Make(&emptyVector);
    wrl::ComPtr<wfc::IVectorView<HSTRING>> emptyView;
    emptyVector->GetView(&emptyView);
    m_pAlternatives = emptyView.Detach();
    // Fire completion with this empty vector.
    IFC_RETURN (AsyncBase::FireCompletion());

    return S_OK;
}

void TextAlternativesOperation::OnCancel()
{

}

void TextAlternativesOperation::OnClose()
{

}

STDMETHODIMP TextAlternativesOperation::GetResults(
    _Inout_ wfc::IVectorView<HSTRING>* *results)
{
    *results = m_pAlternatives;

    RRETURN(S_OK);
}

void TextAlternativesOperation::SetResults(_In_ wfc::IVectorView<HSTRING>* results)
{
    wrl::ComPtr<wfci_::Vector<HSTRING>> contextVector;
    wfci_::Vector<HSTRING>::Make(&contextVector);

    unsigned size;
    results->get_Size(&size);

    for (unsigned i = 0; i < size; i++)
    {
        HSTRING alternate {};
        results->GetAt(i, &alternate);

        HSTRING concatString {};
        InjectAlternateIntoFullText(alternate, &concatString);

        contextVector->Append(concatString);
    }

    wrl::ComPtr<wfc::IVectorView<HSTRING>> contextView;
    contextVector->GetView(&contextView);
    m_pAlternatives = contextView.Detach();

    AsyncBase::FireCompletion();
}

TextAlternativesAsyncCompleteHandler::TextAlternativesAsyncCompleteHandler()
{

}

TextAlternativesAsyncCompleteHandler::~TextAlternativesAsyncCompleteHandler()
{

}

_Check_return_ HRESULT TextAlternativesAsyncCompleteHandler::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*>)))
    {
        *ppObject = static_cast<wf::IAsyncOperationCompletedHandler<wfc::IVectorView<HSTRING>*>*>(this);
    }
    else
    {
        return ComBase::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

// Called when the Linguistic API has completed.
IFACEMETHODIMP TextAlternativesAsyncCompleteHandler::Invoke(
    _In_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>* asyncInfo,
    _In_ wf::AsyncStatus status)
{
    wfc::IVectorView<HSTRING>* result;
    HRESULT hr = asyncInfo->GetResults(&result);
    VERIFYHR(hr);
    if (SUCCEEDED(hr))
    {
        m_spTextAlternativesOperation->SetResults(result);
    }
    else
    {
        IFC_RETURN(m_spTextAlternativesOperation->SetEmptyResults());
    }

    return S_OK;
}

void TextAlternativesAsyncCompleteHandler::SetLinguisticCallback(
    _In_ TextAlternativesOperation* pTextAlternativesOperation)
{
    m_spTextAlternativesOperation = pTextAlternativesOperation;
}
