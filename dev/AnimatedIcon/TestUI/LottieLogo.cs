// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Numerics;
using System.Runtime.InteropServices;
using Windows.Graphics;
using Windows.UI;
using Windows.UI.Composition;

namespace AnimatedIconVisuals
{
    sealed class LottieLogo : IAnimatedVisualSource
    {
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            if (!IsRuntimeCompatible())
            {
                return null;
            }
            return new AnimatedVisual(compositor);
        }

        static bool IsRuntimeCompatible()
        {
            if (!Windows.Foundation.Metadata.ApiInformation.IsTypePresent("Windows.UI.Composition.CompositionGeometricClip"))
            {
                return false;
            }
            return true;
        }

        sealed class AnimatedVisual : IAnimatedVisual
        {
            readonly ID2D1Factory _d2d;
            const long c_durationTicks = 59670000;
            readonly Compositor _c;
            readonly ExpressionAnimation _reusableExpressionAnimation;
            CompositionColorBrush _colorBrush_AlmostTeal_FF007A87;
            CompositionColorBrush _colorBrush_White;
            CompositionPath _compositionPath_00;
            CompositionPath _compositionPath_01;
            CompositionPath _compositionPath_02;
            CompositionPath _compositionPath_03;
            CompositionPath _compositionPath_04;
            CompositionPath _compositionPath_05;
            CompositionPath _compositionPath_06;
            CompositionPath _compositionPath_07;
            CompositionPath _compositionPath_08;
            CubicBezierEasingFunction _cubicBezierEasingFunction_02;
            CubicBezierEasingFunction _cubicBezierEasingFunction_03;
            CubicBezierEasingFunction _cubicBezierEasingFunction_04;
            CubicBezierEasingFunction _cubicBezierEasingFunction_05;
            CubicBezierEasingFunction _cubicBezierEasingFunction_07;
            CubicBezierEasingFunction _cubicBezierEasingFunction_08;
            CubicBezierEasingFunction _cubicBezierEasingFunction_10;
            CubicBezierEasingFunction _cubicBezierEasingFunction_11;
            CubicBezierEasingFunction _cubicBezierEasingFunction_14;
            CubicBezierEasingFunction _cubicBezierEasingFunction_15;
            CubicBezierEasingFunction _cubicBezierEasingFunction_22;
            CompositionEllipseGeometry _ellipse_4p7;
            StepEasingFunction _holdThenStepEasingFunction;
            Vector2KeyFrameAnimation _positionVector2Animation_02;
            Vector2KeyFrameAnimation _positionVector2Animation_03;
            Vector2KeyFrameAnimation _positionVector2Animation_04;
            Vector2KeyFrameAnimation _positionVector2Animation_05;
            Vector2KeyFrameAnimation _positionVector2Animation_06;
            Vector2KeyFrameAnimation _radiusVector2Animation;
            ContainerVisual _root;
            ScalarKeyFrameAnimation _scalarAnimation_0_to_1;
            ExpressionAnimation _scalarExpressionAnimation;
            StepEasingFunction _stepThenHoldEasingFunction;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_02;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_03;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_06;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_11;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_13;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_09;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_10;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_11;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_12;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_16;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_1_to_0_17;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_to_1_02;
            ScalarKeyFrameAnimation _transformMatrix_11ScalarAnimation_to_1_06;
            ScalarKeyFrameAnimation _tStartScalarAnimation_0_to_0p249;
            ScalarKeyFrameAnimation _tStartScalarAnimation_0p87_to_0_02;

            // Rectangle Path 1
            CompositionColorBrush ColorBrush_AlmostDarkTurquoise_FF00D1C1()
            {
                return _c.CreateColorBrush(Color.FromArgb(0xFF, 0x00, 0xD1, 0xC1));
            }

            CompositionColorBrush ColorBrush_AlmostTeal_FF007A87()
            {
                return _colorBrush_AlmostTeal_FF007A87 = _c.CreateColorBrush(Color.FromArgb(0xFF, 0x00, 0x7A, 0x87));
            }

            CompositionColorBrush ColorBrush_White()
            {
                return _colorBrush_White = _c.CreateColorBrush(Color.FromArgb(0xFF, 0xFF, 0xFF, 0xFF));
            }

            CompositionPath CompositionPath_00()
            {
                var result = _compositionPath_00 = new CompositionPath(Geometry_00());
                return result;
            }

            CompositionPath CompositionPath_01()
            {
                var result = _compositionPath_01 = new CompositionPath(Geometry_01());
                return result;
            }

            CompositionPath CompositionPath_02()
            {
                var result = _compositionPath_02 = new CompositionPath(Geometry_02());
                return result;
            }

            CompositionPath CompositionPath_03()
            {
                var result = _compositionPath_03 = new CompositionPath(Geometry_03());
                return result;
            }

            CompositionPath CompositionPath_04()
            {
                var result = _compositionPath_04 = new CompositionPath(Geometry_04());
                return result;
            }

            CompositionPath CompositionPath_05()
            {
                var result = _compositionPath_05 = new CompositionPath(Geometry_05());
                return result;
            }

            CompositionPath CompositionPath_06()
            {
                var result = _compositionPath_06 = new CompositionPath(Geometry_06());
                return result;
            }

            CompositionPath CompositionPath_07()
            {
                var result = _compositionPath_07 = new CompositionPath(Geometry_07());
                return result;
            }

            CompositionPath CompositionPath_08()
            {
                var result = _compositionPath_08 = new CompositionPath(Geometry_08());
                return result;
            }

