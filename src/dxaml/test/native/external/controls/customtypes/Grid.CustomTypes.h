// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private { namespace Tests {
    namespace Controls {
    namespace GridIntegrationTests {

        using namespace Microsoft::UI::Xaml;
        using namespace Platform;
        using namespace Microsoft::UI::Xaml::Controls;

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomGrid sealed : public Microsoft::UI::Xaml::Controls::Grid
        {
            public:
                CustomGrid() {};

                ::Windows::Foundation::Size ArrangeOverride(
                    ::Windows::Foundation::Size finalSize
                ) override
                {
                    return FrameworkElement::ArrangeOverride(finalSize);
                };
        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomTestControl sealed : public Microsoft::UI::Xaml::Controls::Control
        {
        private:
            Microsoft::UI::Xaml::Controls::ColumnDefinition^ columndef;
            Microsoft::UI::Xaml::Controls::RowDefinition^ rowdef;
            Microsoft::UI::Xaml::Controls::ColumnDefinitionCollection^ columndefs;
            Microsoft::UI::Xaml::Controls::RowDefinitionCollection^ rowdefs;

        public:
            CustomTestControl() {};
            property Microsoft::UI::Xaml::Controls::ColumnDefinition^ ColumnDef
            {
                Microsoft::UI::Xaml::Controls::ColumnDefinition^ get() { return columndef;  }
                void set(Microsoft::UI::Xaml::Controls::ColumnDefinition^ value)
                {
                    columndef = value;
                }
            }
            property Microsoft::UI::Xaml::Controls::RowDefinition^ RowDef
            {
                Microsoft::UI::Xaml::Controls::RowDefinition^ get() { return rowdef; }
                void set(Microsoft::UI::Xaml::Controls::RowDefinition^ value)
                {
                    rowdef = value;
                }
            }
            property Microsoft::UI::Xaml::Controls::ColumnDefinitionCollection^ ColumnDefs
            {
                Microsoft::UI::Xaml::Controls::ColumnDefinitionCollection^ get() { return columndefs; }
                void set(Microsoft::UI::Xaml::Controls::ColumnDefinitionCollection^ value)
                {
                    columndefs = value;
                }
            }
            property Microsoft::UI::Xaml::Controls::RowDefinitionCollection^ RowDefs
            {
                Microsoft::UI::Xaml::Controls::RowDefinitionCollection^ get() { return rowdefs; }
                void set(Microsoft::UI::Xaml::Controls::RowDefinitionCollection^ value)
                {
                    rowdefs = value;
                }
            }
        };

    }
} } }
