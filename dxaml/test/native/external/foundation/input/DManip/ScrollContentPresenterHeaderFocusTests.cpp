// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollContentPresenterHeaderFocusTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <FocusTestHelper.h>
#include <TreeHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

// Each scenario places a ScrollViewer with TopLeftHeader/TopHeader/LeftHeader between two outside
// buttons ("before" and "after"). ScrollContentPresenter::ProcessTabStopOverride,
// ProcessCandidateTabStopOverride and GetFirst/LastFocusableElementOverride implement the
// documented order "top-left header -> top header -> left header -> content" (see the comment on
// ScrollContentPresenter::ProcessTabStopOverride) and honor custom TabIndex values set on those four
// direct children. Every step is driven with FocusManager::TryMoveFocus and checked two ways: the
// query-only FocusManager::FindNextElement must predict the same element, and the element reported by
// FocusManager::GetFocusedElement must be the expected one.
//
// The walks enter the ScrollViewer and move between its headers and content; they stop on the last
// element inside the ScrollViewer instead of tabbing out of it.

// ScrollContentPresenter::MeasureOverride only supports Left/Top aligned content when headers are set
// (ERROR_SCROLLVIEWER_UNSUPPORTED_*ALIGNMENT_WITH_HEADER), so every content element below is Left/Top aligned.

