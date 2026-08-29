// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "App.h"
#include "MainModel.g.h"

namespace winrt::BindTestbed::implementation
{
    struct MainModel : MainModelT<MainModel>
    {
        MainModel()
        {
            _model = App::Model;
            _domodel = App::DOModel;
            _modelCX = App::ModelCX;
            _employees = single_threaded_observable_vector<BindTestbedModel::IEmployee>();
            _allFirstNames = single_threaded_observable_vector<hstring>();
            _allLastNames = single_threaded_observable_vector<hstring>();
            this->InitializeValues();
        }

        void InitializeValues()
        {
            _model.InitializeValues();
            _domodel.UpdateValues();
            _modelCX.InitializeValues();
            for (auto const& item : _model.ManagerProp().ReportsList())
            {
                _employees.Append(item);
                _allFirstNames.Append(item.FirstName());
                _allLastNames.Append(item.LastName());
            }
        }

        BindTestbedModel::DataModel Model()
        {
            return _model;
        }

        BindTestbedModel::DOModel DOModel()
        {
            return _domodel;
        }

        BindTestbedCXModel::ModelCX ModelCX()
        {
            return _modelCX;
        }

        wfc::IObservableVector<BindTestbedModel::IEmployee> Employees()
        {
            return _employees;
        }
        void Employees(wfc::IObservableVector<BindTestbedModel::IEmployee> value)
        { 
            _employees = value; 
        }

        wfc::IObservableVector<hstring> AllFirstNames()
        {
            return _allFirstNames;
        }
        void AllFirstNames(wfc::IObservableVector<hstring> value)
        {
            _allFirstNames = value;
        }

        wfc::IObservableVector<hstring> AllLastNames()
        {
            return _allLastNames;
        }
        void AllLastNames(wfc::IObservableVector<hstring> value)
        {
            _allLastNames = value;
        }

    private:
        BindTestbedModel::DataModel _model = nullptr;
        BindTestbedModel::DOModel _domodel = nullptr;
        BindTestbedCXModel::ModelCX _modelCX = nullptr;
        wfc::IObservableVector<BindTestbedModel::IEmployee> _employees = nullptr;
        wfc::IObservableVector<hstring> _allFirstNames = nullptr;
        wfc::IObservableVector<hstring> _allLastNames = nullptr;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct MainModel : MainModelT<MainModel, implementation::MainModel>
    {
    };
}
