// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <UIAutomation.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Flyout {


    class FlyoutAutomationIntegrationTests : public WEX::TestClass<FlyoutAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(FlyoutAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;e7de4cca-1436-4030-80b9-56ef01aa1cae;b34da8d2-333d-40a9-a19c-94b1f9785580")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for Flyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUIAFocusEntersFlyoutWithContentControl)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a flyout with a ContentControl as its content still captures UIA focus.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore, WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesFlyoutSetIsDialogPropertyToTrueByDefault)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the default IsDialog property from flyout dialog's popup is set to true")
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // Requires IUIAutomationElement9 added in RS5
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationNameAndIdPropagateToPresenter)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a flyout with an Automation ID and name propagate them to the presenter.")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationNameAndIdDoNotOverwritePresenterValues)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a flyout with an Automation ID and name does not propagate them to the presenter if they already have values.")
            TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

    private:
        void VerifyFlyoutAutomationName(xaml_controls::Flyout^ flyout, xaml::FrameworkElement^ target, const WCHAR *expectedName);

        static WCHAR s_automationName[];
        static WCHAR s_automationId[];
        static WCHAR s_automationItemStatus[];
        static CONTROLTYPEID s_automationTypeId;

    };

} } } } } }
