// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace RenderingBackendTests
{
    // GPU paint time (W3). Device tests run on WARP, which supports timestamp queries, so they run on GPU-less machines.
    class GpuTimestampProfilerTests
    {
        BEGIN_TEST_CLASS(GpuTimestampProfilerTests)
        END_TEST_CLASS()

        TEST_METHOD(TicksConvertToMicroseconds);
        TEST_METHOD(UnusableIntervalsAreRejected);
        TEST_METHOD(MeasuresClearOnWarp);
        TEST_METHOD(FullRingDropsOldestInsteadOfBlocking);
        TEST_METHOD(UnbalancedCallsAreTolerated);
        TEST_METHOD(UninitializedProfilerIsInert);
    };
}
