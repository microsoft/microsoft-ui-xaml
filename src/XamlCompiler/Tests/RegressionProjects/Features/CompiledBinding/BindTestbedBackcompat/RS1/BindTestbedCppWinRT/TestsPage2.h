// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestsPage2.g.h"

namespace winrt::BindTestbed::implementation
{
    struct TestsPage2 : TestsPage2T<TestsPage2>
    {
        TestsPage2();

        int MyInt() { return myInt; }
        void MyInt(int value) { myInt = value; }

        int IntPropNoINPC() { return intPropNoINPC; }
        void IntPropNoINPC(int value) { intPropNoINPC = value; }

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        hstring ImageUriString() { return imageUriString; }
        void ImageUriString(hstring const& value) { imageUriString = value; }

        hstring NullStringProperty() { return nullStringProperty; }
        void NullStringProperty(hstring const& value) { nullStringProperty = value; }

        BindTestbedModel::IEmployee NullEmployee() { return nullEmployee; }
        void NullEmployee(BindTestbedModel::IEmployee const& value) { nullEmployee = value; }

        BindTestbedModel::IManager NiceManager()
        { 
            auto e = Model().Employees().as<wfc::IVector<BindTestbedModel::IEmployee>>();
            return e.GetAt(0).DirectManager();
        }

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

        wfc::IObservableVector<BindTestbedModel::IEmployee> Employees() { return employees; }
        void Employees(wfc::IObservableVector<BindTestbedModel::IEmployee> value) { employees = value; }

        wfc::IObservableVector<hstring> AllFirstNames() { return allFirstNames; }
        void AllFirstNames(wfc::IObservableVector<hstring> value) { allFirstNames = value; }

        wfc::IObservableVector<hstring> AllLastNames() { return allLastNames; }
        void AllLastNames(wfc::IObservableVector<hstring> value) { allLastNames = value; }

        void InitializeValues();
        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void UndeferElementClick(IInspectable const&, wux::RoutedEventArgs const&);

    private:
        int intPropNoINPC = 0;
        int myInt = 0;
        hstring imageUriString{};
        hstring nullStringProperty{};
        BindTestbedModel::IEmployee nullEmployee{ nullptr };
        BindTestbedModel::DataModel model{ nullptr };
        BindTestbedModel::DOModel domodel{ nullptr };
        wfc::IObservableVector<BindTestbedModel::IEmployee> employees{ nullptr };
        wfc::IObservableVector<hstring> allFirstNames{ nullptr };
        wfc::IObservableVector<hstring> allLastNames{ nullptr };
        static wux::DependencyProperty dpOnPageProperty;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct TestsPage2 : TestsPage2T<TestsPage2, implementation::TestsPage2>
    {
    };
}
