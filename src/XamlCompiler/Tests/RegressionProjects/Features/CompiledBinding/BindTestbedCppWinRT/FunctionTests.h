// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "FunctionTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct FunctionTests : FunctionTestsT<FunctionTests>
    {
        FunctionTests();

        int MyInt() { return myInt; }
        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        void InitializeValues();
        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void StopTrackingClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ReInitializeBindingsClick(IInspectable const&, wux::RoutedEventArgs const&);

        IInspectable FunctionReturningNull()
        {
            return nullptr;
        }

        hstring FunctionOnRootNoArgs()
        {
            return L"FunctionOnRootNoArgs";
        }

    private:
        int myInt = 0;
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct FunctionTests : FunctionTestsT<FunctionTests, implementation::FunctionTests>
    {
    };
}
