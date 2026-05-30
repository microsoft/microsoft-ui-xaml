// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PickerFlyout {

    class PickerFlyoutIntegrationTests : public WEX::TestClass<PickerFlyoutIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PickerFlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"2b4b21d5-e5c0-4fad-bed4-62030fe191a1;eadeac67-1552-4876-a67e-dc1fcb8a6e25;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a PickerFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanClickPickerFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the launch PickerFlyout from the button.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenAndClose)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the open and close PickerFlyout.")
        END_TEST_METHOD()

    private:
        xaml_controls::Button^ SetupPickerFlyoutTest();
        wf::IAsyncOperation<bool>^ OpenPickerFlyout(xaml_controls::Button^ button, xaml_controls::PickerFlyout^ pickerFlyout);
        void ClosePickerFlyout(xaml_controls::PickerFlyout^ pickerFlyout, bool doClickConfirmationButton);
    };


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PickerFlyout
