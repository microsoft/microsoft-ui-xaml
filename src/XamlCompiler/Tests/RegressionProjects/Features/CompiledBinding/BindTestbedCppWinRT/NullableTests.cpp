// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "NullableTests.h"
#include "NullableTests.g.cpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::BindTestbed::implementation
{
    NullableTests::NullableTests()
    {
        InitializeComponent();
    }

    void NullableTests::UpdateValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().UpdateValues();
        DOModel().UpdateValues();
    }

    void NullableTests::ResetValuesClick(IInspectable const&, wux::RoutedEventArgs const&)
    {
        Model().InitializeValues();
        DOModel().UpdateValues();
    }
}
