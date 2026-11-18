// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        // Validates the tab navigation contract of a ScrollViewer whose ScrollContentPresenter hosts
        // TopLeftHeader/TopHeader/LeftHeader elements: the tab order is top-left header -> top header ->
        // left header -> content, and custom TabIndex values on those direct children are honored.
        class ScrollContentPresenterHeaderFocusTests : public WEX::TestClass<ScrollContentPresenterHeaderFocusTests>
        {
        public:
            BEGIN_TEST_CLASS(ScrollContentPresenterHeaderFocusTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(EnterScrollViewerWithDefaultTabIndex)
                TEST_METHOD_PROPERTY(L"Description", L"Without custom TabIndex values, moving focus forward into a ScrollViewer lands on its top-left header and moving backward lands on its content.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabForwardWithCustomTabIndex)
                TEST_METHOD_PROPERTY(L"Description", L"Custom TabIndex values on headers and content define the forward tab order inside the ScrollViewer.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabBackwardWithCustomTabIndex)
                TEST_METHOD_PROPERTY(L"Description", L"Custom TabIndex values on headers and content define the backward tab order inside the ScrollViewer.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabWithEqualTabIndexUsesHeaderPriority)
                TEST_METHOD_PROPERTY(L"Description", L"When headers and content share the same TabIndex the header priority order is used in both directions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabThroughHeadersWithTabIndexAndPanelContent)
                TEST_METHOD_PROPERTY(L"Description", L"With explicit header TabIndex values and panel content, the headers are traversed in TabIndex order in both directions.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabWithOnlyTopLeftHeaderTabIndexUsesTabIndexOrder)
                TEST_METHOD_PROPERTY(L"Description", L"A custom TabIndex on the top-left header alone switches the ScrollViewer to TabIndex-ordered navigation across all headers and the content.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabSkipsDisabledOrNonTabStopHeaders)
                TEST_METHOD_PROPERTY(L"Description", L"With custom TabIndex values, disabled or IsTabStop=False headers are skipped.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TabWithPartialHeaderSet)
                TEST_METHOD_PROPERTY(L"Description", L"A ScrollViewer with only a top header, or only a left header, orders that header and its content by TabIndex.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindFirstAndLastFocusableElementHonorHeaders)
                TEST_METHOD_PROPERTY(L"Description", L"FocusManager.FindFirst/LastFocusableElement on a ScrollViewer with headers return the header/content selected by TabIndex.")
            END_TEST_METHOD()

        private:
            static xaml::FrameworkElement^ LoadContent(Platform::String^ xamlString);
            static void EnsureFocusOn(xaml::FrameworkElement^ root, Platform::String^ name);
            // Comma-separated names of the elements predicted by FindNextElement and focused after each TryMoveFocus.
            struct FocusWalk
            {
                Platform::String^ predicted;
                Platform::String^ focused;
            };

            static FocusWalk MoveFocus(
                xaml::FrameworkElement^ root,
                xaml_input::FocusNavigationDirection direction,
                unsigned int steps);
            static Platform::String^ NameOf(Platform::Object^ element);
        };

    } } }
} } } }
