#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "UserControl.g.h"

namespace winrt::IslandSimple::implementation
{
    struct UserControl : UserControlT<UserControl>
    {
        UserControl();

        int32_t MyProperty();
        void MyProperty(int32_t value);

      
    };
}

namespace winrt::IslandSimple::factory_implementation
{
    struct UserControl : UserControlT<UserControl, implementation::UserControl>
    {
    };
}
