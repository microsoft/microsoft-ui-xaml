// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft {
namespace UI {
namespace Composition {
    class AmbientLight;
    class AnimationController;
    class AnimationPropertyInfo;
    class BooleanKeyFrameAnimation;
    class BounceScalarNaturalMotionAnimation;
    class BounceVector2NaturalMotionAnimation;
    class BounceVector3NaturalMotionAnimation;
    class ColorKeyFrameAnimation;
    class CompositionAnimation;
    class CompositionAnimationGroup;
    class CompositionBackdropBrush;
    class CompositionBatchCompletedEventArgs;
    class CompositionBrush;
    class CompositionCapabilities;
    class CompositionClip;
    class CompositionColorBrush;
    class CompositionColorGradientStop;
    class CompositionColorGradientStopCollection;
    class CompositionCommitBatch;
    class CompositionContainerShape;
    class CompositionDrawingSurface;
    class CompositionEasingFunction;
    class CompositionEffectBrush;
    class CompositionEffectFactory;
    class CompositionEffectSourceParameter;
    class CompositionEllipseGeometry;
    class CompositionGeometricClip;
    class CompositionGeometry;
    class CompositionGradientBrush;
    class CompositionGraphicsDevice;
    class CompositionLight;
    class CompositionLinearGradientBrush;
    class CompositionLineGeometry;
    class CompositionMaskBrush;
    class CompositionMipmapSurface;
    class CompositionNineGridBrush;
    class CompositionObject;
    class CompositionPath;
    class CompositionPathGeometry;
    class CompositionProjectedShadow;
    class CompositionProjectedShadowCaster;
    class CompositionProjectedShadowCasterCollection;
    class CompositionProjectedShadowReceiver;
    class CompositionProjectedShadowReceiverUnorderedCollection;
    class CompositionPropertySet;
    class CompositionRadialGradientBrush;
    class CompositionRectangleGeometry;
    class CompositionRoundedRectangleGeometry;
    class CompositionScopedBatch;
    class CompositionShadow;
    class CompositionShape;
    class CompositionShapeCollection;
    class CompositionSpriteShape;
    class CompositionStrokeDashArray;
    class CompositionSurfaceBrush;
    class CompositionTarget;
    class CompositionTransform;
    class CompositionViewBox;
    class CompositionVirtualDrawingSurface;
    class CompositionVisualSurface;
    class Compositor;
    class ContainerVisual;
    class CubicBezierEasingFunction;
    class DistantLight;
    class DropShadow;
    class ExpressionAnimation;
    class ImplicitAnimationCollection;
    class InitialValueExpressionCollection;
    class InsetClip;
    class KeyFrameAnimation;
    class LayerVisual;
    class LinearEasingFunction;
    class NaturalMotionAnimation;
    class PathKeyFrameAnimation;
    class PointLight;
    class QuaternionKeyFrameAnimation;
    class RedirectVisual;
    class RenderingDeviceReplacedEventArgs;
    class ScalarKeyFrameAnimation;
    class ScalarNaturalMotionAnimation;
    class ShapeVisual;
    class SpotLight;
    class SpringScalarNaturalMotionAnimation;
    class SpringVector2NaturalMotionAnimation;
    class SpringVector3NaturalMotionAnimation;
    class SpriteVisual;
    class StepEasingFunction;
    class Vector2KeyFrameAnimation;
    class Vector2NaturalMotionAnimation;
    class Vector3KeyFrameAnimation;
    class Vector3NaturalMotionAnimation;
    class Vector4KeyFrameAnimation;
    class Visual;
    class VisualCollection;
    class VisualUnorderedCollection;
    interface IAmbientLight;
    interface IAmbientLight2;
    interface IAnimationController;
    interface IAnimationControllerStatics;
    interface IAnimationObject;
    interface IAnimationPropertyInfo;
    interface IBooleanKeyFrameAnimation;
    interface IBounceScalarNaturalMotionAnimation;
    interface IBounceVector2NaturalMotionAnimation;
    interface IBounceVector3NaturalMotionAnimation;
    interface IColorKeyFrameAnimation;
    interface ICompositionAnimation;
    interface ICompositionAnimation2;
    interface ICompositionAnimation3;
    interface ICompositionAnimation4;
    interface ICompositionAnimationBase;
    interface ICompositionAnimationFactory;
    interface ICompositionAnimationGroup;
    interface ICompositionBackdropBrush;
    interface ICompositionBatchCompletedEventArgs;
    interface ICompositionBrush;
    interface ICompositionBrushFactory;
    interface ICompositionCapabilities;
    interface ICompositionCapabilitiesStatics;
    interface ICompositionClip;
    interface ICompositionClip2;
    interface ICompositionClipFactory;
    interface ICompositionColorBrush;
    interface ICompositionColorGradientStop;
    interface ICompositionColorGradientStopCollection;
    interface ICompositionCommitBatch;
    interface ICompositionContainerShape;
    interface ICompositionDrawingSurface;
    interface ICompositionDrawingSurface2;
    interface ICompositionDrawingSurfaceFactory;
    interface ICompositionEasingFunction;
    interface ICompositionEasingFunctionFactory;
    interface ICompositionEffectBrush;
    interface ICompositionEffectFactory;
    interface ICompositionEffectSourceParameter;
    interface ICompositionEffectSourceParameterFactory;
    interface ICompositionEllipseGeometry;
    interface ICompositionGeometricClip;
    interface ICompositionGeometry;
    interface ICompositionGeometryFactory;
    interface ICompositionGradientBrush;
    interface ICompositionGradientBrush2;
    interface ICompositionGradientBrushFactory;
    interface ICompositionGraphicsDevice;
    interface ICompositionGraphicsDevice2;
    interface ICompositionGraphicsDevice3;
    interface ICompositionLight;
    interface ICompositionLight2;
    interface ICompositionLight3;
    interface ICompositionLightFactory;
    interface ICompositionLinearGradientBrush;
    interface ICompositionLineGeometry;
    interface ICompositionMaskBrush;
    interface ICompositionMipmapSurface;
    interface ICompositionNineGridBrush;
    interface ICompositionObject;
    interface ICompositionObject2;
    interface ICompositionObject3;
    interface ICompositionObject4;
    interface ICompositionObjectFactory;
    interface ICompositionObjectStatics;
    interface ICompositionPath;
    interface ICompositionPathFactory;
    interface ICompositionPathGeometry;
    interface ICompositionProjectedShadow;
    interface ICompositionProjectedShadowCaster;
    interface ICompositionProjectedShadowCasterCollection;
    interface ICompositionProjectedShadowCasterCollectionStatics;
    interface ICompositionProjectedShadowReceiver;
    interface ICompositionProjectedShadowReceiverUnorderedCollection;
    interface ICompositionPropertySet;
    interface ICompositionPropertySet2;
    interface ICompositionRadialGradientBrush;
    interface ICompositionRectangleGeometry;
    interface ICompositionRoundedRectangleGeometry;
    interface ICompositionScopedBatch;
    interface ICompositionShadow;
    interface ICompositionShadowFactory;
    interface ICompositionShape;
    interface ICompositionShapeFactory;
    interface ICompositionSpriteShape;
    interface ICompositionSurface;
    interface ICompositionSurfaceBrush;
    interface ICompositionSurfaceBrush2;
    interface ICompositionSurfaceBrush3;
    interface ICompositionTarget;
    interface ICompositionTargetFactory;
    interface ICompositionTransform;
    interface ICompositionTransformFactory;
    interface ICompositionViewBox;
    interface ICompositionVirtualDrawingSurface;
    interface ICompositionVirtualDrawingSurfaceFactory;
    interface ICompositionVisualSurface;
    interface ICompositor;
    interface ICompositor2;
    interface ICompositor3;
    interface ICompositor4;
    interface ICompositor5;
    interface ICompositor6;
    interface ICompositorStatics;
    interface ICompositorWithProjectedShadow;
    interface ICompositorWithRadialGradient;
    interface ICompositorWithVisualSurface;
    interface IContainerVisual;
    interface IContainerVisualFactory;
    interface ICubicBezierEasingFunction;
    interface IDistantLight;
    interface IDistantLight2;
    interface IDropShadow;
    interface IDropShadow2;
    interface IExpressionAnimation;
    interface IImplicitAnimationCollection;
    interface IInsetClip;
    interface IKeyFrameAnimation;
    interface IKeyFrameAnimation2;
    interface IKeyFrameAnimation3;
    interface IKeyFrameAnimationFactory;
    interface ILayerVisual;
    interface ILayerVisual2;
    interface ILinearEasingFunction;
    interface INaturalMotionAnimation;
    interface INaturalMotionAnimationFactory;
    interface IPathKeyFrameAnimation;
    interface IPointLight;
    interface IPointLight2;
    interface IPointLight3;
    interface IQuaternionKeyFrameAnimation;
    interface IRedirectVisual;
    interface IRenderingDeviceReplacedEventArgs;
    interface IScalarKeyFrameAnimation;
    interface IScalarNaturalMotionAnimation;
    interface IScalarNaturalMotionAnimationFactory;
    interface IShapeVisual;
    interface ISpotLight;
    interface ISpotLight2;
    interface ISpotLight3;
    interface ISpringScalarNaturalMotionAnimation;
    interface ISpringVector2NaturalMotionAnimation;
    interface ISpringVector3NaturalMotionAnimation;
    interface ISpriteVisual;
    interface ISpriteVisual2;
    interface IStepEasingFunction;
    interface IVector2KeyFrameAnimation;
    interface IVector2NaturalMotionAnimation;
    interface IVector2NaturalMotionAnimationFactory;
    interface IVector3KeyFrameAnimation;
    interface IVector3NaturalMotionAnimation;
    interface IVector3NaturalMotionAnimationFactory;
    interface IVector4KeyFrameAnimation;
    interface IVisual;
    interface IVisual2;
    interface IVisualCollection;
    interface IVisualElement;
    interface IVisualFactory;
    interface IVisualUnorderedCollection;

    namespace Desktop {
        interface IDesktopWindowContentBridgeInterop;
    }

    // Possibly private?

    enum AnimationStopBehavior : int;
    enum CompositionGetValueStatus : int;
    enum CompositionStretch : int;
    enum CompositionStrokeCap : int;
    interface ICompositionAnimatorPartner;
    interface ICompositionObjectPartner;
    interface ICompositionPropertySet;
    interface IHoverPointerSourcePartner;
    interface IInteropCompositorPartner;
    interface IInteropCompositorPartnerCallback;

    namespace Experimental {
        class ExpCompositionContent;
        class ExpCompositionContentAutomationProviderRequestedEventArgs;
        class ExpCompositionContentEventArgs;
        interface IExpCompositionContentEventArgs;
    }

} // Composition
} // UI
} // Microsoft
XAML_ABI_NAMESPACE_END

