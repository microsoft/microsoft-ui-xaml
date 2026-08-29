// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace SimpleProperties {

        class SimplePropertiesUnitTests : public WEX::TestClass <SimplePropertiesUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(SimplePropertiesUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD(GettersSetters)
            TEST_METHOD(GetUnset)
            TEST_METHOD(SetToDefaultOnFirstSet)
            TEST_METHOD(SetToDefaultAfterNonDefaultSet)
            TEST_METHOD(MultipleSets)
            TEST_METHOD(RT_TypeMismatchThrows)
            TEST_METHOD(LifetimeControlledByHost)
            TEST_METHOD(RegistrationRefCounting)
            TEST_METHOD(ChangeNotificationsAlwaysNotify)
        };
    }
} } } }
