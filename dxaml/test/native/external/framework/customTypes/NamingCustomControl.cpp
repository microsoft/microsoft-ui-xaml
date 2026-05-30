// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NamingCustomControl.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Naming {


    bool NamingCustomControl::Validate()
    {
        auto rootGrid = static_cast<Grid^>(GetTemplateChild("rootGrid"));        
        if( rootGrid == nullptr )
        {
            return false;
        }

        auto nestedGrid = static_cast<Grid^>(GetTemplateChild("nestedGrid"));        
        if( nestedGrid == nullptr )
        {
            return false;
        }

        nestedGrid = static_cast<Grid^>(rootGrid->FindName("nestedGrid"));
        if( nestedGrid == nullptr )
        {
            return false;
        }
        

        auto r = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
        r->Name = "Rectangle";
        rootGrid->Children->Append(r);

        r = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(GetTemplateChild("Rectangle"));        
        if( r != nullptr )
        {
            return false;
        }

        r = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootGrid->FindName("Rectangle"));        
        if( r != nullptr )
        {
            return false;
        }

        r = static_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(this->FindName("Rectangle"));        
        if( r == nullptr )
        {
            return false;
        }

        

        return true;
    }



}}}}}

