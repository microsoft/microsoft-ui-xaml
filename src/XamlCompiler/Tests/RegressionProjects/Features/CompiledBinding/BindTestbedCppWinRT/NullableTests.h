// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Declaration of the NullableTests class.
//

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "NullableTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct NullableTests : NullableTestsT<NullableTests>
    {
        NullableTests();

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);

    private:
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct NullableTests : NullableTestsT<NullableTests, implementation::NullableTests>
    {
    };
}
