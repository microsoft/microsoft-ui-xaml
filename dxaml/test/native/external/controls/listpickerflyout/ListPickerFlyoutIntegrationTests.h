// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListPickerFlyout {

    class ListPickerFlyoutIntegrationTests : public WEX::TestClass<ListPickerFlyoutIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ListPickerFlyoutIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;e7de4cca-1436-4030-80b9-56ef01aa1cae;bdc5c68b-03c1-4217-832c-4c09f99946f4")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ListPickerFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify the properties on the ListPickerFlyout.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectAnItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that selecting a single item will fire the ItemsPicked event")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectMultipleItems)
            TEST_METHOD_PROPERTY(L"Description", L"Verify multiple items selection on ListPickerFlyout.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

    private:
        xaml_controls::ListPickerFlyout^ SetupListPickerFlyoutTest(bool isMultipleSelection, bool shouldLaunchFlyout);
        void ValidateSelectionState(xaml_controls::ListPickerFlyout^ listPickerFlyout, int expectedSelectedIndex);
    };


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListPickerFlyout
