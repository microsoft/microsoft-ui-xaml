// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TreeViewTestPage.g.h"

namespace TestAppCX
{
    public ref class TreeViewData sealed
    {
    private:
        Platform::String^ m_content;
        Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ m_children;

    public:
        TreeViewData(Platform::String^ content) : m_content(content)
        {
            m_content = content;
            m_children = ref new Platform::Collections::Vector<TreeViewData^>();
        }

        property Platform::String^ Content
        {
            Platform::String^ get() { return m_content; }
        }

        property Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ Children
        {
            Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ get() { return m_children; }
        }

    };

    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class TreeViewTestPage sealed
    {

    private:
        Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ m_items = ref new Platform::Collections::Vector<TreeViewData^>();

    public:
        property Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ Items
        {
            Windows::Foundation::Collections::IObservableVector<TreeViewData^>^ get() { return m_items; }
        }
        TreeViewTestPage();
    private:
        void ReplaceAll_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void Clear_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    };

}
