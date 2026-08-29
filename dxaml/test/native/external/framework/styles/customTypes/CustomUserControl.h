// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Styles {
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomUserControl sealed : public Microsoft::UI::Xaml::Controls::UserControl
    {
    public:
        static void RegisterDependencyProperties();
        static void ClearDependencyProperties();
        
    private:
        static Microsoft::UI::Xaml::DependencyProperty^ m_WorkingTagProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomStringProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomIntProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomSizeProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomThicknessProperty;

    public:
        static property Microsoft::UI::Xaml::DependencyProperty^ WorkingTagProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_WorkingTagProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomStringProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomStringProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomIntProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomIntProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomSizeProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomSizeProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomThicknessProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomThicknessProperty; }
        }

        property Object^ WorkingTag
        {
            Object^ get() { return GetValue(WorkingTagProperty); }
            void set(Object^ value) { SetValue(WorkingTagProperty, value); }
        }
        property Platform::String^ CustomString
        {
            Platform::String^ get() { return safe_cast<Platform::String^>(GetValue(CustomStringProperty)); }
            void set(Platform::String^ value) { SetValue(CustomStringProperty, value); }
        }
        property int CustomInt
        {
            int get() { return safe_cast<int>(GetValue(CustomIntProperty)); }
            void set(int value) { SetValue(CustomIntProperty, value); }
        }
        property ::Windows::Foundation::Size CustomSize
        {
            ::Windows::Foundation::Size get() { return safe_cast<::Windows::Foundation::Size>(GetValue(CustomSizeProperty)); }
            void set(::Windows::Foundation::Size value) { SetValue(CustomSizeProperty, value); }
        }
        property Microsoft::UI::Xaml::Thickness CustomThickness
        {
            Microsoft::UI::Xaml::Thickness get() { return safe_cast<Microsoft::UI::Xaml::Thickness>(GetValue(CustomThicknessProperty)); }
            void set(Microsoft::UI::Xaml::Thickness value) { SetValue(CustomThicknessProperty, value); }
        }
    };
} } } } }