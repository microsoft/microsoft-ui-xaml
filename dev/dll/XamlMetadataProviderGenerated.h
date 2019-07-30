// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

STDAPI DllGetActivationFactory(_In_ HSTRING activatibleClassId, _COM_Outptr_ IActivationFactory** factory);

class XamlMetadataProviderGenerated
{
public:
    void RegisterTypes();

    template <typename Factory>
    static winrt::IInspectable ActivateInstanceWithFactory(_In_ PCWSTR typeName)
    {
        auto factory = GetFactory<Factory>(typeName);
        winrt::IInspectable inner;
        return factory.as<Factory>().CreateInstance(nullptr, inner);
    }

    template <typename Factory>
    static Factory GetFactory(_In_ PCWSTR typeName)
    {
        winrt::IActivationFactory _activationFactory{ nullptr };
        Microsoft::WRL::Wrappers::HStringReference activatableClassId{ typeName };

        if (FAILED(DllGetActivationFactory(activatableClassId.Get(), (IActivationFactory**)winrt::put_abi(_activationFactory))))
        {
            return nullptr;
        }
        else
        {
            return _activationFactory.as<Factory>();
        }
    }

    static winrt::IInspectable ActivateInstance(_In_ PCWSTR typeName)
    {
        winrt::IActivationFactory _activationFactory{ nullptr };
        Microsoft::WRL::Wrappers::HStringReference activatableClassId{ typeName };
        winrt::check_hresult(DllGetActivationFactory(activatableClassId.Get(), (IActivationFactory * *)winrt::put_abi(_activationFactory)));

        return _activationFactory.ActivateInstance<winrt::IInspectable>();
    }
};
