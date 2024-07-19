// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Binding markup extension

#include "precomp.h"
#include "XamlQualifiedObject.h"

_Check_return_ HRESULT CBinding::SetBinding(_In_ CDependencyObject* pDependencyObject,  _In_ KnownPropertyIndex propIndex)
{
    IFC_RETURN(FxCallbacks::DependencyObject_SetBinding(pDependencyObject, propIndex, this));
    return S_OK;
}

_Check_return_ HRESULT CRelativeSource::ProvideValue(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ std::shared_ptr<XamlQualifiedObject>& qoResult)
{
    auto qo = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::RelativeSource));
    IFC_RETURN(qo->SetDependencyObject(this));
    qoResult = std::move(qo);

    return S_OK;
}

