// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
namespace Tests { namespace Tools { namespace Shared {

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class TestDataTemplateSelector sealed : public Microsoft::UI::Xaml::Controls::DataTemplateSelector
    {
    public:
        TestDataTemplateSelector()
        {

        }
        property Microsoft::UI::Xaml::DataTemplate^ ItemsGridTemplate
        { 
            Microsoft::UI::Xaml::DataTemplate^ get()
            {
                return _itemsGridTemplate;
            }; 
            void set(Microsoft::UI::Xaml::DataTemplate^ value)
            {
                _itemsGridTemplate = value;
            } 
        }
        property Microsoft::UI::Xaml::DataTemplate^ ItemsListTemplate
        { 
            Microsoft::UI::Xaml::DataTemplate^ get()
            {
                return _itemsListTemplate;
            }; 
            
            void set(Microsoft::UI::Xaml::DataTemplate^ value)
            {
                _itemsListTemplate = value;
            } 
        } 

        Microsoft::UI::Xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item) override
        {
            return _itemsListTemplate ? _itemsListTemplate : _itemsGridTemplate;
        }

        Microsoft::UI::Xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item, Microsoft::UI::Xaml::DependencyObject^ container) override
        {
            return SelectTemplateCore(item);
        }

    private:
        Microsoft::UI::Xaml::DataTemplate^ _itemsGridTemplate;
        Microsoft::UI::Xaml::DataTemplate^ _itemsListTemplate;
    }; // public ref class TestDataTemplateSelector;
} } }