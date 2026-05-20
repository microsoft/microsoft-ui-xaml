// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {
    class WeakReferenceUnitTests
    {
    public:
        BEGIN_TEST_CLASS(WeakReferenceUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(Test_WeakReferenceOptimization)
            TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", L"22621")
        END_TEST_METHOD()

        TEST_METHOD(Test_WeakReferenceParallelAddReference)
        TEST_METHOD(Test_WeakReferenceParallelRelease)
        TEST_METHOD(Test_WeakReferenceParallelReleaseWithFinalRelease)
        TEST_METHOD(Test_WeakReferenceParallelCreateWeakReference)

        TEST_METHOD(Test_WeakReferenceCreateWeakReferenceAndAddRef)
        TEST_METHOD(Test_WeakReferenceCreateWeakReferenceAndRelease)
    };

} } } } }