#define XAML_NAMESPACES L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        // Buttons as direct header/content children, all with the default TabIndex.
        static const wchar_t* s_directChildHeaders =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Panels with two buttons each as header/content children (headers are not direct tab stops), default TabIndex.
        static const wchar_t* s_panelHeaders =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><StackPanel><Button x:Name='tl1' Content='TL1'/><Button x:Name='tl2' Content='TL2'/></StackPanel></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><StackPanel Orientation='Horizontal'><Button x:Name='t1' Content='T1'/><Button x:Name='t2' Content='T2'/></StackPanel></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><StackPanel><Button x:Name='l1' Content='L1'/><Button x:Name='l2' Content='L2'/></StackPanel></ScrollViewer.LeftHeader>"
            L"    <StackPanel Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'><Button x:Name='c1' Content='C1'/><Button x:Name='c2' Content='C2'/></StackPanel>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Button headers with explicit TabIndex values in the documented header priority order, and a panel
        // with two buttons as content. ScrollContentPresenter::GetTabIndex only reports Controls as tab stops,
        // so the panel content does not take part in the TabIndex-ordered navigation between the headers.
        static const wchar_t* s_tabIndexHeadersWithPanelContent =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL' TabIndex='1'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T' TabIndex='2'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L' TabIndex='3'/></ScrollViewer.LeftHeader>"
            L"    <StackPanel TabIndex='4' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'><Button x:Name='c1' Content='C1'/><Button x:Name='c2' Content='C2'/></StackPanel>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Custom TabIndex values reverse the visual header priority: content first, top-left last.
        static const wchar_t* s_customTabIndexHeaders =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL' TabIndex='4'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T' TabIndex='3'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L' TabIndex='2'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' TabIndex='1'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // All direct children share TabIndex 1, so the header priority breaks the ties.
        static const wchar_t* s_equalTabIndexHeaders =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL' TabIndex='1'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T' TabIndex='1'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L' TabIndex='1'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' TabIndex='1'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Only the top-left header has a custom TabIndex; the top header, left header and content keep the
        // default TabIndex. One custom value is enough to switch to the TabIndex-ordered navigation, in which
        // the top-left header (TabIndex 5) comes first and the default-TabIndex ties keep the header priority.
        static const wchar_t* s_topLeftHeaderOnlyTabIndex =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL' TabIndex='5'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Top header disabled and left header IsTabStop=False, with custom TabIndex values so the
        // TabIndex-based helpers must skip them.
        static const wchar_t* s_nonTabStopHeadersWithTabIndex =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopLeftHeader><Button x:Name='topLeft' Content='TL' TabIndex='4'/></ScrollViewer.TopLeftHeader>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T' TabIndex='3' IsEnabled='False'/></ScrollViewer.TopHeader>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L' TabIndex='2' IsTabStop='False'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' TabIndex='1'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Only a TopHeader, ordered after the content by TabIndex.
        static const wchar_t* s_topHeaderOnlyWithTabIndex =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.TopHeader><Button x:Name='top' Content='T' TabIndex='2'/></ScrollViewer.TopHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' TabIndex='1'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        // Only a LeftHeader, ordered before the content by TabIndex.
        static const wchar_t* s_leftHeaderOnlyWithTabIndex =
            L"<StackPanel " XAML_NAMESPACES L">"
            L"  <Button x:Name='before' Content='before'/>"
            L"  <ScrollViewer x:Name='sv' Width='300' Height='200' HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='Auto'>"
            L"    <ScrollViewer.LeftHeader><Button x:Name='left' Content='L' TabIndex='1'/></ScrollViewer.LeftHeader>"
            L"    <Button x:Name='content' Content='C' Width='400' Height='400' HorizontalAlignment='Left' VerticalAlignment='Top' TabIndex='2'/>"
            L"  </ScrollViewer>"
            L"  <Button x:Name='after' Content='after'/>"
            L"</StackPanel>";

        bool ScrollContentPresenterHeaderFocusTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ScrollContentPresenterHeaderFocusTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ScrollContentPresenterHeaderFocusTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        xaml::FrameworkElement^ ScrollContentPresenterHeaderFocusTests::LoadContent(Platform::String^ xamlString)
        {
            xaml::FrameworkElement^ root = nullptr;
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            RunOnUIThread([&]()
            {
                root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(root);
                loadedRegistration.Attach(root, [loadedEvent]() { loadedEvent->Set(); });
                TestServices::WindowHelper->WindowContent = root;
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Precondition: the headers were parented into the ScrollContentPresenter, otherwise the
                // scenario would silently degrade to plain visual-order tab navigation.
                auto scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(root->FindName(L"sv"));
                VERIFY_IS_NOT_NULL(scrollViewer);
                auto presenter = TreeHelper::GetVisualChildByType<xaml_controls::ScrollContentPresenter>(scrollViewer);
                VERIFY_IS_NOT_NULL(presenter);
                for (auto header : { scrollViewer->TopLeftHeader, scrollViewer->TopHeader, scrollViewer->LeftHeader })
                {
                    if (header != nullptr)
                    {
                        VERIFY_IS_TRUE(presenter->Equals(xaml_media::VisualTreeHelper::GetParent(header)));
                    }
                }
            });

            return root;
        }

        void ScrollContentPresenterHeaderFocusTests::EnsureFocusOn(xaml::FrameworkElement^ root, Platform::String^ name)
        {
            xaml_controls::Control^ control = nullptr;
            RunOnUIThread([&]()
            {
                control = safe_cast<xaml_controls::Control^>(root->FindName(name));
                VERIFY_IS_NOT_NULL(control);
            });

            FocusTestHelper::EnsureFocus(control, xaml::FocusState::Keyboard);
            TestServices::WindowHelper->WaitForIdle();
        }

        Platform::String^ ScrollContentPresenterHeaderFocusTests::NameOf(Platform::Object^ element)
        {
            auto frameworkElement = dynamic_cast<xaml::FrameworkElement^>(element);
            if (frameworkElement != nullptr)
            {
                return frameworkElement->Name;
            }
            if (element == nullptr)
            {
                return Platform::StringReference(L"<null>");
            }
            return element->GetType()->FullName;
        }

        ScrollContentPresenterHeaderFocusTests::FocusWalk ScrollContentPresenterHeaderFocusTests::MoveFocus(
            xaml::FrameworkElement^ root,
            xaml_input::FocusNavigationDirection direction,
            unsigned int steps)
        {
            // Actor only: performs the moves and reports what happened; the calling test owns the oracle.
            FocusWalk walk;
            walk.predicted = L"";
            walk.focused = L"";

            for (unsigned int step = 0; step < steps; step++)
            {
                RunOnUIThread([&]()
                {
                    auto options = ref new xaml_input::FindNextElementOptions();
                    options->SearchRoot = root;

                    auto predicted = xaml_input::FocusManager::FindNextElement(direction, options);
                    const bool moved = xaml_input::FocusManager::TryMoveFocus(direction, options);
                    auto focused = xaml_input::FocusManager::GetFocusedElement(root->XamlRoot);

                    Platform::String^ separator = (step == 0) ? L"" : L",";
                    Platform::String^ focusedName = moved ? NameOf(focused) : ref new Platform::String(L"<not-moved>");
                    walk.predicted = walk.predicted + separator + NameOf(predicted);
                    walk.focused = walk.focused + separator + focusedName;
                    LOG_OUTPUT(L"Direction=%d step=%u FindNextElement='%s' focused='%s' moved=%d",
                        static_cast<int>(direction), step, NameOf(predicted)->Data(), NameOf(focused)->Data(), moved);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            return walk;
        }

        void ScrollContentPresenterHeaderFocusTests::EnterScrollViewerWithDefaultTabIndex()
        {
            TestCleanupWrapper cleanup;

            {
                LOG_OUTPUT(L"Direct-child headers: forward entry lands on the top-left header, backward entry on the content.");
                auto root = LoadContent(Platform::StringReference(s_directChildHeaders));

                EnsureFocusOn(root, L"before");
                auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 1);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"topLeft"), forward.focused);
                VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

                EnsureFocusOn(root, L"after");
                auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 1);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"content"), backward.focused);
                VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
            }

            {
                LOG_OUTPUT(L"Panel headers: entry lands inside the first/last region and moves within that region's panel.");
                auto root = LoadContent(Platform::StringReference(s_panelHeaders));

                EnsureFocusOn(root, L"before");
                auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 2);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"tl1,tl2"), forward.focused);
                VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

                EnsureFocusOn(root, L"after");
                auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 2);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"c2,c1"), backward.focused);
                VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
            }
        }

        void ScrollContentPresenterHeaderFocusTests::TabForwardWithCustomTabIndex()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_customTabIndexHeaders));

            EnsureFocusOn(root, L"before");
            auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 4);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"content,left,top,topLeft"), forward.focused);
            VERIFY_ARE_EQUAL(forward.focused, forward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabBackwardWithCustomTabIndex()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_customTabIndexHeaders));

            EnsureFocusOn(root, L"after");
            auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 4);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"topLeft,top,left,content"), backward.focused);
            VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabWithEqualTabIndexUsesHeaderPriority()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_equalTabIndexHeaders));

            EnsureFocusOn(root, L"before");
            auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 4);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"topLeft,top,left,content"), forward.focused);
            VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

            auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 3);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"left,top,topLeft"), backward.focused);
            VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabThroughHeadersWithTabIndexAndPanelContent()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_tabIndexHeadersWithPanelContent));

            EnsureFocusOn(root, L"before");
            auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 3);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"topLeft,top,left"), forward.focused);
            VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

            auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 2);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"top,topLeft"), backward.focused);
            VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabWithOnlyTopLeftHeaderTabIndexUsesTabIndexOrder()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_topLeftHeaderOnlyTabIndex));

            EnsureFocusOn(root, L"before");
            auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 4);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"topLeft,top,left,content"), forward.focused);
            VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

            auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 3);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"left,top,topLeft"), backward.focused);
            VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabSkipsDisabledOrNonTabStopHeaders()
        {
            TestCleanupWrapper cleanup;
            auto root = LoadContent(Platform::StringReference(s_nonTabStopHeadersWithTabIndex));

            EnsureFocusOn(root, L"before");
            auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 2);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"content,topLeft"), forward.focused);
            VERIFY_ARE_EQUAL(forward.focused, forward.predicted);

            auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 1);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"content"), backward.focused);
            VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
        }

        void ScrollContentPresenterHeaderFocusTests::TabWithPartialHeaderSet()
        {
            TestCleanupWrapper cleanup;

            {
                LOG_OUTPUT(L"Only a TopHeader is set (TabIndex after the content).");
                auto root = LoadContent(Platform::StringReference(s_topHeaderOnlyWithTabIndex));

                EnsureFocusOn(root, L"before");
                auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 2);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"content,top"), forward.focused);
                VERIFY_ARE_EQUAL(forward.focused, forward.predicted);
                auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 1);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"content"), backward.focused);
                VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
            }

            {
                LOG_OUTPUT(L"Only a LeftHeader is set (TabIndex before the content).");
                auto root = LoadContent(Platform::StringReference(s_leftHeaderOnlyWithTabIndex));

                EnsureFocusOn(root, L"before");
                auto forward = MoveFocus(root, xaml_input::FocusNavigationDirection::Next, 2);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"left,content"), forward.focused);
                VERIFY_ARE_EQUAL(forward.focused, forward.predicted);
                auto backward = MoveFocus(root, xaml_input::FocusNavigationDirection::Previous, 1);
                VERIFY_ARE_EQUAL(Platform::StringReference(L"left"), backward.focused);
                VERIFY_ARE_EQUAL(backward.focused, backward.predicted);
            }
        }

        void ScrollContentPresenterHeaderFocusTests::FindFirstAndLastFocusableElementHonorHeaders()
        {
            TestCleanupWrapper cleanup;

            {
                LOG_OUTPUT(L"Default TabIndex: first is the top-left header, last is the content.");
                auto root = LoadContent(Platform::StringReference(s_directChildHeaders));

                RunOnUIThread([&]()
                {
                    auto scrollViewer = safe_cast<xaml::DependencyObject^>(root->FindName(L"sv"));
                    auto first = xaml_input::FocusManager::FindFirstFocusableElement(scrollViewer);
                    auto last = xaml_input::FocusManager::FindLastFocusableElement(scrollViewer);
                    LOG_OUTPUT(L"First='%s' Last='%s'", NameOf(first)->Data(), NameOf(last)->Data());
                    VERIFY_IS_TRUE(root->FindName(L"topLeft")->Equals(first));
                    VERIFY_IS_TRUE(root->FindName(L"content")->Equals(last));
                });
            }

            {
                LOG_OUTPUT(L"Custom TabIndex: first is the lowest TabIndex (content), last is the highest (top-left header).");
                auto root = LoadContent(Platform::StringReference(s_customTabIndexHeaders));

                RunOnUIThread([&]()
                {
                    auto scrollViewer = safe_cast<xaml::DependencyObject^>(root->FindName(L"sv"));
                    auto first = xaml_input::FocusManager::FindFirstFocusableElement(scrollViewer);
                    auto last = xaml_input::FocusManager::FindLastFocusableElement(scrollViewer);
                    LOG_OUTPUT(L"First='%s' Last='%s'", NameOf(first)->Data(), NameOf(last)->Data());
                    VERIFY_IS_TRUE(root->FindName(L"content")->Equals(first));
                    VERIFY_IS_TRUE(root->FindName(L"topLeft")->Equals(last));
                });
            }
        }

    } } }
} } } }
