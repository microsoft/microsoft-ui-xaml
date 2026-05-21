// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WUCRenderingScopeGuard.h>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class ShapeTests : public WEX::TestClass<ShapeTests>
{
public:
    BEGIN_TEST_CLASS(ShapeTests)
        TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
        TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d;d04573b8-e899-4822-bb72-9f4743c89d36")
        TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
        TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
    END_TEST_CLASS()

    TEST_CLASS_SETUP(ClassSetup)
    TEST_METHOD_SETUP(TestSetup)

    TEST_METHOD_CLEANUP(TestCleanup)

    BEGIN_TEST_METHOD(EllipseTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an ellipse using old rendering code. Changes size and colors.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(EllipseTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an ellipse using new WUC APIs. Sets various attributes on shape, changes them.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RectangleTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle using old rendering code. Changes size and colors.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RectangleTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle using new WUC APIs. Changes size and colors.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RectangleTestRoundAndSquare)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle using old rendering code. Rounds the corners, then squares them again.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RectangleTestRoundAndSquareWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle using new WUC APIs. Rounds the corners, then squares them.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ZeroSizedRectangle)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a 0x0 rectangle.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LineTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a line using old rendering code. Moves it and changes line caps.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LineTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a line using new WUC APIs. Moves it and changes line caps.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LineTestDash)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a line using old rendering code. Sets dash attributes, changes them.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LineTestDashWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a line using new WUC APIs. Sets dash attributes, changes them.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderPath)
        TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Mask is different
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(LargeStrokeClippedByLayout)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a Path where the stroke thickness spills outside the layout bounds.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ClippedPath)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a Path with a UIElement clip.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderPolygon)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderPolyline)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderPolylineIntersection)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderTransformsWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with basic transforms.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RenderMultiTransformsWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with a CompositeTransform and TransformGroup.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShapeWithLinearGradientBrush)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with a linear gradient brush using old rendering code.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShapeWithLinearGradientBrushWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with a linear gradient brush using WUC APIs.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShapeWithLinearGradientBrushAbsolute)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with a linear gradient brush and Absoulte MappingMode using old rendering code.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(ShapeWithLinearGradientBrushAbsoluteWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders a rectangle with a Linear gradient brush and Absoulte MappingMode using WUC APIs.")
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // WUCShapes_TODO: Temporarily disable until decisions with comp
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(StrokeDashArrayAssert)
        TEST_METHOD_PROPERTY(L"Description", L"Shape.StrokeDashArray is marked as created on-demand. We hit an assert if we try to create it during the render walk, because it marks the element dirty.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithLineTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using old rendering code.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithLineTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using WUC APIs.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithArcTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using WUC APIs, path consists of arcs.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithArcTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using old rendering code, path consists if arcs.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithBezierTestWUC)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using WUC APIs, path contains beziers.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathWithBezierTest)
        TEST_METHOD_PROPERTY(L"Description", L"Renders an path using old rendering code, path contains beziers.")
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(PathTransformTestWUC)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(RasterizationScale)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(MaxTextureSize)
        TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
    END_TEST_METHOD()

private:
    inline Platform::String^ GetResourcesPath() const;

    void EllipseTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void RectangleTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void RectangleTestRoundAndSquareInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void LineTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void LineTestDashInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void RenderPathInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void RenderPolygonInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void RenderPolylineInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void RenderPolylineIntersectionInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void PathTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void ShapeWithLinearGradientBrushInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
    void ShapeWithLinearGradientBrushAbsoluteInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void PathWithLineTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void PathWithArcTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    void PathWithBezierTestInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);

    // Temporarily put WUC versions of tests here to hide them from being executed
    void RenderPathWUC();
    void RenderPolygonWUC();
    void RenderPolylineWUC();
    void RenderPolylineIntersectionWUC();

};

} } } } } }

