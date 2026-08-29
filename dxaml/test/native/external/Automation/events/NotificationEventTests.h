// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace AutomationPeer {

    class NotificationEventTests : public WEX::TestClass<NotificationEventTests>
    {
    public:
        BEGIN_TEST_CLASS(NotificationEventTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(NotificationEventWithDisplayString)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the automation Notification event where we provide a non-empty displayString parameter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NotificationEventWithNullDisplayString)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the automation Notification event where we provide a null displayString parameter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(NotificationEventWithEmptyDisplayString)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the automation Notification event where we provide an empty displayString parameter.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAllEnumValues)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that all values from the NotificationKind and NotificationProcessing enums are correctly piped.")
        END_TEST_METHOD()

    private:
    };

} } } } } }
