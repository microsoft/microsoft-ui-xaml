// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace GraphicsResourceDiagnosticsTests
{
    class DiagnosticsTests
    {
        BEGIN_TEST_CLASS(DiagnosticsTests)
        END_TEST_CLASS()

        TEST_METHOD(CounterSnapshotContract);
        TEST_METHOD(MemorySnapshotContract);
        TEST_METHOD(MemorySnapshotValues);
    };
}
