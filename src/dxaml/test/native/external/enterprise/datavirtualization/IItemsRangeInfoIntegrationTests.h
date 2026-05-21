// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include "DataVirtualizationDataSourceHelper.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    class IItemsRangeInfoIntegrationTests : public WEX::TestClass<IItemsRangeInfoIntegrationTests>
    {
    public:
        IItemsRangeInfoIntegrationTests()
        {
            m_DataSource = ref new Microsoft::UI::Xaml::Tests::Common::DataVirtualizationDataSource(1000);
        }

        BEGIN_TEST_CLASS(IItemsRangeInfoIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInvokeDataSourceRangesChangedWhenScrollingToItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the RangesChanged method for a ListView when the list scrolls to a given item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInvokeDataSourceRangesChangedWhenFlicking)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the RangesChanged method for a ListView when flicking through the list.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInvokeDataSourceRangesChangedWhenItemsSourceChanges)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the RangesChanged method for a ListView when the ItemsSource changes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanInvokeDataSourceRangesChangedWithKeyboardNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke the RangesChanged method for a ListView when the keyboard navigation is used.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //
        
        //
        // Platform:Phone
        //

    private:
        Microsoft::UI::Xaml::Tests::Common::DataVirtualizationDataSource^ m_DataSource;
        xaml_controls::ListView^ SetupEnvironment();
    };

} } } } } }