            // Layer (Shape): Dot-Y
            CompositionContainerShape ContainerShape_00()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_01());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_00());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): Dot-Y
            CompositionContainerShape ContainerShape_01()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_02());
                return result;
            }

            // Transforms for Bncr
            CompositionContainerShape ContainerShape_02()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(164.781998F, 57.4729996F));
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_03());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 60),(my.Position.Y - 60))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_01());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms for Dot-Y
            CompositionContainerShape ContainerShape_03()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(43.2630005F, 59.75F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_01());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 196.791),(my.Position.Y - 266.504))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_00());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", ScalarExpressionAnimation());
                return result;
            }

            // Layer (Shape): E3-Y
            CompositionContainerShape ContainerShape_04()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_05());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_00());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E3-Y
            CompositionContainerShape ContainerShape_05()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_06());
                return result;
            }

            // Transforms for E3-Y
            CompositionContainerShape ContainerShape_06()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(119.167F, 57.4790001F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_02());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 345.124),(my.Position.Y - 261.801))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_02());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): E3-B
            CompositionContainerShape ContainerShape_07()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_08());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_01());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E3-B
            CompositionContainerShape ContainerShape_08()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_09());
                return result;
            }

            // Transforms for E3-Y
            CompositionContainerShape ContainerShape_09()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(119.167F, 57.4790001F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_03());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 345.124),(my.Position.Y - 261.801))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_02);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): I-Y
            CompositionContainerShape ContainerShape_10()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_11());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_01());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): I-Y
            CompositionContainerShape ContainerShape_11()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_12());
                return result;
            }

            // Transforms for I-Y
            CompositionContainerShape ContainerShape_12()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(93.5940018F, 62.8610001F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_04());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 303.802),(my.Position.Y - 282.182))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_03());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): I-B
            CompositionContainerShape ContainerShape_13()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_14());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_02());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): I-B
            CompositionContainerShape ContainerShape_14()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_15());
                return result;
            }

            // Transforms for I-Y
            CompositionContainerShape ContainerShape_15()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(93.5940018F, 62.8610001F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_05());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 303.802),(my.Position.Y - 282.182))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_03);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): E2-Y
            CompositionContainerShape ContainerShape_16()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_17());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_02());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E2-Y
            CompositionContainerShape ContainerShape_17()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_18());
                return result;
            }

            // Transforms for E2-Y
            CompositionContainerShape ContainerShape_18()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(109.092003F, 33.6100006F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_06());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 332.05),(my.Position.Y - 237.932))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_04());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): E2-B
            CompositionContainerShape ContainerShape_19()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_20());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_03());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E2-B
            CompositionContainerShape ContainerShape_20()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_21());
                return result;
            }

            // Transforms for E2-Y
            CompositionContainerShape ContainerShape_21()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(109.092003F, 33.6100006F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_07());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 332.05),(my.Position.Y - 237.932))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_04);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): E1-Y
            CompositionContainerShape ContainerShape_22()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_23());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_03());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E1-Y
            CompositionContainerShape ContainerShape_23()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_24());
                return result;
            }

            // Transforms for E1-Y
            CompositionContainerShape ContainerShape_24()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(113.714996F, 9.14599991F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_08());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 344.672),(my.Position.Y - 214.842))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_05());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): E1-B
            CompositionContainerShape ContainerShape_25()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_26());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_04());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): E1-B
            CompositionContainerShape ContainerShape_26()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_27());
                return result;
            }

            // Transforms for E1-Y
            CompositionContainerShape ContainerShape_27()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(113.714996F, 9.14599991F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_09());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 344.672),(my.Position.Y - 214.842))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_05);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-Y
            CompositionContainerShape ContainerShape_28()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_29());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_04());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-Y
            CompositionContainerShape ContainerShape_29()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_30());
                return result;
            }

            // Transforms for T1a-Y
            CompositionContainerShape ContainerShape_30()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(39.0429993F, 48.6780014F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_10());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 250),(my.Position.Y - 250))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_06());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2b-Y
            CompositionContainerShape ContainerShape_31()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_11());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_05());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2a-Y
            CompositionContainerShape ContainerShape_32()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_12());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_06());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2b-B
            CompositionContainerShape ContainerShape_33()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_13());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_05());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1b-Y
            CompositionContainerShape ContainerShape_34()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_14());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_07());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1b-B
            CompositionContainerShape ContainerShape_35()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_15());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_to_1_02);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): O-Y
            CompositionContainerShape ContainerShape_36()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_37());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_06());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): O-Y
            CompositionContainerShape ContainerShape_37()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_38());
                return result;
            }

            // Transforms for O-Y
            CompositionContainerShape ContainerShape_38()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(-62.7919998F, 73.0569992F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_16());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 196.791),(my.Position.Y - 266.504))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_07());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): O-B
            CompositionContainerShape ContainerShape_39()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_40());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_to_1_06);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): O-B
            CompositionContainerShape ContainerShape_40()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_41());
                return result;
            }

            // Transforms for O-B
            CompositionContainerShape ContainerShape_41()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(-62.7919998F, 73.0569992F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_17());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 196.791),(my.Position.Y - 266.504))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_08());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-Y 2
            CompositionContainerShape ContainerShape_42()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_43());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_07());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-Y 2
            CompositionContainerShape ContainerShape_43()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_44());
                return result;
            }

            // Transforms for T1a-Y 2
            CompositionContainerShape ContainerShape_44()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(39.0429993F, 48.6780014F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_18());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 250),(my.Position.Y - 250))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_06);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2a-B
            CompositionContainerShape ContainerShape_45()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_19());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_08());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-B
            CompositionContainerShape ContainerShape_46()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_47());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_09());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1a-B
            CompositionContainerShape ContainerShape_47()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_48());
                return result;
            }

            // Transforms for T1a-Y
            CompositionContainerShape ContainerShape_48()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(39.0429993F, 48.6780014F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_20());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 250),(my.Position.Y - 250))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", _positionVector2Animation_06);
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): Dot-Y
            CompositionContainerShape ContainerShape_49()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_50());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_08());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): Dot-Y
            CompositionContainerShape ContainerShape_50()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_51());
                return result;
            }

            // Transforms for N
            CompositionContainerShape ContainerShape_51()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(-33.6669998F, 8.18200016F));
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_52());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 60),(my.Position.Y - 60))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_10());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms for Dot-Y
            CompositionContainerShape ContainerShape_52()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(39.875F, 60));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_21());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 196.791),(my.Position.Y - 266.504))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_09());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): L-Y
            CompositionContainerShape ContainerShape_53()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_22());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_10());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): L-B
            CompositionContainerShape ContainerShape_54()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_23());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_1_11());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): Dot1
            CompositionContainerShape ContainerShape_55()
            {
                var result = _c.CreateContainerShape();
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_56());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_to_0());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): Dot1
            CompositionContainerShape ContainerShape_56()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                shapes.Add(ContainerShape_57());
                return result;
            }

            // Transforms for Dot1
            CompositionContainerShape ContainerShape_57()
            {
                var result = _c.CreateContainerShape();
                var propertySet = result.Properties;
                propertySet.InsertVector2("Position", new Vector2(295.770996F, 108.994003F));
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_24());
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Vector2((my.Position.X - 196.791),(my.Position.Y - 266.504))";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("Offset", _reusableExpressionAnimation);
                result.StartAnimation("Position", PositionVector2Animation_11());
                var controller = result.TryGetAnimationController("Position");
                controller.Pause();
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "(_.Progress * 0.9835165) + 0.01648352";
                _reusableExpressionAnimation.SetReferenceParameter("_", _root);
                controller.StartAnimation("Progress", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S1-Y
            CompositionContainerShape ContainerShape_58()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_25());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_09());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S2-Y
            CompositionContainerShape ContainerShape_59()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_26());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_09);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S7
            CompositionContainerShape ContainerShape_60()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_27());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_10());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S8
            CompositionContainerShape ContainerShape_61()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_28());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_10);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S3-Y
            CompositionContainerShape ContainerShape_62()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_29());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_11());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S4-Y
            CompositionContainerShape ContainerShape_63()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_30());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_11);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S5-Y
            CompositionContainerShape ContainerShape_64()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_31());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_11);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S6-Y
            CompositionContainerShape ContainerShape_65()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_32());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_11);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S3-Y 2
            CompositionContainerShape ContainerShape_66()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_33());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_12());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S4-Y 2
            CompositionContainerShape ContainerShape_67()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_34());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_12);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S5-Y 2
            CompositionContainerShape ContainerShape_68()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_35());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_12);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S11
            CompositionContainerShape ContainerShape_69()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_36());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_13());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S12
            CompositionContainerShape ContainerShape_70()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_37());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_14());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S13
            CompositionContainerShape ContainerShape_71()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_38());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_15());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S3-Y 3
            CompositionContainerShape ContainerShape_72()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_39());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_16());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S4-Y 3
            CompositionContainerShape ContainerShape_73()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_40());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_16);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S5-Y 3
            CompositionContainerShape ContainerShape_74()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_41());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_16);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S3-Y 4
            CompositionContainerShape ContainerShape_75()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_42());
                result.StartAnimation("TransformMatrix._11", TransformMatrix_11ScalarAnimation_1_to_0_17());
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S4-Y 4
            CompositionContainerShape ContainerShape_76()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_43());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_17);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Layer (Shape): S5-Y 4
            CompositionContainerShape ContainerShape_77()
            {
                var result = _c.CreateContainerShape();
                result.TransformMatrix = new Matrix3x2(0, 0, 0, 0, 0, 0);
                var shapes = result.Shapes;
                shapes.Add(SpriteShape_44());
                result.StartAnimation("TransformMatrix._11", _transformMatrix_11ScalarAnimation_1_to_0_17);
                var controller = result.TryGetAnimationController("TransformMatrix._11");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "my.TransformMatrix._11";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TransformMatrix._22", _reusableExpressionAnimation);
                return result;
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_00()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0, 0), new Vector2(0, 0.811999977F));
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_01()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.389999986F, 0.707000017F), new Vector2(0.708000004F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_02()
            {
                return _cubicBezierEasingFunction_02 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.180000007F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_03()
            {
                return _cubicBezierEasingFunction_03 = _c.CreateCubicBezierEasingFunction(new Vector2(0.819999993F, 0), new Vector2(0.833000004F, 0.833000004F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_04()
            {
                return _cubicBezierEasingFunction_04 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.833000004F, 0.833000004F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_05()
            {
                return _cubicBezierEasingFunction_05 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.666999996F, 1));
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_06()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0), new Vector2(0.666999996F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_07()
            {
                return _cubicBezierEasingFunction_07 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.119999997F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_08()
            {
                return _cubicBezierEasingFunction_08 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0), new Vector2(0.119999997F, 1));
            }

            // TStart
            CubicBezierEasingFunction CubicBezierEasingFunction_09()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.300999999F, 0), new Vector2(0.833000004F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_10()
            {
                return _cubicBezierEasingFunction_10 = _c.CreateCubicBezierEasingFunction(new Vector2(0.300999999F, 0), new Vector2(0.666999996F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_11()
            {
                return _cubicBezierEasingFunction_11 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.0599999987F, 1));
            }

            // Layer (Shape): T1b-B
            //   Path 1
            //     Path 1.PathGeometry
            //       TrimEnd
            CubicBezierEasingFunction CubicBezierEasingFunction_12()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.209999993F, 1));
            }

            // Radius
            CubicBezierEasingFunction CubicBezierEasingFunction_13()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.333000004F, 0), new Vector2(0.666999996F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_14()
            {
                return _cubicBezierEasingFunction_14 = _c.CreateCubicBezierEasingFunction(new Vector2(0.180000007F, 0), new Vector2(0.34799999F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_15()
            {
                return _cubicBezierEasingFunction_15 = _c.CreateCubicBezierEasingFunction(new Vector2(0.693000019F, 0), new Vector2(0.270000011F, 1));
            }

            // Transforms: O-B
            //   Ellipse Path 1
            //     Ellipse Path 1.EllipseGeometry
            //       TrimStart
            CubicBezierEasingFunction CubicBezierEasingFunction_16()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 1), new Vector2(0.432000011F, 1));
            }

            // Transforms: T1a-Y
            //   Path 1
            //     Path 1.PathGeometry
            //       TrimEnd
            CubicBezierEasingFunction CubicBezierEasingFunction_17()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.672999978F, 1));
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_18()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.25999999F, 1));
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_19()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.74000001F, 0), new Vector2(0.833000004F, 0.833000004F));
            }

            // TStart
            CubicBezierEasingFunction CubicBezierEasingFunction_20()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.703000009F, 0.856999993F));
            }

            // TStart
            CubicBezierEasingFunction CubicBezierEasingFunction_21()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.333000004F, 0.202000007F), new Vector2(0.938000023F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_22()
            {
                return _cubicBezierEasingFunction_22 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.337000012F, 1));
            }

            // TStart
            CubicBezierEasingFunction CubicBezierEasingFunction_23()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.703000009F, 0.82099998F));
            }

            // TStart
            CubicBezierEasingFunction CubicBezierEasingFunction_24()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.0370000005F, 0.167999998F), new Vector2(0.263000011F, 1));
            }

            // Position
            CubicBezierEasingFunction CubicBezierEasingFunction_25()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.823000014F, 0), new Vector2(0.833000004F, 0.833000004F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_26()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.197999999F), new Vector2(0.638000011F, 1));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_27()
            {
                return _c.CreateCubicBezierEasingFunction(new Vector2(0.523000002F, 0), new Vector2(0.795000017F, 1));
            }

            // Transforms: O-Y
            //   Ellipse Path 1
            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_0_0()
            {
                var result = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(0, 0);
                result.StartAnimation("Radius", RadiusVector2Animation());
                var controller = result.TryGetAnimationController("Radius");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: O-B
            //   Ellipse Path 1
            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_0_1()
            {
                var result = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(0, 0);
                result.StartAnimation("Radius", _radiusVector2Animation);
                var controller = result.TryGetAnimationController("Radius");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimStart", TrimStartScalarAnimation_0_to_0p399());
                controller = result.TryGetAnimationController("TrimStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_1_to_0p88());
                controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: Dot-Y
            //   Ellipse Path 1
            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_4p6()
            {
                var result = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(4.5999999F, 4.5999999F);
                return result;
            }

            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_4p7()
            {
                var result = _ellipse_4p7 = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(4.69999981F, 4.69999981F);
                return result;
            }

            IGeometrySource2D Geometry_00()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-13.6639996F, -0.144999996F), 0);
                {
                    var line = new Vector2(75.663002F, 0.289999992F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_01()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(0.859000027F, -21.1429996F), 0);
                {
                    var line = new Vector2(-4.35900021F, 70.3919983F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_02()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-26.6700001F, -0.282999992F), 0);
                {
                    var line = new Vector2(99.1709976F, 0.0659999996F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_03()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-13.6639996F, -0.144999996F), 0);
                {
                    var line = new Vector2(62.1629982F, 0.289999992F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_04()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-30.7199993F, 63.7610016F), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-30.6889992F, 63.1669998F), Point2 = new Vector2(-30.7889996F, 50.8470001F), Point3 = new Vector2(-30.7409992F, 45.1920013F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-30.6650009F, 36.2140007F), Point2 = new Vector2(-37.3429985F, 27.0739994F), Point3 = new Vector2(-37.3969994F, 27.0139999F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-38.5579987F, 25.7140007F), Point2 = new Vector2(-39.7519989F, 24.1469994F), Point3 = new Vector2(-40.6980019F, 22.6609993F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-46.637001F, 13.3339996F), Point2 = new Vector2(-47.8400002F, 0.933000028F), Point3 = new Vector2(-37.8730011F, -7.1170001F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-13.1960001F, -27.0459995F), Point2 = new Vector2(8.96000004F, 11.559F), Point3 = new Vector2(49.5060005F, 11.559F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_05()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(246.649994F, 213.813995F), 0);
                {
                    var line = new Vector2(340.955994F, 213.628006F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_06()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(1.68099999F, -29.9920006F), 0);
                {
                    var line = new Vector2(-1.68099999F, 29.9920006F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_07()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(1.76800001F, -25.9659996F), 0);
                {
                    var line = new Vector2(-1.76800001F, 25.9659996F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_08()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-8.83699989F, -58.2290001F), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-8.83699989F, -58.2290001F), Point2 = new Vector2(-10.1630001F, 29.4950008F), Point3 = new Vector2(-35.8339996F, 33.6619987F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-44.0579987F, 34.9970016F), Point2 = new Vector2(-50.2319984F, 30.0499992F), Point3 = new Vector2(-51.6879997F, 23.1480007F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-53.144001F, 16.2450008F), Point2 = new Vector2(-49.6549988F, 9.15600014F), Point3 = new Vector2(-41.1739998F, 7.29300022F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-17.3570004F, 2.05999994F), Point2 = new Vector2(4.23500013F, 57.1879997F), Point3 = new Vector2(51.7970009F, 44.1780014F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(51.9570007F, 44.1339989F), Point2 = new Vector2(52.6870003F, 43.8740005F), Point3 = new Vector2(53.1879997F, 43.7410011F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(53.6889992F, 43.6080017F), Point2 = new Vector2(68.9710007F, 41.3569984F), Point3 = new Vector2(140.393997F, 43.6720009F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_09()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-67.125F, -112), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-67.125F, -112), Point2 = new Vector2(-73.5579987F, -100.719002F), Point3 = new Vector2(-75.4580002F, -89.9509964F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-78.625F, -72), Point2 = new Vector2(-79.375F, -58.25F), Point3 = new Vector2(-80.375F, -39.25F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_10()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-67.25F, -105.5F), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-67.25F, -105.5F), Point2 = new Vector2(-70.4329987F, -94.9690018F), Point3 = new Vector2(-72.3330002F, -84.2009964F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-75.5F, -66.25F), Point2 = new Vector2(-75.5F, -56.75F), Point3 = new Vector2(-76.5F, -37.75F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_11()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(34.5F, -13.0500002F), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(7.5F, -14.5F), Point2 = new Vector2(-4, -37), Point3 = new Vector2(-35.0460014F, -35.5789986F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-61.4720001F, -34.3689995F), Point2 = new Vector2(-62.25F, -5.75F), Point3 = new Vector2(-62.25F, -5.75F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_12()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-3, 35.9500008F), 0);
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-3, 35.9500008F), Point2 = new Vector2(-1.5F, 7.5F), Point3 = new Vector2(-1.352F, -6.75600004F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-9.90299988F, -15.0190001F), Point2 = new Vector2(-21.5699997F, -20.5790005F), Point3 = new Vector2(-32.0460014F, -20.5790005F) };
                    sink.AddBeziers(in bezier, 1);
                }
                {
                    var bezier = new D2D1_BEZIER_SEGMENT { Point1 = new Vector2(-53.5F, -20.5790005F), Point2 = new Vector2(-42.25F, 4.25F), Point3 = new Vector2(-42.25F, 4.25F) };
                    sink.AddBeziers(in bezier, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_13()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(16.2310009F, 39.0730019F), 0);
                {
                    var line = new Vector2(-32.769001F, 57.3650017F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_14()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(7.44999981F, 21.9500008F), 0);
                {
                    var line = new Vector2(-32.75F, 55.75F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_15()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-94.5F, 37.0730019F), 0);
                {
                    var line = new Vector2(-48.769001F, 55.3650017F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_16()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(-87.5F, 20.9500008F), 0);
                {
                    var line = new Vector2(-48.75F, 54.75F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_17()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(166.731003F, -7.92700005F), 0);
                {
                    var line = new Vector2(136.731003F, 7.11499977F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_18()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(156.449997F, -23.0499992F), 0);
                {
                    var line = new Vector2(132, 2.75F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_19()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(169.5F, 18.073F), 0);
                {
                    var line = new Vector2(137.481003F, 11.3649998F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_20()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(119.5F, -45.0499992F), 0);
                {
                    var line = new Vector2(82.75F, -44.75F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_21()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(119.25F, -20.0499992F), 0);
                {
                    var line = new Vector2(63.5F, -20.5F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_22()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(128, 3.6500001F), 0);
                {
                    var line = new Vector2(78.25F, 3.5F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_23()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(149.623993F, 8.24400043F), 0);
                {
                    var line = new Vector2(136.647995F, 10.1560001F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_24()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(144.429001F, -5.39699984F), 0);
                {
                    var line = new Vector2(132.274994F, 4.73099995F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_25()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(145.677002F, 22.2199993F), 0);
                {
                    var line = new Vector2(134.921997F, 14.7489996F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_26()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(147.699005F, 13.0249996F), 0);
                {
                    var line = new Vector2(133.195007F, 13.21F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_27()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(142.182999F, -5.11199999F), 0);
                {
                    var line = new Vector2(130.029007F, 5.01599979F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            IGeometrySource2D Geometry_28()
            {
                var path = _d2d.CreatePathGeometry();
                var sink = path.Open();
                sink.BeginFigure(new Vector2(142.037994F, 29.2779999F), 0);
                {
                    var line = new Vector2(131.281998F, 21.8069992F);
                    sink.AddLines(in line, 1);
                }
                sink.EndFigure(0);
                sink.Close();
                Marshal.ReleaseComObject(sink);
                var result = new GeometrySource2D(path);
                return result;
            }

            StepEasingFunction HoldThenStepEasingFunction()
            {
                var result = _holdThenStepEasingFunction = _c.CreateStepEasingFunction();
                result.IsFinalStepSingleFrame = true;
                return result;
            }

            // Transforms: E3-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_00()
            {
                var result = _c.CreatePathGeometry(CompositionPath_00());
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p316_0());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: E3-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_01()
            {
                var result = _c.CreatePathGeometry(_compositionPath_00);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p316_1());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: I-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_02()
            {
                var result = _c.CreatePathGeometry(CompositionPath_01());
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p457_0());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: I-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_03()
            {
                var result = _c.CreatePathGeometry(_compositionPath_01);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p457_1());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: E2-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_04()
            {
                var result = _c.CreatePathGeometry(CompositionPath_02());
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p43_0());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: E2-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_05()
            {
                var result = _c.CreatePathGeometry(_compositionPath_02);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p43_1());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: E1-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_06()
            {
                var result = _c.CreatePathGeometry(CompositionPath_03());
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p375_0());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: E1-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_07()
            {
                var result = _c.CreatePathGeometry(_compositionPath_03);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0_to_0p375_1());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_08()
            {
                var result = _c.CreatePathGeometry(CompositionPath_04());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0);
                propertySet.InsertScalar("TEnd", 0);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0_to_0p249());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_0_to_1_0());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2b-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_09()
            {
                var result = _c.CreatePathGeometry(CompositionPath_05());
                result.StartAnimation("TrimStart", TrimStartScalarAnimation_0p29_to_0_0());
                var controller = result.TryGetAnimationController("TrimStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p411_to_0p665_0());
                controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2a-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_10()
            {
                var result = _c.CreatePathGeometry(CompositionPath_06());
                result.StartAnimation("TrimStart", TrimStartScalarAnimation_0p5_to_0_0());
                var controller = result.TryGetAnimationController("TrimStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p5_to_1_0());
                controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2b-B
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_11()
            {
                var result = _c.CreatePathGeometry(_compositionPath_05);
                result.StartAnimation("TrimStart", TrimStartScalarAnimation_0p29_to_0_1());
                var controller = result.TryGetAnimationController("TrimStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p411_to_0p665_1());
                controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1b-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_12()
            {
                var result = _c.CreatePathGeometry(CompositionPath_07());
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p117_to_1_0());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T1b-B
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_13()
            {
                var result = _c.CreatePathGeometry(_compositionPath_07);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p117_to_1_1());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_14()
            {
                var result = _c.CreatePathGeometry(_compositionPath_04);
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0);
                propertySet.InsertScalar("TEnd", 0);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", _tStartScalarAnimation_0_to_0p249);
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_0_to_1_1());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Layer (Shape): T2a-B
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_15()
            {
                var result = _c.CreatePathGeometry(_compositionPath_06);
                result.StartAnimation("TrimStart", TrimStartScalarAnimation_0p5_to_0_1());
                var controller = result.TryGetAnimationController("TrimStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p5_to_1_1());
                controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Transforms: T1a-Y
            //   Path 1
            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_16()
            {
                var result = _c.CreatePathGeometry(_compositionPath_04);
                result.TrimStart = 0.248999998F;
                result.StartAnimation("TrimEnd", TrimEndScalarAnimation_0p249_to_0p891());
                var controller = result.TryGetAnimationController("TrimEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_17()
            {
                var result = _c.CreatePathGeometry(CompositionPath_08());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.800000012F);
                propertySet.InsertScalar("TEnd", 0.810000002F);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p8_to_0());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_0p81_to_0p734_0());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_18()
            {
                var result = _c.CreatePathGeometry(_compositionPath_08);
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.800000012F);
                propertySet.InsertScalar("TEnd", 0.810000002F);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p8_to_0p3());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_0p81_to_0p734_1());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_19()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_09()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_00());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_00());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_20()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_10()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_01());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_01());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_21()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_11()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_02());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_02());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_22()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_12()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", _tStartScalarAnimation_0p87_to_0_02);
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", _tEndScalarAnimation_1_to_0_02);
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_23()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_13()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_03());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_03());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_24()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_14()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_04());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", _tEndScalarAnimation_1_to_0_03);
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_25()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_15()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_05());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_04());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_26()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_16()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_06());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_05());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_27()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_17()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_07());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_06());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_28()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_18()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_08());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", _tEndScalarAnimation_1_to_0_06);
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_29()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_19()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_09());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_07());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_30()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_20()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_10());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_08());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_31()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_21()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_11());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_09());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_32()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_22()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_12());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_10());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_33()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_23()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_13());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_11());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_34()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_24()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_14());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", _tEndScalarAnimation_1_to_0_11);
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_35()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_25()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_15());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_12());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_36()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_26()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_16());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_13());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_37()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_27()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_17());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", _tEndScalarAnimation_1_to_0_13);
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Path 1.PathGeometry
            CompositionPathGeometry PathGeometry_38()
            {
                var result = _c.CreatePathGeometry(new CompositionPath(Geometry_28()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TStart", 0.870000005F);
                propertySet.InsertScalar("TEnd", 1);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Min(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimStart", _reusableExpressionAnimation);
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = "Max(my.TStart, my.TEnd)";
                _reusableExpressionAnimation.SetReferenceParameter("my", result);
                result.StartAnimation("TrimEnd", _reusableExpressionAnimation);
                result.StartAnimation("TStart", TStartScalarAnimation_0p87_to_0_18());
                var controller = result.TryGetAnimationController("TStart");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("TEnd", TEndScalarAnimation_1_to_0_14());
                controller = result.TryGetAnimationController("TEnd");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_00()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(43.2630005F, 59.75F), StepThenHoldEasingFunction());
                result.InsertKeyFrame(0.536312878F, new Vector2(43.2630005F, 59.75F), HoldThenStepEasingFunction());
                result.InsertKeyFrame(0.603351951F, new Vector2(62.5130005F, 59.75F), CubicBezierEasingFunction_00());
                result.InsertKeyFrame(0.642458081F, new Vector2(63.7630005F, 59.75F), CubicBezierEasingFunction_01());
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_01()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(164.781998F, 57.4729996F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.536312878F, new Vector2(164.781998F, 57.4729996F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.553072631F, new Vector2(164.781998F, 55.4729996F), CubicBezierEasingFunction_02());
                result.InsertKeyFrame(0.569832385F, new Vector2(164.781998F, 57.4729996F), CubicBezierEasingFunction_03());
                result.InsertKeyFrame(0.586592197F, new Vector2(164.781998F, 56.9090004F), _cubicBezierEasingFunction_02);
                result.InsertKeyFrame(0.603351951F, new Vector2(164.781998F, 57.4729996F), _cubicBezierEasingFunction_03);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_02()
            {
                var result = _positionVector2Animation_02 = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(119.167F, 57.4790001F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.469273746F, new Vector2(119.167F, 57.4790001F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.513966501F, new Vector2(137.167007F, 57.4790001F), CubicBezierEasingFunction_05());
                result.InsertKeyFrame(0.536312878F, new Vector2(134.167007F, 57.4790001F), CubicBezierEasingFunction_06());
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_03()
            {
                var result = _positionVector2Animation_03 = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(93.5940018F, 62.8610001F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.43575418F, new Vector2(93.5940018F, 62.8610001F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.491620123F, new Vector2(92.6259995F, 82.8290024F), _cubicBezierEasingFunction_07);
                result.InsertKeyFrame(0.513966501F, new Vector2(92.8440018F, 77.8610001F), CubicBezierEasingFunction_08());
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_04()
            {
                var result = _positionVector2Animation_04 = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(109.092003F, 33.6100006F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.463687152F, new Vector2(109.092003F, 33.6100006F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.513966501F, new Vector2(121.092003F, 33.6100006F), _cubicBezierEasingFunction_07);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_05()
            {
                var result = _positionVector2Animation_05 = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(113.714996F, 9.14599991F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.441340774F, new Vector2(113.714996F, 9.14599991F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.491620123F, new Vector2(137.714996F, 9.14599991F), _cubicBezierEasingFunction_07);
                result.InsertKeyFrame(0.513966501F, new Vector2(133.714996F, 9.14599991F), _cubicBezierEasingFunction_08);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_06()
            {
                var result = _positionVector2Animation_06 = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(39.0429993F, 48.6780014F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.312849164F, new Vector2(39.0429993F, 48.6780014F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.357541889F, new Vector2(39.0429993F, 45.6780014F), _cubicBezierEasingFunction_05);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_07()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.SetReferenceParameter("_", _root);
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(-62.7919998F, 73.0569992F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.17318435F, new Vector2(-62.7919998F, 73.0569992F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.196966484F, new Vector2(-53.7919998F, 7.55700016F), _cubicBezierEasingFunction_04);
                result.InsertExpressionKeyFrame(0.245809957F, "(Pow(1 - _.t0, 3) * Vector2((-53.792),7.557)) + (3 * Square(1 - _.t0) * _.t0 * Vector2((-53.792),7.557)) + (3 * (1 - _.t0) * Square(_.t0) * Vector2((-52.82329),(-71.07968))) + (Pow(_.t0, 3) * Vector2((-33.667),(-72.818)))", _stepThenHoldEasingFunction);
                result.InsertExpressionKeyFrame(0.301675886F, "(Pow(1 - _.t0, 3) * Vector2((-33.667),(-72.818))) + (3 * Square(1 - _.t0) * _.t0 * Vector2((-17.45947),(-74.28873))) + (3 * (1 - _.t0) * Square(_.t0) * Vector2((-14.167),102.182)) + (Pow(_.t0, 3) * Vector2((-14.167),102.182))", _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, new Vector2(-14.1669998F, 102.181999F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.351955295F, new Vector2(-14.1669998F, 59.1819992F), CubicBezierEasingFunction_14());
                result.InsertKeyFrame(0.407821238F, new Vector2(-14.1669998F, 62.1819992F), CubicBezierEasingFunction_15());
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_08()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.SetReferenceParameter("_", _root);
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(-62.7919998F, 73.0569992F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.17318435F, new Vector2(-62.7919998F, 73.0569992F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.196966484F, new Vector2(-53.7919998F, 7.55700016F), _cubicBezierEasingFunction_04);
                result.InsertExpressionKeyFrame(0.245809957F, "(Pow(1 - _.t1, 3) * Vector2((-53.792),7.557)) + (3 * Square(1 - _.t1) * _.t1 * Vector2((-53.792),7.557)) + (3 * (1 - _.t1) * Square(_.t1) * Vector2((-52.82329),(-71.07968))) + (Pow(_.t1, 3) * Vector2((-33.667),(-72.818)))", _stepThenHoldEasingFunction);
                result.InsertExpressionKeyFrame(0.301675886F, "(Pow(1 - _.t1, 3) * Vector2((-33.667),(-72.818))) + (3 * Square(1 - _.t1) * _.t1 * Vector2((-17.45947),(-74.28873))) + (3 * (1 - _.t1) * Square(_.t1) * Vector2((-14.167),102.182)) + (Pow(_.t1, 3) * Vector2((-14.167),102.182))", _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, new Vector2(-14.1669998F, 102.181999F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.351955295F, new Vector2(-14.1669998F, 59.1819992F), _cubicBezierEasingFunction_14);
                result.InsertKeyFrame(0.407821238F, new Vector2(-14.1669998F, 62.1819992F), _cubicBezierEasingFunction_15);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_09()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(39.875F, 60), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.156424582F, new Vector2(39.875F, 60), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.301675975F, new Vector2(79.375F, 60), _cubicBezierEasingFunction_04);
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_10()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(-33.6669998F, 8.18200016F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.156424582F, new Vector2(-33.6669998F, 8.18200016F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.223463684F, new Vector2(-33.6669998F, -72.8180008F), CubicBezierEasingFunction_18());
                result.InsertKeyFrame(0.301675975F, new Vector2(-33.6669998F, 102.056999F), CubicBezierEasingFunction_19());
                return result;
            }

            // Position
            Vector2KeyFrameAnimation PositionVector2Animation_11()
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(295.770996F, 108.994003F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.104395606F, new Vector2(35.7709999F, 108.994003F), CubicBezierEasingFunction_25());
                return result;
            }

            // Radius
            Vector2KeyFrameAnimation RadiusVector2Animation()
            {
                var result = _radiusVector2Animation = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, new Vector2(1.5F, 1.5F), _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, new Vector2(1.5F, 1.5F), _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.340782136F, new Vector2(22.2999992F, 22.2999992F), CubicBezierEasingFunction_13());
                return result;
            }

            // The root of the composition.
            ContainerVisual Root()
            {
                var result = _root = _c.CreateContainerVisual();
                var propertySet = result.Properties;
                propertySet.InsertScalar("Progress", 0);
                propertySet.InsertScalar("t0", 0);
                propertySet.InsertScalar("t1", 0);
                var children = result.Children;
                children.InsertAtTop(ShapeVisual());
                result.StartAnimation("t0", ScalarAnimation_0_to_1());
                var controller = result.TryGetAnimationController("t0");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                result.StartAnimation("t1", _scalarAnimation_0_to_1);
                controller = result.TryGetAnimationController("t1");
                controller.Pause();
                controller.StartAnimation("Progress", _scalarExpressionAnimation);
                return result;
            }

            // Rectangle Path 1
            // Rectangle Path 1.RectangleGeometry
            CompositionRoundedRectangleGeometry RoundedRectangle_375x667()
            {
                var result = _c.CreateRoundedRectangleGeometry();
                result.CornerRadius = new Vector2(9.99999997E-07F, 9.99999997E-07F);
                result.Offset = new Vector2(-187.5F, -333.5F);
                result.Size = new Vector2(375, 667);
                return result;
            }

            ScalarKeyFrameAnimation ScalarAnimation_0_to_1()
            {
                var result = _scalarAnimation_0_to_1 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.196966588F, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.245809957F, 1, CubicBezierEasingFunction_26());
                result.InsertKeyFrame(0.245810062F, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675886F, 1, CubicBezierEasingFunction_27());
                return result;
            }

            ExpressionAnimation ScalarExpressionAnimation()
            {
                var result = _scalarExpressionAnimation = _c.CreateExpressionAnimation();
                result.SetReferenceParameter("_", _root);
                result.Expression = "_.Progress";
                return result;
            }

            ShapeVisual ShapeVisual()
            {
                var result = _c.CreateShapeVisual();
                result.Size = new Vector2(375, 667);
                var shapes = result.Shapes;
                // Rectangle Path 1
                shapes.Add(SpriteShape_00());
                // Layer (Shape): Dot-Y
                shapes.Add(ContainerShape_00());
                // Layer (Shape): E3-Y
                shapes.Add(ContainerShape_04());
                // Layer (Shape): E3-B
                shapes.Add(ContainerShape_07());
                // Layer (Shape): I-Y
                shapes.Add(ContainerShape_10());
                // Layer (Shape): I-B
                shapes.Add(ContainerShape_13());
                // Layer (Shape): E2-Y
                shapes.Add(ContainerShape_16());
                // Layer (Shape): E2-B
                shapes.Add(ContainerShape_19());
                // Layer (Shape): E1-Y
                shapes.Add(ContainerShape_22());
                // Layer (Shape): E1-B
                shapes.Add(ContainerShape_25());
                // Layer (Shape): T1a-Y
                shapes.Add(ContainerShape_28());
                // Layer (Shape): T2b-Y
                shapes.Add(ContainerShape_31());
                // Layer (Shape): T2a-Y
                shapes.Add(ContainerShape_32());
                // Layer (Shape): T2b-B
                shapes.Add(ContainerShape_33());
                // Layer (Shape): T1b-Y
                shapes.Add(ContainerShape_34());
                // Layer (Shape): T1b-B
                shapes.Add(ContainerShape_35());
                // Layer (Shape): O-Y
                shapes.Add(ContainerShape_36());
                // Layer (Shape): O-B
                shapes.Add(ContainerShape_39());
                // Layer (Shape): T1a-Y 2
                shapes.Add(ContainerShape_42());
                // Layer (Shape): T2a-B
                shapes.Add(ContainerShape_45());
                // Layer (Shape): T1a-B
                shapes.Add(ContainerShape_46());
                // Layer (Shape): Dot-Y
                shapes.Add(ContainerShape_49());
                // Layer (Shape): L-Y
                shapes.Add(ContainerShape_53());
                // Layer (Shape): L-B
                shapes.Add(ContainerShape_54());
                // Layer (Shape): Dot1
                shapes.Add(ContainerShape_55());
                // Layer (Shape): S1-Y
                shapes.Add(ContainerShape_58());
                // Layer (Shape): S2-Y
                shapes.Add(ContainerShape_59());
                // Layer (Shape): S7
                shapes.Add(ContainerShape_60());
                // Layer (Shape): S8
                shapes.Add(ContainerShape_61());
                // Layer (Shape): S3-Y
                shapes.Add(ContainerShape_62());
                // Layer (Shape): S4-Y
                shapes.Add(ContainerShape_63());
                // Layer (Shape): S5-Y
                shapes.Add(ContainerShape_64());
                // Layer (Shape): S6-Y
                shapes.Add(ContainerShape_65());
                // Layer (Shape): S3-Y 2
                shapes.Add(ContainerShape_66());
                // Layer (Shape): S4-Y 2
                shapes.Add(ContainerShape_67());
                // Layer (Shape): S5-Y 2
                shapes.Add(ContainerShape_68());
                // Layer (Shape): S11
                shapes.Add(ContainerShape_69());
                // Layer (Shape): S12
                shapes.Add(ContainerShape_70());
                // Layer (Shape): S13
                shapes.Add(ContainerShape_71());
                // Layer (Shape): S3-Y 3
                shapes.Add(ContainerShape_72());
                // Layer (Shape): S4-Y 3
                shapes.Add(ContainerShape_73());
                // Layer (Shape): S5-Y 3
                shapes.Add(ContainerShape_74());
                // Layer (Shape): S3-Y 4
                shapes.Add(ContainerShape_75());
                // Layer (Shape): S4-Y 4
                shapes.Add(ContainerShape_76());
                // Layer (Shape): S5-Y 4
                shapes.Add(ContainerShape_77());
                return result;
            }

            // Rectangle Path 1
            CompositionSpriteShape SpriteShape_00()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 187.5F, 333.5F);
                result.FillBrush = ColorBrush_AlmostDarkTurquoise_FF00D1C1();
                result.Geometry = RoundedRectangle_375x667();
                return result;
            }

            // Transforms: Dot-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_01()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 196, 267);
                result.FillBrush = ColorBrush_White();
                result.Geometry = Ellipse_4p6();
                return result;
            }

            // Transforms: E3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_02()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 344.674011F, 261.877014F);
                result.Geometry = PathGeometry_00();
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // Transforms: E3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_03()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 344.739014F, 261.877014F);
                result.Geometry = PathGeometry_01();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // Transforms: I-Y
            // Path 1
            CompositionSpriteShape SpriteShape_04()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 304.13501F, 282.408997F);
                result.Geometry = PathGeometry_02();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Transforms: I-Y
            // Path 1
            CompositionSpriteShape SpriteShape_05()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 304.13501F, 282.408997F);
                result.Geometry = PathGeometry_03();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: E2-Y
            // Path 1
            CompositionSpriteShape SpriteShape_06()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 331.664001F, 238.139999F);
                result.Geometry = PathGeometry_04();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Transforms: E2-Y
            // Path 1
            CompositionSpriteShape SpriteShape_07()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 331.664001F, 238.139999F);
                result.Geometry = PathGeometry_05();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // Transforms: E1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_08()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 344.671997F, 214.841995F);
                result.Geometry = PathGeometry_06();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Transforms: E1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_09()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 344.671997F, 214.841995F);
                result.Geometry = PathGeometry_07();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // Transforms: T1a-Y
            // Path 1
            CompositionSpriteShape SpriteShape_10()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 227.677002F, 234.375F);
                result.Geometry = PathGeometry_08();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer (Shape): T2b-Y
            // Path 1
            CompositionSpriteShape SpriteShape_11()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, -56.5F, 83.5F);
                result.Geometry = PathGeometry_09();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer (Shape): T2a-Y
            // Path 1
            CompositionSpriteShape SpriteShape_12()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 221.197998F, 330.757996F);
                result.Geometry = PathGeometry_10();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer (Shape): T2b-B
            // Path 1
            CompositionSpriteShape SpriteShape_13()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, -56.5F, 83.5F);
                result.Geometry = PathGeometry_11();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer (Shape): T1b-Y
            // Path 1
            CompositionSpriteShape SpriteShape_14()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 186.255997F, 349.080994F);
                result.Geometry = PathGeometry_12();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeLineJoin = CompositionStrokeLineJoin.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer (Shape): T1b-B
            // Path 1
            CompositionSpriteShape SpriteShape_15()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 186.255997F, 349.080994F);
                result.Geometry = PathGeometry_13();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeLineJoin = CompositionStrokeLineJoin.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: O-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_16()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 196, 267);
                result.Geometry = Ellipse_0_0();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 8.80000019F;
                return result;
            }

            // Transforms: O-B
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_17()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 196, 267);
                result.Geometry = Ellipse_0_1();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: T1a-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_18()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 227.677002F, 234.375F);
                result.Geometry = PathGeometry_14();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer (Shape): T2a-B
            // Path 1
            CompositionSpriteShape SpriteShape_19()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 221.197998F, 330.757996F);
                result.Geometry = PathGeometry_15();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: T1a-Y
            // Path 1
            CompositionSpriteShape SpriteShape_20()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 227.677002F, 234.375F);
                result.Geometry = PathGeometry_16();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: Dot-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_21()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 196, 267);
                result.FillBrush = _colorBrush_White;
                result.Geometry = Ellipse_4p7();
                return result;
            }

            // Layer (Shape): L-Y
            // Path 1
            CompositionSpriteShape SpriteShape_22()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 109.529007F, 354.143005F);
                result.Geometry = PathGeometry_17();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer (Shape): L-B
            // Path 1
            CompositionSpriteShape SpriteShape_23()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 109.529007F, 354.143005F);
                result.Geometry = PathGeometry_18();
                result.StrokeBrush = _colorBrush_AlmostTeal_FF007A87;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 10;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Transforms: Dot1
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_24()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 196, 267);
                result.FillBrush = _colorBrush_White;
                result.Geometry = _ellipse_4p7;
                return result;
            }

            // Layer (Shape): S1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_25()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_19();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S2-Y
            // Path 1
            CompositionSpriteShape SpriteShape_26()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_20();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S7
            // Path 1
            CompositionSpriteShape SpriteShape_27()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_21();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S8
            // Path 1
            CompositionSpriteShape SpriteShape_28()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_22();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_29()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_23();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S4-Y
            // Path 1
            CompositionSpriteShape SpriteShape_30()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_24();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S5-Y
            // Path 1
            CompositionSpriteShape SpriteShape_31()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_25();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S6-Y
            // Path 1
            CompositionSpriteShape SpriteShape_32()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_26();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S3-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_33()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_27();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S4-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_34()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_28();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S5-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_35()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_29();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S11
            // Path 1
            CompositionSpriteShape SpriteShape_36()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_30();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S12
            // Path 1
            CompositionSpriteShape SpriteShape_37()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_31();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S13
            // Path 1
            CompositionSpriteShape SpriteShape_38()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(1, 0, 0, 1, 179.5F, 333.5F);
                result.Geometry = PathGeometry_32();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer (Shape): S3-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_39()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F);
                result.Geometry = PathGeometry_33();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S4-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_40()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F);
                result.Geometry = PathGeometry_34();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S5-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_41()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F);
                result.Geometry = PathGeometry_35();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S3-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_42()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F);
                result.Geometry = PathGeometry_36();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S4-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_43()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F);
                result.Geometry = PathGeometry_37();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            // Layer (Shape): S5-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_44()
            {
                var result = _c.CreateSpriteShape();
                result.TransformMatrix = new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F);
                result.Geometry = PathGeometry_38();
                result.StrokeBrush = _colorBrush_White;
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 4;
                result.StrokeThickness = 2;
                return result;
            }

            StepEasingFunction StepThenHoldEasingFunction()
            {
                var result = _stepThenHoldEasingFunction = _c.CreateStepEasingFunction();
                result.IsInitialStepSingleFrame = true;
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0_to_1_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.413407832F, 1, CubicBezierEasingFunction_10());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0_to_1_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 1, _cubicBezierEasingFunction_10);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0p81_to_0p734_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.810000002F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.0893854722F, 0.810000002F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.150837988F, 0.734000027F, CubicBezierEasingFunction_22());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0p81_to_0p734_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.810000002F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.100558661F, 0.810000002F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.162011176F, 0.734000027F, _cubicBezierEasingFunction_22);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_00()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.162011176F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.184357539F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.201117322F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_01()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.162011176F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.184357539F, 0.690559983F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.201117322F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_02()
            {
                var result = _tEndScalarAnimation_1_to_0_02 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.363128483F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.391061455F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.418994427F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_03()
            {
                var result = _tEndScalarAnimation_1_to_0_03 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_04()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.758560002F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_05()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.704559982F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_06()
            {
                var result = _tEndScalarAnimation_1_to_0_06 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.541899443F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.558659196F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.597765386F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_07()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.541899443F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.558659196F, 0.758560002F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.597765386F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_08()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.446927369F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.463687152F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.486033529F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_09()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.469273746F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.486033529F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.508379877F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_10()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.47486034F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.502793312F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.525139689F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_11()
            {
                var result = _tEndScalarAnimation_1_to_0_11 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.458100557F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_12()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0.758560002F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.458100557F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_13()
            {
                var result = _tEndScalarAnimation_1_to_0_13 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0.663559973F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.463687152F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_14()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0.758560002F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.463687152F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // Layer (Shape): E3-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_00()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.469273746F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.569832385F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): I-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_01()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.43575418F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.519553065F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): E2-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_02()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.463687152F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.536312878F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): E1-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_03()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.441340774F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.525139689F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T1a-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_04()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.329608947F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.87150836F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T2b-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_05()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.424580991F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.513966501F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T2a-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_06()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.402234644F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.497206718F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T1b-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_07()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.391061455F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.899441361F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): Dot-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_08()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.156424582F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_09()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_09 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.167597771F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.206703916F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_10()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_10 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.363128483F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_11()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_11 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.357541889F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_12()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_12 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.541899443F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.597765386F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): S11
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_13()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.446927369F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.502793312F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): S12
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_14()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.469273746F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.525139689F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): S13
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_15()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.47486034F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.530726254F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_16()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_16 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.418994427F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.463687152F, 0, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_1_to_0_17()
            {
                var result = _transformMatrix_11ScalarAnimation_1_to_0_17 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.424580991F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.469273746F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): Dot1
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.0949720666F, 0, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): Dot-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_00()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.536312878F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): E3-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_01()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.513966501F, 1, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_02()
            {
                var result = _transformMatrix_11ScalarAnimation_to_1_02 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.452513963F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): E2-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_03()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.480446935F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): E1-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_04()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.469273746F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T2b-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_05()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.458100557F, 1, _holdThenStepEasingFunction);
                return result;
            }

            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_06()
            {
                var result = _transformMatrix_11ScalarAnimation_to_1_06 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T1a-Y 2
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_07()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.329608947F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T2a-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_08()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.418994427F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): T1a-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_09()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.391061455F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): L-Y
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_10()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.0893854722F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Layer (Shape): L-B
            ScalarKeyFrameAnimation TransformMatrix_11ScalarAnimation_to_1_11()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0.100558661F, 1, _holdThenStepEasingFunction);
                return result;
            }

            // Transforms: E3-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p316_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.469273746F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.513966501F, 0.316000015F, CubicBezierEasingFunction_04());
                return result;
            }

            // Transforms: E3-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p316_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.513966501F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.541899443F, 0.316000015F, _cubicBezierEasingFunction_04);
                return result;
            }

            // Transforms: E1-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p375_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.491620123F, 0.375F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Transforms: E1-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p375_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.469273746F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.519553065F, 0.375F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Transforms: E2-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p43_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.463687152F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.513966501F, 0.430000007F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Transforms: E2-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p43_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.480446935F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.530726254F, 0.430000007F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Transforms: I-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p457_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.491620123F, 0.456999987F, CubicBezierEasingFunction_07());
                return result;
            }

            // Transforms: I-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p457_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.452513963F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.508379877F, 0.456999987F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Layer (Shape): T1b-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p117_to_1_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.116999999F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.391061455F, 0.116999999F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.418994427F, 1, _cubicBezierEasingFunction_04);
                return result;
            }

            // Layer (Shape): T1b-B
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p117_to_1_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.116999999F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.452513963F, 0.116999999F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.491620123F, 1, CubicBezierEasingFunction_12());
                return result;
            }

            // Transforms: T1a-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p249_to_0p891()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.248999998F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.391061455F, 0.248999998F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.469273746F, 0.890999973F, CubicBezierEasingFunction_17());
                return result;
            }

            // Layer (Shape): T2b-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p411_to_0p665_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.411000013F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 0.411000013F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.47486034F, 0.665000021F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Layer (Shape): T2b-B
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p411_to_0p665_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.411000013F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.458100557F, 0.411000013F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.508379877F, 0.665000021F, _cubicBezierEasingFunction_07);
                return result;
            }

            // Layer (Shape): T2a-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p5_to_1_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.5F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.402234644F, 0.5F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.458100557F, 1, _cubicBezierEasingFunction_11);
                return result;
            }

            // Layer (Shape): T2a-B
            //   Path 1
            //     Path 1.PathGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p5_to_1_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.5F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0.5F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.47486034F, 1, _cubicBezierEasingFunction_11);
                return result;
            }

            // Transforms: O-B
            //   Ellipse Path 1
            //     Ellipse Path 1.EllipseGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_1_to_0p88()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 1, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 1, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.351955295F, 0.879999995F, _cubicBezierEasingFunction_04);
                return result;
            }

            // Transforms: O-B
            //   Ellipse Path 1
            //     Ellipse Path 1.EllipseGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0_to_0p399()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.351955295F, 0.300000012F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.508379877F, 0.398999989F, CubicBezierEasingFunction_16());
                return result;
            }

            // Layer (Shape): T2b-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p29_to_0_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.289999992F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 0.289999992F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.47486034F, 0, _cubicBezierEasingFunction_07);
                return result;
            }

            // Layer (Shape): T2b-B
            //   Path 1
            //     Path 1.PathGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p29_to_0_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.289999992F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.458100557F, 0.289999992F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.508379877F, 0, _cubicBezierEasingFunction_07);
                return result;
            }

            // Layer (Shape): T2a-Y
            //   Path 1
            //     Path 1.PathGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p5_to_0_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.5F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.402234644F, 0.5F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.458100557F, 0, CubicBezierEasingFunction_11());
                return result;
            }

            // Layer (Shape): T2a-B
            //   Path 1
            //     Path 1.PathGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p5_to_0_1()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.5F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0.5F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.47486034F, 0, _cubicBezierEasingFunction_11);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0_to_0p249()
            {
                var result = _tStartScalarAnimation_0_to_0p249 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.391061455F, 0.248999998F, CubicBezierEasingFunction_09());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p8_to_0()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.800000012F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.0893854722F, 0.800000012F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.111731842F, 0.5F, CubicBezierEasingFunction_20());
                result.InsertKeyFrame(0.156424582F, 0, CubicBezierEasingFunction_21());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p8_to_0p3()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.800000012F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.100558661F, 0.800000012F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.128491625F, 0.5F, CubicBezierEasingFunction_23());
                result.InsertKeyFrame(0.30726257F, 0.300000012F, CubicBezierEasingFunction_24());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_00()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.162011176F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.184357539F, 0.375330001F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.201117322F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_01()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.162011176F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.184357539F, 0.253329992F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.201117322F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_02()
            {
                var result = _tStartScalarAnimation_0p87_to_0_02 = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.363128483F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.391061455F, 0.212329999F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.418994427F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_03()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.421330005F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_04()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.438329995F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_05()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.506330013F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_06()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.301675975F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.318435758F, 0.439330012F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.357541889F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_07()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.541899443F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.558659196F, 0.421330005F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.597765386F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_08()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.541899443F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.558659196F, 0.438329995F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.597765386F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_09()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.541899443F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.558659196F, 0.506330013F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.597765386F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_10()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.446927369F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.463687152F, 0.212329999F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.486033529F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_11()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.469273746F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.486033529F, 0.212329999F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.508379877F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_12()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.47486034F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.502793312F, 0.212329999F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.525139689F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_13()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0.421330005F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.458100557F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_14()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0.438329995F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.458100557F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_15()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.418994427F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.43575418F, 0.506330013F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.458100557F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_16()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0.421330005F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.463687152F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_17()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0.438329995F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.463687152F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_18()
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(0, 0.870000005F, _stepThenHoldEasingFunction);
                result.InsertKeyFrame(0.424580991F, 0.870000005F, _holdThenStepEasingFunction);
                result.InsertKeyFrame(0.441340774F, 0.506330013F, _cubicBezierEasingFunction_04);
                result.InsertKeyFrame(0.463687152F, 0, _cubicBezierEasingFunction_04);
                return result;
            }

            internal AnimatedVisual(Compositor compositor)
            {
                var d2dIid = new Guid(ID2D1Factory_IID);
                _d2d = D2D1CreateFactory(0, in d2dIid, IntPtr.Zero);
                _c = compositor;
                _reusableExpressionAnimation = compositor.CreateExpressionAnimation();
                Root();
            }

            Visual IAnimatedVisual.RootVisual => _root;
            TimeSpan IAnimatedVisual.Duration => TimeSpan.FromTicks(c_durationTicks);
            Vector2 IAnimatedVisual.Size => new Vector2(375, 667);
            void IDisposable.Dispose() => _root?.Dispose();

