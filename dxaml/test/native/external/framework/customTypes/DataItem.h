// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::Bindable]
    public ref class DataItem sealed
    {
    public:
        DataItem(Platform::String^ title, Platform::String^ text);

        property Platform::String^ ButtonText
        {
            Platform::String^ get();
            void set(Platform::String^ value);
        }

        property Platform::String^ Title
        {
            Platform::String^ get();
            void set(Platform::String^ value);
        }

    protected private:
        Platform::String^ m_buttonText;
        Platform::String^ m_title;
    };
} } } }