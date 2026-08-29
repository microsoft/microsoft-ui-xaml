// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "pch.h"

namespace BDTS{

    [wf::Metadata::WebHostHidden]
    public ref class BasicDataTemplateSelector sealed : public xaml_controls::DataTemplateSelector
    {
        public:
            property xaml::DataTemplate^ TemplateWithButton;
            
            xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item) override
            {
                return TemplateWithButton;
            }

             xaml::DataTemplate^ SelectTemplateCore(Platform::Object^ item, xaml::DependencyObject^ container) override
            {
                return TemplateWithButton;
            }
    };

}

