// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutoSuggestBoxTextChangedEventArgs.g.h"
#include "AutoSuggestBox.g.h"
#include "AutoSuggestBox_Partial.h"


namespace DirectUI
{
    PARTIAL_CLASS(AutoSuggestBoxTextChangedEventArgs)
    {
    public:
        AutoSuggestBoxTextChangedEventArgs()
            : m_reason(xaml_controls::AutoSuggestionBoxTextChangeReason_ProgrammaticChange)
            , m_counter(0)
        {
        }

    public:

        IFACEMETHOD(get_Reason)(_Out_ xaml_controls::AutoSuggestionBoxTextChangeReason* outval)
        {
            ARG_VALIDRETURNPOINTER(outval);
            *outval = m_reason;
            RRETURN(S_OK);
        }

        IFACEMETHOD(put_Reason)(_In_ xaml_controls::AutoSuggestionBoxTextChangeReason inval)
        {
            m_reason = inval;
            RRETURN(S_OK);
        }

        virtual _Check_return_ HRESULT CheckCurrentImpl(_Out_ BOOLEAN* pReturnValue)
        {
            HRESULT hr = S_OK;
            Microsoft::WRL::ComPtr<xaml_controls::IAutoSuggestBox> spOwner;

            *pReturnValue = FALSE;
            if (m_wkOwner)
            {
                IFC(m_wkOwner.As(&spOwner));
            }

            if (spOwner)
            {
                *pReturnValue = static_cast<DirectUI::AutoSuggestBox*>(spOwner.Get())->GetTextChangedEventCounter() == m_counter;
            }

        Cleanup:
            RRETURN(hr);
        }

    public:
        void SetCounter(UINT counter)
        {
            m_counter = counter;
        }

        _Check_return_ HRESULT SetOwner(_In_ xaml_controls::IAutoSuggestBox* pOwner)
        {
            Microsoft::WRL::ComPtr<xaml_controls::IAutoSuggestBox> spOwner(pOwner);
            RRETURN(spOwner.AsWeak(&m_wkOwner));
        }

    private:
        xaml_controls::AutoSuggestionBoxTextChangeReason m_reason;
        UINT m_counter;
        Microsoft::WRL::WeakRef m_wkOwner;
    };
}
