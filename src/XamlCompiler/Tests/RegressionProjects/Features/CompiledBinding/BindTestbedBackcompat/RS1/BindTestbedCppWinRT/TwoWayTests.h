// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TwoWayTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct TwoWayTests : TwoWayTestsT<TwoWayTests>
    {
        TwoWayTests();

        int IntPropNoINPC() { return intPropNoINPC; }
        void IntPropNoINPC(int value) { intPropNoINPC = value; }

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        hstring DPOnPage()
        {
            return unbox_value<hstring>(GetValue(dpOnPageProperty));
        }
        void DPOnPage(hstring const& value)
        {
            SetValue(dpOnPageProperty, box_value(value));
        }
        static wux::DependencyProperty DPOnPageProperty()
        {
            return dpOnPageProperty;
        }
        static void DPOnPageProperty(wux::DependencyProperty value)
        {
            dpOnPageProperty = value;
        }

        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);

    private:
        int intPropNoINPC = 0;
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
        static wux::DependencyProperty dpOnPageProperty;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct TwoWayTests : TwoWayTestsT<TwoWayTests, implementation::TwoWayTests>
    {
    };
}
