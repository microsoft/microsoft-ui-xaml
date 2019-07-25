// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

STDAPI DllGetActivationFactory(HSTRING activatibleClassId, IActivationFactory **factory);

class XamlMetadataProviderGenerated
{
public:
    void RegisterTypes();

    template <typename Factory>
    static winrt::IInspectable ActivateInstanceWithFactory(PCWSTR typeName)
    {
        auto factory = GetFactory<Factory>(typeName);
        winrt::IInspectable inner;
        return factory.as<Factory>().CreateInstance(nullptr, inner);
    }

    template <typename Factory>
    static Factory GetFactory(PCWSTR typeName)
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

    static winrt::IInspectable ActivateInstance(PCWSTR typeName)
    {
        winrt::IActivationFactory _activationFactory{ nullptr };
        Microsoft::WRL::Wrappers::HStringReference activatableClassId{ typeName };
        winrt::check_hresult(DllGetActivationFactory(activatableClassId.Get(), (IActivationFactory**)winrt::put_abi(_activationFactory)));

        return _activationFactory.ActivateInstance<winrt::IInspectable>();
    }
};

