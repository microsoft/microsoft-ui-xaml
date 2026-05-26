// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"
#include "FakeDXamlCore.h"


namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {
    class LifetimeUnitTests
    {
    public:
        BEGIN_TEST_CLASS(LifetimeUnitTests)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        BEGIN_TEST_METHOD(CanInstantiateLifetimeObjects)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can instantiate lifetime objects.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanWalkReferenceTrackers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can walk reference tracker objects.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(LockingTest)
            TEST_METHOD_PROPERTY(L"Description", L"Description")
        END_TEST_METHOD()

    private:

        DirectUI::FakeDXamlCore* m_dxamlCore;

    };

} } } } }
