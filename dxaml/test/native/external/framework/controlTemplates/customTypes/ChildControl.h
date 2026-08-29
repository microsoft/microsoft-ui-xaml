// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CustomControl.h"

namespace Tests { namespace Native { namespace External { namespace Framework { namespace ControlTemplates {
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class ChildControl : public AbstractCustomControl
    {

    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class GrandchildControl : public ChildControl
    {

    };

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class OtherCustomControl : public Microsoft::UI::Xaml::Controls::Control
    {

    };

} } } } }