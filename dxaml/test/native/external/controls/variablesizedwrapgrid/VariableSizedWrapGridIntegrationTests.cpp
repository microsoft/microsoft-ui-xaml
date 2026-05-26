// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "VariableSizedWrapGridIntegrationTests.h"
#include "PanelsHelper.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace VariableSizedWrapGrid {

    bool VariableSizedWrapGridIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool VariableSizedWrapGridIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool VariableSizedWrapGridIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void VariableSizedWrapGridIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::VariableSizedWrapGrid>::CanInstantiate();
    }

    void VariableSizedWrapGridIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::VariableSizedWrapGrid>::CanEnterAndLeaveLiveTree();
    }

    void VariableSizedWrapGridIntegrationTests::CanWrapItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        auto panel = PanelsHelper::AddPanelWithContent<xaml_controls::VariableSizedWrapGrid>(PanelsHelper::CreateDefaultPanelContent(s_itemCount), Orientation::Horizontal);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * (i % 3), 100.0f * (i / 3)));
        }

        PanelsHelper::VerifyItemPositions(panel, expectedPositions);
    }

    void VariableSizedWrapGridIntegrationTests::CanWrapItemsVertically()
    {
        TestCleanupWrapper cleanup;
        auto panel = PanelsHelper::AddPanelWithContent<xaml_controls::VariableSizedWrapGrid>(PanelsHelper::CreateDefaultPanelContent(s_itemCount), Orientation::Vertical);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * (i / 3), 100.0f * (i % 3)));
        }

        PanelsHelper::VerifyItemPositions(panel, expectedPositions);
    }

    void VariableSizedWrapGridIntegrationTests::CanChangeRowAndColumnSpans()
    {
        TestCleanupWrapper cleanup;
        auto panelContent = PanelsHelper::CreateDefaultPanelContent(s_itemCount);

        RunOnUIThread([&]()
        {
            xaml_controls::VariableSizedWrapGrid::SetColumnSpan(panelContent->GetAt(0), 2);
            xaml_controls::VariableSizedWrapGrid::SetRowSpan(panelContent->GetAt(2), 2);
        });

        auto panel = PanelsHelper::AddPanelWithContent<xaml_controls::VariableSizedWrapGrid>(panelContent, Orientation::Horizontal);
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        expectedPositions->Append(wf::Point(50.0f, 0.0f));
        expectedPositions->Append(wf::Point(200.0f, 0.0f));
        expectedPositions->Append(wf::Point(0.0f, 150.0f));
        expectedPositions->Append(wf::Point(100.0f, 100.0f));
        expectedPositions->Append(wf::Point(200.0f, 100.0f));
        expectedPositions->Append(wf::Point(100.0f, 200.0f));
        expectedPositions->Append(wf::Point(200.0f, 200.0f));

        PanelsHelper::VerifyItemPositions(panel, expectedPositions);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::VariableSizedWrapGrid
