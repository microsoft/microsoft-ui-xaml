// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class AutoScrollBehaviorTests : public WEX::TestClass<AutoScrollBehaviorTests>
        {
        public:
            BEGIN_TEST_CLASS(AutoScrollBehaviorTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(AutoScrollHorizontallyInGridView)
                TEST_METHOD_PROPERTY(L"Description", L"Auto-scrolls the content of a horizontal GridView with touch.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // DCPP: Test shutdown assert on WPF - ~CDirectManipulationService still has m_mapViewports
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MouseWheelScrollDuringTouchAutoScroll)
                TEST_METHOD_PROPERTY(L"Description", L"Scroll with mouse-wheel while auto-scrolling the content of a horizontal GridView with touch.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // DCPP: Test shutdown assert on WPF - ~CDirectManipulationService still has m_mapViewports
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AutoScrollVerticallyInListView)
                TEST_METHOD_PROPERTY(L"Description", L"Auto-scrolls the content of a vertical ListView with mouse.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
           END_TEST_METHOD()

            BEGIN_TEST_METHOD(MouseWheelScrollDuringMouseAutoScroll)
                TEST_METHOD_PROPERTY(L"Description", L"Scroll with mouse-wheel while auto-scrolling the content of a vertical ListView with mouse.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Converting to WPF Mode leads to Error: Verify: IsGreaterThan(alsoScrollWithMouseWheel ? 200.0 : 1400.0, scrollViewer->VerticalOffset) - Values (200.000000l, 222.000000l)
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;

            void AutoScrollHorizontallyInGridView(bool alsoScrollWithMouseWheel, INT firstItemCenterX);
            void AutoScrollVerticallyInListView(bool alsoScrollWithMouseWheel);
        };
    } } }
} } } }
