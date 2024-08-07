﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//
//       LottieGen version:
//           7.1.0+ge1fa92580f
//       
//       Command:
//           LottieGen -Language Cppwinrt -WinUIVersion 2.4 -InputFile AnimatedChevronRightDownSmallVisualSource.json
//       
//       Input file:
//           AnimatedChevronRightDownSmallVisualSource.json (27906 bytes created 23:37-07:00 Oct 5 2021)
//       
//       LottieGen source:
//           http://aka.ms/Lottie
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------
// ____________________________________
// |       Object stats       | Count |
// |__________________________|_______|
// | All CompositionObjects   |    37 |
// |--------------------------+-------|
// | Expression animators     |     3 |
// | KeyFrame animators       |     2 |
// | Reference parameters     |     3 |
// | Expression operations    |     4 |
// |--------------------------+-------|
// | Animated brushes         |     1 |
// | Animated gradient stops  |     - |
// | ExpressionAnimations     |     2 |
// | PathKeyFrameAnimations   |     - |
// |--------------------------+-------|
// | ContainerVisuals         |     2 |
// | ShapeVisuals             |     1 |
// |--------------------------+-------|
// | ContainerShapes          |     1 |
// | CompositionSpriteShapes  |     1 |
// |--------------------------+-------|
// | Brushes                  |     1 |
// | Gradient stops           |     - |
// | CompositionVisualSurface |     - |
// ------------------------------------
#include "pch.h"
#include "AnimatedVisuals\AnimatedChevronRightDownSmallVisualSource.h"
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Composition.h>
#include "d2d1.h"
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <Windows.Graphics.Interop.h>
#include <winrt/Windows.Graphics.Effects.h>

using namespace winrt::Microsoft::UI::Xaml::Controls;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Numerics;
using namespace winrt::Windows::Graphics;
using namespace winrt::Windows::UI;
using namespace winrt::Microsoft::UI::Composition;
using TimeSpan = winrt::Windows::Foundation::TimeSpan;

namespace winrt::Microsoft::UI::Xaml::Controls::AnimatedVisuals
{
    CppWinRTActivatableClassWithBasicFactory(AnimatedChevronRightDownSmallVisualSource)
}
#include "AnimatedVisuals\AnimatedChevronRightDownSmallVisualSource.g.cpp"

