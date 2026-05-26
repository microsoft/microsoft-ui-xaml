// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
namespace Tests { namespace Tools { namespace Shared {

    public enum class ThoughtProcess
    {
        Yes,
        No,
        MaybeSo
    };

[::Windows::Foundation::Metadata::WebHostHiddenAttribute]
[Microsoft::UI::Xaml::Data::BindableAttribute]
public ref class CustomUserControl sealed : public Microsoft::UI::Xaml::Controls::UserControl
{
public:
    static void RegisterDependencyProperties();
    static void ClearDependencyProperties();
    CustomUserControl();

private:
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomIntProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomDoubleProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomUnknownProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomUserCollectionProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomAttachedProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomBrushProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomChildProperty;
    static Microsoft::UI::Xaml::DependencyProperty^ m_CustomThoughtsProperty;
    
public:
    property Microsoft::UI::Xaml::DataTemplate^ UnregisteredDataTemplate;
  
    static property Microsoft::UI::Xaml::DependencyProperty^ CustomIntProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomIntProperty; }
    }

    property int CustomInt
    {
        int get() { return safe_cast<int>(GetValue(CustomIntProperty)); }
        void set(int value) { SetValue(CustomIntProperty, value); }
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomDoubleProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomDoubleProperty; }
    }

    property double CustomDouble
    {
        double get() { return safe_cast<int>(GetValue(CustomDoubleProperty)); }
        void set(double value) { SetValue(CustomDoubleProperty, value); }
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomUnknownProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomUnknownProperty; }
    }

    property Object^ CustomUnknown
    {
        Object^ get() { return GetValue(CustomUnknownProperty); }
        void set(Object^ value) { SetValue(CustomUnknownProperty, value); }
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomUserCollectionProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomUserCollectionProperty; }
    }

    property ::Windows::Foundation::Collections::IVector<DependencyObject^>^ CustomUserCollection
    {
        ::Windows::Foundation::Collections::IVector<DependencyObject^>^ get() { return safe_cast<::Windows::Foundation::Collections::IVector<DependencyObject^>^>(GetValue(CustomUserCollectionProperty)); }
        void set(::Windows::Foundation::Collections::IVector<DependencyObject^>^ value) { SetValue(CustomUserCollectionProperty, value); }
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomAttachedProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomAttachedProperty; }
    }

    static void SetCustomAttached(Microsoft::UI::Xaml::DependencyObject^ element, int value)
    {
        element->SetValue(CustomAttachedProperty, value);
    }

    static int GetCustomAttached(Microsoft::UI::Xaml::DependencyObject^ element)
    {
        return static_cast<int>(element->GetValue(CustomAttachedProperty));
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomBrushProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomBrushProperty; }
    }

    property Microsoft::UI::Xaml::Media::Brush^ CustomBrush
    {
        Microsoft::UI::Xaml::Media::Brush^ get() { return safe_cast<Microsoft::UI::Xaml::Media::Brush^>(GetValue(CustomBrushProperty)); }
        void set(Microsoft::UI::Xaml::Media::Brush^ value) { SetValue(CustomBrushProperty, value); }
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomChildProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomChildProperty; }
    }

    property CustomUserControl^ CustomChild
    {
        CustomUserControl^ get() { return safe_cast<CustomUserControl^>(GetValue(CustomChildProperty));}
        void set(CustomUserControl^ value) { SetValue(CustomChildProperty, value); }
    }

    static void SetCustomChild(Microsoft::UI::Xaml::DependencyObject^ element, CustomUserControl^ value)
    {
        element->SetValue(CustomChildProperty, value);
    }

    static CustomUserControl^ GetCustomChild(Microsoft::UI::Xaml::DependencyObject^ element)
    {
        return static_cast<CustomUserControl^>(element->GetValue(CustomChildProperty));
    }

    static property Microsoft::UI::Xaml::DependencyProperty^ CustomThoughtsProperty
    {
        Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomThoughtsProperty; }
    }

    property ThoughtProcess CustomThoughts
    {
        ThoughtProcess get() { return static_cast<ThoughtProcess>(GetValue(CustomThoughtsProperty));}
        void set(ThoughtProcess value) { SetValue(CustomThoughtsProperty, value);}
    }
    
    static void SetCustomThoughts(Microsoft::UI::Xaml::DependencyObject^ element, ThoughtProcess value)
    {
        element->SetValue(CustomThoughtsProperty, value);
    }

    static ThoughtProcess GetCustomThoughts(Microsoft::UI::Xaml::DependencyObject^ element)
    {
        return static_cast<ThoughtProcess>(element->GetValue(CustomThoughtsProperty));
    }

    property Microsoft::UI::Xaml::Media::SolidColorBrush^ CustomBrushNonDP;
    property double CustomDoubleNonDP;
};

} } }

