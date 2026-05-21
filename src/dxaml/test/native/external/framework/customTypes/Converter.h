// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace xaml_interop = ::Microsoft::UI::Xaml::Interop;
namespace xaml_media = ::Microsoft::UI::Xaml::Media;
namespace wxaml_interop = ::Windows::UI::Xaml::Interop;

using namespace Platform;

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class IdentityConverter sealed : public Microsoft::UI::Xaml::Data::IValueConverter
    {
        // If incoming value is a SolidColorBrush, then we'll copy its Color into a new Brush
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            auto valueAsBrush = dynamic_cast<xaml_media::SolidColorBrush^>(value);

            if (valueAsBrush != nullptr)
            {
                return ref new xaml_media::SolidColorBrush(valueAsBrush->Color);
            }

            return value;
        }

        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return value;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class ThrowingConverter sealed : public Microsoft::UI::Xaml::Data::IValueConverter
    {
        // Converter that always throws
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            throw ref new Platform::InvalidCastException;
        }

        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            throw ref new Platform::FailureException;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class NullConverter sealed : public Microsoft::UI::Xaml::Data::IValueConverter
    {
        // Converter that always returns null
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return nullptr;
        }

        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return nullptr;
        }
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DoubleConverter sealed : public Microsoft::UI::Xaml::Data::IValueConverter
    {
    public:
        virtual Object^ Convert(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return (int)value * 2;
        }

        virtual Object^ ConvertBack(Object^ value, wxaml_interop::TypeName targetType, Object^ parameter, String^ language)
        {
            return value;
        }
    };
} } } }