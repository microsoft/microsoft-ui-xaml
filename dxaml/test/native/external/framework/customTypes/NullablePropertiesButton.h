// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework {

    [Microsoft::UI::Xaml::Data::Bindable]
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class NullablePropertiesButton sealed
        : public Microsoft::UI::Xaml::Controls::Button
    {
    internal:
        static void InitDPs();
        static void ResetDPs();

    public:
        property Platform::IBox<double>^ NullableDouble
        {
            Platform::IBox<double>^ get() { return (Platform::IBox<double>^)GetValue(s_nullableDoubleProperty); }
            void set(Platform::IBox<double>^ value) { SetValue(s_nullableDoubleProperty, value); }
        }

    private:
        static Microsoft::UI::Xaml::DependencyProperty^ s_nullableDoubleProperty;
    };

} } } }