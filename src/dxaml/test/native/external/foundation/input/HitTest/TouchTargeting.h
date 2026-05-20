// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Input { namespace HitTest {

class TouchTargeting : public WEX::TestClass<TouchTargeting>
{
public:
    BEGIN_TEST_CLASS(TouchTargeting)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
        TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")    // Event times out
        TEST_CLASS_PROPERTY(L"Ignore", L"TRUE") // TODO 20928844: Re-enable after investigating why software injection causes this to fail.
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(Rectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Does touch hit testing on a single Rectangle")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(TwoRectangles)
        TEST_METHOD_PROPERTY(L"Description", L"Does touch hit testing with two Rectangles")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanvasAndRectangle1)
        TEST_METHOD_PROPERTY(L"Description", L"Does touch hit testing with a Canvas and a child Rectangle")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(CanvasAndRectangle2)
        TEST_METHOD_PROPERTY(L"Description", L"Does more touch hit testing with a Canvas and a child Rectangle")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(OverlappingRectangles)
        TEST_METHOD_PROPERTY(L"Description", L"Does more touch hit testing with overlapping Rectangles")
        TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
    END_TEST_METHOD()

    // OneCore and WindowsCore VMs have OneCoreTransforms enabled, but they don't raise the OnTouchHitTesting event
    // required for fuzzy hit testing. The only VM that raises that event is desktop, so we simulate the OneCoreTransform
    // setup on desktop.
    BEGIN_TEST_METHOD(SimulateOneCoreTransforms_Plateau)
        TEST_METHOD_PROPERTY(L"Description", L"Simulates OneCoreTransforms mode on desktop with non 100% plateau scale")
    END_TEST_METHOD()

private:
    void SimulateOneCoreTransforms_PlateauScale(float plateauScale);

    inline Platform::String^ GetResourcesPath() const;
};

} } } } } } }
