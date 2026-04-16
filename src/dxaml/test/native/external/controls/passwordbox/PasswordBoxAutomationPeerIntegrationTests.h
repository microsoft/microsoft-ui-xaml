// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

//  Copyright (c) Microsoft Corporation.  All rights reserved.



namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PasswordBox {

    class PasswordBoxAutomationPeerIntegrationTests : public WEX::TestClass<PasswordBoxAutomationPeerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PasswordBoxAutomationPeerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we return a reasonable AutomationProperties.Name in the event the app developer does not specify one.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPasswordBoxPlaceholderTextIsMovedToDescribedBy)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that placeholder text is moved to DescribedBy.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
    };

} } } } } }
