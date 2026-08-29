// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    class ComboBoxAutomationIntegrationTests : public WEX::TestClass<ComboBoxAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ComboBoxAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
            TEST_CLASS_PROPERTY(L"ThreadingModel", L"MTA")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)


        BEGIN_TEST_METHOD(ValidateSizeOfPropertiesForFaceplate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates \"Size of\" UIA properties with collapsed ComboBox")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDefaultAutomationNameFallback)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that automationname is properly surfaced through a set of fallback behaviors.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that ComboBoxAutomationPeer supports the Window Pattern when the ComboBox is open.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyClosedComboBoxReportsCorrectNameWithDisplayMemberPath)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that ComboBoxAutomationPeer reports the correct name when closed and DisplayMemberPath is set")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFaceplateContentPresenterAutomationPeerName)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that FaceplateContentPresenterAutomationPeer::GetNameCore reports the correct values when the ComboBox selection changes while closed.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyClosedComboBoxReportsCorrectAutomationNameWhenSet)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the ComboBoxAutomationPeer reports the correct name when one is set on the DataTemplate.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyComboBoxDoesNotStopHeaderNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that header navigation can occur across a ComboBox.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyEditableComboBoxHasTextBox)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ComboBox with IsEditable = true has a TextBox as a child in the UIA tree.")
        END_TEST_METHOD()

    private:
        void VerifySelectedUIAutomationElementNameMatchesExpectedString(Platform::String^ automationId, Platform::String^ expectedString);
    };

} } } } } }
