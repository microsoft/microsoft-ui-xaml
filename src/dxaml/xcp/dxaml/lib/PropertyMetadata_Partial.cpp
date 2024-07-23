// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyMetadata.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
PropertyMetadata::Create(
    _In_opt_ IInspectable* pDefaultValue,
    _Outptr_ IPropertyMetadata** ppMetadata)
{
    RRETURN(PropertyMetadata::Create(
        pDefaultValue,
        NULL, // pCreateDefaultValueCallback
        NULL, // pPropertyChangedCallback
        NULL, // pOuter
        NULL, // ppInner
        ppMetadata));
}

_Check_return_ HRESULT
PropertyMetadata::Create(
    _In_opt_ IInspectable* pDefaultValue,
    _In_ xaml::ICreateDefaultValueCallback* pCreateDefaultValueCallback,
    _In_opt_ IPropertyChangedCallback* pPropertyChangedCallback,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IPropertyMetadata* pInstance = NULL;

    IFCPTR(ppInstance);

    IFC(ctl::AggregableActivationFactory<PropertyMetadata>::CreateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));

    IFC(static_cast<PropertyMetadata*>(pInstance)->put_DefaultValue(pDefaultValue));
    IFC(static_cast<PropertyMetadata*>(pInstance)->put_CreateDefaultValueCallback(pCreateDefaultValueCallback));
    IFC(static_cast<PropertyMetadata*>(pInstance)->put_PropertyChangedCallback(pPropertyChangedCallback));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInner);
    ReleaseInterface(pInstance);
    RRETURN(hr);
}

_Check_return_ HRESULT
PropertyMetadata::CreateWithInt32DefaultValue(
    _In_ INT32 nDefaultValue,
    _Outptr_ IPropertyMetadata** ppMetadata)
{
    HRESULT hr = S_OK;
    IInspectable* pDefaultValue = NULL;

    IFC(PropertyValue::CreateFromInt32(nDefaultValue, &pDefaultValue));
    IFC(PropertyMetadata::Create(pDefaultValue, ppMetadata));

Cleanup:
    ReleaseInterface(pDefaultValue);
    RRETURN(hr);
}

_Check_return_ HRESULT
PropertyMetadata::CreateWithBooleanDefaultValue(
    _In_ BOOLEAN bDefaultValue,
    _Outptr_ IPropertyMetadata** ppMetadata)
{
    HRESULT hr = S_OK;
    IInspectable* pDefaultValue = NULL;

    IFC(PropertyValue::CreateFromBoolean(bDefaultValue, &pDefaultValue));
    IFC(PropertyMetadata::Create(pDefaultValue, ppMetadata));

Cleanup:
    ReleaseInterface(pDefaultValue);
    RRETURN(hr);
}

// Constructors.
_Check_return_ HRESULT PropertyMetadataFactory::CreateInstanceWithDefaultValueImpl(
    _In_opt_ IInspectable* pDefaultValue,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        pDefaultValue,
        NULL, // pCreateDefaultValueCallback
        NULL, // pPropertyChangedCallback
        pOuter,
        ppInner,
        ppInstance));
}

_Check_return_ HRESULT PropertyMetadataFactory::CreateInstanceWithDefaultValueAndCallbackImpl(
    _In_opt_ IInspectable* pDefaultValue,
    _In_opt_ IPropertyChangedCallback* pPropertyChangedCallback,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        pDefaultValue,
        NULL, // pCreateDefaultValueCallback
        pPropertyChangedCallback,
        pOuter,
        ppInner,
        ppInstance));
}

// Static methods.
_Check_return_ HRESULT PropertyMetadataFactory::CreateWithDefaultValueImpl(
    _In_opt_ IInspectable* pDefaultValue,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        pDefaultValue,
        NULL, // pCreateDefaultValueCallback
        NULL, // pPropertyChangedCallback
        NULL, // pOuter
        NULL, // ppInner
        ppInstance));
}

_Check_return_ HRESULT PropertyMetadataFactory::CreateWithDefaultValueAndCallbackImpl(
    _In_opt_ IInspectable* pDefaultValue,
    _In_opt_ IPropertyChangedCallback* pPropertyChangedCallback,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        pDefaultValue,
        NULL, // pCreateDefaultValueCallback
        pPropertyChangedCallback,
        NULL, // pOuter
        NULL, // ppInner
        ppInstance));
}

_Check_return_ HRESULT PropertyMetadataFactory::CreateWithFactoryImpl(
    _In_ xaml::ICreateDefaultValueCallback* pCreateDefaultValueCallback,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        NULL, // pDefaultValue
        pCreateDefaultValueCallback,
        NULL, // pPropertyChangedCallback
        NULL, // pOuter
        NULL, // ppInner
        ppInstance));
}

_Check_return_ HRESULT PropertyMetadataFactory::CreateWithFactoryAndCallbackImpl(
    _In_ xaml::ICreateDefaultValueCallback* pCreateDefaultValueCallback,
    _In_opt_ IPropertyChangedCallback* pPropertyChangedCallback,
    _Outptr_ IPropertyMetadata** ppInstance)
{
    RRETURN(PropertyMetadata::Create(
        NULL, // pDefaultValue
        pCreateDefaultValueCallback,
        pPropertyChangedCallback,
        NULL, // pOuter
        NULL, // ppInner
        ppInstance));
}