class CanvasGeometry : public winrt::implements<CanvasGeometry,
    IGeometrySource2D,
    ::ABI::Windows::Graphics::IGeometrySource2DInterop>
{
    winrt::com_ptr<ID2D1Geometry> _geometry{ nullptr };

public:
    CanvasGeometry(winrt::com_ptr<ID2D1Geometry> geometry)
        : _geometry{ geometry }
    { }

    // IGeometrySource2D.
    winrt::com_ptr<ID2D1Geometry> Geometry() { return _geometry; }

    // IGeometrySource2DInterop.
    IFACEMETHODIMP GetGeometry(ID2D1Geometry** value) override
    {
        _geometry.copy_to(value);
        return S_OK;
    }

    // IGeometrySource2DInterop.
    IFACEMETHODIMP TryGetGeometryUsingFactory(ID2D1Factory*, ID2D1Geometry**) override
    {
        return E_NOTIMPL;
    }
};
class AnimatedChevronRightDownSmallVisualSource_AnimatedVisual : public winrt::implements<AnimatedChevronRightDownSmallVisualSource_AnimatedVisual,
        winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual,
        IClosable>
{
    winrt::com_ptr<ID2D1Factory> _d2dFactory{ nullptr };
    static constexpr int64_t c_durationTicks{ 36666666L };
    Compositor const _c{ nullptr };
    ExpressionAnimation const _reusableExpressionAnimation{ nullptr };
    CompositionPropertySet const _themeProperties{ nullptr };
    ContainerVisual _root{ nullptr };
    CubicBezierEasingFunction _cubicBezierEasingFunction_0{ nullptr };
    CubicBezierEasingFunction _cubicBezierEasingFunction_1{ nullptr };
    CubicBezierEasingFunction _cubicBezierEasingFunction_2{ nullptr };
    ExpressionAnimation _rootProgress{ nullptr };
    StepEasingFunction _holdThenStepEasingFunction{ nullptr };

    static void StartProgressBoundAnimation(
        CompositionObject target,
        winrt::hstring animatedPropertyName,
        CompositionAnimation animation,
        ExpressionAnimation controllerProgressExpression)
    {
        target.StartAnimation(animatedPropertyName, animation);
        const auto controller = target.TryGetAnimationController(animatedPropertyName);
        controller.Pause();
        controller.StartAnimation(L"Progress", controllerProgressExpression);
    }

    void BindProperty(
        CompositionObject target,
        winrt::hstring animatedPropertyName,
        winrt::hstring expression,
        winrt::hstring referenceParameterName,
        CompositionObject referencedObject)
    {
        _reusableExpressionAnimation.ClearAllParameters();
        _reusableExpressionAnimation.Expression(expression);
        _reusableExpressionAnimation.SetReferenceParameter(referenceParameterName, referencedObject);
        target.StartAnimation(animatedPropertyName, _reusableExpressionAnimation);
    }

    ScalarKeyFrameAnimation CreateScalarKeyFrameAnimation(float initialProgress, float initialValue, CompositionEasingFunction initialEasingFunction)
    {
        const auto result = _c.CreateScalarKeyFrameAnimation();
        result.Duration(TimeSpan{ c_durationTicks });
        result.InsertKeyFrame(initialProgress, initialValue, initialEasingFunction);
        return result;
    }

    Vector2KeyFrameAnimation CreateVector2KeyFrameAnimation(float initialProgress, winrt::float2 initialValue, CompositionEasingFunction initialEasingFunction)
    {
        const auto result = _c.CreateVector2KeyFrameAnimation();
        result.Duration(TimeSpan{ c_durationTicks });
        result.InsertKeyFrame(initialProgress, initialValue, initialEasingFunction);
        return result;
    }

    CompositionSpriteShape CreateSpriteShape(CompositionGeometry geometry, winrt::float3x2 transformMatrix, CompositionBrush fillBrush)
    {
        const auto result = _c.CreateSpriteShape(geometry);
        result.TransformMatrix(transformMatrix);
        result.FillBrush(fillBrush);
        return result;
    }

    // - - - - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - - - - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2
    // - - ShapeGroup: Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // RotationDegrees:270
    winrt::com_ptr<CanvasGeometry> Geometry()
    {
        winrt::com_ptr<ID2D1PathGeometry> path{ nullptr };
        winrt::check_hresult(_d2dFactory->CreatePathGeometry(path.put()));
        winrt::com_ptr<ID2D1GeometrySink> sink{ nullptr };
        winrt::check_hresult(path->Open(sink.put()));
        sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        sink->BeginFigure({ -3.85400009F, -2.10400009F }, D2D1_FIGURE_BEGIN_FILLED);
        sink->AddBezier({ { -3.65899992F, -2.29900002F }, { -3.34100008F, -2.29900002F }, { -3.14599991F, -2.10400009F } });
        sink->AddLine({ 0.0F, 1.04299998F });
        sink->AddLine({ 3.14599991F, -2.10400009F });
        sink->AddBezier({ { 3.34100008F, -2.29900002F }, { 3.65899992F, -2.29900002F }, { 3.85400009F, -2.10400009F } });
        sink->AddBezier({ { 4.04899979F, -1.90900004F }, { 4.04899979F, -1.59099996F }, { 3.85400009F, -1.39600003F } });
        sink->AddLine({ 0.354000002F, 2.10400009F });
        sink->AddBezier({ { 0.158999994F, 2.29900002F }, { -0.158999994F, 2.29900002F }, { -0.354000002F, 2.10400009F } });
        sink->AddLine({ -3.85400009F, -1.39600003F });
        sink->AddBezier({ { -4.04899979F, -1.59099996F }, { -4.04899979F, -1.90900004F }, { -3.85400009F, -2.10400009F } });
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
        winrt::check_hresult(sink->Close());
        auto result = winrt::make_self<CanvasGeometry>(path);
        return result;
    }

    // - - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2
    // ShapeGroup: Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // RotationDegrees:270
    // Color bound to theme property value: Foreground
    CompositionColorBrush ThemeColor_Foreground()
    {
        const auto result = _c.CreateColorBrush();
        BindProperty(result, L"Color", L"ColorRGB(_theme.Foreground.W,_theme.Foreground.X,_theme.Foreground.Y,_theme.Foreground.Z)", L"_theme", _themeProperties);
        return result;
    }

    // - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff Controls
    // - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 - TreeViewExpand
    // - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2
    CompositionContainerShape ContainerShape()
    {
        const auto result = _c.CreateContainerShape();
        result.Scale({ 4.0F, 4.0F });
        // ShapeGroup: Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
        // Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
        // RotationDegrees:270
        result.Shapes().Append(SpriteShape());
        StartProgressBoundAnimation(result, L"RotationAngleInDegrees", RotationAngleInDegreesScalarAnimation_90_to_90(), RootProgress());
        StartProgressBoundAnimation(result, L"Offset", OffsetVector2Animation(), _rootProgress);
        return result;
    }

    // - - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2
    // ShapeGroup: Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1 Group 1
    // RotationDegrees:270
    CompositionPathGeometry PathGeometry()
    {
        return _c.CreatePathGeometry(CompositionPath(CanvasGeometryToIGeometrySource2D(Geometry())));
    }

    // - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer
    // 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2
    // Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1 Path 1
    // Path 1 Path 1 Path 1 Path 1 Path 1 Path 1
    CompositionSpriteShape SpriteShape()
    {
        // Rotation:-90 degrees
        const auto geometry = PathGeometry();
        const auto result = CreateSpriteShape(geometry, { 0.0F, -1.0F, 1.0F, 0.0F, 0.0F, 0.0F }, ThemeColor_Foreground());
        return result;
    }

    // Transforms for Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff Controls -
    // 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 - TreeViewExpand -
    // 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 - NormalOffToNormalOn
    // Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff Controls - 08 -
    // TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 - TreeViewExpand - 07 -
    // PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand - 08 -
    // PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn Scale(1,1,0)
    ContainerVisual ContainerVisual_0()
    {
        const auto result = _c.CreateContainerVisual();
        result.Clip(InsetClip_0());
        result.Size({ 48.0F, 48.0F });
        // Scale:<1, 1>
        result.TransformMatrix({ 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F });
        // Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
        // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
        // Layer 2
        result.Children().InsertAtTop(ShapeVisual_0());
        return result;
    }

    // The root of the composition.
    ContainerVisual Root()
    {
        const auto result = _root = _c.CreateContainerVisual();
        const auto propertySet = result.Properties();
        propertySet.InsertScalar(L"Progress", 0.0F);
        // PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff Controls -
        // 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 - TreeViewExpand -
        // 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 - NormalOffToNormalOn
        // Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff Controls - 08 -
        // TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 - TreeViewExpand - 07 -
        // PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand - 08 -
        // PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
        // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
        // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
        // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
        // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
        // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
        // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
        // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
        // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
        // TreeViewExpand - 18 - PressedOffToNormalOn
        result.Children().InsertAtTop(ContainerVisual_0());
        return result;
    }

    CubicBezierEasingFunction CubicBezierEasingFunction_0()
    {
        return _cubicBezierEasingFunction_0 = _c.CreateCubicBezierEasingFunction({ 0.166999996F, 0.166999996F }, { 0.0F, 1.0F });
    }

    CubicBezierEasingFunction CubicBezierEasingFunction_1()
    {
        return _cubicBezierEasingFunction_1 = _c.CreateCubicBezierEasingFunction({ 0.850000024F, 0.0F }, { 0.75F, 1.0F });
    }

    CubicBezierEasingFunction CubicBezierEasingFunction_2()
    {
        return _cubicBezierEasingFunction_2 = _c.CreateCubicBezierEasingFunction({ 0.349999994F, 0.0F }, { 0.0F, 1.0F });
    }

    ExpressionAnimation RootProgress()
    {
        const auto result = _rootProgress = _c.CreateExpressionAnimation(L"_.Progress");
        result.SetReferenceParameter(L"_", _root);
        return result;
    }

    // PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff Controls -
    // 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 - TreeViewExpand -
    // 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 - NormalOffToNormalOn
    // Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff Controls - 08 -
    // TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 - TreeViewExpand - 07 -
    // PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand - 08 -
    // PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    InsetClip InsetClip_0()
    {
        const auto result = _c.CreateInsetClip();
        return result;
    }

    // - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer
    // 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2
    // Rotation
    ScalarKeyFrameAnimation RotationAngleInDegreesScalarAnimation_90_to_90()
    {
        // Frame 0.
        const auto result = CreateScalarKeyFrameAnimation(0.0F, 90.0F, HoldThenStepEasingFunction());
        // Frame 9.
        result.InsertKeyFrame(0.0409090891F, 0.0F, CubicBezierEasingFunction_0());
        // Frame 10.
        result.InsertKeyFrame(0.0454545468F, 90.0F, _holdThenStepEasingFunction);
        // Frame 20.
        result.InsertKeyFrame(0.0909090936F, 90.0F, _holdThenStepEasingFunction);
        // Frame 29.
        result.InsertKeyFrame(0.131818175F, 0.0F, _cubicBezierEasingFunction_0);
        // Frame 30.
        result.InsertKeyFrame(0.13636364F, 0.0F, _holdThenStepEasingFunction);
        // Frame 40.
        result.InsertKeyFrame(0.181818187F, 90.0F, _cubicBezierEasingFunction_0);
        // Frame 60.
        result.InsertKeyFrame(0.272727281F, 0.0F, _holdThenStepEasingFunction);
        // Frame 70.
        result.InsertKeyFrame(0.318181813F, 0.0F, _holdThenStepEasingFunction);
        // Frame 79.
        result.InsertKeyFrame(0.359090894F, 90.0F, _cubicBezierEasingFunction_0);
        // Frame 80.
        result.InsertKeyFrame(0.363636374F, 90.0F, _holdThenStepEasingFunction);
        // Frame 89.
        result.InsertKeyFrame(0.404545456F, 0.0F, _cubicBezierEasingFunction_0);
        // Frame 90.
        result.InsertKeyFrame(0.409090906F, 90.0F, _holdThenStepEasingFunction);
        // Frame 100.
        result.InsertKeyFrame(0.454545468F, 90.0F, _holdThenStepEasingFunction);
        // Frame 109.
        result.InsertKeyFrame(0.49545455F, 0.0F, _cubicBezierEasingFunction_0);
        // Frame 110.
        result.InsertKeyFrame(0.5F, 0.0F, _holdThenStepEasingFunction);
        // Frame 119.
        result.InsertKeyFrame(0.540909111F, 90.0F, _cubicBezierEasingFunction_0);
        // Frame 120.
        result.InsertKeyFrame(0.545454562F, 0.0F, _holdThenStepEasingFunction);
        // Frame 130.
        result.InsertKeyFrame(0.590909064F, 0.0F, _holdThenStepEasingFunction);
        // Frame 139.
        result.InsertKeyFrame(0.631818175F, 90.0F, _cubicBezierEasingFunction_0);
        // Frame 140.
        result.InsertKeyFrame(0.636363626F, 0.0F, _holdThenStepEasingFunction);
        // Frame 149.
        result.InsertKeyFrame(0.677272737F, 90.0F, _cubicBezierEasingFunction_0);
        // Frame 150.
        result.InsertKeyFrame(0.681818187F, 0.0F, _holdThenStepEasingFunction);
        // Frame 160.
        result.InsertKeyFrame(0.727272749F, 0.0F, _holdThenStepEasingFunction);
        // Frame 170.
        result.InsertKeyFrame(0.772727251F, 90.0F, _holdThenStepEasingFunction);
        // Frame 179.
        result.InsertKeyFrame(0.813636363F, 0.0F, _cubicBezierEasingFunction_0);
        // Frame 180.
        result.InsertKeyFrame(0.818181813F, 90.0F, _holdThenStepEasingFunction);
        return result;
    }

    // PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff Controls -
    // 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 - TreeViewExpand -
    // 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 - NormalOffToNormalOn
    // Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff Controls - 08 -
    // TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 - TreeViewExpand - 07 -
    // PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand - 08 -
    // PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2
    ShapeVisual ShapeVisual_0()
    {
        const auto result = _c.CreateShapeVisual();
        result.Size({ 48.0F, 48.0F });
        result.Shapes().Append(ContainerShape());
        return result;
    }

    StepEasingFunction HoldThenStepEasingFunction()
    {
        const auto result = _holdThenStepEasingFunction = _c.CreateStepEasingFunction();
        result.IsFinalStepSingleFrame(true);
        return result;
    }

    // - - PreComp layer: Controls - 08 - TreeViewExpand - 01 - NormalOnToNormalOff
    // Controls - 08 - TreeViewExpand - 02 - NormalOnToPointerOverOn Controls - 08 -
    // TreeViewExpand - 03 - NormalOnToPressedOn Controls - 08 - TreeViewExpand - 04 -
    // NormalOffToNormalOn Controls - 08 - TreeViewExpand - 05 -NormalOffToPointerOverOff
    // Controls - 08 - TreeViewExpand - 06 - NormalOffToPressedOff Controls - 08 -
    // TreeViewExpand - 07 - PointerOverOnToPointerOverOff Controls - 08 - TreeViewExpand
    // - 08 - PointerOverOnToNormalOn Controls - 08 - TreeViewExpand - 09 -
    // PointerOverOnToPressedOn Controls - 08 - TreeViewExpand - 10 -
    // PointerOverOffToPointerOverOn Controls - 08 - TreeViewExpand - 11 -
    // PointerOverOffToNormalOff Controls - 08 - TreeViewExpand - 12 -
    // PointerOverOffToPressedOff Controls - 08 - TreeViewExpand - 13 -
    // PressedOnToPressedOff Controls - 08 - TreeViewExpand - 14 -
    // PressedOnToPointerOverOff Controls - 08 - TreeViewExpand - 15 -
    // PressedOnToNormalOff Controls - 08 - TreeViewExpand - 16 - PressedOffToPressedOn
    // Controls - 08 - TreeViewExpand - 17 - PressedOffToPointerOverOn Controls - 08 -
    // TreeViewExpand - 18 - PressedOffToNormalOn
    // - Shape tree root for layer: Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer
    // 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2 Layer 2
    // Layer 2
    // Offset
    Vector2KeyFrameAnimation OffsetVector2Animation()
    {
        // Frame 0.
        const auto result = CreateVector2KeyFrameAnimation(0.0F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 10.
        result.InsertKeyFrame(0.0454545468F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 30.
        result.InsertKeyFrame(0.13636364F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 40.
        result.InsertKeyFrame(0.181818187F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 45.
        result.InsertKeyFrame(0.204545453F, { 24.0F, 30.0F }, CubicBezierEasingFunction_1());
        // Frame 59.
        result.InsertKeyFrame(0.268181831F, { 24.0F, 24.0F }, CubicBezierEasingFunction_2());
        // Frame 60.
        result.InsertKeyFrame(0.272727281F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 70.
        result.InsertKeyFrame(0.318181813F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 80.
        result.InsertKeyFrame(0.363636374F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 90.
        result.InsertKeyFrame(0.409090906F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 100.
        result.InsertKeyFrame(0.454545468F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 110.
        result.InsertKeyFrame(0.5F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 120.
        result.InsertKeyFrame(0.545454562F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 130.
        result.InsertKeyFrame(0.590909064F, { 24.1709995F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 140.
        result.InsertKeyFrame(0.636363626F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 180.
        result.InsertKeyFrame(0.818181813F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 185.
        result.InsertKeyFrame(0.840909064F, { 24.0F, 30.0F }, _cubicBezierEasingFunction_1);
        // Frame 199.
        result.InsertKeyFrame(0.904545426F, { 24.0F, 24.0F }, _cubicBezierEasingFunction_2);
        // Frame 200.
        result.InsertKeyFrame(0.909090936F, { 24.0F, 24.0F }, _holdThenStepEasingFunction);
        // Frame 205.
        result.InsertKeyFrame(0.931818187F, { 24.0F, 30.0F }, _cubicBezierEasingFunction_1);
        // Frame 219.
        result.InsertKeyFrame(0.99545455F, { 24.0F, 24.0F }, _cubicBezierEasingFunction_2);
        return result;
    }

    static IGeometrySource2D CanvasGeometryToIGeometrySource2D(winrt::com_ptr<CanvasGeometry> geo)
    {
        return geo.as<IGeometrySource2D>();
    }

public:
    AnimatedChevronRightDownSmallVisualSource_AnimatedVisual(
        Compositor compositor,
        CompositionPropertySet themeProperties)
        : _c{compositor}
        , _themeProperties{themeProperties}
        , _reusableExpressionAnimation(compositor.CreateExpressionAnimation())
    {
        winrt::check_hresult(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, _d2dFactory.put()));
        const auto _ = Root();
    }

    void Close()
    {
        if (_root)
        {
            _root.Close();
        }
    }

    TimeSpan Duration() const
    {
        return TimeSpan{ c_durationTicks };
    }

    Visual RootVisual() const
    {
        return _root;
    }

    winrt::float2 Size() const
    {
        return { 48.0F, 48.0F };
    }

    static bool IsRuntimeCompatible()
    {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 7);
    }
};

winrt::float4 AnimatedChevronRightDownSmallVisualSource::ColorAsVector4(Color color)
{
    return { static_cast<float>(color.R), static_cast<float>(color.G), static_cast<float>(color.B), static_cast<float>(color.A) };
}

CompositionPropertySet AnimatedChevronRightDownSmallVisualSource::EnsureThemeProperties(Compositor compositor)
{
    if (_themeProperties == nullptr)
    {
        _themeProperties = compositor.CreatePropertySet();
        _themeProperties.InsertVector4(L"Foreground", ColorAsVector4((Color)_themeForeground));
    }

    return _themeProperties;
}

Color AnimatedChevronRightDownSmallVisualSource::Foreground()
{
    return _themeForeground;
}

void AnimatedChevronRightDownSmallVisualSource::Foreground(Color value)
{
    _themeForeground = value;
    if (_themeProperties != nullptr)
    {
        _themeProperties.InsertVector4(L"Foreground", ColorAsVector4((Color)_themeForeground));
    }
}

winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual AnimatedChevronRightDownSmallVisualSource::TryCreateAnimatedVisual(
    Compositor const& compositor)
{
    IInspectable diagnostics = nullptr;
    return TryCreateAnimatedVisual(compositor, diagnostics);
}

winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual AnimatedChevronRightDownSmallVisualSource::TryCreateAnimatedVisual(
    Compositor const& compositor,
    IInspectable& diagnostics)
{
    const auto _ = EnsureThemeProperties(compositor);
    diagnostics = nullptr;

    if (AnimatedChevronRightDownSmallVisualSource_AnimatedVisual::IsRuntimeCompatible())
    {
        return winrt::make<AnimatedChevronRightDownSmallVisualSource_AnimatedVisual>(
            compositor,
            _themeProperties);
    }

    return nullptr;
}

double AnimatedChevronRightDownSmallVisualSource::FrameCount()
{
    return 220.0;
}

double AnimatedChevronRightDownSmallVisualSource::Framerate()
{
    return 60.0;
}

TimeSpan AnimatedChevronRightDownSmallVisualSource::Duration()
{
    return TimeSpan{ 36666666L };
}

double AnimatedChevronRightDownSmallVisualSource::FrameToProgress(double frameNumber)
{
    return frameNumber / 220.0;
}

winrt::Windows::Foundation::Collections::IMapView<hstring, double> AnimatedChevronRightDownSmallVisualSource::Markers()
{
    return winrt::single_threaded_map<winrt::hstring, double>(
        std::map<winrt::hstring, double>
        {
            { L"NormalOnToNormalOff_Start", 0.0 },
            { L"NormalOnToNormalOff_End", 0.0411363636363636 },
            { L"NormalOnToPointerOverOn_Start", 0.0456818181818182 },
            { L"NormalOnToPointerOverOn_End", 0.0865909090909091 },
            { L"NormalOnToPressedOn_Start", 0.0911363636363636 },
            { L"NormalOnToPressedOn_End", 0.132045454545455 },
            { L"NormalOffToNormalOn_Start", 0.136590909090909 },
            { L"NormalOffToNormalOn_End", 0.268409090909091 },
            { L"NormalOffToPointerOverOff_Start", 0.272954545454545 },
            { L"NormalOffToPointerOverOff_End", 0.313863636363636 },
            { L"NormalOffToPressedOff_Start", 0.318409090909091 },
            { L"NormalOffToPressedOff_End", 0.359318181818182 },
            { L"PointerOverOnToPointerOverOff_Start", 0.363863636363636 },
            { L"PointerOverOnToPointerOverOff_End", 0.404772727272727 },
            { L"PointerOverOnToNormalOn_Start", 0.409318181818182 },
            { L"PointerOverOnToNormalOn_End", 0.450227272727273 },
            { L"PointerOverOnToPressedOn_Start", 0.454772727272727 },
            { L"PointerOverOnToPressedOn_End", 0.495681818181818 },
            { L"PointerOverOffToPointerOverOn_Start", 0.500227272727273 },
            { L"PointerOverOffToPointerOverOn_End", 0.541136363636364 },
            { L"PointerOverOffToNormalOff_Start", 0.545681818181818 },
            { L"PointerOverOffToNormalOff_End", 0.586590909090909 },
            { L"PointerOverOffToPressedOff_Start", 0.591136363636364 },
            { L"PointerOverOffToPressedOff_End", 0.632045454545455 },
            { L"PressedOnToPressedOff_Start", 0.636590909090909 },
            { L"PressedOnToPressedOff_End", 0.6775 },
            { L"PressedOnToPointerOverOff_Start", 0.682045454545455 },
            { L"PressedOnToPointerOverOff_End", 0.722954545454546 },
            { L"PressedOnToNormalOff_Start", 0.7275 },
            { L"PressedOnToNormalOff_End", 0.768409090909091 },
            { L"PressedOffToPressedOn_Start", 0.772954545454546 },
            { L"PressedOffToPressedOn_End", 0.813863636363636 },
            { L"PressedOffToPointerOverOn_Start", 0.818409090909091 },
            { L"PressedOffToPointerOverOn_End", 0.904772727272727 },
            { L"PressedOffToNormalOn_Start", 0.909318181818182 },
            { L"PressedOffToNormalOn_End", 0.995681818181818 },
        }
    ).GetView();
}

void AnimatedChevronRightDownSmallVisualSource::SetColorProperty(hstring const& propertyName, Color value)
{
    if (propertyName == L"Foreground")
    {
        _themeForeground = value;
    }
    else
    {
        return;
    }

    if (_themeProperties != nullptr)
    {
        _themeProperties.InsertVector4(propertyName, ColorAsVector4(value));
    }
}

void AnimatedChevronRightDownSmallVisualSource::SetScalarProperty(hstring const&, double)
{
}
