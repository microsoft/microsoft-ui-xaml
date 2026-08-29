// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CustomMetadataRegistrar.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace XamlUIABridge {

    class XamlUIABridgeIntegrationTests : public WEX::TestClass<XamlUIABridgeIntegrationTests>
    {

    public:
        BEGIN_TEST_CLASS(XamlUIABridgeIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dd3493a5-54ef-4337-93bc-89b726406385")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyUIAHitTesting)
            TEST_METHOD_PROPERTY(L"Description", L"Validates if UIA hit testing works for XAML UIA Bridge")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusedElement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates if UIA Focused Element works for XAML UIA Bridge")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates if UIA Navigation works for XAML UIA Bridge")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCustomNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates if UIA Custom Navigation works for XAML UIA Bridge")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPropertyChangedEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates UIA PropertyChangedEvents for XAML UIA Bridge")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateLandmarkTypeOnAutomationPeer)
            TEST_METHOD_PROPERTY(L"Description", L"Validates default LandmarkType properties for a pure AutomationPeer.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

    private:
        void SetupXAMLUIABridge();
    };

} } } } } }
