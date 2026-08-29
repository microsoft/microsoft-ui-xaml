// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace RangeSelection {

    class RangeSelectionIntegrationTests : public WEX::TestClass < RangeSelectionIntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(RangeSelectionIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")

            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(ValidateSelectedRangesAfterSelectedIndexChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedRanges functions properly after the SelectedIndex property has changed")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedRangesAfterSelectedItemReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedRanges functions properly after a selected item is clicked and dragged into a different location")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectAll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectAll functions properly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectRange functions properly")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDeselectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that DeselectRange functions properly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRangeSelectUnrealizedItemsWithShift)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can select a range with shift for which some items might not be realized.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        xaml_controls::ListView^ SetupEnvironment(const unsigned int size);
    };

} } } } } }
