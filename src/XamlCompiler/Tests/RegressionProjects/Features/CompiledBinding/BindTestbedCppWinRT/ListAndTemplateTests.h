// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "ListAndTemplateTests.g.h"

namespace winrt::BindTestbed::implementation
{
    struct ListAndTemplateTests : ListAndTemplateTestsT<ListAndTemplateTests>
    {
        ListAndTemplateTests();

        BindTestbedModel::DataModel Model() { return model; }
        void Model(BindTestbedModel::DataModel value) { model = value; }

        BindTestbedModel::DOModel DOModel() { return domodel; }
        void DOModel(BindTestbedModel::DOModel value) { domodel = value; }

        //TODO: Convert BindTestbedModelCX to C++/WinRT
        // BindTestbedCXModel::ModelCX ModelCX() { return modelCX; }
        //void ModelCX(BindTestbedCXModel::ModelCX value) { modelCX = value; }

        wfc::IObservableVector<BindTestbedModel::IEmployee> Employees() { return employees; }
        void Employees(wfc::IObservableVector<BindTestbedModel::IEmployee> value) { employees = value; }

        wfc::IObservableVector<hstring> AllFirstNames() { return allFirstNames; }
        void AllFirstNames(wfc::IObservableVector<hstring> value) { allFirstNames = value; }

        wfc::IObservableVector<hstring> AllLastNames() { return allLastNames; }
        void AllLastNames(wfc::IObservableVector<hstring> value) { allLastNames = value; }

        IInspectable SomeButtonContent() { return someButton().Content(); }
        void SomeButtonContent(IInspectable value) { someButton().Content(value); }

        void UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&);
        void ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&);

    private:
        wfc::IObservableVector<BindTestbedModel::IEmployee> employees = nullptr;
        wfc::IObservableVector<hstring> allFirstNames = nullptr;
        wfc::IObservableVector<hstring> allLastNames = nullptr;
        BindTestbedModel::DataModel model = nullptr;
        BindTestbedModel::DOModel domodel = nullptr;
        //TODO: Convert BindTestbedModelCX to C++/WinRT
        //BindTestbedCXModel::ModelCX modelCX = nullptr;//
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct ListAndTemplateTests : ListAndTemplateTestsT<ListAndTemplateTests, implementation::ListAndTemplateTests>
    {
    };
}
