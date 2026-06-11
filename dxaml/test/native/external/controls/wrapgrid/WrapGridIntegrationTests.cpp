// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WrapGridIntegrationTests.h"
#include "PanelsHelper.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace WrapGrid {

    bool WrapGridIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool WrapGridIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool WrapGridIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void WrapGridIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::WrapGrid>::CanInstantiate();
    }

    void WrapGridIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;
        Generic::FrameworkElementTests<ItemsControl>::CanEnterAndLeaveLiveTree(PanelsHelper::CreateItemsControlWithPanel<ItemsControl, xaml_controls::WrapGrid>());
    }

    void WrapGridIntegrationTests::CanWrapItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::WrapGrid>(s_itemCount, L"Orientation='Horizontal'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            wf::Point expectedPoint = wf::Point(100.0f * (i % 3), 100.0f * (i / 3));
            expectedPositions->Append(expectedPoint);
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

    void WrapGridIntegrationTests::CanWrapItemsVertically()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::WrapGrid>(s_itemCount, L"Orientation='Vertical'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * (i / 3), 100.0f * (i % 3)));
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::WrapGrid
