// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AnotherBlankUserControl.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StaticLibInRuntimeComponent::implementation
{
    AnotherBlankUserControl::AnotherBlankUserControl()
    {
        InitializeComponent();
    }

    int32_t AnotherBlankUserControl::Dummy()
    {
        throw hresult_not_implemented();
    }

    void AnotherBlankUserControl::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void AnotherBlankUserControl::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
