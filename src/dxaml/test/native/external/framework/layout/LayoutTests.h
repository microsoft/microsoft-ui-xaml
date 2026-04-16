// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layout {

class LayoutTests : public WEX::TestClass<LayoutTests>
{
public:
    BEGIN_TEST_CLASS(LayoutTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(LayoutRoundingIncludesMargins)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(SliderWithMarginNoLayoutCycle)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ProgressBarWithBorderNoLayoutCycle)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ScrollBarMarginRoundingNoLayoutCycle)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
    END_TEST_METHOD()

private:
    void LayoutRoundingIncludesMarginsCommon();
};

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout

