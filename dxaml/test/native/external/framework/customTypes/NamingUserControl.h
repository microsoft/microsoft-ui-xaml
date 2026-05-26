// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace Tests { namespace Native { namespace External { namespace Framework { namespace Naming {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class NamingUserControl sealed : public Microsoft::UI::Xaml::Controls::UserControl
    {
    public:
        NamingUserControl();
    };

}}}}}

