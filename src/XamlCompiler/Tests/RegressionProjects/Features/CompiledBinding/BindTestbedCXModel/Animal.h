// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace BindTestbedCXModel
{
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class Animal sealed
    {
        ::Platform::String^ _name;
        ::Platform::String^ _color;

    public:
        Animal(::Platform::String^ name, ::Platform::String^ color);

        property ::Platform::String^ Name 
        { 
            ::Platform::String^ get()
            {
                return _name;
            }
        }

        property ::Platform::String^ Color
        {
            ::Platform::String^ get()
            {
                return _color;
            }
        }


        void Click_RegularArgsOnAnimal(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);

        void Click_NoArgsOnAnimal();

        void Click_BaseArgsOnAnimal(Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ e);

        void Click_OverloadedArgsOnAnimal(Platform::Object^ sender, Platform::Object^ e);
    };
}