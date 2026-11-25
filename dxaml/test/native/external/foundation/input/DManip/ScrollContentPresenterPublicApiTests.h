// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        // Validates the public ScrollContentPresenter scrolling APIs that are not reached through
        // ScrollViewer input routing: MouseWheelUp/Down/Left/Right, MakeVisible(UIElement, Rect),
        // offset validation and the static dependency property identifiers.
        class ScrollContentPresenterPublicApiTests : public WEX::TestClass<ScrollContentPresenterPublicApiTests>
        {
        public:
            BEGIN_TEST_CLASS(ScrollContentPresenterPublicApiTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(MouseWheelMethodsScrollContent)
                TEST_METHOD_PROPERTY(L"Description", L"MouseWheelDown/Up scroll vertically (or horizontally when vertical scrolling is disabled), MouseWheelRight/Left scroll horizontally, and all clamp at the scroll bounds.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(OffsetRequestsBeyondTheEndAreClampedWhenRequested)
                TEST_METHOD_PROPERTY(L"Description", L"SetHorizontalOffset/MouseWheelRight requests past the scrollable end are clamped to ExtentWidth - ViewportWidth when they are requested, as reported by ScrollViewer.ViewChanging.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MakeVisibleSimpleOverloadBringsElementIntoView)
                TEST_METHOD_PROPERTY(L"Description", L"MakeVisible(UIElement, Rect) scrolls an out-of-view element into the viewport and leaves an in-view element's offsets unchanged.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetOffsetWithNaNThrowsAndStaticPropertiesExist)
                TEST_METHOD_PROPERTY(L"Description", L"SetHorizontalOffset/SetVerticalOffset reject NaN without changing the offsets; the static dependency property identifiers map to the instance properties.")
            END_TEST_METHOD()

        private:
            static xaml_controls::ScrollContentPresenter^ LoadScrollViewer(
                bool canScrollVertically,
                _Out_ xaml_controls::ScrollViewer^* scrollViewer,
                _Out_ xaml::FrameworkElement^* root);
        };

    } } }
} } } }
