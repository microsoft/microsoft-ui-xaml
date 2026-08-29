// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>
//  Copyright (c) Microsoft Corporation.  All rights reserved.


#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ContentDialog {

    class ContentDialogAutomationIntegrationTests : public WEX::TestClass<ContentDialogAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ContentDialogAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyDefaultAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Validates reasonable default AutomationProperties.Name for ContentDialog if developer does not specify a Name property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesSupportEssentialPatterns)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ContentDialog supports the Window UI automation pattern.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesPopupUseDialogsAutomationId)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the popup automation peer uses the ContentDialog's automation id.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesAutomationNameUpdateWithInMarkupDialogs)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ContentDialogs defined in markup will have their popup's automation name update when the title updates.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesContentDialogSetIsDialogPropertyToTrueByDefault)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the default IsDialog property from content dialog's popup is set to true")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // Requires IUIAutomationElement9 added in RS5
        END_TEST_METHOD()

    private:
        static wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ OpenContentDialog(xaml_controls::ContentDialog^ contentDialog);
        static void CloseContentDialog(xaml_controls::ContentDialog^ contentDialog);
        static xaml_controls::ContentDialog^ SetupContentDialog();
        static void VerifyContentDialogAutomationName(xaml_controls::ContentDialog^ contentDialog, const WCHAR *expectedName);

        static WCHAR s_contentDialogName[];
    };

} } } } } }
