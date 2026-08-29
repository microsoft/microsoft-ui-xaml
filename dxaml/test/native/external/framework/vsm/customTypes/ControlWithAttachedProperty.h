// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Vsm {
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class ControlWithAttachedProperty sealed : public Microsoft::UI::Xaml::Controls::Control
    {
    public:
        static void RegisterDependencyProperties();
        static void ClearDependencyProperties();
        
    private:
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomAttachedProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomAttachedIntProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomAttachedBrushProperty;
        static Microsoft::UI::Xaml::DependencyProperty^ m_CustomAttachedBrushCollectionProperty;

    public:
        static property Microsoft::UI::Xaml::DependencyProperty^ CustomAttachedProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomAttachedProperty; }
        }

        static void SetCustomAttached(Microsoft::UI::Xaml::Controls::Control^ target, Platform::String^ value)
        {
            target->SetValue(CustomAttachedProperty, value);
        }
        static Platform::String^ GetCustomAttached(Microsoft::UI::Xaml::Controls::Control^ target)
        {
            return safe_cast<Platform::String^>(target->GetValue(CustomAttachedProperty));
        }

        static property Microsoft::UI::Xaml::DependencyProperty^ CustomAttachedIntProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomAttachedIntProperty; }
        }

        static void SetCustomAttachedInt(Microsoft::UI::Xaml::Controls::Control^ target, int value)
        {
            target->SetValue(CustomAttachedIntProperty, value);
        }
        static int GetCustomAttachedInt(Microsoft::UI::Xaml::Controls::Control^ target)
        {
            return safe_cast<int>(target->GetValue(CustomAttachedIntProperty));
        }

        static property Microsoft::UI::Xaml::DependencyProperty^ CustomAttachedBrushProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomAttachedBrushProperty; }
        }

        static void SetCustomAttachedBrush(Microsoft::UI::Xaml::Controls::Control^ target, Microsoft::UI::Xaml::Media::Brush^ value)
        {
            target->SetValue(CustomAttachedBrushProperty, value);
        }
        static Microsoft::UI::Xaml::Media::Brush^ GetCustomAttachedBrush(Microsoft::UI::Xaml::Controls::Control^ target)
        {
            return safe_cast<Microsoft::UI::Xaml::Media::Brush^>(target->GetValue(CustomAttachedBrushProperty));
        }

        static property Microsoft::UI::Xaml::DependencyProperty^ CustomAttachedBrushCollectionProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_CustomAttachedBrushCollectionProperty; }
        }

        static void SetCustomAttachedBrushCollection(Microsoft::UI::Xaml::Controls::Control^ target, Microsoft::UI::Xaml::Media::BrushCollection^ value)
        {
            target->SetValue(CustomAttachedBrushCollectionProperty, value);
        }
        static Microsoft::UI::Xaml::Media::BrushCollection^ GetCustomAttachedBrushCollection(Microsoft::UI::Xaml::Controls::Control^ target)
        {
            return safe_cast<Microsoft::UI::Xaml::Media::BrushCollection^>(target->GetValue(CustomAttachedBrushCollectionProperty));
        }
    };
} } } } }