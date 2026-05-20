// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Gestures {
        class BasicGestureManipulationTests : public WEX::TestClass<BasicGestureManipulationTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicGestureManipulationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(DManipAndGestureModeCombinations)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies valid ManipulationModes combinations.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateEventSequence)
                TEST_METHOD_PROPERTY(L"Description", L"Validates firing and order of manipulation events .")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DragARectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging with the mouse via the Touch Interaction Engine.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // args->Cumulative.Translation.X mismatch
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Fix Gesture manipulation tests that were broken with lifting
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DragARectangleInScrollViewer)
                TEST_METHOD_PROPERTY(L"Description", L"Validates dragging with the mouse via the Touch Interaction Engine within a ScrollViewer.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScaleARectangleInScrollViewer)
                TEST_METHOD_PROPERTY(L"Description", L"Validates scaling with touch via the Touch Interaction Engine within a ScrollViewer.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // GetForCurrentView no lonnger available in Desktop
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
             END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScaleARectangleInScrollViewerWithInertia)
                TEST_METHOD_PROPERTY(L"Description", L"Validates scaling with touch via the Touch Interaction Engine within a ScrollViewer with inertia.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // GetForCurrentView no lonnger available in Desktop
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetSystemAndScaleInPointerPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Set up manipulation scaling within a ScrollViewer in PointerPressed handler.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // GetForCurrentView no lonnger available in Desktop
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ResetScaleInPointerPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Reset manipulation scaling in PointerPressed handler and zoom in ScrollViewer with DManip.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // GetForCurrentView no lonnger available in Desktop
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanAZoomableRectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Pan a ScrollViewer with DManip by touching a zoomable Rectangle.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PanARectangleWithManualCompletion)
                TEST_METHOD_PROPERTY(L"Description", L"Pan a rectangle with inertia, and complete the manipulation when inertia-based deltas are firing.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") // Mouse input injection functions (e.g. InputHelper::DragFromCenter) don't work reliably on OneCore
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomARectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates zooming via the Touch Interaction Engine.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InertiaStartingArgs)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the ManipulationIntertiaStartingEventArgs.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LeaveTreeInInertia)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the ability to successfully leave the tree in ManipulationDelta during inertia.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // [WinUI3]BasicGestureManipulationTests::LeaveTreeInInertia is unreliable
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RotateARectangle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates a rotation manipulation fires an event with the expected cumulative rotation")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Implement InputManager.IMInjectMTRotate and enable this test
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RotateARectangleWithDesiredRotation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates a rotation manipulation honors inertial DesiredRotation property")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Implement InputManager.IMInjectMTRotate and enable this test
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RotateARectangleWithDesiredDeceleration)
                TEST_METHOD_PROPERTY(L"Description", L"Validates a rotation manipulation honors inertial DesiredDeceleration property")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Implement InputManager.IMInjectMTRotate and enable this test
            END_TEST_METHOD()
            
        private:
            struct RectRotateHelperData
            {
                float expectedStartAngle = 0.0f;
                float expectedEndAngle = 90.0f;
                float setDesiredRotationTo = 0.0f;
                float setDesiredDecelerationTo = 0.0f;
                float errorMargin = 1.0f;
            };
            void RotateARectangleHelper(const RectRotateHelperData& data);
            void ScaleARectangleInScrollViewer(bool useInertia, bool setManipulationModeInPointerPressed);
        };
    } } }
} } } }
