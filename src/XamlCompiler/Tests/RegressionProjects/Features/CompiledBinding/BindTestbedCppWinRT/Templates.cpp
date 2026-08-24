// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Templates.h"
#include "Templates.g.cpp"
#include "DetectLeaksPage.h"

namespace winrt::BindTestbed::implementation
{
    Templates::Templates()
    {
        InitializeComponent();
        DetectLeaksPage::TrackObject(*this, xaml_typename<BindTestbed::Templates>().Name);
    }
}