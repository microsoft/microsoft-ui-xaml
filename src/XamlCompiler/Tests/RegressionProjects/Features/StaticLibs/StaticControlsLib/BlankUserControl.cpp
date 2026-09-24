// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "BlankUserControl.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::StaticControlsLib::implementation
{
    BlankUserControl::BlankUserControl()
    {
        InitializeComponent();
    }

    int32_t BlankUserControl::Dummy()
    {
        throw hresult_not_implemented();
    }

    void BlankUserControl::Dummy(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void BlankUserControl::ClickHandler(IInspectable const&, RoutedEventArgs const&)
    {
        Button().Content(box_value(L"Clicked"));
    }
}
