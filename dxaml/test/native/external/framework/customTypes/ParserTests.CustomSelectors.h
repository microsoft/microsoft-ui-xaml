// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Parser {
    namespace Selectors
    {
        // Unsealed so it can be used as an x:Class root element and as an intermediate base class.
        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomListViewBase
            : public Microsoft::UI::Xaml::Controls::ListView
        {
        internal:
            CustomListViewBase() {}
        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomListView sealed
            : public CustomListViewBase
        {
        public:
            CustomListView() {}
        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomGridView sealed
            : public Microsoft::UI::Xaml::Controls::GridView
        {
        public:
            CustomGridView() {}
        };

        [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
        [Microsoft::UI::Xaml::Data::BindableAttribute]
        public ref class CustomComboBox sealed
            : public Microsoft::UI::Xaml::Controls::ComboBox
        {
        public:
            CustomComboBox() {}
        };
    }
} } } } }
