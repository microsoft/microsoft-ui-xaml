// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace Gestures {

class BasicGestureTests : public WEX::TestClass<BasicGestureTests>
{
public:
    BEGIN_TEST_CLASS(BasicGestureTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(TapEmptyTree)
        TEST_METHOD_PROPERTY(L"Description", L"Call TouchHitTestingHandler::OnTouchHitTesting without a tree. Don't crash.")
        // This test tears down the tree and recovers by shutting down and reinitializing Xaml. ShutdownXaml is skipped for non-desktop SKUs
        // due to a stale DComp tree associated with the window after the DComp device is recreated. See comment
        // in WindowHelper::ShutdownXaml.
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Window stuck at alt menu
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TapARectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the tap gesture on a Rectangle via the Touch Interaction Engine.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DestroyOnTapARectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the ability to destroy the element from the tap handler")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ReparentRectangleDuringTap)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the ability to reparent an element during a tap gesture.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(DoubleTapARectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the double-tap gesture on a Rectangle via the Touch Interaction Engine.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RightTapARectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the right-tap gesture on a Rectangle via the Touch Interaction Engine.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RightTapARectangleUsingPen)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the right-tap gesture using pen on a Rectangle via the Touch Interaction Engine.")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(HoldARectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Validates the hold gesture on a Rectangle via the Touch Interaction Engine.")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // Fix Gesture manipulation tests that were broken with lifting
    END_TEST_METHOD()

private:
    void SetupElements(_Out_ Microsoft::UI::Xaml::Shapes::Rectangle^* pRect, _Out_opt_ Microsoft::UI::Xaml::Controls::Canvas^* pCanvas = nullptr);
};

} } } } } } }
