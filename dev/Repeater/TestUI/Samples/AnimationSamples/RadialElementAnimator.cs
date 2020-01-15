// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;

using AnimationContext = Microsoft.UI.Xaml.Controls.AnimationContext;
using ElementAnimator = Microsoft.UI.Xaml.Controls.ElementAnimator;

namespace MUXControlsTestApp.Samples
{
    public class RadialElementAnimator : ElementAnimator
    {
        private const double DefaultAnimationDurationInMs = 600.0;

        private static double _animationSlowdownFactor = 1.0;
        public static double AnimationSlowdownFactor
        {
            get
            {
                return _animationSlowdownFactor;
            }
            set
            {
                _animationSlowdownFactor = value;
            }
        }

        protected override bool HasShowAnimationCore(UIElement element, AnimationContext context)
        {
            return true;
        }
        protected override bool HasHideAnimationCore(UIElement element, AnimationContext context)
        {
            return true;
        }

        protected override bool HasBoundsChangeAnimationCore(
            UIElement element,
            AnimationContext context,
            Rect oldBounds,
            Rect newBounds)
        {
            return true;
        }

        protected override void StartShowAnimation(UIElement element, AnimationContext context)
        {
            var visual = ElementCompositionPreview.GetElementVisual(element);
            var compositor = visual.Compositor;

            var parent = (FrameworkElement)VisualTreeHelper.GetParent(element);
            var to = LayoutInformation.GetLayoutSlot((FrameworkElement)element);
            var from = to;
            from.X = (parent.ActualWidth - ((FrameworkElement)element).ActualWidth) / 2;
            from.Y = (parent.ActualHeight - ((FrameworkElement)element).ActualHeight) / 2;

            var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
            AnimateOffset(element, visual, compositor, from, to);
            batch.End();
            batch.Completed += delegate { OnShowAnimationCompleted(element); };
        }

        protected override void StartHideAnimation(UIElement element, AnimationContext context)
        {
            var visual = ElementCompositionPreview.GetElementVisual(element);
            var compositor = visual.Compositor;

            var fadeOutAnimation = compositor.CreateScalarKeyFrameAnimation();
            fadeOutAnimation.InsertExpressionKeyFrame(0.0f, "this.CurrentValue");
            fadeOutAnimation.InsertKeyFrame(1.0f, 0.0f);
            fadeOutAnimation.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);

            var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
            visual.StartAnimation("Opacity", fadeOutAnimation);
            batch.End();
            batch.Completed += delegate { OnHideAnimationCompleted(element); };
        }

        protected override void StartBoundsChangeAnimation(UIElement element, AnimationContext context, Rect oldBounds, Rect newBounds)
        {
            var outerVisual = ElementCompositionPreview.GetElementVisual(element);
            var innerVisual = ElementCompositionPreview.GetElementVisual((UIElement)VisualTreeHelper.GetChild(element, 0));
            var compositor = outerVisual.Compositor;

            var parent = (FrameworkElement)VisualTreeHelper.GetParent(element);
            var center = new Point(parent.ActualWidth / 2, parent.ActualHeight / 2);
            var from = new Point((oldBounds.Left + oldBounds.Right) / 2 - center.X, -(oldBounds.Top + oldBounds.Bottom) / 2 + center.Y);
            var to = new Point((newBounds.Left + newBounds.Right) / 2 - center.X, -(newBounds.Top + newBounds.Bottom) / 2 + center.Y);
            var fromLength = Math.Sqrt(from.X * from.X + from.Y * from.Y);
            var toLength = Math.Sqrt(to.X * to.X + to.Y * to.Y);
            from.X /= fromLength;
            from.Y /= fromLength;
            to.X /= toLength;
            to.Y /= toLength;

            var angle = Math.Acos(Math.Min(1, to.X * from.X + to.Y * from.Y));
            var sign = Math.Sign(to.X * from.Y - to.Y * from.X);
            if (sign == 0) sign = Math.Sign(from.Y - to.Y);
            if (double.IsNaN(angle)) angle = 0;

            outerVisual.CenterPoint = new Vector3((float)(center.X - newBounds.X), (float)(center.Y - newBounds.Y), 0);
            innerVisual.CenterPoint = new Vector3((float)(newBounds.Width / 2), (float)(newBounds.Height / 2), 0);

            var outerRotate = compositor.CreateScalarKeyFrameAnimation();
            {
                outerRotate.InsertExpressionKeyFrame(0.0f, (-sign * angle).ToString());
                outerRotate.InsertKeyFrame(1.0f, 0.0f);
                outerRotate.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);
            }

            var innerRotate = compositor.CreateScalarKeyFrameAnimation();
            {
                innerRotate.InsertExpressionKeyFrame(0.0f,(sign * angle).ToString());
                innerRotate.InsertKeyFrame(1.0f, 0.0f);
                innerRotate.Duration = TimeSpan.FromMilliseconds(DefaultAnimationDurationInMs * AnimationSlowdownFactor);
            }

            var batch = compositor.CreateScopedBatch(CompositionBatchTypes.Animation);
            outerVisual.StartAnimation("RotationAngle", outerRotate);
            innerVisual.StartAnimation("RotationAngle", innerRotate);
            batch.End();
            batch.Completed += delegate { OnBoundsChangeAnimationCompleted(element); };
        }

        private void AnimateOffset(UIElement element, Visual visual, Compositor compositor, Rect oldBounds, Rect newBounds)
        {
            var offsetAnimation = compositor.CreateVector2KeyFrameAnimation();

            offsetAnimation.SetVector2Parameter("delta", new Vector2(
                (float)(oldBounds.X - newBounds.X),
                (float)(oldBounds.Y - newBounds.Y)));
            offsetAnimation.SetVector2Parameter("final", new Vector2());
            offsetAnimation.InsertExpressionKeyFrame(0.0f, "this.CurrentValue + delta");
            offsetAnimation.InsertExpressionKeyFrame(1.0f, "final");
            offsetAnimation.Duration = TimeSpan.FromMilliseconds(
                DefaultAnimationDurationInMs * AnimationSlowdownFactor);

            visual.StartAnimation("TransformMatrix._41_42", offsetAnimation);
        }
    }
}