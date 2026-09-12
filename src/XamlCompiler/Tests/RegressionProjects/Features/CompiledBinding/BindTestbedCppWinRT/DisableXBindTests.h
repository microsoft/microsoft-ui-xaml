// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "DisableXBindTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct DisableXBindTests : DisableXBindTestsT<DisableXBindTests>
    {
        DisableXBindTests();

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        // See the note in BindTestbed.idl: the binding on this uses TargetNullValue, which
        // requires a nullable type, and hstring is not one.
        IInspectable FunctionReturningNull() { return nullptr; }

        void Click_RegularArgs(IInspectable const& sender, wux::RoutedEventArgs const& e);
        void Click_NoArgs();
        void On_Loaded(IInspectable const& sender, wux::RoutedEventArgs const& e);

    private:
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct DisableXBindTests : DisableXBindTestsT<DisableXBindTests, implementation::DisableXBindTests>
    {
    };
}
