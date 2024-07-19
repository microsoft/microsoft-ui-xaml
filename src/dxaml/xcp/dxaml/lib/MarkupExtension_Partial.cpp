// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MarkupExtension.g.h"

using namespace DirectUI;

_Check_return_ HRESULT MarkupExtension::ProvideValueImpl(_Outptr_ IInspectable** ppReturnValue)
{
    return S_OK;
}

_Check_return_ HRESULT MarkupExtension::ProvideValueWithIXamlServiceProviderImpl(_In_ xaml::IXamlServiceProvider* pServiceProvider, _Outptr_ IInspectable** ppReturnValue)
{
    UNREFERENCED_PARAMETER(pServiceProvider);

    // Default implementation invokes the parameterless overload so that pre-RS4 custom markup extensions
    // (which only had the parameterless version of ProvideValue) continue to work as expected
    return ProvideValueProtected(ppReturnValue);
}
