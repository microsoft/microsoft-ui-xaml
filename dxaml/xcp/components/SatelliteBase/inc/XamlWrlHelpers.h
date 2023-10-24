// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    template <typename T, typename U>
    __inline wrl::ComPtr<T> As(_In_ const wrl::ComPtr<U>& spInsp)
    {
        wrl::ComPtr<T> spOutPtr;
        if (nullptr != spInsp)
        {
            spInsp.As(&spOutPtr);
        }
        return spOutPtr;
    }

    template <typename T, typename U>
    __inline bool Is(_In_ const wrl::ComPtr<U>& spInsp)
    {
        wrl::ComPtr<T> spAsTargetType;
        return SUCCEEDED(spInsp.As(&spAsTargetType)) ? true : false;
    }

    template <typename T, typename U>
    __inline bool Is(_In_ U* pInsp)
    {
        wrl::ComPtr<U> spInsp(pInsp);
        wrl::ComPtr<T> spAsTargetType;
        return SUCCEEDED(spInsp.As(&spAsTargetType)) ? true : false;
    }

    __inline IInspectable* iinspectable_cast(_In_opt_ IInspectable* pInterface)
    {
        return pInterface;
    }

    HRESULT
    InitializeDependencyProperty(
        _In_ LPCWSTR propertyNameString,
        _In_ LPCWSTR propertyTypeNameString,
        _In_ LPCWSTR ownerTypeNameString,
        bool isAttached,
        _In_opt_ IInspectable* defaultValue,
        _In_opt_ xaml::IPropertyChangedCallback* propertyChangedCallback,
        _COM_Outptr_ xaml::IDependencyProperty** returnValue);

#define GET_DEPENDENCY_PROPERTY(STATICS_TYPE, TARGET_TYPE_AS_STRING, PROPERTY_NAME, VARIABLE_NAME) \
        do { \
            wrl::ComPtr<STATICS_TYPE> statics; \
            if (SUCCEEDED(hr)) \
            { \
                hr = wf::GetActivationFactory( \
                    wrl_wrappers::HStringReference(TARGET_TYPE_AS_STRING).Get(), \
                    &statics); \
            } \
            if (SUCCEEDED(hr)) \
            { \
                hr = statics->get_##PROPERTY_NAME##Property(&VARIABLE_NAME); \
            } \
        } \
        while (0, 0)

    HRESULT
    SetDefaultStyleKey(
        _In_ IInspectable* pInspectable,
        _In_ LPCWSTR pwszDefaultStyleKey);

    HRESULT
    GetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _Out_ wrl::ComPtr<IInspectable>* propertyValue);

    // TODO: Remove once codegen property setters are available
    HRESULT
    SetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _In_ wrl::ComPtr<IInspectable> propertyValue);

    // These methods are provided for convienience. It's very common that
    // properties will be primitive WinRT types, and need to be returned as
    // an IPropveryValue.
    HRESULT
    GetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _Out_ wrl::ComPtr<wf::IPropertyValue>* propertyValue);


    // TODO: Remove once codegen property setters are available
    HRESULT
    SetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _In_ wrl::ComPtr<wf::IPropertyValue> propertyValue);

    HRESULT
    CreateStringPropertyValue(
        _In_ HSTRING value,
        _Out_ wrl::ComPtr<wf::IPropertyValue>* propertyValue);

    template<typename T>
    HRESULT AttachTemplatePart(
        _In_ xaml_controls::IControlProtected* pControlProtected,
        _In_z_ const WCHAR* pPartName,
        _Outptr_result_maybenull_ T** ppReturn)
    {
        *ppReturn = nullptr;

        wrl::ComPtr<xaml::IDependencyObject> spPartAsDO;

        IFC_RETURN(pControlProtected->GetTemplateChild(
            wrl_wrappers::HStringReference(pPartName).Get(),
            &spPartAsDO));

        if (spPartAsDO)
        {
            // Ignore this HRESULT. The control could have been
            // retemplated with an incorrectly templated part and
            // we should silently fail.
            IGNOREHR(spPartAsDO.CopyTo(ppReturn));
        }

        return S_OK;
    }

}
