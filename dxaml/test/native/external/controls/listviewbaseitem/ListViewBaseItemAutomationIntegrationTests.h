// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseItem {

    class ListViewBaseItemAutomationIntegrationTests : public WEX::TestClass<ListViewBaseItemAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ListViewBaseItemAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;eadeac67-1552-4876-a67e-dc1fcb8a6e25")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyDefaultAutomationNameContent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we return a reasonable AutomationProperties.Name in the event the app developer does not specify one.")
        END_TEST_METHOD()
    };

} } } } } }
