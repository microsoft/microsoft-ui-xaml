// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class DependencyLocatorUnitTests : public WEX::TestClass<DependencyLocatorUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(DependencyLocatorUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ValidateRegisterGetGlobal)
                TEST_METHOD_PROPERTY(L"Description", L"Validates we can successfully register an instance of a dependency.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRegisterGetPerThread)
                TEST_METHOD_PROPERTY(L"Description", L"Validates we can successfully register an instance of a dependency for per-thread lifetime.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSimultaneousThreadAndProcessProviders)
                TEST_METHOD_PROPERTY(L"Description", L"Validates per-process and per-thread lifetimes can coexist.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateArrowOperator)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the arrow operator works.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePerProcessDestructorCalled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that per-process destructors are called.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePerThreadDestructorCalled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that per-thread destructors are called.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateGetWithMultipleObjects)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that when multiple objects are registered the correct instances are returned.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateUniqueObjectPerThread)
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a single unique object is created for every thread.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNestedSerivceDependency)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that a service can resolve a subdependency at creation.")
            END_TEST_METHOD()
        };
    }
} } } }
