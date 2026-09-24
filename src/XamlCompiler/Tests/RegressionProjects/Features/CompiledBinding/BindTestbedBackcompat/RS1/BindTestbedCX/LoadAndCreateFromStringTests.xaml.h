// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "MainPage.xaml.h"
#include "LoadAndCreateFromStringTests.g.h"

namespace BindTestbed
{
    /// <summary>
    /// x:Load Tests
    /// </summary>
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class LoadAndCreateFromStringTests sealed
    {
    public:
        LoadAndCreateFromStringTests();
        property BindTestbedModel::DataModel^ Model;
        property BindTestbedModel::DOModel^ DOModel;

        void InitializeValues();
        void UpdateValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void ResetValuesClick(Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ e);
    private:
        void Button_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void LoadInnerPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void UnloadInnerPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void LoadOuterPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
        void UnloadOuterPanel_Click(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);
    };
}
