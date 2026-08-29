// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlServiceProviderContext.h"
#include "XamlQualifiedObject.h"

_Check_return_ HRESULT 
CNullExtension::ProvideValue(
            _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qoResult)
{
    CValue nullValue;

    auto qo = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::DependencyObject));

    // Use the valueNull value instead of valueObject with a pointer of NULL, 
    // the property system knows how to correctly deal with this. 
    nullValue.SetNull();
    IFC_RETURN(qo->SetValue(nullValue));

    qoResult = std::move(qo);
    
    return S_OK;
}

