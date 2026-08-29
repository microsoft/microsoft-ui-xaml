// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Tools { namespace Shared {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class IconConverter sealed : public Microsoft::UI::Xaml::Data::IValueConverter
    {
    public:
        virtual Platform::Object^ Convert(Platform::Object^ value, ::Windows::UI::Xaml::Interop::TypeName targetType, Platform::Object^ parameter, Platform::String^ language);
        virtual Platform::Object^ ConvertBack(Platform::Object^ value, ::Windows::UI::Xaml::Interop::TypeName targetType, Platform::Object^ parameter, Platform::String^ language);
    };
} } }