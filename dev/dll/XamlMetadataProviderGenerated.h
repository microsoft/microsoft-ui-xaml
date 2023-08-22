// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

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
        winrt::hstring activatableClassId{ typeName };

        if (FAILED(WINRT_GetActivationFactory(winrt::get_abi(activatableClassId), winrt::put_abi(_activationFactory))))
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
        winrt::hstring activatableClassId{ typeName };
        winrt::check_hresult(WINRT_GetActivationFactory(winrt::get_abi(activatableClassId), winrt::put_abi(_activationFactory)));

        return _activationFactory.ActivateInstance<winrt::IInspectable>();
    }
};
