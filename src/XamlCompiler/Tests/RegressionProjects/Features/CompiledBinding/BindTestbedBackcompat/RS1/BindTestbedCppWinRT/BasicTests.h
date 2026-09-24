// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "BasicTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct BasicTests : BasicTestsT<BasicTests>
    {
        BasicTests();

        int MyInt() { return myInt; }
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

        hstring NonDPOnPage() { return nonDPOnPage; }
        void  NonDPOnPage(hstring const& value) { nonDPOnPage = value; }

        void InitializeValues();
        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void StopTrackingClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ReInitializeBindingsClick(IInspectable const&, wux::RoutedEventArgs const&);

    private:
        int myInt = 0;
        BindTestbedModel::DataModel model = nullptr ;
        BindTestbedModel::DOModel domodel = nullptr;
        hstring nonDPOnPage;
        static wux::DependencyProperty dpOnPageProperty;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct BasicTests : BasicTestsT<BasicTests, implementation::BasicTests>
    {
    };
}
