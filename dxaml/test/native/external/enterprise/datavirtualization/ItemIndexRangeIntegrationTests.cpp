// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemIndexRangeIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    bool ItemIndexRangeIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ItemIndexRangeIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ItemIndexRangeIntegrationTests::ValidateFirstIndex()
    {
        RunOnUIThread([&]()
        {
            xaml_data::ItemIndexRange^ itemIndexRange = ref new xaml::Data::ItemIndexRange(1, 5);
            VERIFY_ARE_EQUAL(itemIndexRange->FirstIndex, 1);
        });
    }

    void ItemIndexRangeIntegrationTests::ValidateLength()
    {
        RunOnUIThread([&]()
        {
            xaml_data::ItemIndexRange^ itemIndexRange = ref new xaml::Data::ItemIndexRange(0, 5);
            VERIFY_ARE_EQUAL(itemIndexRange->Length, (unsigned int)5);
        });
    }

    void ItemIndexRangeIntegrationTests::ValidateLastIndex()
    {
        RunOnUIThread([&]()
        {
            xaml_data::ItemIndexRange^ itemIndexRange = ref new xaml::Data::ItemIndexRange(0, 5);
            VERIFY_ARE_EQUAL(itemIndexRange->LastIndex, 4);
        });
    }

    void ItemIndexRangeIntegrationTests::CanCreateAndUseOffUIThread()
    {
        xaml_data::ItemIndexRange^ itemIndexRange = ref new xaml::Data::ItemIndexRange(0, 5);

        VERIFY_ARE_EQUAL(itemIndexRange->FirstIndex, 0);
        VERIFY_ARE_EQUAL(itemIndexRange->Length, (unsigned int)5);
        VERIFY_ARE_EQUAL(itemIndexRange->LastIndex, 4);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::DataVirtualization