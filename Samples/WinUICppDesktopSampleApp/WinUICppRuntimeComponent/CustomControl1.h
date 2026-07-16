// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "CustomControl1.g.h"

namespace winrt::WinUICppRuntimeComponent::implementation
{
    struct CustomControl1 : CustomControl1T<CustomControl1>
    {
        CustomControl1();

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::WinUICppRuntimeComponent::factory_implementation
{
    struct CustomControl1 : CustomControl1T<CustomControl1, implementation::CustomControl1>
    {
    };
}
