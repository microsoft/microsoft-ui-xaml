// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{

    template<typename T>
    struct Impl_GetApplicationResource
    {
        static HRESULT call(_In_ HSTRING resourceKey, _Out_ T* pValue);
    };

    template<typename T>
    struct Impl_GetApplicationResource<T*>
    {
        static HRESULT call(_In_ HSTRING resourceKey, _Out_ T** pValue);
    };

    struct ApplicationResourceHelpers
    {

        template<typename T>
        static _Check_return_ HRESULT GetApplicationResource(_In_ HSTRING resourceKey, _Out_ T* pValue)
        {
            return Impl_GetApplicationResource<T>::call(resourceKey, pValue);
        }

    };

    template<typename T>
    _Check_return_ HRESULT Impl_GetApplicationResource<T>::call(
        _In_ HSTRING resourceKey,
        _Out_ T* pValue)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<wf::IReference<T>> spResourceAsReference;

        IFC(ApplicationResourceHelpers::GetApplicationResource(resourceKey, spResourceAsReference.GetAddressOf()));
        IFC(spResourceAsReference->get_Value(pValue));

    Cleanup:
        RRETURN(hr);
    }

    template<typename T>
    _Check_return_ HRESULT Impl_GetApplicationResource<T*>::call(
        _In_ HSTRING resourceKey,
        _Outptr_ T** ppValue)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IApplicationStatics> spApplicationStatics;
        wrl::ComPtr<xaml::IApplication> spApplication;
        wrl::ComPtr<xaml::IResourceDictionary> spResourceDictionary;
        wrl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> spResourceMap;
        wrl::ComPtr<IInspectable> spBoxedKeyStr;
        wrl::ComPtr<IInspectable> spResourceAsI;

        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Application).Get(),
            &spApplicationStatics));
        IFC(spApplicationStatics->get_Current(&spApplication));
        IFC(spApplication->get_Resources(&spResourceDictionary));
        IFC(spResourceDictionary.As(&spResourceMap));
        IFC(Private::ValueBoxer::CreateString(resourceKey, &spBoxedKeyStr));
        IFC(spResourceMap->Lookup(spBoxedKeyStr.Get(), &spResourceAsI));
        IFC(spResourceAsI.CopyTo(ppValue));

    Cleanup:
        RRETURN(hr);
    }
}
