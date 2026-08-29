// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ShapeTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <XamlFileTestEngine.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::Windows::Foundation;
using namespace ::Windows::Storage::Streams;

using namespace test_infra;
using namespace Concurrency;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ ShapeTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool ShapeTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ShapeTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ShapeTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::EllipseTest()
{
    EllipseTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::EllipseTestWUC()
{
    EllipseTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::EllipseTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Ellipse^ ellipse;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Height = 100;
        ellipse->Width = 100;
        ellipse->StrokeThickness = 10;
        ellipse->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        ellipse->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
        stackPanel->Children->Append(ellipse);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        ellipse->Height = 50;
        ellipse->Width = 50;
        ellipse->StrokeThickness = 5;
        ellipse->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        ellipse->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::RectangleTest()
{
    RectangleTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RectangleTestWUC()
{
    RectangleTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RectangleTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->StrokeThickness = 10;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        rect->Height = 50;
        rect->Width = 50;
        rect->StrokeThickness = 5;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

////////////////////////////

void ShapeTests::RectangleTestRoundAndSquare()
{
    RectangleTestRoundAndSquareInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RectangleTestRoundAndSquareWUC()
{
    RectangleTestRoundAndSquareInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RectangleTestRoundAndSquareInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->StrokeThickness = 10;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        rect->RadiusX = 10;
        rect->RadiusY = 10;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        rect->RadiusX = 0;
        rect->RadiusY = 0;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");
}

////////////////////////////

void ShapeTests::ZeroSizedRectangle()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto windowHelper = TestServices::WindowHelper;

    RunOnUIThread([&]()
    {
        xaml_shapes::Rectangle^ rect = ref new xaml_shapes::Rectangle();
        rect->StrokeThickness = 1;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);

        StackPanel^ stackPanel = ref new StackPanel();
        stackPanel->Width = 0;
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::LineTest()
{
    LineTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::LineTestWUC()
{
    LineTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::LineTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Line^ line;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        line = ref new xaml_shapes::Line();
        line->X1 = 10;
        line->Y1 = 10;
        line->X2 = 100;
        line->Y2 = 100;
        line->StrokeThickness = 20;
        line->StrokeStartLineCap = PenLineCap::Flat;
        line->StrokeEndLineCap = PenLineCap::Triangle;
        line->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        stackPanel->Children->Append(line);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        line->X1 = 20;
        line->Y1 = 20;
        line->X2 = 130;
        line->Y2 = 130;
        line->StrokeThickness = 10;
        line->StrokeStartLineCap = PenLineCap::Round;
        line->StrokeEndLineCap = PenLineCap::Square;
        line->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::LineTestDash()
{
    LineTestDashInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::LineTestDashWUC()
{
    LineTestDashInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::LineTestDashInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Line^ line;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        line = ref new xaml_shapes::Line();
        line->X1 = 10;
        line->Y1 = 10;
        line->X2 = 300;
        line->Y2 = 300;
        line->StrokeThickness = 20;

        line->StrokeDashCap = PenLineCap::Round;
        line->StrokeDashOffset = 0; // default
        line->StrokeDashArray = ref new DoubleCollection();
        line->StrokeDashArray->Append(2);
        line->StrokeDashArray->Append(4);

        line->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        stackPanel->Children->Append(line);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        line->StrokeDashCap = PenLineCap::Triangle;
        line->StrokeDashOffset = 10;
        line->StrokeDashArray = ref new DoubleCollection();
        line->StrokeDashArray->Append(4);
        line->StrokeDashArray->Append(2);

    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

void ShapeTests::PathWithLineTest()
{
    PathWithLineTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::PathWithLineTestWUC()
{
    PathWithLineTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::PathWithLineTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;

    xaml_shapes::Path^ path;
    PathGeometry^ pathGeometry;
    LineSegment^ line1;
    LineSegment^ line2;
    LineSegment^ line3;
    PathSegmentCollection^ segCollection;
    PathFigure^ pathFigure;

    //Two lines to form a unclosed path
    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        path = ref new xaml_shapes::Path();

        pathGeometry = ref new PathGeometry();

        line1 = ref new LineSegment();
        Point point1;
        point1.X = 100;
        point1.Y = 100;
        line1->Point = point1;

        line2 = ref new LineSegment();
        Point point2;
        point2.X = 100.79f;
        point2.Y = 50.81f;
        line2->Point = point2;

        segCollection = ref new PathSegmentCollection();
        segCollection->Append(line1);
        segCollection->Append(line2);

        pathFigure = ref new PathFigure();
        Point startPoint;
        startPoint.X = 10.001f;
        startPoint.Y = 100.999f;
        pathFigure->StartPoint = startPoint;
        pathFigure->Segments = segCollection;
        pathFigure->IsClosed = false;

        PathFigureCollection^ figCollection = ref new PathFigureCollection();
        figCollection->Append(pathFigure);

        pathGeometry->Figures = figCollection;

        path->Data = pathGeometry;
        path->Height = 200;
        path->Width = 200;
        path->StrokeThickness = 4;
        path->Stroke = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        path->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        canvas->Children->Append(path);
        windowHelper->WindowContent = canvas;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    //Add a line and form a closed path
    RunOnUIThread([&]()
    {
        line3 = ref new LineSegment();
        Point point3;
        point3.X = 10;
        point3.Y = 50;
        line3->Point = point3;

        segCollection->Append(line3);

        Point startPoint;
        startPoint.X = 10;
        startPoint.Y = 100;
        pathFigure->StartPoint = startPoint;
        pathFigure->IsClosed = true;

        path->StrokeThickness = 2;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

void ShapeTests::PathWithArcTest()
{
    PathWithArcTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::PathWithArcTestWUC()
{
    PathWithArcTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::PathWithArcTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;

    xaml_shapes::Path^ path;
    PathGeometry^ pathGeometry;
    ArcSegment^ arc1;
    ArcSegment^ arc2;
    LineSegment^ line1;

    PathSegmentCollection^ segCollection;
    PathFigure^ pathFigure;

    //One arc and one line to form a path
    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        path = ref new xaml_shapes::Path();

        pathGeometry = ref new PathGeometry();

        arc1 = ref new ArcSegment();
        Point point1;
        point1.X = 80;
        point1.Y = 80;
        arc1->Point = point1;
        Size size;
        size.Width = 50;
        size.Height = 50;
        arc1->Size = size;
        arc1->RotationAngle = 45;
        arc1->IsLargeArc = false;
        arc1->SweepDirection = SweepDirection::Clockwise;

        line1 = ref new LineSegment();
        Point point2;
        point2.X = 150;
        point2.Y = 150;
        line1->Point = point2;

        segCollection = ref new PathSegmentCollection();
        segCollection->Append(arc1);
        segCollection->Append(line1);

        pathFigure = ref new PathFigure();
        Point startPoint;
        startPoint.X = 20.01f;
        startPoint.Y = 20.99f;
        pathFigure->StartPoint = startPoint;
        pathFigure->Segments = segCollection;
        pathFigure->IsClosed = false;

        PathFigureCollection^ figCollection = ref new PathFigureCollection();
        figCollection->Append(pathFigure);

        pathGeometry->Figures = figCollection;

        path->Data = pathGeometry;
        path->Height = 200;
        path->Width = 200;
        path->StrokeThickness = 4;
        path->Stroke = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        path->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        canvas->Children->Append(path);
        windowHelper->WindowContent = canvas;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    //Add an arc to form an closed path
    RunOnUIThread([&]()
    {
        arc2 = ref new ArcSegment();
        Point point;
        point.X = 60;
        point.Y = 120;
        arc2->Point = point;
        Size size;
        size.Width = 50;
        size.Height = 50;
        arc2->Size = size;
        arc2->RotationAngle = 45;
        arc2->IsLargeArc = false;
        arc2->SweepDirection = SweepDirection::Counterclockwise;

        segCollection->Append(arc2);
        pathFigure->IsClosed = true;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");
}

void ShapeTests::PathWithBezierTest()
{
    PathWithBezierTestInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::PathWithBezierTestWUC()
{
    PathWithBezierTestInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::PathWithBezierTestInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;

    xaml_shapes::Path^ path;
    PathGeometry^ pathGeometry;
    PathFigure^ pathFigure;

    //A combination of Bezier, quadratic Bezier, line and arc
    //to form a unclosed path
    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        path = ref new xaml_shapes::Path();

        pathGeometry = ref new PathGeometry();

        BezierSegment^ bezier1 = ref new BezierSegment();
        Point bPoint1;
        bPoint1.X = 100;
        bPoint1.Y = 0;
        Point bPoint2;
        bPoint2.X = 200;
        bPoint2.Y = 200;
        Point bPoint3;
        bPoint3.X = 300;
        bPoint3.Y = 100;
        bezier1->Point1 = bPoint1;
        bezier1->Point2 = bPoint2;
        bezier1->Point3 = bPoint3;

        LineSegment^ line1 = ref new LineSegment();
        Point lPoint;
        lPoint.X = 250;
        lPoint.Y = 250;
        line1->Point = lPoint;

        ArcSegment^ arc = ref new ArcSegment();
        Point aPoint;
        aPoint.X = 200;
        aPoint.Y = 200;
        arc->Point = aPoint;
        Size size;
        size.Width = 50;
        size.Height = 50;
        arc->Size = size;
        arc->RotationAngle = 45;
        arc->IsLargeArc = false;
        arc->SweepDirection = SweepDirection::Clockwise;

        QuadraticBezierSegment^ qBezier = ref new  QuadraticBezierSegment();
        Point qbPoint1;
        qbPoint1.X = 175;
        qbPoint1.Y = 175;
        Point qbPoint2;
        qbPoint2.X = 150;
        qbPoint2.Y = 100;
        qBezier->Point1 = qbPoint1;
        qBezier->Point2 = qbPoint2;

        PathSegmentCollection^ segCollection = ref new PathSegmentCollection();
        segCollection->Append(bezier1);
        segCollection->Append(line1);
        segCollection->Append(arc);
        segCollection->Append(qBezier);

        pathFigure = ref new PathFigure();
        Point startPoint;
        startPoint.X = 10.01f;
        startPoint.Y = 10.99f;
        pathFigure->StartPoint = startPoint;
        pathFigure->Segments = segCollection;
        pathFigure->IsClosed = false;

        PathFigureCollection^ figCollection = ref new PathFigureCollection();
        figCollection->Append(pathFigure);

        pathGeometry->Figures = figCollection;

        path->Data = pathGeometry;
        path->Height = 400;
        path->Width = 400;
        path->StrokeThickness = 4;
        path->Stroke = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        path->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        canvas->Children->Append(path);
        windowHelper->WindowContent = canvas;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "1");

    //close the path
    RunOnUIThread([&]()
    {
        pathFigure->IsClosed = true;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "2");

    //A PolyBezier and a PolyQuadraticBezier to form an closed path
    RunOnUIThread([&]()
    {
        PolyBezierSegment^ pBezier = ref new PolyBezierSegment();
        PointCollection^ pointCollection1 = ref new PointCollection();
        Point pbPoint1;
        pbPoint1.X = 0;
        pbPoint1.Y = 0;
        Point pbPoint2;
        pbPoint2.X = 100;
        pbPoint2.Y = 0;
        Point pbPoint3;
        pbPoint3.X = 200;
        pbPoint3.Y = 50;
        Point pbPoint4;
        pbPoint4.X = 200;
        pbPoint4.Y = 0;
        Point pbPoint5;
        pbPoint5.X = 300;
        pbPoint5.Y = 0;
        Point pbPoint6;
        pbPoint6.X = 400;
        pbPoint6.Y = 50;
        pointCollection1->Append(pbPoint1);
        pointCollection1->Append(pbPoint2);
        pointCollection1->Append(pbPoint3);
        pointCollection1->Append(pbPoint4);
        pointCollection1->Append(pbPoint5);
        pointCollection1->Append(pbPoint6);
        pBezier->Points = pointCollection1;

        PolyQuadraticBezierSegment^ pqBezier = ref new PolyQuadraticBezierSegment();
        PointCollection^ pointCollection2 = ref new PointCollection();
        Point pqbPoint1;
        pqbPoint1.X = 200;
        pqbPoint1.Y = 200;
        Point pqbPoint2;
        pqbPoint2.X = 300;
        pqbPoint2.Y = 100;
        Point pqbPoint3;
        pqbPoint3.X = 0;
        pqbPoint3.Y = 200;
        Point pqbPoint4;
        pqbPoint4.X = 30;
        pqbPoint4.Y = 400;
        pointCollection2->Append(pqbPoint1);
        pointCollection2->Append(pqbPoint2);
        pointCollection2->Append(pqbPoint3);
        pointCollection2->Append(pqbPoint4);
        pqBezier->Points = pointCollection2;

        PathSegmentCollection^ segCollection = ref new PathSegmentCollection();
        segCollection->Append(pBezier);
        segCollection->Append(pqBezier);

        Point startPoint;
        startPoint.X = 10;
        startPoint.Y = 100;
        pathFigure->StartPoint = startPoint;
        pathFigure->Segments = segCollection;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "3");
}

void ShapeTests::PathTransformTestWUC()
{
    //We first form a path that consists of line, arc, Bezier and quadraticBezier
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCShapesSynchronousCompTree);

    auto windowHelper = TestServices::WindowHelper;

    xaml_shapes::Path^ path;
    PathGeometry^ pathGeometry;
    PathFigure^ pathFigure;

    RunOnUIThread([&]()
    {
        Canvas^ canvas = ref new Canvas();
        path = ref new xaml_shapes::Path();

        pathGeometry = ref new PathGeometry();

        BezierSegment^ bezier1 = ref new BezierSegment();
        Point bPoint1;
        bPoint1.X = 100;
        bPoint1.Y = 0;
        Point bPoint2;
        bPoint2.X = 200;
        bPoint2.Y = 200;
        Point bPoint3;
        bPoint3.X = 300;
        bPoint3.Y = 100;
        bezier1->Point1 = bPoint1;
        bezier1->Point2 = bPoint2;
        bezier1->Point3 = bPoint3;

        LineSegment^ line1 = ref new LineSegment();
        Point lPoint;
        lPoint.X = 250;
        lPoint.Y = 250;
        line1->Point = lPoint;

        ArcSegment^ arc = ref new ArcSegment();
        Point aPoint;
        aPoint.X = 200;
        aPoint.Y = 200;
        arc->Point = aPoint;
        Size size;
        size.Width = 50;
        size.Height = 50;
        arc->Size = size;
        arc->RotationAngle = 45;
        arc->IsLargeArc = false;
        arc->SweepDirection = SweepDirection::Clockwise;

        QuadraticBezierSegment^ qBezier = ref new  QuadraticBezierSegment();
        Point qbPoint1;
        qbPoint1.X = 175;
        qbPoint1.Y = 175;
        Point qbPoint2;
        qbPoint2.X = 150;
        qbPoint2.Y = 100;
        qBezier->Point1 = qbPoint1;
        qBezier->Point2 = qbPoint2;

        PathSegmentCollection^ segCollection = ref new PathSegmentCollection();
        segCollection->Append(bezier1);
        segCollection->Append(line1);
        segCollection->Append(arc);
        segCollection->Append(qBezier);

        pathFigure = ref new PathFigure();
        Point startPoint;
        startPoint.X = 10.01f;
        startPoint.Y = 10.99f;
        pathFigure->StartPoint = startPoint;
        pathFigure->Segments = segCollection;
        pathFigure->IsClosed = false;

        PathFigureCollection^ figCollection = ref new PathFigureCollection();
        figCollection->Append(pathFigure);

        pathGeometry->Figures = figCollection;

        path->Data = pathGeometry;
        path->Height = 200;
        path->Width = 200;
        path->StrokeThickness = 4;
        path->Stroke = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
        path->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

        canvas->Children->Append(path);
        windowHelper->WindowContent = canvas;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    //Transform on the shape: RotateTransform
    RunOnUIThread([&]()
    {
        auto rotate = ref new RotateTransform();
        rotate->Angle = 45;
        path->RenderTransform = rotate;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    //Transform on the shape: ScaleTransform
    RunOnUIThread([&]()
    {
        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 3;
        path->RenderTransform = scale;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

    //Transform on the shape: SkewTransform
    RunOnUIThread([&]()
    {
        auto skew = ref new SkewTransform();
        skew->AngleX = -30;
        skew->AngleY = 10;
        path->RenderTransform = skew;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");

    //Transform on the shape: TranslateTransform
    RunOnUIThread([&]()
    {
        auto translate = ref new TranslateTransform();
        translate->X = 50;
        translate->Y = 50;
        path->RenderTransform = translate;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "5");

    //Transform on the shape: MatrixTransform
    RunOnUIThread([&]()
    {
        auto transform = ref new MatrixTransform();
        Matrix matrix = { 1, 0.5, 1, 1, 50, 50 };
        transform->Matrix = matrix;
        path->RenderTransform = transform;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "6");

    //Transform on the shape: CompositeTransform
    RunOnUIThread([&]()
    {
        // Create composite transform of a scale, skew, and rotate transform
        auto composite = ref new CompositeTransform();
        composite->SkewX = 30;
        composite->Rotation = 45;
        composite->ScaleX = 0.8;
        composite->ScaleY = 0.8;
        path->RenderTransform = composite;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "7");

    //Transform on the shape: TransformGroup
    RunOnUIThread([&]()
    {
        auto group = ref new TransformGroup();

        auto skew = ref new SkewTransform();
        skew->AngleX = -30;
        skew->AngleY = 10;

        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 3;

        group->Children->Append(skew);
        group->Children->Append(scale);

        path->RenderTransform = group;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "8");
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::RenderPath()
{
    RenderPathInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RenderPathWUC()
{
    RenderPathInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RenderPathInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;

    StackPanel^ root = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"Path.xaml"));
    RunOnUIThread([&]()
    {
        windowHelper->WindowContent = root;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::RenderPolygon()
{
    RenderPolygonInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RenderPolygonWUC()
{
    RenderPolygonInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RenderPolygonInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Polygon^ polygon;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();

        polygon = ref new xaml_shapes::Polygon();
        polygon->StrokeThickness = 5;
        polygon->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        polygon->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

        auto points = ref new PointCollection();
        points->Append(::Windows::Foundation::Point(10, 200));
        points->Append(::Windows::Foundation::Point(60, 140));
        points->Append(::Windows::Foundation::Point(130, 140));
        points->Append(::Windows::Foundation::Point(180, 200));
        polygon->Points = points;

        stackPanel->Children->Append(polygon);

        windowHelper->WindowContent = stackPanel;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::RenderPolyline()
{
    RenderPolylineInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RenderPolylineWUC()
{
    RenderPolylineInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RenderPolylineInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Polyline^ polyline;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();

        polyline = ref new xaml_shapes::Polyline();
        polyline->StrokeThickness = 5;
        polyline->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);
        polyline->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

        auto points = ref new PointCollection();
        points->Append(::Windows::Foundation::Point(10, 200));
        points->Append(::Windows::Foundation::Point(60, 140));
        points->Append(::Windows::Foundation::Point(130, 140));
        points->Append(::Windows::Foundation::Point(180, 200));
        polyline->Points = points;

        stackPanel->Children->Append(polyline);

        windowHelper->WindowContent = stackPanel;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

////////////////////////////

void ShapeTests::RenderPolylineIntersection()
{
    RenderPolylineIntersectionInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::RenderPolylineIntersectionWUC()
{
    RenderPolylineIntersectionInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::RenderPolylineIntersectionInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Polyline^ line;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        line = ref new xaml_shapes::Polyline();
        line->StrokeThickness = 10;
        line->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);

        auto points = ref new PointCollection();
        points->Append(::Windows::Foundation::Point(100, 100));
        points->Append(::Windows::Foundation::Point(300, 150));
        points->Append(::Windows::Foundation::Point(100, 200));
        line->Points = points;

        line->StrokeLineJoin = PenLineJoin::Miter;

        stackPanel->Children->Append(line);
        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        line->StrokeLineJoin = PenLineJoin::Round;
        line->StrokeMiterLimit = 1.5;

    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        line->StrokeLineJoin = PenLineJoin::Bevel;
        line->StrokeMiterLimit = 2.0;

    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::RenderTransformsWUC()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCShapesSynchronousCompTree);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->StrokeThickness = 5;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        auto rotate = ref new RotateTransform();
        rotate->Angle = 45;
        rect->RenderTransform = rotate;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 3;
        rect->RenderTransform = scale;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

    RunOnUIThread([&]()
    {
        auto skew = ref new SkewTransform();
        skew->AngleX = -30;
        skew->AngleY = 10;
        rect->RenderTransform = skew;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "4");

    RunOnUIThread([&]()
    {
        auto translate = ref new TranslateTransform();
        translate->X = 50;
        translate->Y = 50;
        rect->RenderTransform = translate;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "5");

    RunOnUIThread([&]()
    {
        auto transform = ref new MatrixTransform();
        Matrix matrix = { 1, 0.5, 1, 1, 50, 50 };
        transform->Matrix = matrix;
        rect->RenderTransform = transform;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "6");
}

////////////////////////////

void ShapeTests::RenderMultiTransformsWUC()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCShapesSynchronousCompTree);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 100;
        rect->Height = 100;
        rect->StrokeThickness = 5;
        rect->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        rect->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Yellow);
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        // Create composite transform of a scale, skew, and rotate transform
        auto composite = ref new CompositeTransform();
        composite->SkewX = 30;
        composite->Rotation = 45;
        composite->ScaleX = 0.8;
        composite->ScaleY = 0.8;
        rect->RenderTransform = composite;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        auto group = ref new TransformGroup();

        auto skew = ref new SkewTransform();
        skew->AngleX = -30;
        skew->AngleY = 10;

        auto scale = ref new ScaleTransform();
        scale->ScaleX = 2;
        scale->ScaleY = 3;

        group->Children->Append(skew);
        group->Children->Append(scale);

        rect->RenderTransform = group;
    });
    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::ShapeWithLinearGradientBrush()
{
    ShapeWithLinearGradientBrushInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::ShapeWithLinearGradientBrushWUC()
{
    ShapeWithLinearGradientBrushInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::ShapeWithLinearGradientBrushInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;

    RunOnUIThread([&]()
    {
        GradientStop^ stop1a = ref new GradientStop();
        stop1a->Color = Microsoft::UI::Colors::Green;
        stop1a->Offset = 0.39;

        GradientStop^ stop1b = ref new GradientStop();
        stop1b->Color = Microsoft::UI::Colors::Blue;
        stop1b->Offset = 0.41;

        GradientStopCollection^ stopCollection1 = ref new GradientStopCollection();
        stopCollection1->Append(stop1a);
        stopCollection1->Append(stop1b);

        LinearGradientBrush^ strokeBrush = ref new LinearGradientBrush();
        strokeBrush->StartPoint = ::Windows::Foundation::Point(0.0f, 0);
        strokeBrush->EndPoint = ::Windows::Foundation::Point(1.0f, 0);
        strokeBrush->GradientStops = stopCollection1;

        GradientStop^ stop2a = ref new GradientStop();
        stop2a->Color = Microsoft::UI::Colors::Red;
        stop2a->Offset = 0.39;

        GradientStop^ stop2b = ref new GradientStop();
        stop2b->Color = Microsoft::UI::Colors::Orange;
        stop2b->Offset = 0.41;

        GradientStopCollection^ stopCollection2 = ref new GradientStopCollection();
        stopCollection2->Append(stop2a);
        stopCollection2->Append(stop2b);

        LinearGradientBrush^ fillBrush = ref new LinearGradientBrush();
        fillBrush->StartPoint = ::Windows::Foundation::Point(0.0f, 0);
        fillBrush->EndPoint = ::Windows::Foundation::Point(1.0f, 0);
        fillBrush->GradientStops = stopCollection2;

        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 200;
        rect->Height = 100;
        rect->StrokeThickness = 20;
        rect->Stroke = strokeBrush;
        rect->Fill = fillBrush;
        stackPanel->Children->Append(rect);

        windowHelper->WindowContent = stackPanel;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

////////////////////////////

void ShapeTests::StrokeDashArrayAssert()
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Ellipse^ ellipse;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating Ellipse under BitmapCache.");
        ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 100;
        ellipse->Height = 100;
        ellipse->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
        ellipse->StrokeThickness = 10;

        StackPanel^ root = ref new StackPanel();
        root->CacheMode = ref new BitmapCache();
        root->Children->Append(ellipse);
        windowHelper->WindowContent = root;
    });
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving Ellipse a Stroke.");
        ellipse->Stroke = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);
    });
    LOG_OUTPUT(L"> Do not hit an assert when rendering the Stroke without a StrokeDashArray.");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Giving Ellipse a StrokeDashArray.");
        ellipse->StrokeDashArray = ref new DoubleCollection();
    });
    LOG_OUTPUT(L"> Do not hit an assert when rendering a blank StrokeDashArray.");
    windowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Removing StrokeDashArray.");
        ellipse->StrokeDashArray = nullptr;
    });
    LOG_OUTPUT(L"> Do not crash when rendering a null StrokeDashArray.");
    windowHelper->WaitForIdle();
}

////////////////////////////////////////////////////////////////////////////

void ShapeTests::ShapeWithLinearGradientBrushAbsolute()
{
    ShapeWithLinearGradientBrushAbsoluteInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ShapeTests::ShapeWithLinearGradientBrushAbsoluteWUC()
{
    ShapeWithLinearGradientBrushAbsoluteInternal(DCompRendering::WUCShapesSynchronousCompTree);
}

void ShapeTests::ShapeWithLinearGradientBrushAbsoluteInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(dcompRendering);

    auto windowHelper = TestServices::WindowHelper;
    xaml_shapes::Rectangle^ rect;
    xaml_shapes::Rectangle^ rect2;

    RunOnUIThread([&]()
    {
        GradientStop^ stop1a = ref new GradientStop();
        stop1a->Color = Microsoft::UI::Colors::Green;
        stop1a->Offset = 0.01;

        GradientStop^ stop1b = ref new GradientStop();
        stop1b->Color = Microsoft::UI::Colors::Blue;
        stop1b->Offset = 0.03;

        GradientStopCollection^ stopCollection1 = ref new GradientStopCollection();
        stopCollection1->Append(stop1a);
        stopCollection1->Append(stop1b);

        LinearGradientBrush^ strokeBrush = ref new LinearGradientBrush();
        strokeBrush->MappingMode = BrushMappingMode::Absolute;

        strokeBrush->StartPoint = ::Windows::Foundation::Point(10.0f, 0);
        strokeBrush->EndPoint = ::Windows::Foundation::Point(30.0f, 0);
        strokeBrush->GradientStops = stopCollection1;

        GradientStop^ stop2a = ref new GradientStop();
        stop2a->Color = Microsoft::UI::Colors::Red;
        stop2a->Offset = 0.01;

        GradientStop^ stop2b = ref new GradientStop();
        stop2b->Color = Microsoft::UI::Colors::Orange;
        stop2b->Offset = 0.03;

        GradientStopCollection^ stopCollection2 = ref new GradientStopCollection();
        stopCollection2->Append(stop2a);
        stopCollection2->Append(stop2b);

        LinearGradientBrush^ fillBrush = ref new LinearGradientBrush();
        fillBrush->MappingMode = BrushMappingMode::Absolute;
        fillBrush->StartPoint = ::Windows::Foundation::Point(10.0f, 0);
        fillBrush->EndPoint = ::Windows::Foundation::Point(30.0f, 0);
        fillBrush->GradientStops = stopCollection2;

        StackPanel^ stackPanel = ref new StackPanel();
        rect = ref new xaml_shapes::Rectangle();
        rect->Width = 200;
        rect->Height = 100;
        rect->StrokeThickness = 20;
        rect->Stroke = strokeBrush;
        rect->Fill = fillBrush;
        stackPanel->Children->Append(rect);

        rect2 = ref new xaml_shapes::Rectangle();
        rect2->Width = 200;
        rect2->Height = 100;
        rect2->Fill = fillBrush;
        stackPanel->Children->Append(rect2);

        windowHelper->WindowContent = stackPanel;
    });

    windowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void ShapeTests::RasterizationScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 100;
        ellipse->Height = 100;
        ellipse->Stroke = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        ellipse->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        ellipse->StrokeThickness = 10;

        xaml_shapes::Rectangle^ rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 50;
        rectangle->Height = 50;
        rectangle->RasterizationScale = 2;
        rectangle->Stroke = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        rectangle->StrokeThickness = 10;
        rectangle->RadiusX = 20;
        rectangle->RadiusY = 10;

        PathGeometry^ pathGeometry = ref new PathGeometry();
        {
            auto line1 = ref new LineSegment();
            Point point1;
            point1.X = 100;
            point1.Y = 100;
            line1->Point = point1;

            auto line2 = ref new LineSegment();
            Point point2;
            point2.X = 50;
            point2.Y = 100;
            line2->Point = point2;

            auto segCollection = ref new PathSegmentCollection();
            segCollection->Append(line1);
            segCollection->Append(line2);

            auto pathFigure = ref new PathFigure();
            Point startPoint;
            startPoint.X = 20;
            startPoint.Y = 10;
            pathFigure->StartPoint = startPoint;
            pathFigure->Segments = segCollection;
            pathFigure->IsClosed = false;

            PathFigureCollection^ figCollection = ref new PathFigureCollection();
            figCollection->Append(pathFigure);

            pathGeometry->Figures = figCollection;
        }

        xaml_shapes::Path^ path = ref new xaml_shapes::Path();
        path->Width = 100;
        path->Height = 100;
        path->Data = pathGeometry;
        path->Stroke = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        path->StrokeThickness = 10;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->RasterizationScale = 2;
        rootCanvas->Children->Append(ellipse);
        rootCanvas->Children->Append(rectangle);
        Canvas::SetTop(rectangle, 50);
        rootCanvas->Children->Append(path);
        Canvas::SetTop(path, 100);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ShapeTests::LargeStrokeClippedByLayout()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        PathGeometry^ pathGeometry = ref new PathGeometry();
        {
            auto line1 = ref new LineSegment();
            Point point1;
            point1.X = 100;
            point1.Y = 100;
            line1->Point = point1;

            auto line2 = ref new LineSegment();
            Point point2;
            point2.X = 50;
            point2.Y = 100;
            line2->Point = point2;

            auto segCollection = ref new PathSegmentCollection();
            segCollection->Append(line1);
            segCollection->Append(line2);

            auto pathFigure = ref new PathFigure();
            Point startPoint;
            startPoint.X = 20;
            startPoint.Y = 10;
            pathFigure->StartPoint = startPoint;
            pathFigure->Segments = segCollection;
            pathFigure->IsClosed = false;

            PathFigureCollection^ figCollection = ref new PathFigureCollection();
            figCollection->Append(pathFigure);

            pathGeometry->Figures = figCollection;
        }

        xaml_shapes::Path^ path = ref new xaml_shapes::Path();
        path->Width = 100;
        path->Height = 100;
        path->Data = pathGeometry;
        path->Stroke = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        path->StrokeThickness = 50;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(path);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ShapeTests::ClippedPath()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        PathGeometry^ pathGeometry = ref new PathGeometry();
        {
            auto line1 = ref new LineSegment();
            Point point1;
            point1.X = 100;
            point1.Y = 100;
            line1->Point = point1;

            auto line2 = ref new LineSegment();
            Point point2;
            point2.X = 50;
            point2.Y = 100;
            line2->Point = point2;

            auto segCollection = ref new PathSegmentCollection();
            segCollection->Append(line1);
            segCollection->Append(line2);

            auto pathFigure = ref new PathFigure();
            Point startPoint;
            startPoint.X = 20;
            startPoint.Y = 10;
            pathFigure->StartPoint = startPoint;
            pathFigure->Segments = segCollection;
            pathFigure->IsClosed = false;

            PathFigureCollection^ figCollection = ref new PathFigureCollection();
            figCollection->Append(pathFigure);

            pathGeometry->Figures = figCollection;
        }

        RectangleGeometry^ clip = ref new RectangleGeometry();
        clip->Rect = Rect(0, 0, 50, 50);

        xaml_shapes::Path^ path = ref new xaml_shapes::Path();
        path->Width = 100;
        path->Height = 100;
        path->Data = pathGeometry;
        path->Stroke = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        path->StrokeThickness = 10;
        path->Clip = clip;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(path);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ShapeTests::MaxTextureSize()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 10000; // Needs to be scaled down
        ellipse->Height = 50;   // Fits in max texture size
        ellipse->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(ellipse);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } }
