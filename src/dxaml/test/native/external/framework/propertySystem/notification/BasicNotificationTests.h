// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace External { namespace Frameworks { namespace PropertySystem {
    namespace Notification {
        class NotificationRecord
        {
        public:
            NotificationRecord()
                : handler(nullptr)
                , sender(nullptr)
                , prop(nullptr)
                , value(nullptr) {}

            Platform::String^   handler;
            Platform::String^   sender;
            Platform::String^   prop;
            Platform::String^   value;
        };

        class BasicNotificationTests : public WEX::TestClass<BasicNotificationTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicNotificationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(DependencyNotificationTests)
                TEST_METHOD_PROPERTY(L"Description", L"Validates adding/removing/accessing DO notifications")
            END_TEST_METHOD()
        };
    }
} } } } } } }
