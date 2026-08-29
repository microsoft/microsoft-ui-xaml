// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Tests {
    namespace Sample {

        // Custom types MUST be sealed and have a public constructor
        // if you'd like them to be activatable. 
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomButton sealed
            : public Microsoft::UI::Xaml::Controls::Button
        {
        public:
            CustomButton() {}

            static property Microsoft::UI::Xaml::DependencyProperty^ CustomPropertyProperty
            {
                Microsoft::UI::Xaml::DependencyProperty^ get() { return s_customPropertyProperty; }
            }

            property Object^ CustomProperty
            {
                Object^ get() { return GetValue(CustomPropertyProperty); }
                void set(Object^ value) { SetValue(CustomPropertyProperty, value); }
            }

            // Because of the dynamic nature of test DLLs we need to not make dependency
            // properties static initialized, but instead registered and unregistered using
            // these methods, which are to be called after we register the metadata provider
            // and after the test class is done executing.
            static void RegisterDependencyProperties();
            static void ClearDependencyProperties();

        private:
            static Microsoft::UI::Xaml::DependencyProperty^ s_customPropertyProperty;
        };
    }
} }