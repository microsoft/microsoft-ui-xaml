// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "CustomControl1.h"
#if __has_include("CustomControl1.g.cpp")
#include "CustomControl1.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppRuntimeComponent::implementation
{
    CustomControl1::CustomControl1()
    {
        DefaultStyleKey(winrt::box_value(L"WinUICppRuntimeComponent.CustomControl1"));
    }

    int32_t CustomControl1::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void CustomControl1::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
