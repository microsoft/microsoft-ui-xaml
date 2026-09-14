// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <HostingModeTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

class LayoutTests : public WEX::TestClass<LayoutTests>
{
public:
    BEGIN_TEST_CLASS(LayoutTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_HOSTING_MODE_DEFAULT()
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(LayoutRoundingIncludesMargins)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SliderWithMarginNoLayoutCycle)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProgressBarWithBorderNoLayoutCycle)
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScrollBarMarginRoundingNoLayoutCycle)
    END_TEST_METHOD()

private:
    void LayoutRoundingIncludesMarginsCommon();
};

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout

