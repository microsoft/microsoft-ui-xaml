// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

IActivationFactory* ctl::BetterActivationFactoryCreator::GetForDO(KnownTypeIndex eTypeIndex)
{
    return nullptr;
}

IFACEMETHODIMP ctl::BetterCoreObjectActivationFactory::ActivateInstance(_Outptr_ IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstance(_In_ IInspectable* pOuter, _Outptr_ IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT ctl::BetterAggregableAbstractCoreObjectActivationFactory::ActivateInstance(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pValue, _Outptr_ IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}
