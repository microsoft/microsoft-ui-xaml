// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    class SchedulerUnitTests : public WEX::TestClass<SchedulerUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(SchedulerUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Unit")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_METHOD(ShouldWaitForVBlank);

        // A frame is needed immediately. The scheduler decides whether we need to throttle to the refresh rate. These
        // are tests for the algorithm.
        TEST_METHOD(OnImmediateUIThreadFrame_60Hz);
        TEST_METHOD(OnImmediateUIThreadFrame_120Hz);
        TEST_METHOD(OnImmediateUIThreadFrame_60Hzto120Hz);
    };

} } } } }
