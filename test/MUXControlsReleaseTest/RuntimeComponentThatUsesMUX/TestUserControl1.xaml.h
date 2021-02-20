#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "TestUserControl1.g.h"

namespace winrt::RuntimeComponentThatUsesMUX::implementation
{
    struct TestUserControl1 : TestUserControl1T<TestUserControl1>
    {
        TestUserControl1();
    };
}

namespace winrt::RuntimeComponentThatUsesMUX::factory_implementation
{
    struct TestUserControl1 : TestUserControl1T<TestUserControl1, implementation::TestUserControl1>
    {
    };
}
