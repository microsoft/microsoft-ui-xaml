// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// TestsPage2.xaml.h
// Declaration of the TestsPage2 class.
//

#pragma once

#include "MainPage.xaml.h"
#include "TestsPage2.g.h"

namespace BindTestbed
{
    /// <summary>
    /// Additional tests
    /// </summary>
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class TestsPage2 sealed
    {
    public:
        TestsPage2();
        property int MyInt;
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;
        property int IntPropNoINPC;

        property ::Windows::Foundation::Collections::IObservableVector<BindTestbedModel::IEmployee^>^ Employees;
        property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllFirstNames;
        property ::Windows::Foundation::Collections::IObservableVector<::Platform::String^>^ AllLastNames;
        property BindTestbedModel::IEmployee^ NullEmployee;
        property ::Platform::String^ NullStringProperty;
        property ::Platform::String^ ImageUriString;

        property ::Platform::String^ DPOnPage
        {
            ::Platform::String^ get()
            {
                return (::Platform::String^)GetValue(DPOnPageProperty);
            }
            void set(::Platform::String^ value)
            {
                SetValue(DPOnPageProperty, value);
            }
        }

        property BindTestbedModel::IManager^ NiceManager
        { 
            BindTestbedModel::IManager^ get()
            { 
                return Model->Employees->GetAt(0)->DirectManager; 
            } 
        }

        void InitializeValues();
        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void UndeferElementClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);

    private:
        static ::Microsoft::UI::Xaml::DependencyProperty^ _DPOnPageProperty;

    public:
        static property ::Microsoft::UI::Xaml::DependencyProperty^ DPOnPageProperty
        {
            ::Microsoft::UI::Xaml::DependencyProperty^ get()
            {
                return _DPOnPageProperty;
            }
            void set(::Microsoft::UI::Xaml::DependencyProperty^ value)
            {
                _DPOnPageProperty = value;
            }
        }
    };
}
