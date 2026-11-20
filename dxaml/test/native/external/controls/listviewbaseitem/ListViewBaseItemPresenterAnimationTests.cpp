// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListViewBaseItemPresenterAnimationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <cmath>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

// The item chrome (CListViewBaseItemChrome) queues animation commands when a ListViewItem enters a
// visual state and ListViewItemPresenter::GoToElementStateCore runs them through
// ListViewBaseItemPresenter::VisitAnimationCommand:
//  - Bottom/Top/Left/RightReorderHint translate the presenter by ReorderHintOffset, and NoReorderHint
//    returns it, either immediately (no transitions) or through an animation whose Completed handler
//    releases the storyboard (ListViewBaseItemPresenter::OnReorderHintReturnCompleted).
// The animation drives the element's transition target, which is part of the element's local transform,
// so its steady-state effect is observable through TransformToVisual. WaitForIdle only returns once
// running (finite) storyboards have completed, so animated transitions are observed at their end state.

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseItem {

    static const double c_reorderHintOffset = 20.0;
    static const double c_tolerance = 0.5;

    static bool AreClose(double expected, double actual)
    {
        return std::abs(expected - actual) <= c_tolerance;
    }

    bool ListViewBaseItemPresenterAnimationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ListViewBaseItemPresenterAnimationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ListViewBaseItemPresenterAnimationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    xaml_controls::ListViewItem^ ListViewBaseItemPresenterAnimationTests::LoadListViewAndGetFirstItem()
    {
        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, Loaded);

        RunOnUIThread([&]()
        {
            // Another test may have cached the rounded-chrome theme resource; re-read the framework default.
            TestServices::Utilities->DeleteResourceDictionaryCaches();

            listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                L"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    Width='300' Height='300' HorizontalAlignment='Left' VerticalAlignment='Top'>"
                L"  <ListView.ItemTemplate>"
                L"    <DataTemplate><Border x:Name='itemRoot' Width='100' Height='40' Background='Red'/></DataTemplate>"
                L"  </ListView.ItemTemplate>"
                L"  <x:String>Item 0</x:String>"
                L"  <x:String>Item 1</x:String>"
                L"  <x:String>Item 2</x:String>"
                L"</ListView>"));
            VERIFY_IS_NOT_NULL(listView);
            loadedRegistration.Attach(listView, [loadedEvent]() { loadedEvent->Set(); });
            TestServices::WindowHelper->WindowContent = listView;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItem = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listViewItem);
            VERIFY_IS_NOT_NULL(TreeHelper::GetVisualChildByType<xaml_primitives::ListViewItemPresenter>(listViewItem));
            VERIFY_IS_NOT_NULL(TreeHelper::GetVisualChildByName(listViewItem, L"itemRoot"));
        });

        return listViewItem;
    }

    wf::Rect ListViewBaseItemPresenterAnimationTests::GetBoundsRelativeTo(xaml::FrameworkElement^ element, xaml::UIElement^ relativeTo)
    {
        return element->TransformToVisual(relativeTo)->TransformBounds(
            wf::Rect(0, 0, static_cast<float>(element->ActualWidth), static_cast<float>(element->ActualHeight)));
    }

    void ListViewBaseItemPresenterAnimationTests::GoToStateAndWaitForIdle(xaml_controls::Control^ control, Platform::String^ stateName, bool useTransitions)
    {
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"GoToState('%s', useTransitions=%d)", stateName->Data(), useTransitions);
            xaml::VisualStateManager::GoToState(control, stateName, useTransitions);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ListViewBaseItemPresenterAnimationTests::ReorderHintStatesOffsetAndReturnContent()
    {
        TestCleanupWrapper cleanup;
        auto listViewItem = LoadListViewAndGetFirstItem();
        xaml_primitives::ListViewItemPresenter^ presenter = nullptr;
        wf::Rect restingBounds = {};

        RunOnUIThread([&]()
        {
            presenter = TreeHelper::GetVisualChildByType<xaml_primitives::ListViewItemPresenter>(listViewItem);
            presenter->ReorderHintOffset = c_reorderHintOffset;
            VERIFY_ARE_EQUAL(c_reorderHintOffset, presenter->ReorderHintOffset);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            restingBounds = GetBoundsRelativeTo(presenter, listViewItem);
            LOG_OUTPUT(L"Resting presenter bounds {%f,%f,%f,%f}", restingBounds.X, restingBounds.Y, restingBounds.Width, restingBounds.Height);
        });

        auto verifyOffset = [&](Platform::String^ context, double expectedDeltaX, double expectedDeltaY)
        {
            RunOnUIThread([&]()
            {
                auto bounds = GetBoundsRelativeTo(presenter, listViewItem);
                LOG_OUTPUT(L"%s presenter bounds {%f,%f,%f,%f}, expected delta (%f,%f)",
                    context->Data(), bounds.X, bounds.Y, bounds.Width, bounds.Height, expectedDeltaX, expectedDeltaY);
                VERIFY_IS_TRUE(AreClose(restingBounds.X + expectedDeltaX, bounds.X));
                VERIFY_IS_TRUE(AreClose(restingBounds.Y + expectedDeltaY, bounds.Y));
                VERIFY_IS_TRUE(AreClose(restingBounds.Width, bounds.Width));
                VERIFY_IS_TRUE(AreClose(restingBounds.Height, bounds.Height));
            });
        };

        GoToStateAndWaitForIdle(listViewItem, L"BottomReorderHint", false /*useTransitions*/);
        verifyOffset(L"BottomReorderHint", 0.0, c_reorderHintOffset);

        // Switching hints clears the previous one before applying the new direction.
        GoToStateAndWaitForIdle(listViewItem, L"TopReorderHint", false /*useTransitions*/);
        verifyOffset(L"TopReorderHint", 0.0, -c_reorderHintOffset);

        GoToStateAndWaitForIdle(listViewItem, L"LeftReorderHint", false /*useTransitions*/);
        verifyOffset(L"LeftReorderHint", -c_reorderHintOffset, 0.0);

        // Animated return: the item slides back and the return storyboard completes.
        GoToStateAndWaitForIdle(listViewItem, L"NoReorderHint", true /*useTransitions*/);
        verifyOffset(L"NoReorderHint (animated)", 0.0, 0.0);

        // Animated hint start, then immediate return without transitions.
        GoToStateAndWaitForIdle(listViewItem, L"RightReorderHint", true /*useTransitions*/);
        verifyOffset(L"RightReorderHint (animated)", c_reorderHintOffset, 0.0);

        GoToStateAndWaitForIdle(listViewItem, L"NoReorderHint", false /*useTransitions*/);
        verifyOffset(L"NoReorderHint (steady state)", 0.0, 0.0);

        // A new hint after the return still applies, i.e. the returned storyboard was released.
        GoToStateAndWaitForIdle(listViewItem, L"BottomReorderHint", false /*useTransitions*/);
        verifyOffset(L"BottomReorderHint after return", 0.0, c_reorderHintOffset);
        GoToStateAndWaitForIdle(listViewItem, L"NoReorderHint", false /*useTransitions*/);
        verifyOffset(L"Final NoReorderHint", 0.0, 0.0);
    }

} } } } } }
