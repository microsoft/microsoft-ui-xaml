// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "EventTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct EventTests : EventTestsT<EventTests>
    {
        EventTests();

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        void InitializeValues();
        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);

        void Click_RegularArgs(IInspectable const& sender, wux::RoutedEventArgs const& e);
        void Click_NoArgs();
        void Click_BaseArgs(IInspectable const& sender, wux::RoutedEventArgs const& e);
        void Click_OverloadedArgs();
        void Click_OverloadedArgs(IInspectable const& sender, IInspectable const& e);

    private:
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct EventTests : EventTestsT<EventTests, implementation::EventTests>
    {
    };
}
