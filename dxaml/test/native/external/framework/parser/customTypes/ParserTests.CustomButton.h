// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Tests { namespace Framework { namespace Parser { 
    namespace Primitive
    {
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomButton
            : public Microsoft::UI::Xaml::Controls::Button
        {
        internal:
            CustomButton() {}
        public:
            static property Microsoft::UI::Xaml::DependencyProperty^ CustomPropertyProperty
            {
                Microsoft::UI::Xaml::DependencyProperty^ get() { return s_customPropertyProperty; }
            }

            property Platform::String^ CustomProperty
            {
                Platform::String^ get() { return safe_cast<Platform::String^>(GetValue(CustomPropertyProperty)); }
                void set(Platform::String^ value) { SetValue(CustomPropertyProperty, value); }
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

    namespace Control
    {
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class AwesomeParserButton sealed
            : public Private::Tests::Framework::Parser::Primitive::CustomButton
        {
        public:
            AwesomeParserButton() {}
        };
    }
} } } }