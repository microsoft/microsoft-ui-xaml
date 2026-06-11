// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Controls { namespace Frame {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class FirstTestPage sealed : public Microsoft::UI::Xaml::Controls::Page
    {
    private:
        static Microsoft::UI::Xaml::Navigation::NavigationCacheMode m_CacheMode;
        static int m_Counter;

    public:
        static property Microsoft::UI::Xaml::Navigation::NavigationCacheMode CacheMode
        {
            Microsoft::UI::Xaml::Navigation::NavigationCacheMode get() { return m_CacheMode; }
            void set(Microsoft::UI::Xaml::Navigation::NavigationCacheMode value) { m_CacheMode = value; }
        }
        static property int Counter
        {
            int get() { return m_Counter; }
            void set(int value) { m_Counter = value; }
        }
        property int InstanceCounter;

        FirstTestPage();
    };

} } } } }