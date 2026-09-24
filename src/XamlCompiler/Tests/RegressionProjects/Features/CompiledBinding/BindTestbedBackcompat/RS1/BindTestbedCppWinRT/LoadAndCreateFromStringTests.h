// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "LoadAndCreateFromStringTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct LoadAndCreateFromStringTests : LoadAndCreateFromStringTestsT<LoadAndCreateFromStringTests>
    {
        LoadAndCreateFromStringTests();

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        void InitializeValues();
        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        /*
        void Button_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void LoadInnerPanel_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void UnloadInnerPanel_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void LoadOuterPanel_Click(IInspectable const&, wux::RoutedEventArgs const&);
        void UnloadOuterPanel_Click(IInspectable const&, wux::RoutedEventArgs const&);
        */

    private:
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct LoadAndCreateFromStringTests : LoadAndCreateFromStringTestsT<LoadAndCreateFromStringTests, implementation::LoadAndCreateFromStringTests>
    {
    };
}
