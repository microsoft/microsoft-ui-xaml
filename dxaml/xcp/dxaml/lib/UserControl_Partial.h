// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "UserControl.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(UserControl)
    {
    public:

        static _Check_return_ HRESULT RegisterAppBarsCallback(_In_ CDependencyObject* nativeDO);
        static _Check_return_ HRESULT UnregisterAppBarsCallback(_In_ CDependencyObject* nativeDO);

         _Check_return_ HRESULT FindNameInPage(
            _In_ HSTRING strElementName, 
            _In_ bool fIsCalledFromUserControl,
            _Outptr_ IInspectable **ppObj) override;

         _Check_return_ HRESULT GetCalculatedDefaultStyleKey(
            _Outptr_result_maybenull_ const CClassInfo** ppType,
            _Outptr_result_maybenull_ IInspectable** ppBoxedKey) override;

    protected:
        virtual _Check_return_ HRESULT RegisterAppBars()
        {
            RRETURN(S_OK);
        }

        virtual _Check_return_ HRESULT UnregisterAppBars()
        {
            RRETURN(S_OK);
        }
    };
}
