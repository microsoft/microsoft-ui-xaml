// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// TwoWayTests.xaml.h
// Declaration of the TwoWayTests class.
//

#pragma once

#include "TwoWayTests.g.h"

using namespace std;

namespace BindTestbedCX
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public ref class TwoWayTests sealed
    {
    public:
        TwoWayTests();
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;
        property int IntPropNoINPC;

        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);

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

