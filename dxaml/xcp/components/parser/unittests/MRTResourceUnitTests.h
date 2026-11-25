// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {

    class MRTResourceUnitTests
    {
    public:
        BEGIN_TEST_CLASS(MRTResourceUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(EmbeddedDataMemoryRetainsResource)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(RepeatedLoadsShareEmbeddedData)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StreamRetainsEmbeddedDataMemory)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ImageMemoryReleasesResourceOnWorkerThread)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ConcurrentMemoryReleases)
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(FailedLoadClearsOutput)
        END_TEST_METHOD()
    };

} } } } }
