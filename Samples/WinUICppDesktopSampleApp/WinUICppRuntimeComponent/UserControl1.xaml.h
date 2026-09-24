// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "UserControl1.g.h"

namespace winrt::WinUICppRuntimeComponent::implementation
{
    struct UserControl1 : UserControl1T<UserControl1>
    {
        UserControl1();
    };
}

namespace winrt::WinUICppRuntimeComponent::factory_implementation
{
    struct UserControl1 : UserControl1T<UserControl1, implementation::UserControl1>
    {
    };
}
