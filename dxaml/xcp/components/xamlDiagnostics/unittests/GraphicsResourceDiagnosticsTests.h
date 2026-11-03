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

        TEST_METHOD_SETUP(Setup);
        TEST_METHOD_CLEANUP(Cleanup);

        TEST_METHOD(ResourceSnapshotContract);
        TEST_METHOD(ResourceSnapshotValues);
        TEST_METHOD(RuntimeDisabledSnapshot);

    private:
        bool m_wasProfilingEnabled = false;
    };
}
