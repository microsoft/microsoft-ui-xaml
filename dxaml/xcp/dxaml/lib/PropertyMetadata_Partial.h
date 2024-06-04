// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyMetadata.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(PropertyMetadata)
    {
    public:
        static _Check_return_ HRESULT Create(
            _In_opt_ IInspectable* pDefaultValue,
            _Outptr_ xaml::IPropertyMetadata** ppMetadata);

        static _Check_return_ HRESULT Create(
            _In_opt_ IInspectable* pDefaultValue,
            _In_ xaml::ICreateDefaultValueCallback* pCreateDefaultValueCallback,
            _In_opt_ xaml::IPropertyChangedCallback* pPropertyChangedCallback,
            _In_opt_ IInspectable* pOuter,
            _Outptr_ IInspectable** ppInner,
            _Outptr_ IPropertyMetadata** ppInstance);

        static _Check_return_ HRESULT CreateWithInt32DefaultValue(
            _In_ INT32 nDefaultValue,
            _Outptr_ IPropertyMetadata** ppMetadata);

        static _Check_return_ HRESULT CreateWithBooleanDefaultValue(
            _In_ BOOLEAN bDefaultValue,
            _Outptr_ IPropertyMetadata** ppMetadata);

        template<class T>
        static _Check_return_ HRESULT CreateWithReferenceDefaultValue(
            _In_ T value,
            _Outptr_ IPropertyMetadata** ppMetadata)
        {
            HRESULT hr = S_OK;
            IInspectable* pDefaultValue = NULL;

            IFC(PropertyValue::CreateReference<T>(value, &pDefaultValue));
            IFC(PropertyMetadata::Create(pDefaultValue, ppMetadata));

        Cleanup:
            ReleaseInterface(pDefaultValue);
            RRETURN(hr);
        }
    };
}
