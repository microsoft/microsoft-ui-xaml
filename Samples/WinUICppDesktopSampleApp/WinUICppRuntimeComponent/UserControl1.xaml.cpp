// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "UserControl1.xaml.h"
#if __has_include("UserControl1.g.cpp")
#include "UserControl1.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppRuntimeComponent::implementation
{
    UserControl1::UserControl1()
    {
        InitializeComponent();
    }
}
