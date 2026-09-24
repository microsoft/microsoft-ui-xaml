// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "MyUserControl1.g.h"

namespace winrt::BindTestbed::implementation
{
    struct MyUserControl1 : MyUserControl1T<MyUserControl1>
    {
        MyUserControl1();

        hstring Property1()
        {
            return unbox_value<hstring>(GetValue(property1Property));
        }
        void Property1(hstring const& value)
        {
            SetValue(property1Property, box_value(value));
        }

        static wux::DependencyProperty Property1Property()
        {
            return property1Property;
        }
        static void Property1Property(wux::DependencyProperty value)
        {
            property1Property = value;
        }

        void aLazyTextBlock_Tapped(IInspectable const&, wux::Input::TappedRoutedEventArgs const&);

    private:
        static wux::DependencyProperty property1Property;
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct MyUserControl1 : MyUserControl1T<MyUserControl1, implementation::MyUserControl1>
    {
    };
}
