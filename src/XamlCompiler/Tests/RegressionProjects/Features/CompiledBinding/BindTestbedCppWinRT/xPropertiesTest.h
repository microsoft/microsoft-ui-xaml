// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "xPropertiesTest.g.h"

namespace winrt::BindTestbed::implementation
{
    struct xPropertiesTest : xPropertiesTestT<xPropertiesTest>
    {
        xPropertiesTest();

        // Stands in for the x:Property declarations the C# and C++/CX versions of this page use;
        // see the comment in xPropertiesTest.xaml for why they cannot be expressed in markup here.
        hstring TestStr() { return testStr; }
        void TestStr(hstring const& value) { testStr = value; }

        BindTestbedModel::Diameter TestCreateFromString() { return testCreateFromString; }
        void TestCreateFromString(BindTestbedModel::Diameter const& value) { testCreateFromString = value; }

    private:
        hstring testStr{ L"default value" };
        BindTestbedModel::Diameter testCreateFromString{ nullptr };
    };
}

namespace winrt::BindTestbed::factory_implementation
{
    struct xPropertiesTest : xPropertiesTestT<xPropertiesTest, implementation::xPropertiesTest>
    {
    };
}
