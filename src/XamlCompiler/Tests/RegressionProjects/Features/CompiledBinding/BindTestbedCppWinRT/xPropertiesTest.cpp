// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "xPropertiesTest.h"
#include "xPropertiesTest.g.cpp"

namespace winrt::BindTestbed::implementation
{
    xPropertiesTest::xPropertiesTest()
    {
        testCreateFromString = BindTestbedModel::Diameter::MakeNewDiameter(L"400").as<BindTestbedModel::Diameter>();
        InitializeComponent();
    }
}
