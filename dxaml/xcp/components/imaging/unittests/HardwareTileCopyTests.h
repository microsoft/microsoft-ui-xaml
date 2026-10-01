// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
namespace Foundation { namespace Imaging {

class HardwareTileCopyTests : public WEX::TestClass<HardwareTileCopyTests>
{
public:
    BEGIN_TEST_CLASS(HardwareTileCopyTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)

    TEST_METHOD(BelowThresholdUsesHeap)
    TEST_METHOD(AtThresholdReleasesScratch)
    TEST_METHOD(AboveThresholdReleasesScratch)
    TEST_METHOD(FinalStripReleasesOriginalAllocation)
    TEST_METHOD(VirtualTilesPreservePixels)
    TEST_METHOD(CopyPixelsFailureReleasesScratch)
    TEST_METHOD(LockFailureReleasesScratch)
};

} } } } } }
