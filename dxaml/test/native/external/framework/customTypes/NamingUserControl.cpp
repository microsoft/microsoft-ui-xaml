// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NamingUserControl.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Naming {


    NamingUserControl::NamingUserControl()
    {
        Application::LoadComponent(
            this,
            ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/framework/naming/NamingUserControl.xaml"),
            Primitives::ComponentResourceLocation::Application);
    }

}}}}}

