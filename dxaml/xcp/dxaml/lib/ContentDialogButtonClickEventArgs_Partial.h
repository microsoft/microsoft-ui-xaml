// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentDialogButtonClickEventArgs.g.h"
#include "ContentDialogButtonClickDeferral.g.h"
#include "DeferralManager.h"

namespace DirectUI
{
    PARTIAL_CLASS(ContentDialogButtonClickEventArgs)
    {

    public:

        ContentDialogButtonClickEventArgs() :
            m_deferralGeneration(0)
        {}

        _Check_return_ HRESULT Initialize(
            _In_ DeferralManager<ContentDialogButtonClickDeferral>* pDeferralManager,
            _In_ ULONG deferralGeneration)
        {
            m_deferralGeneration = deferralGeneration;
            m_spDeferralManager = pDeferralManager;
            RRETURN(S_OK);
        }

        using ContentDialogButtonClickEventArgsGenerated::Initialize;

        _Check_return_ HRESULT GetDeferralImpl(
            _Outptr_ xaml_controls::IContentDialogButtonClickDeferral** returnValue)
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<ContentDialogButtonClickDeferral> spDeferral;

            IFC(m_spDeferralManager->GetDeferral(m_deferralGeneration, &spDeferral));
            IFC(spDeferral.CopyTo(returnValue));

        Cleanup:
            RRETURN(hr);
        }

    private:

        ULONG m_deferralGeneration;
        ctl::ComPtr<DeferralManager<ContentDialogButtonClickDeferral>> m_spDeferralManager;
    };
}
