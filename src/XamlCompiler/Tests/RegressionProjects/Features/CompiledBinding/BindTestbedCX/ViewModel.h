// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace BindTestbedCX
{
    using namespace ::Platform;
    using namespace ::Windows::Foundation::Collections;

    public ref class CastingTestsVM sealed :
        public Microsoft::UI::Xaml::Data::INotifyPropertyChanged
    {
    public:

        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        property Platform::String^ Username
        {
            Platform::String^ get();
            void set(Platform::String^ value);
        }

        property bool IsVisible
        {
            bool get();
            void set(bool value);
        }

        property Platform::IBox<bool>^ IsVisibleNullable
        {
            Platform::IBox<bool>^ get();
            void set(Platform::IBox<bool>^ value);
        }

        property Microsoft::UI::Xaml::Visibility VisibilityValue
        {
            Microsoft::UI::Xaml::Visibility get();
            void set(Microsoft::UI::Xaml::Visibility value);
        }

        property bool IsChecked
        {
            bool get();
            void set(bool value);
        }

        property double DoubleVal
        {
            double get();
            void set(double value);
        }

        property int IntVal
        {
            int get();
            void set(int value);
        }

        property Platform::String^ Prefix
        {
            Platform::String^ get();
        }

        property double Postfix
        {
            double get();
        }

        Platform::String^ CombineStringWithInt(Platform::String^ str, int number);

    private:

        void RaisePropertyChanged(Platform::String^ propertyName);

        Platform::String^ _username;
        bool _isVisible = false;
        Microsoft::UI::Xaml::Visibility _visibilityValue = Microsoft::UI::Xaml::Visibility::Collapsed;
        bool _isChecked = true;
        double _doubleVal = 15.0;
        int _intVal = 20;
    };

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class MainModel sealed
    {
    public:
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;
        property BindTestbedCXModel::ModelCX^ ModelCX;

        property IObservableVector<BindTestbedModel::IEmployee^>^ Employees;
        property IObservableVector<String^>^ AllFirstNames;
        property IObservableVector<String^>^ AllLastNames;

        MainModel()
        {
            this->Model = App::Model;
            this->DOModel = App::DOModel;
            this->ModelCX = App::ModelCX;
            this->Employees = ref new ::Platform::Collections::Vector<BindTestbedModel::IEmployee ^>();
            this->AllFirstNames = ref new ::Platform::Collections::Vector<String ^>();
            this->AllLastNames = ref new ::Platform::Collections::Vector<String ^>();

            this->InitializeValues();
        }

    private:
        void InitializeValues()
        {
            this->Model->InitializeValues();
            this->DOModel->UpdateValues();
            this->ModelCX->InitializeValues();

            for each (BindTestbedModel::IEmployee^ e in Model->ManagerProp->ReportsList)
            {
                this->Employees->Append(e);
                this->AllFirstNames->Append(e->FirstName);
                this->AllLastNames->Append(e->LastName);
            }
        }
    };
}