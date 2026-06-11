// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class Interactivity sealed : public Microsoft::UI::Xaml::DependencyObject
    {
    public:
        static void RegisterDependencyProperties();
        static void ClearDependencyProperties();

    private:
        static Microsoft::UI::Xaml::DependencyProperty^ m_BehaviorsProperty;

    public:
        static property Microsoft::UI::Xaml::DependencyProperty^ BehaviorsProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_BehaviorsProperty; }
        }
        static void SetBehaviors(Microsoft::UI::Xaml::DependencyObject^ target, Microsoft::UI::Xaml::DependencyObjectCollection^ value)
        {
            target->SetValue(m_BehaviorsProperty, value);
        }
        static Microsoft::UI::Xaml::DependencyObjectCollection^ GetBehaviors(Microsoft::UI::Xaml::DependencyObject^ target)
        {
            auto value = static_cast<Microsoft::UI::Xaml::DependencyObjectCollection^>(target->GetValue(m_BehaviorsProperty));
            if (value == nullptr)
            {
                value = ref new Microsoft::UI::Xaml::DependencyObjectCollection();
                target->SetValue(m_BehaviorsProperty, value);
            }
            return value;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class MyBehavior sealed : public Microsoft::UI::Xaml::DependencyObject
    {
    public:
        static void RegisterDependencyProperties();
        static void ClearDependencyProperties();
        
    private:
        static Microsoft::UI::Xaml::DependencyProperty^ m_ActionsProperty;

    public:
        static property Microsoft::UI::Xaml::DependencyProperty^ ActionsProperty
        {
            Microsoft::UI::Xaml::DependencyProperty^ get() { return m_ActionsProperty; }
        }

        property Microsoft::UI::Xaml::DependencyObjectCollection^ Actions
        {
            Microsoft::UI::Xaml::DependencyObjectCollection^ get()
            {
                auto value = static_cast<Microsoft::UI::Xaml::DependencyObjectCollection^>(GetValue(m_ActionsProperty));
                if (value == nullptr)
                {
                    value = ref new Microsoft::UI::Xaml::DependencyObjectCollection();
                    SetValue(m_ActionsProperty, value);
                }
                return value;
            }
        }
    };

} } } }