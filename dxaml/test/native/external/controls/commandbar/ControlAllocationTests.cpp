// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ControlAllocationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <CustomTypeMetadataProvider.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

    bool ControlAllocationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool ControlAllocationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());
        return true;
    }

    bool ControlAllocationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ControlAllocationTests::MeasureCommandBarAllocations()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::CommandBar^ cmdBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto hasLoadedEvent = std::make_shared<Event>();
        auto hasUnloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Unloaded);

        // Get initial allocation counts
        UINT64 initialAllocCount = TestServices::WindowHelper->GetAllocationCount();
        UINT64 initialAllocSize = TestServices::WindowHelper->GetAllocationSize();
        UINT64 initialDeallocCount = TestServices::WindowHelper->GetDeallocationCount();

        LOG_OUTPUT(L"Initial allocation count: %llu, size: %llu bytes, deallocation count: %llu",
            initialAllocCount, initialAllocSize, initialDeallocCount);

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            cmdBar = safe_cast<xaml_controls::CommandBar^>(xaml_markup::XamlReader::Load(
                LR"(<CommandBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">
                        <AppBarToggleButton Label="btn1"/>
                        <AppBarButton Label="btn2"/>
                        <CommandBar.SecondaryCommands>
                            <AppBarToggleButton Label="btn3"/>
                            <AppBarButton Label="btn4"/>
                        </CommandBar.SecondaryCommands>
                    </CommandBar>)"));

            loadedRegistration.Attach(cmdBar, [&]() { hasLoadedEvent->Set(); });
            unloadedRegistration.Attach(cmdBar, [&]() { hasUnloadedEvent->Set(); });
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            page->BottomAppBar = cmdBar;
        });
        hasLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Get allocation counts after creating CommandBar
        const UINT64 afterCreateAllocCount = TestServices::WindowHelper->GetAllocationCount();
        const UINT64 afterCreateAllocSize = TestServices::WindowHelper->GetAllocationSize();
        const UINT64 afterCreateDeallocCount = TestServices::WindowHelper->GetDeallocationCount();

        const UINT64 expectedAllocationCount = 46100;

        LOG_OUTPUT(L"After CommandBar creation:");
        LOG_OUTPUT(L"  Allocation count: %llu (expected: %llu)", afterCreateAllocCount, expectedAllocationCount);
        LOG_OUTPUT(L"  Allocation size: %llu bytes", afterCreateAllocSize);
        LOG_OUTPUT(L"  Deallocation count: %llu", afterCreateDeallocCount);
        LOG_OUTPUT(L"  Total allocations due to CommandBar creation: %lld", 
            static_cast<INT64>(afterCreateAllocCount - initialAllocCount));
        LOG_OUTPUT(L"  Outstanding allocations due to CommandBar creation: %lld", 
            static_cast<INT64>(afterCreateAllocCount - initialAllocCount) - static_cast<INT64>(afterCreateDeallocCount - initialDeallocCount));

#ifdef DBG
        VERIFY_IS_GREATER_THAN(
            expectedAllocationCount + 50 /*buffer for minor variations*/,
            afterCreateAllocCount);
#else
        LOG_OUTPUT(L"Skipping allocation count verification in free build because we don't count allocations in free.");
#endif
        

        // Remove from tree
        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        hasUnloadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } }
