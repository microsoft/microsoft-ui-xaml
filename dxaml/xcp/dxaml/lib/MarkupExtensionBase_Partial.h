// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    PARTIAL_CLASS(MarkupExtensionBase)
    {
    public:
        virtual _Check_return_ HRESULT ProvideValue(_In_ xaml::IXamlServiceProvider* pServiceProvider, _Outptr_ IInspectable** ppValue, _Out_ KnownTypeIndex* peTypeIndex)
        {
            RRETURN(E_NOTIMPL);
        }
    };
}
