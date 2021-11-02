﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Graphics.Canvas.Geometry;
using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Graphics;
using Windows.UI;
using Windows.UI.Composition;

namespace AnimatedVisuals
{
    sealed class LottieLogo
        : Microsoft.UI.Xaml.Controls.IAnimatedVisualSource
        , Microsoft.UI.Xaml.Controls.IAnimatedVisualSource2
    {
        // Animation duration: 5.967 seconds.
        internal const long c_durationTicks = 59666666;

        public Microsoft.UI.Xaml.Controls.IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor)
        {
            object ignored = null;
            return TryCreateAnimatedVisual(compositor, out ignored);
        }

        public Microsoft.UI.Xaml.Controls.IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;

            if (LottieLogo_AnimatedVisual.IsRuntimeCompatible())
            {
                var v =
                    new LottieLogo_AnimatedVisual(
                        compositor
                        );
                v.InstantiateAnimations(0.0);
                return v;
            }

            return null;
        }

        /// <summary>
        /// Gets the number of frames in the animation.
        /// </summary>
        public double FrameCount => 179d;

        /// <summary>
        /// Gets the frame rate of the animation.
        /// </summary>
        public double Framerate => 30d;

        /// <summary>
        /// Gets the duration of the animation.
        /// </summary>
        public TimeSpan Duration => TimeSpan.FromTicks(c_durationTicks);

        /// <summary>
        /// Converts a zero-based frame number to the corresponding progress value denoting the
        /// start of the frame.
        /// </summary>
        public double FrameToProgress(double frameNumber)
        {
            return frameNumber / 179d;
        }

        /// <summary>
        /// Returns a map from marker names to corresponding progress values.
        /// </summary>
        public IReadOnlyDictionary<string, double> Markers =>
            new Dictionary<string, double>
            {
            };

        /// <summary>
        /// Sets the color property with the given name, or does nothing if no such property
        /// exists.
        /// </summary>
        public void SetColorProperty(string propertyName, Color value)
        {
        }

        /// <summary>
        /// Sets the scalar property with the given name, or does nothing if no such property
        /// exists.
        /// </summary>
        public void SetScalarProperty(string propertyName, double value)
        {
        }

        sealed class LottieLogo_AnimatedVisual : Microsoft.UI.Xaml.Controls.IAnimatedVisual2
        {
            const long c_durationTicks = 59666666;
            readonly Compositor _c;
            readonly ExpressionAnimation _reusableExpressionAnimation;
            CompositionColorBrush _colorBrush_AlmostTeal_FF007A87;
            CompositionColorBrush _colorBrush_White;
            CompositionContainerShape _containerShape_00;
            CompositionContainerShape _containerShape_01;
            CompositionContainerShape _containerShape_02;
            CompositionContainerShape _containerShape_03;
            CompositionContainerShape _containerShape_04;
            CompositionContainerShape _containerShape_05;
            CompositionContainerShape _containerShape_06;
            CompositionContainerShape _containerShape_07;
            CompositionContainerShape _containerShape_08;
            CompositionContainerShape _containerShape_09;
            CompositionContainerShape _containerShape_10;
            CompositionContainerShape _containerShape_11;
            CompositionContainerShape _containerShape_12;
            CompositionContainerShape _containerShape_13;
            CompositionContainerShape _containerShape_14;
            CompositionContainerShape _containerShape_15;
            CompositionContainerShape _containerShape_16;
            CompositionContainerShape _containerShape_17;
            CompositionContainerShape _containerShape_18;
            CompositionContainerShape _containerShape_19;
            CompositionContainerShape _containerShape_20;
            CompositionContainerShape _containerShape_21;
            CompositionContainerShape _containerShape_22;
            CompositionEllipseGeometry _ellipse_0_0;
            CompositionEllipseGeometry _ellipse_0_1;
            CompositionEllipseGeometry _ellipse_4p7;
            CompositionPath _path_0;
            CompositionPath _path_1;
            CompositionPath _path_2;
            CompositionPath _path_3;
            CompositionPath _path_4;
            CompositionPath _path_5;
            CompositionPath _path_6;
            CompositionPath _path_7;
            CompositionPath _path_8;
            CompositionPathGeometry _pathGeometry_00;
            CompositionPathGeometry _pathGeometry_01;
            CompositionPathGeometry _pathGeometry_02;
            CompositionPathGeometry _pathGeometry_03;
            CompositionPathGeometry _pathGeometry_04;
            CompositionPathGeometry _pathGeometry_05;
            CompositionPathGeometry _pathGeometry_06;
            CompositionPathGeometry _pathGeometry_07;
            CompositionPathGeometry _pathGeometry_08;
            CompositionPathGeometry _pathGeometry_09;
            CompositionPathGeometry _pathGeometry_10;
            CompositionPathGeometry _pathGeometry_11;
            CompositionPathGeometry _pathGeometry_12;
            CompositionPathGeometry _pathGeometry_13;
            CompositionPathGeometry _pathGeometry_14;
            CompositionPathGeometry _pathGeometry_15;
            CompositionPathGeometry _pathGeometry_16;
            CompositionPathGeometry _pathGeometry_17;
            CompositionPathGeometry _pathGeometry_18;
            CompositionPathGeometry _pathGeometry_19;
            CompositionPathGeometry _pathGeometry_20;
            CompositionPathGeometry _pathGeometry_21;
            CompositionPathGeometry _pathGeometry_22;
            CompositionPathGeometry _pathGeometry_23;
            CompositionPathGeometry _pathGeometry_24;
            CompositionPathGeometry _pathGeometry_25;
            CompositionPathGeometry _pathGeometry_26;
            CompositionPathGeometry _pathGeometry_27;
            CompositionPathGeometry _pathGeometry_28;
            CompositionPathGeometry _pathGeometry_29;
            CompositionPathGeometry _pathGeometry_30;
            CompositionPathGeometry _pathGeometry_31;
            CompositionPathGeometry _pathGeometry_32;
            CompositionPathGeometry _pathGeometry_33;
            CompositionPathGeometry _pathGeometry_34;
            CompositionPathGeometry _pathGeometry_35;
            CompositionPathGeometry _pathGeometry_36;
            CompositionPathGeometry _pathGeometry_37;
            CompositionPathGeometry _pathGeometry_38;
            CompositionSpriteShape _spriteShape_11;
            CompositionSpriteShape _spriteShape_12;
            CompositionSpriteShape _spriteShape_13;
            CompositionSpriteShape _spriteShape_14;
            CompositionSpriteShape _spriteShape_15;
            CompositionSpriteShape _spriteShape_19;
            CompositionSpriteShape _spriteShape_22;
            CompositionSpriteShape _spriteShape_23;
            CompositionSpriteShape _spriteShape_36;
            CompositionSpriteShape _spriteShape_37;
            CompositionSpriteShape _spriteShape_38;
            ContainerVisual _root;
            CubicBezierEasingFunction _cubicBezierEasingFunction_0;
            CubicBezierEasingFunction _cubicBezierEasingFunction_1;
            CubicBezierEasingFunction _cubicBezierEasingFunction_2;
            CubicBezierEasingFunction _cubicBezierEasingFunction_3;
            CubicBezierEasingFunction _cubicBezierEasingFunction_4;
            CubicBezierEasingFunction _cubicBezierEasingFunction_5;
            CubicBezierEasingFunction _cubicBezierEasingFunction_6;
            CubicBezierEasingFunction _cubicBezierEasingFunction_7;
            CubicBezierEasingFunction _cubicBezierEasingFunction_8;
            ExpressionAnimation _rootProgress;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_02;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_03;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_06;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_11;
            ScalarKeyFrameAnimation _tEndScalarAnimation_1_to_0_13;
            ScalarKeyFrameAnimation _tStartScalarAnimation_0_to_0p249;
            ScalarKeyFrameAnimation _tStartScalarAnimation_0p87_to_0_02;
            StepEasingFunction _holdThenStepEasingFunction;
            StepEasingFunction _stepThenHoldEasingFunction;
            Vector2KeyFrameAnimation _offsetVector2Animation_02;
            Vector2KeyFrameAnimation _offsetVector2Animation_03;
            Vector2KeyFrameAnimation _offsetVector2Animation_04;
            Vector2KeyFrameAnimation _offsetVector2Animation_05;
            Vector2KeyFrameAnimation _offsetVector2Animation_06;
            Vector2KeyFrameAnimation _radiusVector2Animation;
            Vector2KeyFrameAnimation _shapeVisibilityAnimation_04;

            public void InstantiateAnimations(double progressHint)
            {
                float currentProgress = (float)progressHint;
                StartProgressBoundAnimation(_containerShape_00, "Offset", OffsetVector2Animation_01(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_00, "Scale", ShapeVisibilityAnimation_00(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_01, "Offset", OffsetVector2Animation_00(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_02, "Offset", OffsetVector2Animation_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_02, "Scale", ShapeVisibilityAnimation_01(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_03, "Offset", OffsetVector2Animation_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_03, "Scale", ShapeVisibilityAnimation_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_04, "Offset", OffsetVector2Animation_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_04, "Scale", ShapeVisibilityAnimation_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_05, "Offset", OffsetVector2Animation_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_05, "Scale", ShapeVisibilityAnimation_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_06, "Offset", OffsetVector2Animation_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_06, "Scale", ShapeVisibilityAnimation_05(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_07, "Offset", OffsetVector2Animation_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_07, "Scale", ShapeVisibilityAnimation_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_08, "Offset", OffsetVector2Animation_05(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_08, "Scale", ShapeVisibilityAnimation_07(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_09, "Offset", OffsetVector2Animation_05(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_09, "Scale", ShapeVisibilityAnimation_08(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_10, "Offset", OffsetVector2Animation_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_10, "Scale", ShapeVisibilityAnimation_09(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_11, "Offset", OffsetVector2Animation_07(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_11, "Scale", ShapeVisibilityAnimation_14(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_12, "Offset", OffsetVector2Animation_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_12, "Scale", ShapeVisibilityAnimation_15(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_13, "Offset", OffsetVector2Animation_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_13, "Scale", ShapeVisibilityAnimation_17(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_14, "Offset", OffsetVector2Animation_09(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_14, "Scale", ShapeVisibilityAnimation_18(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_15, "Offset", OffsetVector2Animation_08(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_16, "Scale", ShapeVisibilityAnimation_21(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_17, "Scale", ShapeVisibilityAnimation_22(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_18, "Scale", ShapeVisibilityAnimation_23(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_19, "Scale", ShapeVisibilityAnimation_24(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_20, "Scale", ShapeVisibilityAnimation_25(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_21, "Scale", ShapeVisibilityAnimation_29(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_containerShape_22, "Scale", ShapeVisibilityAnimation_30(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_ellipse_0_0, "Radius", RadiusVector2Animation(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_ellipse_0_1, "Radius", RadiusVector2Animation(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_ellipse_0_1, "TrimStart", TrimStartScalarAnimation_0_to_0p399(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_ellipse_0_1, "TrimEnd", TrimEndScalarAnimation_1_to_0p88(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_00, "TrimEnd", TrimEndScalarAnimation_0_to_0p316_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_01, "TrimEnd", TrimEndScalarAnimation_0_to_0p316_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_02, "TrimEnd", TrimEndScalarAnimation_0_to_0p457_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_03, "TrimEnd", TrimEndScalarAnimation_0_to_0p457_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_04, "TrimEnd", TrimEndScalarAnimation_0_to_0p43_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_05, "TrimEnd", TrimEndScalarAnimation_0_to_0p43_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_06, "TrimEnd", TrimEndScalarAnimation_0_to_0p375_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_07, "TrimEnd", TrimEndScalarAnimation_0_to_0p375_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_08, "TStart", TStartScalarAnimation_0_to_0p249(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_08, "TEnd", TEndScalarAnimation_0_to_1_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_09, "TrimStart", TrimStartScalarAnimation_0p29_to_0_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_09, "TrimEnd", TrimEndScalarAnimation_0p411_to_0p665_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_10, "TrimStart", TrimStartScalarAnimation_0p5_to_0_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_10, "TrimEnd", TrimEndScalarAnimation_0p5_to_1_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_11, "TrimStart", TrimStartScalarAnimation_0p29_to_0_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_11, "TrimEnd", TrimEndScalarAnimation_0p411_to_0p665_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_12, "TrimEnd", TrimEndScalarAnimation_0p117_to_1_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_13, "TrimEnd", TrimEndScalarAnimation_0p117_to_1_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_14, "TStart", TStartScalarAnimation_0_to_0p249(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_14, "TEnd", TEndScalarAnimation_0_to_1_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_15, "TrimStart", TrimStartScalarAnimation_0p5_to_0_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_15, "TrimEnd", TrimEndScalarAnimation_0p5_to_1_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_16, "TrimEnd", TrimEndScalarAnimation_0p249_to_0p891(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_17, "TStart", TStartScalarAnimation_0p8_to_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_17, "TEnd", TEndScalarAnimation_0p81_to_0p734_0(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_18, "TStart", TStartScalarAnimation_0p8_to_0p3(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_18, "TEnd", TEndScalarAnimation_0p81_to_0p734_1(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_19, "TStart", TStartScalarAnimation_0p87_to_0_00(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_19, "TEnd", TEndScalarAnimation_1_to_0_00(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_20, "TStart", TStartScalarAnimation_0p87_to_0_01(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_20, "TEnd", TEndScalarAnimation_1_to_0_01(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_21, "TStart", TStartScalarAnimation_0p87_to_0_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_21, "TEnd", TEndScalarAnimation_1_to_0_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_22, "TStart", TStartScalarAnimation_0p87_to_0_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_22, "TEnd", TEndScalarAnimation_1_to_0_02(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_23, "TStart", TStartScalarAnimation_0p87_to_0_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_23, "TEnd", TEndScalarAnimation_1_to_0_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_24, "TStart", TStartScalarAnimation_0p87_to_0_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_24, "TEnd", TEndScalarAnimation_1_to_0_03(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_25, "TStart", TStartScalarAnimation_0p87_to_0_05(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_25, "TEnd", TEndScalarAnimation_1_to_0_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_26, "TStart", TStartScalarAnimation_0p87_to_0_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_26, "TEnd", TEndScalarAnimation_1_to_0_05(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_27, "TStart", TStartScalarAnimation_0p87_to_0_07(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_27, "TEnd", TEndScalarAnimation_1_to_0_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_28, "TStart", TStartScalarAnimation_0p87_to_0_08(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_28, "TEnd", TEndScalarAnimation_1_to_0_06(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_29, "TStart", TStartScalarAnimation_0p87_to_0_09(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_29, "TEnd", TEndScalarAnimation_1_to_0_07(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_30, "TStart", TStartScalarAnimation_0p87_to_0_10(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_30, "TEnd", TEndScalarAnimation_1_to_0_08(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_31, "TStart", TStartScalarAnimation_0p87_to_0_11(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_31, "TEnd", TEndScalarAnimation_1_to_0_09(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_32, "TStart", TStartScalarAnimation_0p87_to_0_12(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_32, "TEnd", TEndScalarAnimation_1_to_0_10(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_33, "TStart", TStartScalarAnimation_0p87_to_0_13(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_33, "TEnd", TEndScalarAnimation_1_to_0_11(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_34, "TStart", TStartScalarAnimation_0p87_to_0_14(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_34, "TEnd", TEndScalarAnimation_1_to_0_11(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_35, "TStart", TStartScalarAnimation_0p87_to_0_15(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_35, "TEnd", TEndScalarAnimation_1_to_0_12(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_36, "TStart", TStartScalarAnimation_0p87_to_0_16(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_36, "TEnd", TEndScalarAnimation_1_to_0_13(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_37, "TStart", TStartScalarAnimation_0p87_to_0_17(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_37, "TEnd", TEndScalarAnimation_1_to_0_13(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_38, "TStart", TStartScalarAnimation_0p87_to_0_18(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_pathGeometry_38, "TEnd", TEndScalarAnimation_1_to_0_14(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_11, "Scale", ShapeVisibilityAnimation_10(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_12, "Scale", ShapeVisibilityAnimation_11(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_13, "Scale", ShapeVisibilityAnimation_12(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_14, "Scale", ShapeVisibilityAnimation_13(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_15, "Scale", ShapeVisibilityAnimation_04(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_19, "Scale", ShapeVisibilityAnimation_16(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_22, "Scale", ShapeVisibilityAnimation_19(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_23, "Scale", ShapeVisibilityAnimation_20(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_36, "Scale", ShapeVisibilityAnimation_26(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_37, "Scale", ShapeVisibilityAnimation_27(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_spriteShape_38, "Scale", ShapeVisibilityAnimation_28(), RootProgress(),currentProgress);
                StartProgressBoundAnimation(_root.Properties, "t0", t0ScalarAnimation_0_to_1(), RootProgress(),currentProgress);
            }
            public void DestroyAnimations()
            {
                _containerShape_00.StopAnimation("Offset");
                _containerShape_00.StopAnimation("Scale");
                _containerShape_01.StopAnimation("Offset");
                _containerShape_02.StopAnimation("Offset");
                _containerShape_02.StopAnimation("Scale");
                _containerShape_03.StopAnimation("Offset");
                _containerShape_03.StopAnimation("Scale");
                _containerShape_04.StopAnimation("Offset");
                _containerShape_04.StopAnimation("Scale");
                _containerShape_05.StopAnimation("Offset");
                _containerShape_05.StopAnimation("Scale");
                _containerShape_06.StopAnimation("Offset");
                _containerShape_06.StopAnimation("Scale");
                _containerShape_07.StopAnimation("Offset");
                _containerShape_07.StopAnimation("Scale");
                _containerShape_08.StopAnimation("Offset");
                _containerShape_08.StopAnimation("Scale");
                _containerShape_09.StopAnimation("Offset");
                _containerShape_09.StopAnimation("Scale");
                _containerShape_10.StopAnimation("Offset");
                _containerShape_10.StopAnimation("Scale");
                _containerShape_11.StopAnimation("Offset");
                _containerShape_11.StopAnimation("Scale");
                _containerShape_12.StopAnimation("Offset");
                _containerShape_12.StopAnimation("Scale");
                _containerShape_13.StopAnimation("Offset");
                _containerShape_13.StopAnimation("Scale");
                _containerShape_14.StopAnimation("Offset");
                _containerShape_14.StopAnimation("Scale");
                _containerShape_15.StopAnimation("Offset");
                _containerShape_16.StopAnimation("Scale");
                _containerShape_17.StopAnimation("Scale");
                _containerShape_18.StopAnimation("Scale");
                _containerShape_19.StopAnimation("Scale");
                _containerShape_20.StopAnimation("Scale");
                _containerShape_21.StopAnimation("Scale");
                _containerShape_22.StopAnimation("Scale");
                _ellipse_0_0.StopAnimation("Radius");
                _ellipse_0_1.StopAnimation("Radius");
                _ellipse_0_1.StopAnimation("TrimStart");
                _ellipse_0_1.StopAnimation("TrimEnd");
                _pathGeometry_00.StopAnimation("TrimEnd");
                _pathGeometry_01.StopAnimation("TrimEnd");
                _pathGeometry_02.StopAnimation("TrimEnd");
                _pathGeometry_03.StopAnimation("TrimEnd");
                _pathGeometry_04.StopAnimation("TrimEnd");
                _pathGeometry_05.StopAnimation("TrimEnd");
                _pathGeometry_06.StopAnimation("TrimEnd");
                _pathGeometry_07.StopAnimation("TrimEnd");
                _pathGeometry_08.StopAnimation("TStart");
                _pathGeometry_08.StopAnimation("TEnd");
                _pathGeometry_09.StopAnimation("TrimStart");
                _pathGeometry_09.StopAnimation("TrimEnd");
                _pathGeometry_10.StopAnimation("TrimStart");
                _pathGeometry_10.StopAnimation("TrimEnd");
                _pathGeometry_11.StopAnimation("TrimStart");
                _pathGeometry_11.StopAnimation("TrimEnd");
                _pathGeometry_12.StopAnimation("TrimEnd");
                _pathGeometry_13.StopAnimation("TrimEnd");
                _pathGeometry_14.StopAnimation("TStart");
                _pathGeometry_14.StopAnimation("TEnd");
                _pathGeometry_15.StopAnimation("TrimStart");
                _pathGeometry_15.StopAnimation("TrimEnd");
                _pathGeometry_16.StopAnimation("TrimEnd");
                _pathGeometry_17.StopAnimation("TStart");
                _pathGeometry_17.StopAnimation("TEnd");
                _pathGeometry_18.StopAnimation("TStart");
                _pathGeometry_18.StopAnimation("TEnd");
                _pathGeometry_19.StopAnimation("TStart");
                _pathGeometry_19.StopAnimation("TEnd");
                _pathGeometry_20.StopAnimation("TStart");
                _pathGeometry_20.StopAnimation("TEnd");
                _pathGeometry_21.StopAnimation("TStart");
                _pathGeometry_21.StopAnimation("TEnd");
                _pathGeometry_22.StopAnimation("TStart");
                _pathGeometry_22.StopAnimation("TEnd");
                _pathGeometry_23.StopAnimation("TStart");
                _pathGeometry_23.StopAnimation("TEnd");
                _pathGeometry_24.StopAnimation("TStart");
                _pathGeometry_24.StopAnimation("TEnd");
                _pathGeometry_25.StopAnimation("TStart");
                _pathGeometry_25.StopAnimation("TEnd");
                _pathGeometry_26.StopAnimation("TStart");
                _pathGeometry_26.StopAnimation("TEnd");
                _pathGeometry_27.StopAnimation("TStart");
                _pathGeometry_27.StopAnimation("TEnd");
                _pathGeometry_28.StopAnimation("TStart");
                _pathGeometry_28.StopAnimation("TEnd");
                _pathGeometry_29.StopAnimation("TStart");
                _pathGeometry_29.StopAnimation("TEnd");
                _pathGeometry_30.StopAnimation("TStart");
                _pathGeometry_30.StopAnimation("TEnd");
                _pathGeometry_31.StopAnimation("TStart");
                _pathGeometry_31.StopAnimation("TEnd");
                _pathGeometry_32.StopAnimation("TStart");
                _pathGeometry_32.StopAnimation("TEnd");
                _pathGeometry_33.StopAnimation("TStart");
                _pathGeometry_33.StopAnimation("TEnd");
                _pathGeometry_34.StopAnimation("TStart");
                _pathGeometry_34.StopAnimation("TEnd");
                _pathGeometry_35.StopAnimation("TStart");
                _pathGeometry_35.StopAnimation("TEnd");
                _pathGeometry_36.StopAnimation("TStart");
                _pathGeometry_36.StopAnimation("TEnd");
                _pathGeometry_37.StopAnimation("TStart");
                _pathGeometry_37.StopAnimation("TEnd");
                _pathGeometry_38.StopAnimation("TStart");
                _pathGeometry_38.StopAnimation("TEnd");
                _spriteShape_11.StopAnimation("Scale");
                _spriteShape_12.StopAnimation("Scale");
                _spriteShape_13.StopAnimation("Scale");
                _spriteShape_14.StopAnimation("Scale");
                _spriteShape_15.StopAnimation("Scale");
                _spriteShape_19.StopAnimation("Scale");
                _spriteShape_22.StopAnimation("Scale");
                _spriteShape_23.StopAnimation("Scale");
                _spriteShape_36.StopAnimation("Scale");
                _spriteShape_37.StopAnimation("Scale");
                _spriteShape_38.StopAnimation("Scale");
                _root.Properties.StopAnimation("t0");
            }
            static void StartProgressBoundAnimation(
                CompositionObject target,
                string animatedPropertyName,
                CompositionAnimation animation,
                ExpressionAnimation controllerProgressExpression, float currentProgress)
            {
                target.StartAnimation(animatedPropertyName, animation);
                var controller = target.TryGetAnimationController(animatedPropertyName);
                controller.Progress = currentProgress;
                controller.Pause();
                controller.StartAnimation("Progress", controllerProgressExpression);
            }

            void BindProperty(
                CompositionObject target,
                string animatedPropertyName,
                string expression,
                string referenceParameterName,
                CompositionObject referencedObject)
            {
                _reusableExpressionAnimation.ClearAllParameters();
                _reusableExpressionAnimation.Expression = expression;
                _reusableExpressionAnimation.SetReferenceParameter(referenceParameterName, referencedObject);
                target.StartAnimation(animatedPropertyName, _reusableExpressionAnimation);
            }

            ScalarKeyFrameAnimation CreateScalarKeyFrameAnimation(float initialProgress, float initialValue, CompositionEasingFunction initialEasingFunction)
            {
                var result = _c.CreateScalarKeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(initialProgress, initialValue, initialEasingFunction);
                return result;
            }

            Vector2KeyFrameAnimation CreateVector2KeyFrameAnimation(float initialProgress, Vector2 initialValue, CompositionEasingFunction initialEasingFunction)
            {
                var result = _c.CreateVector2KeyFrameAnimation();
                result.Duration = TimeSpan.FromTicks(c_durationTicks);
                result.InsertKeyFrame(initialProgress, initialValue, initialEasingFunction);
                return result;
            }

            CompositionSpriteShape CreateSpriteShape(CompositionGeometry geometry, Matrix3x2 transformMatrix)
            {
                var result = _c.CreateSpriteShape(geometry);
                result.TransformMatrix = transformMatrix;
                return result;
            }

            CompositionSpriteShape CreateSpriteShape(CompositionGeometry geometry, Matrix3x2 transformMatrix, CompositionBrush fillBrush)
            {
                var result = _c.CreateSpriteShape(geometry);
                result.TransformMatrix = transformMatrix;
                result.FillBrush = fillBrush;
                return result;
            }

            CanvasGeometry Geometry_00()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-13.6639996F, -0.144999996F));
                    builder.AddLine(new Vector2(75.663002F, 0.289999992F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_01()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(0.859000027F, -21.1429996F));
                    builder.AddLine(new Vector2(-4.35900021F, 70.3919983F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_02()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-26.6700001F, -0.282999992F));
                    builder.AddLine(new Vector2(99.1709976F, 0.0659999996F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_03()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-13.6639996F, -0.144999996F));
                    builder.AddLine(new Vector2(62.1629982F, 0.289999992F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_04()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-30.7199993F, 63.7610016F));
                    builder.AddCubicBezier(new Vector2(-30.6889992F, 63.1669998F), new Vector2(-30.7889996F, 50.8470001F), new Vector2(-30.7409992F, 45.1920013F));
                    builder.AddCubicBezier(new Vector2(-30.6650009F, 36.2140007F), new Vector2(-37.3429985F, 27.0739994F), new Vector2(-37.3969994F, 27.0139999F));
                    builder.AddCubicBezier(new Vector2(-38.5579987F, 25.7140007F), new Vector2(-39.7519989F, 24.1469994F), new Vector2(-40.6980019F, 22.6609993F));
                    builder.AddCubicBezier(new Vector2(-46.637001F, 13.3339996F), new Vector2(-47.8400002F, 0.933000028F), new Vector2(-37.8730011F, -7.1170001F));
                    builder.AddCubicBezier(new Vector2(-13.1960001F, -27.0459995F), new Vector2(8.96000004F, 11.559F), new Vector2(49.5060005F, 11.559F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_05()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(246.649994F, 213.813995F));
                    builder.AddLine(new Vector2(340.955994F, 213.628006F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_06()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(1.68099999F, -29.9920006F));
                    builder.AddLine(new Vector2(-1.68099999F, 29.9920006F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_07()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(1.76800001F, -25.9659996F));
                    builder.AddLine(new Vector2(-1.76800001F, 25.9659996F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_08()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-8.83699989F, -58.2290001F));
                    builder.AddCubicBezier(new Vector2(-8.83699989F, -58.2290001F), new Vector2(-10.1630001F, 29.4950008F), new Vector2(-35.8339996F, 33.6619987F));
                    builder.AddCubicBezier(new Vector2(-44.0579987F, 34.9970016F), new Vector2(-50.2319984F, 30.0499992F), new Vector2(-51.6879997F, 23.1480007F));
                    builder.AddCubicBezier(new Vector2(-53.144001F, 16.2450008F), new Vector2(-49.6549988F, 9.15600014F), new Vector2(-41.1739998F, 7.29300022F));
                    builder.AddCubicBezier(new Vector2(-17.3570004F, 2.05999994F), new Vector2(4.23500013F, 57.1879997F), new Vector2(51.7970009F, 44.1780014F));
                    builder.AddCubicBezier(new Vector2(51.9570007F, 44.1339989F), new Vector2(52.6870003F, 43.8740005F), new Vector2(53.1879997F, 43.7410011F));
                    builder.AddCubicBezier(new Vector2(53.6889992F, 43.6080017F), new Vector2(68.9710007F, 41.3569984F), new Vector2(140.393997F, 43.6720009F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_09()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-67.125F, -112F));
                    builder.AddCubicBezier(new Vector2(-67.125F, -112F), new Vector2(-73.5579987F, -100.719002F), new Vector2(-75.4580002F, -89.9509964F));
                    builder.AddCubicBezier(new Vector2(-78.625F, -72F), new Vector2(-79.375F, -58.25F), new Vector2(-80.375F, -39.25F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_10()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-67.25F, -105.5F));
                    builder.AddCubicBezier(new Vector2(-67.25F, -105.5F), new Vector2(-70.4329987F, -94.9690018F), new Vector2(-72.3330002F, -84.2009964F));
                    builder.AddCubicBezier(new Vector2(-75.5F, -66.25F), new Vector2(-75.5F, -56.75F), new Vector2(-76.5F, -37.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_11()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(34.5F, -13.0500002F));
                    builder.AddCubicBezier(new Vector2(7.5F, -14.5F), new Vector2(-4F, -37F), new Vector2(-35.0460014F, -35.5789986F));
                    builder.AddCubicBezier(new Vector2(-61.4720001F, -34.3689995F), new Vector2(-62.25F, -5.75F), new Vector2(-62.25F, -5.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_12()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-3F, 35.9500008F));
                    builder.AddCubicBezier(new Vector2(-3F, 35.9500008F), new Vector2(-1.5F, 7.5F), new Vector2(-1.352F, -6.75600004F));
                    builder.AddCubicBezier(new Vector2(-9.90299988F, -15.0190001F), new Vector2(-21.5699997F, -20.5790005F), new Vector2(-32.0460014F, -20.5790005F));
                    builder.AddCubicBezier(new Vector2(-53.5F, -20.5790005F), new Vector2(-42.25F, 4.25F), new Vector2(-42.25F, 4.25F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_13()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(16.2310009F, 39.0730019F));
                    builder.AddLine(new Vector2(-32.769001F, 57.3650017F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_14()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(7.44999981F, 21.9500008F));
                    builder.AddLine(new Vector2(-32.75F, 55.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_15()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-94.5F, 37.0730019F));
                    builder.AddLine(new Vector2(-48.769001F, 55.3650017F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_16()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(-87.5F, 20.9500008F));
                    builder.AddLine(new Vector2(-48.75F, 54.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_17()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(166.731003F, -7.92700005F));
                    builder.AddLine(new Vector2(136.731003F, 7.11499977F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_18()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(156.449997F, -23.0499992F));
                    builder.AddLine(new Vector2(132F, 2.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_19()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(169.5F, 18.073F));
                    builder.AddLine(new Vector2(137.481003F, 11.3649998F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_20()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(119.5F, -45.0499992F));
                    builder.AddLine(new Vector2(82.75F, -44.75F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_21()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(119.25F, -20.0499992F));
                    builder.AddLine(new Vector2(63.5F, -20.5F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_22()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(128F, 3.6500001F));
                    builder.AddLine(new Vector2(78.25F, 3.5F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_23()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(149.623993F, 8.24400043F));
                    builder.AddLine(new Vector2(136.647995F, 10.1560001F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_24()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(144.429001F, -5.39699984F));
                    builder.AddLine(new Vector2(132.274994F, 4.73099995F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_25()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(145.677002F, 22.2199993F));
                    builder.AddLine(new Vector2(134.921997F, 14.7489996F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_26()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(147.699005F, 13.0249996F));
                    builder.AddLine(new Vector2(133.195007F, 13.21F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_27()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(142.182999F, -5.11199999F));
                    builder.AddLine(new Vector2(130.029007F, 5.01599979F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            CanvasGeometry Geometry_28()
            {
                CanvasGeometry result;
                using (var builder = new CanvasPathBuilder(null))
                {
                    builder.BeginFigure(new Vector2(142.037994F, 29.2779999F));
                    builder.AddLine(new Vector2(131.281998F, 21.8069992F));
                    builder.EndFigure(CanvasFigureLoop.Open);
                    result = CanvasGeometry.CreatePath(builder);
                }
                return result;
            }

            // - Layer aggregator
            // Offset:<187.5, 333.5>
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

            // Layer aggregator
            // Transforms for Bncr
            CompositionContainerShape ContainerShape_00()
            {
                if (_containerShape_00 != null) return _containerShape_00;
                var result = _containerShape_00 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: Dot-Y
                result.Shapes.Add(ContainerShape_01());
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            // Transforms for Dot-Y
            CompositionContainerShape ContainerShape_01()
            {
                if (_containerShape_01 != null) return _containerShape_01;
                var result = _containerShape_01 = _c.CreateContainerShape();
                // ShapeGroup: Ellipse 1 Offset:<196, 267>
                result.Shapes.Add(SpriteShape_01());
                return result;
            }

            // Layer aggregator
            // Transforms for E3-Y
            CompositionContainerShape ContainerShape_02()
            {
                if (_containerShape_02 != null) return _containerShape_02;
                var result = _containerShape_02 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 1 Offset:<344.674, 261.877>
                result.Shapes.Add(SpriteShape_02());
                return result;
            }

            // Layer aggregator
            // Transforms for E3-Y
            CompositionContainerShape ContainerShape_03()
            {
                if (_containerShape_03 != null) return _containerShape_03;
                var result = _containerShape_03 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: E3-B Offset:<0.06500244, 0>
                result.Shapes.Add(SpriteShape_03());
                return result;
            }

            // Layer aggregator
            // Transforms for I-Y
            CompositionContainerShape ContainerShape_04()
            {
                if (_containerShape_04 != null) return _containerShape_04;
                var result = _containerShape_04 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 6 Offset:<304.135, 282.409>
                result.Shapes.Add(SpriteShape_04());
                return result;
            }

            // Layer aggregator
            // Transforms for I-Y
            CompositionContainerShape ContainerShape_05()
            {
                if (_containerShape_05 != null) return _containerShape_05;
                var result = _containerShape_05 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: I-B
                result.Shapes.Add(SpriteShape_05());
                return result;
            }

            // Layer aggregator
            // Transforms for E2-Y
            CompositionContainerShape ContainerShape_06()
            {
                if (_containerShape_06 != null) return _containerShape_06;
                var result = _containerShape_06 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 3 Offset:<331.664, 238.14>
                result.Shapes.Add(SpriteShape_06());
                return result;
            }

            // Layer aggregator
            // Transforms for E2-Y
            CompositionContainerShape ContainerShape_07()
            {
                if (_containerShape_07 != null) return _containerShape_07;
                var result = _containerShape_07 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: E2-B
                result.Shapes.Add(SpriteShape_07());
                return result;
            }

            // Layer aggregator
            // Transforms for E1-Y
            CompositionContainerShape ContainerShape_08()
            {
                if (_containerShape_08 != null) return _containerShape_08;
                var result = _containerShape_08 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 2 Offset:<344.672, 214.842>
                result.Shapes.Add(SpriteShape_08());
                return result;
            }

            // Layer aggregator
            // Transforms for E1-Y
            CompositionContainerShape ContainerShape_09()
            {
                if (_containerShape_09 != null) return _containerShape_09;
                var result = _containerShape_09 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: E1-B
                result.Shapes.Add(SpriteShape_09());
                return result;
            }

            // Layer aggregator
            // Transforms for T1a-Y
            CompositionContainerShape ContainerShape_10()
            {
                if (_containerShape_10 != null) return _containerShape_10;
                var result = _containerShape_10 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 9 Offset:<227.677, 234.375>
                result.Shapes.Add(SpriteShape_10());
                return result;
            }

            // Layer aggregator
            // Transforms for O-Y
            CompositionContainerShape ContainerShape_11()
            {
                if (_containerShape_11 != null) return _containerShape_11;
                var result = _containerShape_11 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                var shapes = result.Shapes;
                // ShapeGroup: Ellipse 1 Offset:<196, 267>
                shapes.Add(SpriteShape_16());
                // ShapeGroup: Ellipse 1 Offset:<196, 267>
                shapes.Add(SpriteShape_17());
                return result;
            }

            // Layer aggregator
            // Transforms for T1a-Y 2
            CompositionContainerShape ContainerShape_12()
            {
                if (_containerShape_12 != null) return _containerShape_12;
                var result = _containerShape_12 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Group 9 Offset:<227.677, 234.375>
                result.Shapes.Add(SpriteShape_18());
                return result;
            }

            // Layer aggregator
            // Transforms for T1a-Y
            CompositionContainerShape ContainerShape_13()
            {
                if (_containerShape_13 != null) return _containerShape_13;
                var result = _containerShape_13 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: T1a-B
                result.Shapes.Add(SpriteShape_20());
                return result;
            }

            // Layer aggregator
            // Transforms for N
            CompositionContainerShape ContainerShape_14()
            {
                if (_containerShape_14 != null) return _containerShape_14;
                var result = _containerShape_14 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // Transforms: Dot-Y
                result.Shapes.Add(ContainerShape_15());
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            // Transforms for Dot-Y
            CompositionContainerShape ContainerShape_15()
            {
                if (_containerShape_15 != null) return _containerShape_15;
                var result = _containerShape_15 = _c.CreateContainerShape();
                // ShapeGroup: Ellipse 1 Offset:<196, 267>
                result.Shapes.Add(SpriteShape_21());
                return result;
            }

            // Layer aggregator
            // Transforms for Dot1
            CompositionContainerShape ContainerShape_16()
            {
                if (_containerShape_16 != null) return _containerShape_16;
                var result = _containerShape_16 = _c.CreateContainerShape();
                // Offset:<154.457, 287.822>
                result.TransformMatrix = new Matrix3x2(1F, 0F, 0F, 1F, 154.457001F, 287.821991F);
                // ShapeGroup: Ellipse 1 Offset:<196, 267>
                result.Shapes.Add(SpriteShape_24());
                _containerShape_16.StartAnimation("Offset", OffsetVector2Animation_10());
                var controller = _containerShape_16.TryGetAnimationController("Offset");
                controller.Pause();
                BindProperty(controller, "Progress", "_.Progress*0.9835165+0.01648352", "_", _root);
                return result;
            }

            // Layer aggregator
            // Layer: S1-Y
            CompositionContainerShape ContainerShape_17()
            {
                if (_containerShape_17 != null) return _containerShape_17;
                var result = _containerShape_17 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_25());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_26());
                return result;
            }

            // Layer aggregator
            // Layer: S7
            CompositionContainerShape ContainerShape_18()
            {
                if (_containerShape_18 != null) return _containerShape_18;
                var result = _containerShape_18 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_27());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_28());
                return result;
            }

            // Layer aggregator
            // Layer: S3-Y
            CompositionContainerShape ContainerShape_19()
            {
                if (_containerShape_19 != null) return _containerShape_19;
                var result = _containerShape_19 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_29());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_30());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_31());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_32());
                return result;
            }

            // Layer aggregator
            // Layer: S3-Y 2
            CompositionContainerShape ContainerShape_20()
            {
                if (_containerShape_20 != null) return _containerShape_20;
                var result = _containerShape_20 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_33());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_34());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_35());
                return result;
            }

            // Layer aggregator
            // Layer: S3-Y 3
            CompositionContainerShape ContainerShape_21()
            {
                if (_containerShape_21 != null) return _containerShape_21;
                var result = _containerShape_21 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_39());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_40());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_41());
                return result;
            }

            // Layer aggregator
            // Layer: S3-Y 4
            CompositionContainerShape ContainerShape_22()
            {
                if (_containerShape_22 != null) return _containerShape_22;
                var result = _containerShape_22 = _c.CreateContainerShape();
                var shapes = result.Shapes;
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_42());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_43());
                // Offset:<154.457, 287.822>
                shapes.Add(SpriteShape_44());
                return result;
            }

            // - - Layer aggregator
            // - Layer: O-Y
            // ShapeGroup: Ellipse 1 Offset:<196, 267>
            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_0_0()
            {
                if (_ellipse_0_0 != null) return _ellipse_0_0;
                var result = _ellipse_0_0 = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(0F, 0F);
                return result;
            }

            // - - Layer aggregator
            // - Layer: O-Y
            // ShapeGroup: Ellipse 1 Offset:<196, 267>
            // Ellipse Path 1.EllipseGeometry
            CompositionEllipseGeometry Ellipse_0_1()
            {
                if (_ellipse_0_1 != null) return _ellipse_0_1;
                var result = _ellipse_0_1 = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(0F, 0F);
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: Dot-Y
            // - Transforms: Dot-Y
            // ShapeGroup: Ellipse 1 Offset:<196, 267>
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
                if (_ellipse_4p7 != null) return _ellipse_4p7;
                var result = _ellipse_4p7 = _c.CreateEllipseGeometry();
                result.Center = new Vector2(0.800000012F, -0.5F);
                result.Radius = new Vector2(4.69999981F, 4.69999981F);
                return result;
            }

            CompositionPath Path_0()
            {
                if (_path_0 != null) return _path_0;
                var result = _path_0 = new CompositionPath(Geometry_00());
                return result;
            }

            CompositionPath Path_1()
            {
                if (_path_1 != null) return _path_1;
                var result = _path_1 = new CompositionPath(Geometry_01());
                return result;
            }

            CompositionPath Path_2()
            {
                if (_path_2 != null) return _path_2;
                var result = _path_2 = new CompositionPath(Geometry_02());
                return result;
            }

            CompositionPath Path_3()
            {
                if (_path_3 != null) return _path_3;
                var result = _path_3 = new CompositionPath(Geometry_03());
                return result;
            }

            CompositionPath Path_4()
            {
                if (_path_4 != null) return _path_4;
                var result = _path_4 = new CompositionPath(Geometry_04());
                return result;
            }

            CompositionPath Path_5()
            {
                if (_path_5 != null) return _path_5;
                var result = _path_5 = new CompositionPath(Geometry_05());
                return result;
            }

            CompositionPath Path_6()
            {
                if (_path_6 != null) return _path_6;
                var result = _path_6 = new CompositionPath(Geometry_06());
                return result;
            }

            CompositionPath Path_7()
            {
                if (_path_7 != null) return _path_7;
                var result = _path_7 = new CompositionPath(Geometry_07());
                return result;
            }

            CompositionPath Path_8()
            {
                if (_path_8 != null) return _path_8;
                var result = _path_8 = new CompositionPath(Geometry_08());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E3-Y
            // ShapeGroup: Group 1 Offset:<344.674, 261.877>
            CompositionPathGeometry PathGeometry_00()
            {
                if (_pathGeometry_00 != null) return _pathGeometry_00;
                var result = _pathGeometry_00 = _c.CreatePathGeometry(Path_0());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E3-B
            // Transforms: E3-B Offset:<0.06500244, 0>
            CompositionPathGeometry PathGeometry_01()
            {
                if (_pathGeometry_01 != null) return _pathGeometry_01;
                var result = _pathGeometry_01 = _c.CreatePathGeometry(Path_0());
                return result;
            }

            // - - Layer aggregator
            // - Layer: I-Y
            // ShapeGroup: Group 6 Offset:<304.135, 282.409>
            CompositionPathGeometry PathGeometry_02()
            {
                if (_pathGeometry_02 != null) return _pathGeometry_02;
                var result = _pathGeometry_02 = _c.CreatePathGeometry(Path_1());
                return result;
            }

            // - - Layer aggregator
            // - Layer: I-B
            // Transforms: I-B
            CompositionPathGeometry PathGeometry_03()
            {
                if (_pathGeometry_03 != null) return _pathGeometry_03;
                var result = _pathGeometry_03 = _c.CreatePathGeometry(Path_1());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E2-Y
            // ShapeGroup: Group 3 Offset:<331.664, 238.14>
            CompositionPathGeometry PathGeometry_04()
            {
                if (_pathGeometry_04 != null) return _pathGeometry_04;
                var result = _pathGeometry_04 = _c.CreatePathGeometry(Path_2());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E2-B
            // Transforms: E2-B
            CompositionPathGeometry PathGeometry_05()
            {
                if (_pathGeometry_05 != null) return _pathGeometry_05;
                var result = _pathGeometry_05 = _c.CreatePathGeometry(Path_2());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E1-Y
            // ShapeGroup: Group 2 Offset:<344.672, 214.842>
            CompositionPathGeometry PathGeometry_06()
            {
                if (_pathGeometry_06 != null) return _pathGeometry_06;
                var result = _pathGeometry_06 = _c.CreatePathGeometry(Path_3());
                return result;
            }

            // - - Layer aggregator
            // - Layer: E1-B
            // Transforms: E1-B
            CompositionPathGeometry PathGeometry_07()
            {
                if (_pathGeometry_07 != null) return _pathGeometry_07;
                var result = _pathGeometry_07 = _c.CreatePathGeometry(Path_3());
                return result;
            }

            CompositionPathGeometry PathGeometry_08()
            {
                if (_pathGeometry_08 != null) return _pathGeometry_08;
                var result = _pathGeometry_08 = _c.CreatePathGeometry(Path_4());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 0F);
                propertySet.InsertScalar("TStart", 0F);
                BindProperty(_pathGeometry_08, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_08);
                BindProperty(_pathGeometry_08, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_08);
                return result;
            }

            // - Layer aggregator
            // Layer: T2b-Y
            CompositionPathGeometry PathGeometry_09()
            {
                if (_pathGeometry_09 != null) return _pathGeometry_09;
                var result = _pathGeometry_09 = _c.CreatePathGeometry(Path_5());
                return result;
            }

            // - Layer aggregator
            // Layer: T2a-Y
            CompositionPathGeometry PathGeometry_10()
            {
                if (_pathGeometry_10 != null) return _pathGeometry_10;
                var result = _pathGeometry_10 = _c.CreatePathGeometry(Path_6());
                return result;
            }

            // - Layer aggregator
            // Layer: T2b-B
            CompositionPathGeometry PathGeometry_11()
            {
                if (_pathGeometry_11 != null) return _pathGeometry_11;
                var result = _pathGeometry_11 = _c.CreatePathGeometry(Path_5());
                return result;
            }

            // - Layer aggregator
            // Layer: T1b-Y
            CompositionPathGeometry PathGeometry_12()
            {
                if (_pathGeometry_12 != null) return _pathGeometry_12;
                var result = _pathGeometry_12 = _c.CreatePathGeometry(Path_7());
                return result;
            }

            // - Layer aggregator
            // Layer: T1b-B
            CompositionPathGeometry PathGeometry_13()
            {
                if (_pathGeometry_13 != null) return _pathGeometry_13;
                var result = _pathGeometry_13 = _c.CreatePathGeometry(Path_7());
                return result;
            }

            CompositionPathGeometry PathGeometry_14()
            {
                if (_pathGeometry_14 != null) return _pathGeometry_14;
                var result = _pathGeometry_14 = _c.CreatePathGeometry(Path_4());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 0F);
                propertySet.InsertScalar("TStart", 0F);
                BindProperty(_pathGeometry_14, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_14);
                BindProperty(_pathGeometry_14, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_14);
                return result;
            }

            // - Layer aggregator
            // Layer: T2a-B
            CompositionPathGeometry PathGeometry_15()
            {
                if (_pathGeometry_15 != null) return _pathGeometry_15;
                var result = _pathGeometry_15 = _c.CreatePathGeometry(Path_6());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T1a-B
            // Transforms: T1a-B
            CompositionPathGeometry PathGeometry_16()
            {
                if (_pathGeometry_16 != null) return _pathGeometry_16;
                var result = _pathGeometry_16 = _c.CreatePathGeometry(Path_4());
                result.TrimStart = 0.248999998F;
                return result;
            }

            CompositionPathGeometry PathGeometry_17()
            {
                if (_pathGeometry_17 != null) return _pathGeometry_17;
                var result = _pathGeometry_17 = _c.CreatePathGeometry(Path_8());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 0.810000002F);
                propertySet.InsertScalar("TStart", 0.800000012F);
                BindProperty(_pathGeometry_17, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_17);
                BindProperty(_pathGeometry_17, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_17);
                return result;
            }

            CompositionPathGeometry PathGeometry_18()
            {
                if (_pathGeometry_18 != null) return _pathGeometry_18;
                var result = _pathGeometry_18 = _c.CreatePathGeometry(Path_8());
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 0.810000002F);
                propertySet.InsertScalar("TStart", 0.800000012F);
                BindProperty(_pathGeometry_18, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_18);
                BindProperty(_pathGeometry_18, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_18);
                return result;
            }

            CompositionPathGeometry PathGeometry_19()
            {
                if (_pathGeometry_19 != null) return _pathGeometry_19;
                var result = _pathGeometry_19 = _c.CreatePathGeometry(new CompositionPath(Geometry_09()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_19, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_19);
                BindProperty(_pathGeometry_19, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_19);
                return result;
            }

            CompositionPathGeometry PathGeometry_20()
            {
                if (_pathGeometry_20 != null) return _pathGeometry_20;
                var result = _pathGeometry_20 = _c.CreatePathGeometry(new CompositionPath(Geometry_10()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_20, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_20);
                BindProperty(_pathGeometry_20, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_20);
                return result;
            }

            CompositionPathGeometry PathGeometry_21()
            {
                if (_pathGeometry_21 != null) return _pathGeometry_21;
                var result = _pathGeometry_21 = _c.CreatePathGeometry(new CompositionPath(Geometry_11()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_21, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_21);
                BindProperty(_pathGeometry_21, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_21);
                return result;
            }

            CompositionPathGeometry PathGeometry_22()
            {
                if (_pathGeometry_22 != null) return _pathGeometry_22;
                var result = _pathGeometry_22 = _c.CreatePathGeometry(new CompositionPath(Geometry_12()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_22, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_22);
                BindProperty(_pathGeometry_22, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_22);
                return result;
            }

            CompositionPathGeometry PathGeometry_23()
            {
                if (_pathGeometry_23 != null) return _pathGeometry_23;
                var result = _pathGeometry_23 = _c.CreatePathGeometry(new CompositionPath(Geometry_13()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_23, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_23);
                BindProperty(_pathGeometry_23, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_23);
                return result;
            }

            CompositionPathGeometry PathGeometry_24()
            {
                if (_pathGeometry_24 != null) return _pathGeometry_24;
                var result = _pathGeometry_24 = _c.CreatePathGeometry(new CompositionPath(Geometry_14()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_24, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_24);
                BindProperty(_pathGeometry_24, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_24);
                return result;
            }

            CompositionPathGeometry PathGeometry_25()
            {
                if (_pathGeometry_25 != null) return _pathGeometry_25;
                var result = _pathGeometry_25 = _c.CreatePathGeometry(new CompositionPath(Geometry_15()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_25, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_25);
                BindProperty(_pathGeometry_25, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_25);
                return result;
            }

            CompositionPathGeometry PathGeometry_26()
            {
                if (_pathGeometry_26 != null) return _pathGeometry_26;
                var result = _pathGeometry_26 = _c.CreatePathGeometry(new CompositionPath(Geometry_16()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_26, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_26);
                BindProperty(_pathGeometry_26, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_26);
                return result;
            }

            CompositionPathGeometry PathGeometry_27()
            {
                if (_pathGeometry_27 != null) return _pathGeometry_27;
                var result = _pathGeometry_27 = _c.CreatePathGeometry(new CompositionPath(Geometry_17()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_27, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_27);
                BindProperty(_pathGeometry_27, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_27);
                return result;
            }

            CompositionPathGeometry PathGeometry_28()
            {
                if (_pathGeometry_28 != null) return _pathGeometry_28;
                var result = _pathGeometry_28 = _c.CreatePathGeometry(new CompositionPath(Geometry_18()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_28, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_28);
                BindProperty(_pathGeometry_28, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_28);
                return result;
            }

            CompositionPathGeometry PathGeometry_29()
            {
                if (_pathGeometry_29 != null) return _pathGeometry_29;
                var result = _pathGeometry_29 = _c.CreatePathGeometry(new CompositionPath(Geometry_19()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_29, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_29);
                BindProperty(_pathGeometry_29, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_29);
                return result;
            }

            CompositionPathGeometry PathGeometry_30()
            {
                if (_pathGeometry_30 != null) return _pathGeometry_30;
                var result = _pathGeometry_30 = _c.CreatePathGeometry(new CompositionPath(Geometry_20()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_30, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_30);
                BindProperty(_pathGeometry_30, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_30);
                return result;
            }

            CompositionPathGeometry PathGeometry_31()
            {
                if (_pathGeometry_31 != null) return _pathGeometry_31;
                var result = _pathGeometry_31 = _c.CreatePathGeometry(new CompositionPath(Geometry_21()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_31, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_31);
                BindProperty(_pathGeometry_31, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_31);
                return result;
            }

            CompositionPathGeometry PathGeometry_32()
            {
                if (_pathGeometry_32 != null) return _pathGeometry_32;
                var result = _pathGeometry_32 = _c.CreatePathGeometry(new CompositionPath(Geometry_22()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_32, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_32);
                BindProperty(_pathGeometry_32, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_32);
                return result;
            }

            CompositionPathGeometry PathGeometry_33()
            {
                if (_pathGeometry_33 != null) return _pathGeometry_33;
                var result = _pathGeometry_33 = _c.CreatePathGeometry(new CompositionPath(Geometry_23()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_33, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_33);
                BindProperty(_pathGeometry_33, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_33);
                return result;
            }

            CompositionPathGeometry PathGeometry_34()
            {
                if (_pathGeometry_34 != null) return _pathGeometry_34;
                var result = _pathGeometry_34 = _c.CreatePathGeometry(new CompositionPath(Geometry_24()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_34, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_34);
                BindProperty(_pathGeometry_34, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_34);
                return result;
            }

            CompositionPathGeometry PathGeometry_35()
            {
                if (_pathGeometry_35 != null) return _pathGeometry_35;
                var result = _pathGeometry_35 = _c.CreatePathGeometry(new CompositionPath(Geometry_25()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_35, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_35);
                BindProperty(_pathGeometry_35, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_35);
                return result;
            }

            CompositionPathGeometry PathGeometry_36()
            {
                if (_pathGeometry_36 != null) return _pathGeometry_36;
                var result = _pathGeometry_36 = _c.CreatePathGeometry(new CompositionPath(Geometry_26()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_36, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_36);
                BindProperty(_pathGeometry_36, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_36);
                return result;
            }

            CompositionPathGeometry PathGeometry_37()
            {
                if (_pathGeometry_37 != null) return _pathGeometry_37;
                var result = _pathGeometry_37 = _c.CreatePathGeometry(new CompositionPath(Geometry_27()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_37, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_37);
                BindProperty(_pathGeometry_37, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_37);
                return result;
            }

            CompositionPathGeometry PathGeometry_38()
            {
                if (_pathGeometry_38 != null) return _pathGeometry_38;
                var result = _pathGeometry_38 = _c.CreatePathGeometry(new CompositionPath(Geometry_28()));
                var propertySet = result.Properties;
                propertySet.InsertScalar("TEnd", 1F);
                propertySet.InsertScalar("TStart", 0.870000005F);
                BindProperty(_pathGeometry_38, "TrimStart", "Min(my.TStart,my.TEnd)", "my", _pathGeometry_38);
                BindProperty(_pathGeometry_38, "TrimEnd", "Max(my.TStart,my.TEnd)", "my", _pathGeometry_38);
                return result;
            }

            // - Layer aggregator
            // Offset:<187.5, 333.5>
            // Rectangle Path 1.RectangleGeometry
            CompositionRectangleGeometry Rectangle_375x667()
            {
                var result = _c.CreateRectangleGeometry();
                result.Offset = new Vector2(-187.5F, -333.5F);
                result.Size = new Vector2(375F, 667F);
                return result;
            }

            // Layer aggregator
            // Rectangle Path 1
            CompositionSpriteShape SpriteShape_00()
            {
                // Offset:<187.5, 333.5>
                var geometry = Rectangle_375x667();
                var result = CreateSpriteShape(geometry, new Matrix3x2(1F, 0F, 0F, 1F, 187.5F, 333.5F), ColorBrush_AlmostDarkTurquoise_FF00D1C1());;
                return result;
            }

            // - - Layer aggregator
            // - Layer: Dot-Y
            // Transforms: Dot-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_01()
            {
                // Offset:<196, 267>
                var geometry = Ellipse_4p6();
                var result = CreateSpriteShape(geometry, new Matrix3x2(1F, 0F, 0F, 1F, 196F, 267F), ColorBrush_White());;
                return result;
            }

            // - Layer aggregator
            // Layer: E3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_02()
            {
                // Offset:<344.674, 261.877>
                var result = CreateSpriteShape(PathGeometry_00(), new Matrix3x2(1F, 0F, 0F, 1F, 344.674011F, 261.877014F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // - Layer aggregator
            // Layer: E3-B
            // Path 1
            CompositionSpriteShape SpriteShape_03()
            {
                // Offset:<344.739, 261.877>
                var result = CreateSpriteShape(PathGeometry_01(), new Matrix3x2(1F, 0F, 0F, 1F, 344.739014F, 261.877014F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // - Layer aggregator
            // Layer: I-Y
            // Path 1
            CompositionSpriteShape SpriteShape_04()
            {
                // Offset:<304.135, 282.409>
                var result = CreateSpriteShape(PathGeometry_02(), new Matrix3x2(1F, 0F, 0F, 1F, 304.13501F, 282.408997F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // - Layer aggregator
            // Layer: I-B
            // Path 1
            CompositionSpriteShape SpriteShape_05()
            {
                // Offset:<304.135, 282.409>
                var result = CreateSpriteShape(PathGeometry_03(), new Matrix3x2(1F, 0F, 0F, 1F, 304.13501F, 282.408997F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - Layer aggregator
            // Layer: E2-Y
            // Path 1
            CompositionSpriteShape SpriteShape_06()
            {
                // Offset:<331.664, 238.14>
                var result = CreateSpriteShape(PathGeometry_04(), new Matrix3x2(1F, 0F, 0F, 1F, 331.664001F, 238.139999F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // - Layer aggregator
            // Layer: E2-B
            // Path 1
            CompositionSpriteShape SpriteShape_07()
            {
                // Offset:<331.664, 238.14>
                var result = CreateSpriteShape(PathGeometry_05(), new Matrix3x2(1F, 0F, 0F, 1F, 331.664001F, 238.139999F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // - Layer aggregator
            // Layer: E1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_08()
            {
                // Offset:<344.672, 214.842>
                var result = CreateSpriteShape(PathGeometry_06(), new Matrix3x2(1F, 0F, 0F, 1F, 344.671997F, 214.841995F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // - Layer aggregator
            // Layer: E1-B
            // Path 1
            CompositionSpriteShape SpriteShape_09()
            {
                // Offset:<344.672, 214.842>
                var result = CreateSpriteShape(PathGeometry_07(), new Matrix3x2(1F, 0F, 0F, 1F, 344.671997F, 214.841995F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.56200027F;
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-Y
            // Path 1
            CompositionSpriteShape SpriteShape_10()
            {
                // Offset:<227.677, 234.375>
                var result = CreateSpriteShape(PathGeometry_08(), new Matrix3x2(1F, 0F, 0F, 1F, 227.677002F, 234.375F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_11()
            {
                // Offset:<-56.5, 83.5>
                if (_spriteShape_11 != null) return _spriteShape_11;
                var result = _spriteShape_11 = CreateSpriteShape(PathGeometry_09(), new Matrix3x2(1F, 0F, 0F, 1F, -56.5F, 83.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_12()
            {
                // Offset:<221.198, 330.758>
                if (_spriteShape_12 != null) return _spriteShape_12;
                var result = _spriteShape_12 = CreateSpriteShape(PathGeometry_10(), new Matrix3x2(1F, 0F, 0F, 1F, 221.197998F, 330.757996F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_13()
            {
                // Offset:<-56.5, 83.5>
                if (_spriteShape_13 != null) return _spriteShape_13;
                var result = _spriteShape_13 = CreateSpriteShape(PathGeometry_11(), new Matrix3x2(1F, 0F, 0F, 1F, -56.5F, 83.5F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_14()
            {
                // Offset:<186.256, 349.081>
                if (_spriteShape_14 != null) return _spriteShape_14;
                var result = _spriteShape_14 = CreateSpriteShape(PathGeometry_12(), new Matrix3x2(1F, 0F, 0F, 1F, 186.255997F, 349.080994F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeLineJoin = CompositionStrokeLineJoin.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_15()
            {
                // Offset:<186.256, 349.081>
                if (_spriteShape_15 != null) return _spriteShape_15;
                var result = _spriteShape_15 = CreateSpriteShape(PathGeometry_13(), new Matrix3x2(1F, 0F, 0F, 1F, 186.255997F, 349.080994F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeLineJoin = CompositionStrokeLineJoin.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - Layer aggregator
            // Layer: O-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_16()
            {
                // Offset:<196, 267>
                var result = CreateSpriteShape(Ellipse_0_0(), new Matrix3x2(1F, 0F, 0F, 1F, 196F, 267F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeThickness = 8.80000019F;
                return result;
            }

            // - Layer aggregator
            // Layer: O-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_17()
            {
                // Offset:<196, 267>
                var result = CreateSpriteShape(Ellipse_0_1(), new Matrix3x2(1F, 0F, 0F, 1F, 196F, 267F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_18()
            {
                // Offset:<227.677, 234.375>
                var result = CreateSpriteShape(PathGeometry_14(), new Matrix3x2(1F, 0F, 0F, 1F, 227.677002F, 234.375F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_19()
            {
                // Offset:<221.198, 330.758>
                if (_spriteShape_19 != null) return _spriteShape_19;
                var result = _spriteShape_19 = CreateSpriteShape(PathGeometry_15(), new Matrix3x2(1F, 0F, 0F, 1F, 221.197998F, 330.757996F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Square;
                result.StrokeStartCap = CompositionStrokeCap.Square;
                result.StrokeEndCap = CompositionStrokeCap.Square;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-B
            // Path 1
            CompositionSpriteShape SpriteShape_20()
            {
                // Offset:<227.677, 234.375>
                var result = CreateSpriteShape(PathGeometry_16(), new Matrix3x2(1F, 0F, 0F, 1F, 227.677002F, 234.375F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - - Layer aggregator
            // - Layer: Dot-Y
            // Transforms: Dot-Y
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_21()
            {
                // Offset:<196, 267>
                var geometry = Ellipse_4p7();
                var result = CreateSpriteShape(geometry, new Matrix3x2(1F, 0F, 0F, 1F, 196F, 267F), ColorBrush_White());;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_22()
            {
                // Offset:<109.52901, 354.143>
                if (_spriteShape_22 != null) return _spriteShape_22;
                var result = _spriteShape_22 = CreateSpriteShape(PathGeometry_17(), new Matrix3x2(1F, 0F, 0F, 1F, 109.529007F, 354.143005F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 8.39999962F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_23()
            {
                // Offset:<109.52901, 354.143>
                if (_spriteShape_23 != null) return _spriteShape_23;
                var result = _spriteShape_23 = CreateSpriteShape(PathGeometry_18(), new Matrix3x2(1F, 0F, 0F, 1F, 109.529007F, 354.143005F));;
                result.StrokeBrush = ColorBrush_AlmostTeal_FF007A87();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 5F;
                result.StrokeThickness = 9.19400024F;
                return result;
            }

            // - Layer aggregator
            // Layer: Dot1
            // Ellipse Path 1
            CompositionSpriteShape SpriteShape_24()
            {
                // Offset:<196, 267>
                var geometry = Ellipse_4p7();
                var result = CreateSpriteShape(geometry, new Matrix3x2(1F, 0F, 0F, 1F, 196F, 267F), ColorBrush_White());;
                return result;
            }

            // - Layer aggregator
            // Layer: S1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_25()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_19(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // - Layer aggregator
            // Layer: S1-Y
            // Path 1
            CompositionSpriteShape SpriteShape_26()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_20(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // - Layer aggregator
            // Layer: S7
            // Path 1
            CompositionSpriteShape SpriteShape_27()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_21(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // - Layer aggregator
            // Layer: S7
            // Path 1
            CompositionSpriteShape SpriteShape_28()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_22(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_29()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_23(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_30()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_24(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_31()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_25(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y
            // Path 1
            CompositionSpriteShape SpriteShape_32()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_26(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_33()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_27(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_34()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_28(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 2
            // Path 1
            CompositionSpriteShape SpriteShape_35()
            {
                // Offset:<179.5, 333.5>
                var result = CreateSpriteShape(PathGeometry_29(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_36()
            {
                // Offset:<179.5, 333.5>
                if (_spriteShape_36 != null) return _spriteShape_36;
                var result = _spriteShape_36 = CreateSpriteShape(PathGeometry_30(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_37()
            {
                // Offset:<179.5, 333.5>
                if (_spriteShape_37 != null) return _spriteShape_37;
                var result = _spriteShape_37 = CreateSpriteShape(PathGeometry_31(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // Layer aggregator
            // Path 1
            CompositionSpriteShape SpriteShape_38()
            {
                // Offset:<179.5, 333.5>
                if (_spriteShape_38 != null) return _spriteShape_38;
                var result = _spriteShape_38 = CreateSpriteShape(PathGeometry_32(), new Matrix3x2(1F, 0F, 0F, 1F, 179.5F, 333.5F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 1.5F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_39()
            {
                // Offset:<212.662, 248.428>, Rotation:97.90000401019934 degrees
                var result = CreateSpriteShape(PathGeometry_33(), new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_40()
            {
                // Offset:<212.662, 248.428>, Rotation:97.90000401019934 degrees
                var result = CreateSpriteShape(PathGeometry_34(), new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 3
            // Path 1
            CompositionSpriteShape SpriteShape_41()
            {
                // Offset:<212.662, 248.428>, Rotation:97.90000401019934 degrees
                var result = CreateSpriteShape(PathGeometry_35(), new Matrix3x2(-0.137444615F, 0.99050945F, -0.99050945F, -0.137444615F, 212.662003F, 248.427994F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_42()
            {
                // Offset:<207.662, 419.42798>, Rotation:-89.09999525232098 degrees,
                // Scale:<0.99999994, 0.99999994>
                var result = CreateSpriteShape(PathGeometry_36(), new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_43()
            {
                // Offset:<207.662, 419.42798>, Rotation:-89.09999525232098 degrees,
                // Scale:<0.99999994, 0.99999994>
                var result = CreateSpriteShape(PathGeometry_37(), new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 4
            // Path 1
            CompositionSpriteShape SpriteShape_44()
            {
                // Offset:<207.662, 419.42798>, Rotation:-89.09999525232098 degrees,
                // Scale:<0.99999994, 0.99999994>
                var result = CreateSpriteShape(PathGeometry_38(), new Matrix3x2(0.0157073997F, -0.999876618F, 0.999876618F, 0.0157073997F, 207.662003F, 419.427979F));;
                result.StrokeBrush = ColorBrush_White();
                result.StrokeDashCap = CompositionStrokeCap.Round;
                result.StrokeStartCap = CompositionStrokeCap.Round;
                result.StrokeEndCap = CompositionStrokeCap.Round;
                result.StrokeMiterLimit = 2F;
                result.StrokeThickness = 2F;
                return result;
            }

            // The root of the composition.
            ContainerVisual Root()
            {
                if (_root != null) return _root;
                var result = _root = _c.CreateContainerVisual();
                var propertySet = result.Properties;
                propertySet.InsertScalar("Progress", 0F);
                propertySet.InsertScalar("t0", 0F);
                // Layer aggregator
                result.Children.InsertAtTop(ShapeVisual_0());
                return result;
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_0()
            {
                return _cubicBezierEasingFunction_0 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.180000007F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_1()
            {
                return _cubicBezierEasingFunction_1 = _c.CreateCubicBezierEasingFunction(new Vector2(0.819999993F, 0F), new Vector2(0.833000004F, 0.833000004F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_2()
            {
                return _cubicBezierEasingFunction_2 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.833000004F, 0.833000004F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_3()
            {
                return _cubicBezierEasingFunction_3 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.666999996F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_4()
            {
                return _cubicBezierEasingFunction_4 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.119999997F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_5()
            {
                return _cubicBezierEasingFunction_5 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0F), new Vector2(0.119999997F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_6()
            {
                return _cubicBezierEasingFunction_6 = _c.CreateCubicBezierEasingFunction(new Vector2(0.300999999F, 0F), new Vector2(0.666999996F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_7()
            {
                return _cubicBezierEasingFunction_7 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.0599999987F, 1F));
            }

            CubicBezierEasingFunction CubicBezierEasingFunction_8()
            {
                return _cubicBezierEasingFunction_8 = _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.337000012F, 1F));
            }

            ExpressionAnimation RootProgress()
            {
                if (_rootProgress != null) return _rootProgress;
                var result = _rootProgress = _c.CreateExpressionAnimation("_.Progress");
                result.SetReferenceParameter("_", _root);
                return result;
            }

            ScalarKeyFrameAnimation t0ScalarAnimation_0_to_1()
            {
                // Frame 35.26.
                var result = CreateScalarKeyFrameAnimation(0.196966499F, 0F, StepThenHoldEasingFunction());
                result.SetReferenceParameter("_", _root);
                // Frame 44.
                result.InsertKeyFrame(0.245810047F, 1F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.197999999F), new Vector2(0.638000011F, 1F)));
                // Frame 44.
                result.InsertKeyFrame(0.245810062F, 0F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675946F, 1F, _c.CreateCubicBezierEasingFunction(new Vector2(0.523000002F, 0F), new Vector2(0.795000017F, 1F)));
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0_to_1_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0F, HoldThenStepEasingFunction());
                // Frame 74.
                result.InsertKeyFrame(0.413407832F, 1F, CubicBezierEasingFunction_6());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0_to_1_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 1F, CubicBezierEasingFunction_6());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0p81_to_0p734_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.810000002F, StepThenHoldEasingFunction());
                // Frame 16.
                result.InsertKeyFrame(0.0893854722F, 0.810000002F, HoldThenStepEasingFunction());
                // Frame 27.
                result.InsertKeyFrame(0.150837988F, 0.734000027F, CubicBezierEasingFunction_8());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_0p81_to_0p734_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.810000002F, StepThenHoldEasingFunction());
                // Frame 18.
                result.InsertKeyFrame(0.100558661F, 0.810000002F, HoldThenStepEasingFunction());
                // Frame 29.
                result.InsertKeyFrame(0.162011176F, 0.734000027F, CubicBezierEasingFunction_8());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_00()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 29.
                result.InsertKeyFrame(0.162011176F, 1F, HoldThenStepEasingFunction());
                // Frame 33.
                result.InsertKeyFrame(0.184357539F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 36.
                result.InsertKeyFrame(0.201117322F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_01()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 29.
                result.InsertKeyFrame(0.162011176F, 1F, HoldThenStepEasingFunction());
                // Frame 33.
                result.InsertKeyFrame(0.184357539F, 0.690559983F, CubicBezierEasingFunction_2());
                // Frame 36.
                result.InsertKeyFrame(0.201117322F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_02()
            {
                // Frame 0.
                if (_tEndScalarAnimation_1_to_0_02 != null) return _tEndScalarAnimation_1_to_0_02;
                var result = _tEndScalarAnimation_1_to_0_02 = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 65.
                result.InsertKeyFrame(0.363128483F, 1F, HoldThenStepEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_03()
            {
                // Frame 0.
                if (_tEndScalarAnimation_1_to_0_03 != null) return _tEndScalarAnimation_1_to_0_03;
                var result = _tEndScalarAnimation_1_to_0_03 = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 1F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_04()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 1F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.758560002F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_05()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 1F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.704559982F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_06()
            {
                // Frame 0.
                if (_tEndScalarAnimation_1_to_0_06 != null) return _tEndScalarAnimation_1_to_0_06;
                var result = _tEndScalarAnimation_1_to_0_06 = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 1F, HoldThenStepEasingFunction());
                // Frame 100.
                result.InsertKeyFrame(0.558659196F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_07()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 1F, HoldThenStepEasingFunction());
                // Frame 100.
                result.InsertKeyFrame(0.558659196F, 0.758560002F, CubicBezierEasingFunction_2());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_08()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 80.
                result.InsertKeyFrame(0.446927369F, 1F, HoldThenStepEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 87.
                result.InsertKeyFrame(0.486033529F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_09()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, 1F, HoldThenStepEasingFunction());
                // Frame 87.
                result.InsertKeyFrame(0.486033529F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_10()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 1F, HoldThenStepEasingFunction());
                // Frame 90.
                result.InsertKeyFrame(0.502793312F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 94.
                result.InsertKeyFrame(0.525139689F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_11()
            {
                // Frame 0.
                if (_tEndScalarAnimation_1_to_0_11 != null) return _tEndScalarAnimation_1_to_0_11;
                var result = _tEndScalarAnimation_1_to_0_11 = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 1F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_12()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 1F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0.758560002F, CubicBezierEasingFunction_2());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_13()
            {
                // Frame 0.
                if (_tEndScalarAnimation_1_to_0_13 != null) return _tEndScalarAnimation_1_to_0_13;
                var result = _tEndScalarAnimation_1_to_0_13 = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 1F, HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0.663559973F, CubicBezierEasingFunction_2());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TEnd
            ScalarKeyFrameAnimation TEndScalarAnimation_1_to_0_14()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 1F, HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0.758560002F, CubicBezierEasingFunction_2());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E2-Y
            // - ShapeGroup: Group 3 Offset:<331.664, 238.14>
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p43_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, 0.430000007F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E2-B
            // - Transforms: E2-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p43_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 86.
                result.InsertKeyFrame(0.480446935F, 0F, HoldThenStepEasingFunction());
                // Frame 95.
                result.InsertKeyFrame(0.530726254F, 0.430000007F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E3-Y
            // - ShapeGroup: Group 1 Offset:<344.674, 261.877>
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p316_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, 0F, HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, 0.316000015F, CubicBezierEasingFunction_2());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E3-B
            // - Transforms: E3-B Offset:<0.06500244, 0>
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p316_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, 0F, HoldThenStepEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 0.316000015F, CubicBezierEasingFunction_2());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E1-Y
            // - ShapeGroup: Group 2 Offset:<344.672, 214.842>
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p375_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0F, HoldThenStepEasingFunction());
                // Frame 88.
                result.InsertKeyFrame(0.491620123F, 0.375F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: E1-B
            // - Transforms: E1-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p375_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, 0F, HoldThenStepEasingFunction());
                // Frame 93.
                result.InsertKeyFrame(0.519553065F, 0.375F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: I-Y
            // - ShapeGroup: Group 6 Offset:<304.135, 282.409>
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p457_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0F, HoldThenStepEasingFunction());
                // Frame 88.
                result.InsertKeyFrame(0.491620123F, 0.456999987F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: I-B
            // - Transforms: I-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0_to_0p457_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 81.
                result.InsertKeyFrame(0.452513963F, 0F, HoldThenStepEasingFunction());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0.456999987F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2a-Y
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p5_to_1_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.5F, StepThenHoldEasingFunction());
                // Frame 72.
                result.InsertKeyFrame(0.402234644F, 0.5F, HoldThenStepEasingFunction());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 1F, CubicBezierEasingFunction_7());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2a-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p5_to_1_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.5F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0.5F, HoldThenStepEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 1F, CubicBezierEasingFunction_7());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T1b-Y
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p117_to_1_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.116999999F, StepThenHoldEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, 0.116999999F, HoldThenStepEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 1F, CubicBezierEasingFunction_2());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T1b-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p117_to_1_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.116999999F, StepThenHoldEasingFunction());
                // Frame 81.
                result.InsertKeyFrame(0.452513963F, 0.116999999F, HoldThenStepEasingFunction());
                // Frame 88.
                result.InsertKeyFrame(0.491620123F, 1F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.209999993F, 1F)));
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: T1a-B
            // - Transforms: T1a-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p249_to_0p891()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.248999998F, StepThenHoldEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, 0.248999998F, HoldThenStepEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, 0.890999973F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.672999978F, 1F)));
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2b-Y
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p411_to_0p665_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.411000013F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 0.411000013F, HoldThenStepEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 0.665000021F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2b-B
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_0p411_to_0p665_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.411000013F, StepThenHoldEasingFunction());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0.411000013F, HoldThenStepEasingFunction());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0.665000021F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: O-Y
            // - ShapeGroup: Ellipse 1 Offset:<196, 267>
            // Ellipse Path 1.EllipseGeometry
            // TrimEnd
            ScalarKeyFrameAnimation TrimEndScalarAnimation_1_to_0p88()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 1F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 1F, HoldThenStepEasingFunction());
                // Frame 63.
                result.InsertKeyFrame(0.351955295F, 0.879999995F, CubicBezierEasingFunction_2());
                return result;
            }

            // - - - Layer aggregator
            // - - Layer: O-Y
            // - ShapeGroup: Ellipse 1 Offset:<196, 267>
            // Ellipse Path 1.EllipseGeometry
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0_to_0p399()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0F, HoldThenStepEasingFunction());
                // Frame 63.
                result.InsertKeyFrame(0.351955295F, 0.300000012F, CubicBezierEasingFunction_2());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0.398999989F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 1F), new Vector2(0.432000011F, 1F)));
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2a-Y
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p5_to_0_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.5F, StepThenHoldEasingFunction());
                // Frame 72.
                result.InsertKeyFrame(0.402234644F, 0.5F, HoldThenStepEasingFunction());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_7());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2a-B
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p5_to_0_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.5F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0.5F, HoldThenStepEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 0F, CubicBezierEasingFunction_7());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2b-Y
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p29_to_0_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.289999992F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 0.289999992F, HoldThenStepEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 0F, CubicBezierEasingFunction_4());
                return result;
            }

            // - - Layer aggregator
            // - Layer: T2b-B
            // TrimStart
            ScalarKeyFrameAnimation TrimStartScalarAnimation_0p29_to_0_1()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.289999992F, StepThenHoldEasingFunction());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0.289999992F, HoldThenStepEasingFunction());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0F, CubicBezierEasingFunction_4());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0_to_0p249()
            {
                // Frame 0.
                if (_tStartScalarAnimation_0_to_0p249 != null) return _tStartScalarAnimation_0_to_0p249;
                var result = _tStartScalarAnimation_0_to_0p249 = CreateScalarKeyFrameAnimation(0F, 0F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0F, HoldThenStepEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, 0.248999998F, _c.CreateCubicBezierEasingFunction(new Vector2(0.300999999F, 0F), new Vector2(0.833000004F, 1F)));
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p8_to_0()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.800000012F, StepThenHoldEasingFunction());
                // Frame 16.
                result.InsertKeyFrame(0.0893854722F, 0.800000012F, HoldThenStepEasingFunction());
                // Frame 20.
                result.InsertKeyFrame(0.111731842F, 0.5F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.703000009F, 0.856999993F)));
                // Frame 28.
                result.InsertKeyFrame(0.156424582F, 0F, _c.CreateCubicBezierEasingFunction(new Vector2(0.333000004F, 0.202000007F), new Vector2(0.938000023F, 1F)));
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p8_to_0p3()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.800000012F, StepThenHoldEasingFunction());
                // Frame 18.
                result.InsertKeyFrame(0.100558661F, 0.800000012F, HoldThenStepEasingFunction());
                // Frame 23.
                result.InsertKeyFrame(0.128491625F, 0.5F, _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.703000009F, 0.82099998F)));
                // Frame 55.
                result.InsertKeyFrame(0.30726257F, 0.300000012F, _c.CreateCubicBezierEasingFunction(new Vector2(0.0370000005F, 0.167999998F), new Vector2(0.263000011F, 1F)));
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_00()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 29.
                result.InsertKeyFrame(0.162011176F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 33.
                result.InsertKeyFrame(0.184357539F, 0.375330001F, CubicBezierEasingFunction_2());
                // Frame 36.
                result.InsertKeyFrame(0.201117322F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_01()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 29.
                result.InsertKeyFrame(0.162011176F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 33.
                result.InsertKeyFrame(0.184357539F, 0.253329992F, CubicBezierEasingFunction_2());
                // Frame 36.
                result.InsertKeyFrame(0.201117322F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_02()
            {
                // Frame 0.
                if (_tStartScalarAnimation_0p87_to_0_02 != null) return _tStartScalarAnimation_0p87_to_0_02;
                var result = _tStartScalarAnimation_0p87_to_0_02 = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 65.
                result.InsertKeyFrame(0.363128483F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, 0.212329999F, CubicBezierEasingFunction_2());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_03()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.421330005F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_04()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.438329995F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_05()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.506330013F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_06()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 57.
                result.InsertKeyFrame(0.318435758F, 0.439330012F, CubicBezierEasingFunction_2());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_07()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 100.
                result.InsertKeyFrame(0.558659196F, 0.421330005F, CubicBezierEasingFunction_2());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_08()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 100.
                result.InsertKeyFrame(0.558659196F, 0.438329995F, CubicBezierEasingFunction_2());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_09()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 100.
                result.InsertKeyFrame(0.558659196F, 0.506330013F, CubicBezierEasingFunction_2());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_10()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 80.
                result.InsertKeyFrame(0.446927369F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0.212329999F, CubicBezierEasingFunction_2());
                // Frame 87.
                result.InsertKeyFrame(0.486033529F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_11()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 87.
                result.InsertKeyFrame(0.486033529F, 0.212329999F, CubicBezierEasingFunction_2());
                // Frame 91.
                result.InsertKeyFrame(0.508379877F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_12()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 90.
                result.InsertKeyFrame(0.502793312F, 0.212329999F, CubicBezierEasingFunction_2());
                // Frame 94.
                result.InsertKeyFrame(0.525139689F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_13()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0.421330005F, CubicBezierEasingFunction_2());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_14()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0.438329995F, CubicBezierEasingFunction_2());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_15()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, 0.506330013F, CubicBezierEasingFunction_2());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_16()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0.421330005F, CubicBezierEasingFunction_2());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_17()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0.438329995F, CubicBezierEasingFunction_2());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // TStart
            ScalarKeyFrameAnimation TStartScalarAnimation_0p87_to_0_18()
            {
                // Frame 0.
                var result = CreateScalarKeyFrameAnimation(0F, 0.870000005F, StepThenHoldEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, 0.870000005F, HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, 0.506330013F, CubicBezierEasingFunction_2());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, 0F, CubicBezierEasingFunction_2());
                return result;
            }

            // Layer aggregator
            ShapeVisual ShapeVisual_0()
            {
                var result = _c.CreateShapeVisual();
                result.Size = new Vector2(375F, 667F);
                var shapes = result.Shapes;
                // Offset:<187.5, 333.5>
                shapes.Add(SpriteShape_00());
                // Layer: Dot-Y
                shapes.Add(ContainerShape_00());
                // Layer: E3-Y
                shapes.Add(ContainerShape_02());
                // Layer: E3-B
                shapes.Add(ContainerShape_03());
                // Layer: I-Y
                shapes.Add(ContainerShape_04());
                // Layer: I-B
                shapes.Add(ContainerShape_05());
                // Layer: E2-Y
                shapes.Add(ContainerShape_06());
                // Layer: E2-B
                shapes.Add(ContainerShape_07());
                // Layer: E1-Y
                shapes.Add(ContainerShape_08());
                // Layer: E1-B
                shapes.Add(ContainerShape_09());
                // Layer: T1a-Y
                shapes.Add(ContainerShape_10());
                // Layer: T2b-Y
                shapes.Add(SpriteShape_11());
                // Layer: T2a-Y
                shapes.Add(SpriteShape_12());
                // Layer: T2b-B
                shapes.Add(SpriteShape_13());
                // Layer: T1b-Y
                shapes.Add(SpriteShape_14());
                // Layer: T1b-B
                shapes.Add(SpriteShape_15());
                // Layer: O-Y
                shapes.Add(ContainerShape_11());
                // Layer: T1a-Y 2
                shapes.Add(ContainerShape_12());
                // Layer: T2a-B
                shapes.Add(SpriteShape_19());
                // Layer: T1a-B
                shapes.Add(ContainerShape_13());
                // Layer: Dot-Y
                shapes.Add(ContainerShape_14());
                // Layer: L-Y
                shapes.Add(SpriteShape_22());
                // Layer: L-B
                shapes.Add(SpriteShape_23());
                // Layer: Dot1
                shapes.Add(ContainerShape_16());
                // Layer: S1-Y
                shapes.Add(ContainerShape_17());
                // Layer: S7
                shapes.Add(ContainerShape_18());
                // Layer: S3-Y
                shapes.Add(ContainerShape_19());
                // Layer: S3-Y 2
                shapes.Add(ContainerShape_20());
                // Layer: S11
                shapes.Add(SpriteShape_36());
                // Layer: S12
                shapes.Add(SpriteShape_37());
                // Layer: S13
                shapes.Add(SpriteShape_38());
                // Layer: S3-Y 3
                shapes.Add(ContainerShape_21());
                // Layer: S3-Y 4
                shapes.Add(ContainerShape_22());
                return result;
            }

            StepEasingFunction HoldThenStepEasingFunction()
            {
                if (_holdThenStepEasingFunction != null) return _holdThenStepEasingFunction;
                var result = _holdThenStepEasingFunction = _c.CreateStepEasingFunction();
                result.IsFinalStepSingleFrame = true;
                return result;
            }

            StepEasingFunction StepThenHoldEasingFunction()
            {
                if (_stepThenHoldEasingFunction != null) return _stepThenHoldEasingFunction;
                var result = _stepThenHoldEasingFunction = _c.CreateStepEasingFunction();
                result.IsInitialStepSingleFrame = true;
                return result;
            }

            // - - Layer aggregator
            // - Layer: Dot-Y
            // Transforms: Dot-Y
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_00()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(-153.528F, -206.753998F), StepThenHoldEasingFunction());
                // Frame 96.
                result.InsertKeyFrame(0.536312878F, new Vector2(-153.528F, -206.753998F), HoldThenStepEasingFunction());
                // Frame 108.
                result.InsertKeyFrame(0.603351951F, new Vector2(-134.278F, -206.753998F), _c.CreateCubicBezierEasingFunction(new Vector2(0F, 0F), new Vector2(0F, 0.811999977F)));
                // Frame 115.
                result.InsertKeyFrame(0.642458081F, new Vector2(-133.028F, -206.753998F), _c.CreateCubicBezierEasingFunction(new Vector2(0.389999986F, 0.707000017F), new Vector2(0.708000004F, 1F)));
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_01()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(104.781998F, -2.52699995F), StepThenHoldEasingFunction());
                // Frame 96.
                result.InsertKeyFrame(0.536312878F, new Vector2(104.781998F, -2.52699995F), HoldThenStepEasingFunction());
                // Frame 99.
                result.InsertKeyFrame(0.553072631F, new Vector2(104.781998F, -4.52699995F), CubicBezierEasingFunction_0());
                // Frame 102.
                result.InsertKeyFrame(0.569832385F, new Vector2(104.781998F, -2.52699995F), CubicBezierEasingFunction_1());
                // Frame 105.
                result.InsertKeyFrame(0.586592197F, new Vector2(104.781998F, -3.09100008F), CubicBezierEasingFunction_0());
                // Frame 108.
                result.InsertKeyFrame(0.603351951F, new Vector2(104.781998F, -2.52699995F), CubicBezierEasingFunction_1());
                return result;
            }

            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_02()
            {
                // Frame 0.
                if (_offsetVector2Animation_02 != null) return _offsetVector2Animation_02;
                var result = _offsetVector2Animation_02 = CreateVector2KeyFrameAnimation(0F, new Vector2(-225.957001F, -204.322006F), StepThenHoldEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, new Vector2(-225.957001F, -204.322006F), HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(-207.957001F, -204.322006F), CubicBezierEasingFunction_3());
                // Frame 96.
                result.InsertKeyFrame(0.536312878F, new Vector2(-210.957001F, -204.322006F), _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0F), new Vector2(0.666999996F, 1F)));
                return result;
            }

            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_03()
            {
                // Frame 0.
                if (_offsetVector2Animation_03 != null) return _offsetVector2Animation_03;
                var result = _offsetVector2Animation_03 = CreateVector2KeyFrameAnimation(0F, new Vector2(-210.207993F, -219.320999F), StepThenHoldEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, new Vector2(-210.207993F, -219.320999F), HoldThenStepEasingFunction());
                // Frame 88.
                result.InsertKeyFrame(0.491620123F, new Vector2(-211.175995F, -199.352997F), CubicBezierEasingFunction_4());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(-210.957993F, -204.320999F), CubicBezierEasingFunction_5());
                return result;
            }

            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_04()
            {
                // Frame 0.
                if (_offsetVector2Animation_04 != null) return _offsetVector2Animation_04;
                var result = _offsetVector2Animation_04 = CreateVector2KeyFrameAnimation(0F, new Vector2(-222.957993F, -204.322006F), StepThenHoldEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, new Vector2(-222.957993F, -204.322006F), HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(-210.957993F, -204.322006F), CubicBezierEasingFunction_4());
                return result;
            }

            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_05()
            {
                // Frame 0.
                if (_offsetVector2Animation_05 != null) return _offsetVector2Animation_05;
                var result = _offsetVector2Animation_05 = CreateVector2KeyFrameAnimation(0F, new Vector2(-230.957001F, -205.695999F), StepThenHoldEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, new Vector2(-230.957001F, -205.695999F), HoldThenStepEasingFunction());
                // Frame 88.
                result.InsertKeyFrame(0.491620123F, new Vector2(-206.957001F, -205.695999F), CubicBezierEasingFunction_4());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(-210.957001F, -205.695999F), CubicBezierEasingFunction_5());
                return result;
            }

            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_06()
            {
                // Frame 0.
                if (_offsetVector2Animation_06 != null) return _offsetVector2Animation_06;
                var result = _offsetVector2Animation_06 = CreateVector2KeyFrameAnimation(0F, new Vector2(-210.957001F, -201.322006F), StepThenHoldEasingFunction());
                // Frame 56.
                result.InsertKeyFrame(0.312849164F, new Vector2(-210.957001F, -201.322006F), HoldThenStepEasingFunction());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, new Vector2(-210.957001F, -204.322006F), CubicBezierEasingFunction_3());
                return result;
            }

            // - Layer aggregator
            // Layer: O-Y
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_07()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(-259.583008F, -193.447006F), StepThenHoldEasingFunction());
                result.SetReferenceParameter("_", _root);
                // Frame 31.
                result.InsertKeyFrame(0.17318435F, new Vector2(-259.583008F, -193.447006F), HoldThenStepEasingFunction());
                // Frame 35.26.
                result.InsertKeyFrame(0.196966484F, new Vector2(-250.582993F, -258.946991F), CubicBezierEasingFunction_2());
                // Frame 44.
                result.InsertExpressionKeyFrame(0.245810047F, "Pow(1-_.t0,3)*Vector2(-250.583,-258.947)+(3*Square(1-_.t0)*_.t0*Vector2(-250.583,-258.947))+(3*(1-_.t0)*Square(_.t0)*Vector2(-249.6143,-337.5837))+(Pow(_.t0,3)*Vector2(-230.458,-339.322))", StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertExpressionKeyFrame(0.301675946F, "Pow(1-_.t0,3)*Vector2(-230.458,-339.322)+(3*Square(1-_.t0)*_.t0*Vector2(-214.2505,-340.7927))+(3*(1-_.t0)*Square(_.t0)*Vector2(-210.958,-164.322))+(Pow(_.t0,3)*Vector2(-210.958,-164.322))", StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(-210.957993F, -164.322006F), StepThenHoldEasingFunction());
                // Frame 63.
                result.InsertKeyFrame(0.351955295F, new Vector2(-210.957993F, -207.322006F), _c.CreateCubicBezierEasingFunction(new Vector2(0.180000007F, 0F), new Vector2(0.34799999F, 1F)));
                // Frame 73.
                result.InsertKeyFrame(0.407821238F, new Vector2(-210.957993F, -204.322006F), _c.CreateCubicBezierEasingFunction(new Vector2(0.693000019F, 0F), new Vector2(0.270000011F, 1F)));
                return result;
            }

            // - - Layer aggregator
            // - Layer: Dot-Y
            // Transforms: Dot-Y
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_08()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(-156.916F, -206.503998F), StepThenHoldEasingFunction());
                // Frame 28.
                result.InsertKeyFrame(0.156424582F, new Vector2(-156.916F, -206.503998F), HoldThenStepEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(-117.416F, -206.503998F), CubicBezierEasingFunction_2());
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_09()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(-93.6669998F, -51.8180008F), StepThenHoldEasingFunction());
                // Frame 28.
                result.InsertKeyFrame(0.156424582F, new Vector2(-93.6669998F, -51.8180008F), HoldThenStepEasingFunction());
                // Frame 40.
                result.InsertKeyFrame(0.223463684F, new Vector2(-93.6669998F, -132.817993F), _c.CreateCubicBezierEasingFunction(new Vector2(0.166999996F, 0.166999996F), new Vector2(0.25999999F, 1F)));
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(-93.6669998F, 42.0569992F), _c.CreateCubicBezierEasingFunction(new Vector2(0.74000001F, 0F), new Vector2(0.833000004F, 0.833000004F)));
                return result;
            }

            // - Layer aggregator
            // Layer: Dot1
            // Offset
            Vector2KeyFrameAnimation OffsetVector2Animation_10()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(98.9800034F, -157.509995F), HoldThenStepEasingFunction());
                // Frame 18.69.
                result.InsertKeyFrame(0.104395606F, new Vector2(-161.020004F, -157.509995F), _c.CreateCubicBezierEasingFunction(new Vector2(0.823000014F, 0F), new Vector2(0.833000004F, 0.833000004F)));
                return result;
            }

            // Radius
            Vector2KeyFrameAnimation RadiusVector2Animation()
            {
                // Frame 0.
                if (_radiusVector2Animation != null) return _radiusVector2Animation;
                var result = _radiusVector2Animation = CreateVector2KeyFrameAnimation(0F, new Vector2(1.5F, 1.5F), StepThenHoldEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(1.5F, 1.5F), HoldThenStepEasingFunction());
                // Frame 61.
                result.InsertKeyFrame(0.340782136F, new Vector2(22.2999992F, 22.2999992F), _c.CreateCubicBezierEasingFunction(new Vector2(0.333000004F, 0F), new Vector2(0.666999996F, 1F)));
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_00()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 96.
                result.InsertKeyFrame(0.536312878F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E3-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_01()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 102.
                result.InsertKeyFrame(0.569832385F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E3-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_02()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: I-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_03()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 78.
                result.InsertKeyFrame(0.43575418F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 93.
                result.InsertKeyFrame(0.519553065F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            Vector2KeyFrameAnimation ShapeVisibilityAnimation_04()
            {
                // Frame 0.
                if (_shapeVisibilityAnimation_04 != null) return _shapeVisibilityAnimation_04;
                var result = _shapeVisibilityAnimation_04 = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 81.
                result.InsertKeyFrame(0.452513963F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E2-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_05()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 96.
                result.InsertKeyFrame(0.536312878F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E2-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_06()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 86.
                result.InsertKeyFrame(0.480446935F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E1-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_07()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 79.
                result.InsertKeyFrame(0.441340774F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 94.
                result.InsertKeyFrame(0.525139689F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: E1-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_08()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_09()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 59.
                result.InsertKeyFrame(0.329608947F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 156.
                result.InsertKeyFrame(0.87150836F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T2b-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_10()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 92.
                result.InsertKeyFrame(0.513966501F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T2a-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_11()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 72.
                result.InsertKeyFrame(0.402234644F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 89.
                result.InsertKeyFrame(0.497206718F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T2b-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_12()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 82.
                result.InsertKeyFrame(0.458100557F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T1b-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_13()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 161.
                result.InsertKeyFrame(0.899441361F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: O-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_14()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-Y 2
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_15()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 59.
                result.InsertKeyFrame(0.329608947F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T2a-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_16()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: T1a-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_17()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 70.
                result.InsertKeyFrame(0.391061455F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: Dot-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_18()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 28.
                result.InsertKeyFrame(0.156424582F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: L-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_19()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 16.
                result.InsertKeyFrame(0.0893854722F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: L-B
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_20()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 18.
                result.InsertKeyFrame(0.100558661F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: Dot1
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_21()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 17.
                result.InsertKeyFrame(0.0949720666F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S1-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_22()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 30.
                result.InsertKeyFrame(0.167597771F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 37.
                result.InsertKeyFrame(0.206703916F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S7
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_23()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 65.
                result.InsertKeyFrame(0.363128483F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_24()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 54.
                result.InsertKeyFrame(0.301675975F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 64.
                result.InsertKeyFrame(0.357541889F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 2
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_25()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 97.
                result.InsertKeyFrame(0.541899443F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 107.
                result.InsertKeyFrame(0.597765386F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S11
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_26()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 80.
                result.InsertKeyFrame(0.446927369F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 90.
                result.InsertKeyFrame(0.502793312F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S12
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_27()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 94.
                result.InsertKeyFrame(0.525139689F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S13
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_28()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 85.
                result.InsertKeyFrame(0.47486034F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 95.
                result.InsertKeyFrame(0.530726254F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 3
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_29()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 75.
                result.InsertKeyFrame(0.418994427F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 83.
                result.InsertKeyFrame(0.463687152F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            // - Layer aggregator
            // Layer: S3-Y 4
            Vector2KeyFrameAnimation ShapeVisibilityAnimation_30()
            {
                // Frame 0.
                var result = CreateVector2KeyFrameAnimation(0F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                // Frame 76.
                result.InsertKeyFrame(0.424580991F, new Vector2(1F, 1F), HoldThenStepEasingFunction());
                // Frame 84.
                result.InsertKeyFrame(0.469273746F, new Vector2(0F, 0F), HoldThenStepEasingFunction());
                return result;
            }

            internal LottieLogo_AnimatedVisual(
                Compositor compositor
                )
            {
                _c = compositor;
                _reusableExpressionAnimation = compositor.CreateExpressionAnimation();
                Root();
            }

            public Visual RootVisual => _root;
            public TimeSpan Duration => TimeSpan.FromTicks(c_durationTicks);
            public Vector2 Size => new Vector2(375F, 667F);
            void IDisposable.Dispose() => _root?.Dispose();

            internal static bool IsRuntimeCompatible()
            {
                return Windows.Foundation.Metadata.ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 12);
            }
        }
    }
}
