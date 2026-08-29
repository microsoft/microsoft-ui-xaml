// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Styles {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DefaultStyleResourceUriControl sealed : public Microsoft::UI::Xaml::Controls::Control
    {
    public:
        DefaultStyleResourceUriControl();
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DefaultStyleResourceUriControlWithEmptyDefaultStyleKey sealed : public Microsoft::UI::Xaml::Controls::Control
    {
    public:
        DefaultStyleResourceUriControlWithEmptyDefaultStyleKey();
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DefaultStyleResourceUriControlWithInvalidDefaultStyleKey sealed : public Microsoft::UI::Xaml::Controls::Control
    {
    public:
        DefaultStyleResourceUriControlWithInvalidDefaultStyleKey();
    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class InvalidDefaultStyleResourceUriControl sealed : public Microsoft::UI::Xaml::Controls::Control
    {
    public:
        InvalidDefaultStyleResourceUriControl();
    };

} } } } }
