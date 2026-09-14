// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <HostingModeTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Pointer {
        class BasicPointerTests : public WEX::TestClass<BasicPointerTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicPointerTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_HOSTING_MODE_DEFAULT()
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CanDragARectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging a Rectangle in a Canvas with the left mouse button and raw pointer events.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TouchFuzzyHitTest)
                TEST_METHOD_PROPERTY(L"Description", L"Test fuzzy hit-testing for touch in LTR/RTL.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ProtectedCursorOnNonLiveElement)
                TEST_METHOD_PROPERTY(L"Description", L"Set UIElement.ProtectedCursor on an element that's not ever been the live tree")
            END_TEST_METHOD()
        };

    class BasicPointerTestsUap : public WEX::TestClass<BasicPointerTestsUap>
    {
    public:
        BEGIN_TEST_CLASS(BasicPointerTestsUap)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"MasterFile:ClassName", L"BasicPointerTests")
                TEST_CLASS_HOSTING_MODE(UAP)
            END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VisualTreeHelperHitTest)
        TEST_METHOD_PROPERTY(L"Description", L"Test VisualTreeHelper.FindElementsInHostCoordinates in LTR/RTL at different scale levels.")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        // Crash in test code
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(PointerRoutedAway)
        TEST_METHOD_PROPERTY(L"Description", L"Inject a WM_POINTERROUTEDAWAY message and expect a PointerCaptureLost event.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        // This test injects WM_POINTERROUTEDAWAY, which doesn't work on Islands
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Replace PointerPointStatics::GetCurrentPoint and GetIntermediatePointsTransformed
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLeftMouseClick)
        TEST_METHOD_PROPERTY(L"Description", L"Validates that InputHelper->LeftMouseClick is working on OneCore")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        // [DCPP-test] WPF tests are failing with AnimationIdle timeout during test cleanup
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
        END_TEST_METHOD()
    };
    } } }
} } } }