#pragma warning disable 0649
            ref struct D2D1_RECT_F
            {
                internal float left;
                internal float top;
                internal float right;
                internal float bottom;
            }

            ref struct D2D1_ROUNDED_RECT
            {
                internal D2D1_RECT_F rect;
                internal float radiusX;
                internal float radiusY;
            }

#pragma warning restore 0649

            ref struct D2D1_BEZIER_SEGMENT
            {
                internal Vector2 Point1;
                internal Vector2 Point2;
                internal Vector2 Point3;
            }

            const string ID2D1Factory_IID = "06152247-6f50-465a-9245-118bfd3b6007";
            [DllImport("D2D1", PreserveSig = false)]
            static extern ID2D1Factory D2D1CreateFactory(int type, in Guid riid, IntPtr options);

            [Guid(ID2D1Factory_IID), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
            interface ID2D1Factory
            {
                void _unused0();
                void _unused1();
                [return: MarshalAs(UnmanagedType.IUnknown)] object CreateRectangleGeometry(in D2D1_RECT_F rectangle);
                [return: MarshalAs(UnmanagedType.IUnknown)] object CreateRoundedRectangleGeometry(in D2D1_ROUNDED_RECT roundedRectangle);
                void _unused4();
                void _unused5();
                void _unused6();
                ID2D1PathGeometry CreatePathGeometry();
            }

            [Guid("0657AF73-53FD-47CF-84FF-C8492D2A80A3"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown), ComImport]
            interface IGeometrySource2DInterop
            {
                ID2D1Geometry GetGeometry();
                ID2D1Geometry TryGetGeometryUsingFactory(ID2D1Factory factory);
            }

            [Guid("2cd906a1-12e2-11dc-9fed-001143a055f9"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown), ComImport]
            interface ID2D1Geometry
            {
            }

            [Guid("2cd906a5-12e2-11dc-9fed-001143a055f9"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown), ComImport]
            interface ID2D1PathGeometry : ID2D1Geometry
            {
                void _unused0();
                void _unused1();
                void _unused2();
                void _unused3();
                void _unused4();
                void _unused5();
                void _unused6();
                void _unused7();
                void _unused8();
                void _unused9();
                void _unused10();
                void _unused11();
                void _unused12();
                void _unused13();
                ID2D1GeometrySink Open();
            }

            [Guid("2cd9069f-12e2-11dc-9fed-001143a055f9"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown), ComImport]
            interface ID2D1GeometrySink
            {
                [PreserveSig] void SetFillMode(int fillMode);
                void _unused1();
                [PreserveSig] void BeginFigure(Vector2 startPoint, int figureBegin);
                [PreserveSig] void AddLines(in Vector2 points, int count);
                [PreserveSig] void AddBeziers(in D2D1_BEZIER_SEGMENT beziers, int count);
                [PreserveSig] void EndFigure(int figureEnd);
                void Close();
            }

            sealed class GeometrySource2D : Windows.Graphics.IGeometrySource2D, IGeometrySource2DInterop
            {
                readonly ID2D1Geometry _geometry;

                internal GeometrySource2D(ID2D1Geometry geometry)
                {
                    _geometry = geometry;
                }
                ID2D1Geometry IGeometrySource2DInterop.GetGeometry() => _geometry;
                ID2D1Geometry IGeometrySource2DInterop.TryGetGeometryUsingFactory(ID2D1Factory factory) => throw new NotImplementedException();
            }
        }
    }
}
