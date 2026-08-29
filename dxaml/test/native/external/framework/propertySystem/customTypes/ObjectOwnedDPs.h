// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace PropertySystem {
    
    [Microsoft::UI::Xaml::Data::Bindable]
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class SomeTypeWithDPsOwnedByObject sealed
    {
    internal:
        static void InitDPs();
        static void ResetDPs();

    public:
        static property Microsoft::UI::Xaml::DependencyProperty^ TextProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return s_textProperty; }
        }

        static void SetText(Microsoft::UI::Xaml::DependencyObject^ obj, Platform::String^ value)
        {
            obj->SetValue(TextProperty, value);
        }

        static Platform::String^ GetText(Microsoft::UI::Xaml::DependencyObject^ obj)
        {
            return safe_cast<Platform::String^>(obj->GetValue(TextProperty));
        }

    private:
        static Microsoft::UI::Xaml::DependencyProperty^ s_textProperty;
    };

    [Microsoft::UI::Xaml::Data::Bindable]
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class DOWithDPsOwnedByObject sealed
        : public Microsoft::UI::Xaml::FrameworkElement
    {
    internal:
        static void InitDPs();
        static void ResetDPs();

    public:
        property Platform::String^ Text
        {
            Platform::String^ get() { return (Platform::String^)GetValue(s_textProperty); }
            void set(Platform::String^ value) { SetValue(s_textProperty, value); }
        }
        
        property Microsoft::UI::Xaml::Data::Binding^ Binding
        {
            Microsoft::UI::Xaml::Data::Binding^ get() { return (Microsoft::UI::Xaml::Data::Binding^)GetValue(s_bindingProperty); }
            void set(Microsoft::UI::Xaml::Data::Binding^ value) { SetValue(s_bindingProperty, value); }
        }


    private:
        static Microsoft::UI::Xaml::DependencyProperty^ s_textProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ s_bindingProperty;
    };

} } } } }