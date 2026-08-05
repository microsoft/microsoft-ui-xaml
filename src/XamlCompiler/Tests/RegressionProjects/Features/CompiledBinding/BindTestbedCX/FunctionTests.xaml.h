// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "FunctionTests.g.h"

namespace BindTestbedCX
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class FunctionTests sealed
    {
    public:
        FunctionTests();
        property int MyInt;
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;

        void InitializeValues();
        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void StopTrackingClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ReInitializeBindingsClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);

        ::Platform::String^ FunctionReturningNull()
        {
            return nullptr;
        }

        ::Platform::String^ FunctionOnRootNoArgs()
        {
            return "FunctionOnRootNoArgs";
        }
    };
}
