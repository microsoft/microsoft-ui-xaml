// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "IItemsRangeInfoIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    bool IItemsRangeInfoIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool IItemsRangeInfoIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void IItemsRangeInfoIntegrationTests::CanInvokeDataSourceRangesChangedWhenScrollingToItem()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();

        ItemsRangeInfoStatics::Reset();

        RunOnUIThread([&]()
        {
            listView->ScrollIntoView(m_DataSource->GetAt(20));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ItemsRangeInfoStatics::RangesChangedInvoked);
    }

    void IItemsRangeInfoIntegrationTests::CanInvokeDataSourceRangesChangedWhenFlicking()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();

        ItemsRangeInfoStatics::Reset();

        TestServices::InputHelper->Flick(listView, FlickDirection::North);

        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ItemsRangeInfoStatics::RangesChangedInvoked);
    }

    void IItemsRangeInfoIntegrationTests::CanInvokeDataSourceRangesChangedWithKeyboardNavigation()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();
        int downPresses = 5;

        RunOnUIThread([&]()
        {
            listView->Focus(FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        ItemsRangeInfoStatics::Reset();

        // pressing down multiple times so we scroll through the items
        while(downPresses-- >= 0)
        {
            TestServices::KeyboardHelper->Down();
        }

        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ItemsRangeInfoStatics::RangesChangedInvoked);
    }

    void IItemsRangeInfoIntegrationTests::CanInvokeDataSourceRangesChangedWhenItemsSourceChanges()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();
        auto newDataSource = ref new Microsoft::UI::Xaml::Tests::Common::DataVirtualizationDataSource(500);

        ItemsRangeInfoStatics::Reset();

        RunOnUIThread([&]()
        {
            listView->ItemsSource = newDataSource;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(ItemsRangeInfoStatics::RangesChangedInvoked);
    }

    xaml_controls::ListView^ IItemsRangeInfoIntegrationTests::SetupEnvironment()
    {
        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            listView = dynamic_cast<xaml_controls::ListView^> (xaml_markup::XamlReader::Load(
                L"<ListView Height='200' Width='200' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));
            VERIFY_IS_NOT_NULL(listView);

            listView->ItemsSource = m_DataSource;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::DataVirtualization
