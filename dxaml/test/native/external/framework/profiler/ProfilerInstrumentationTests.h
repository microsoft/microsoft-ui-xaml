// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#ifndef WINUI_PROFILER_STANDALONE
#include <wex.common.h>
#include <wextestclass.h>

class ProfilerInstrumentationTests
{
public:
    BEGIN_TEST_CLASS(ProfilerInstrumentationTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Unit")
    END_TEST_CLASS()
    TEST_METHOD(ProtocolAndSequence)
};
#endif
