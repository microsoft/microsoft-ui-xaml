// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace External { namespace Controls { namespace CustomTypes {

    // Custom types MUST be sealed and have a public constructor
    // if you'd like them to be activatable.
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class CustomListViewItemPanel sealed 
        : public Microsoft::UI::Xaml::Controls::Panel
    {
    public:
        CustomListViewItemPanel();

    protected:
        virtual ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size availableSize) override;
        virtual ::Windows::Foundation::Size ArrangeOverride(::Windows::Foundation::Size finalSize) override;

    private:
        Microsoft::UI::Xaml::Controls::TextBlock^ textBlock;
    };
}}}}