// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Styles {

    public enum class CustomEnumValues
    {
        CustomEnumValue0 = 0,
        CustomEnumValue1 = 1
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomControl sealed : public Microsoft::UI::Xaml::Controls::Control
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
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomFontFamilyProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomStyleProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomEnumProperty;

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
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomFontFamilyProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomFontFamilyProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomStyleProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomStyleProperty; }
        }
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomEnumProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomEnumProperty; }
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
        property Microsoft::UI::Xaml::Thickness CustomFontFamily
        {
            Microsoft::UI::Xaml::Thickness get() { return safe_cast<Microsoft::UI::Xaml::Thickness>(GetValue(CustomFontFamilyProperty)); }
            void set(Microsoft::UI::Xaml::Thickness value) { SetValue(CustomFontFamilyProperty, value); }
        }
        property Microsoft::UI::Xaml::Style^ CustomStyle
        {
            Microsoft::UI::Xaml::Style^ get() { return safe_cast<Microsoft::UI::Xaml::Style^>(GetValue(CustomStyleProperty)); }
            void set(Microsoft::UI::Xaml::Style^ value) { SetValue(CustomStyleProperty, value); }
        }
        property CustomEnumValues CustomEnum
        {
            CustomEnumValues get() { return safe_cast<CustomEnumValues>(GetValue(CustomEnumProperty)); }
            void set(CustomEnumValues value) { SetValue(CustomEnumProperty, value); }
        }

        property double NotDp;

        Microsoft::UI::Xaml::DependencyObject^ PublicGetTemplateChild(Platform::String^ name)
        {
            return GetTemplateChild(name);
        }
    };
} } } } }