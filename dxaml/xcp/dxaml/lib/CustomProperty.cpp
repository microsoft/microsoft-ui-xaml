// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A default implementation of ICustomProperty, which can be used to describe internal 
//      properties on non-DOs.

#include "precomp.h"
#include "CustomProperty.h"

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

_Check_return_ HRESULT
CustomProperty::CreateObjectProperty(
    _In_ HSTRING hName,
    _In_ GetValueFunction getValueFunc,
    _Outptr_ xaml_data::ICustomProperty** ppProperty)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CustomProperty_Object> spProperty;

    IFC(ctl::make(&spProperty));
    IFC(WindowsDuplicateString(hName, &spProperty->m_hName));
    spProperty->m_funcGetValue = getValueFunc;

    *ppProperty = spProperty.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CustomProperty::CreateInt32Property(
    _In_ HSTRING hName,
    _In_ GetValueFunction getValueFunc,
    _Outptr_ xaml_data::ICustomProperty** ppProperty)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CustomProperty_Int32> spProperty;

    IFC(ctl::make(&spProperty));
    IFC(WindowsDuplicateString(hName, &spProperty->m_hName));
    spProperty->m_funcGetValue = getValueFunc;

    *ppProperty = spProperty.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CustomProperty::CreateBooleanProperty(
    _In_ HSTRING hName,
    _In_ GetValueFunction getValueFunc,
    _Outptr_ xaml_data::ICustomProperty** ppProperty)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<CustomProperty_Boolean> spProperty;

    IFC(ctl::make(&spProperty));
    IFC(WindowsDuplicateString(hName, &spProperty->m_hName));
    spProperty->m_funcGetValue = getValueFunc;

    *ppProperty = spProperty.Detach();

Cleanup:
    RRETURN(hr);
}
