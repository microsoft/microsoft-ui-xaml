// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DeferralManager.h"

namespace DirectUI
{
    PARTIAL_CLASS(ContentDialogClosingDeferral)
    {
    public:

        ContentDialogClosingDeferral() :
            m_isComplete(false),
            m_generation(0)
        {}

        _Check_return_ HRESULT Initialize(
            _In_ DeferralManager<ContentDialogClosingDeferral>* pManager,
            _In_ ULONG deferralGeneration)
        {
            m_generation = deferralGeneration;
            m_spManager = pManager;
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT CompleteImpl()
        {
            HRESULT hr = S_OK;

            if (m_isComplete)
            {
                IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_DEFERRAL_COMPLETED));
            }

            m_isComplete = true;
            IFC(m_spManager->CompleteDeferral(m_generation));
            m_spManager.Reset();

        Cleanup:
            RRETURN(hr);
        }

    private:
        ctl::ComPtr<DeferralManager<ContentDialogClosingDeferral>> m_spManager;
        boolean m_isComplete;
        ULONG m_generation;
    };
}
