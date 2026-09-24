// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "LonelyStaticBinding.h"
#include "LonelyStaticBinding.g.cpp"

namespace winrt::BindTestbed::implementation
{
    LonelyStaticBinding::LonelyStaticBinding()
    {
        InitializeComponent();
    }
}
