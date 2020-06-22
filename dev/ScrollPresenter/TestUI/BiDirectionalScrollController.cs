// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;

using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using ScrollingAnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingScrollMode = Microsoft.UI.Xaml.Controls.ScrollingScrollMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollControllerInteractionRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerInteractionRequestedEventArgs;
using ScrollControllerScrollToRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerScrollToRequestedEventArgs;
using ScrollControllerScrollByRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerScrollByRequestedEventArgs;
using ScrollControllerAddScrollVelocityRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerAddScrollVelocityRequestedEventArgs;

namespace MUXControlsTestApp.Utilities
{
    public class BiDirectionalScrollControllerScrollingScrollCompletedEventArgs
    {
        internal BiDirectionalScrollControllerScrollingScrollCompletedEventArgs(int offsetsChangeCorrelationId)
        {
            OffsetsChangeCorrelationId = offsetsChangeCorrelationId;
        }

        public int OffsetsChangeCorrelationId
        {
            get;
            set;
        }
    }

    public sealed class BiDirectionalScrollController : ContentControl
    {
        private class UniScrollControllerScrollingScrollCompletedEventArgs
        {
            public UniScrollControllerScrollingScrollCompletedEventArgs(int offsetChangeCorrelationId)
            {
                OffsetChangeCorrelationId = offsetChangeCorrelationId;
            }

            public int OffsetChangeCorrelationId
            {
                get;
                set;
            }
        }

        private class UniScrollController : IScrollController
        {
            public event TypedEventHandler<IScrollController, object> InteractionInfoChanged;
            public event TypedEventHandler<IScrollController, ScrollControllerInteractionRequestedEventArgs> InteractionRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerScrollToRequestedEventArgs> ScrollToRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerScrollByRequestedEventArgs> ScrollByRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerAddScrollVelocityRequestedEventArgs> AddScrollVelocityRequested;
            public event TypedEventHandler<IScrollController, UniScrollControllerScrollingScrollCompletedEventArgs> ScrollCompleted;

            public UniScrollController(BiDirectionalScrollController owner, Orientation orientation)
            {
                Owner = owner;
                Orientation = orientation;
                ScrollableExtent = 0.0;
                Offset = 0.0;
                Viewport = 0.0;
            }

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
            public bool AreScrollControllerInteractionsAllowed
            {
                get;
                private set;
            }
#endif

#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
            public bool AreScrollerInteractionsAllowed
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_AreScrollerInteractionsAllowed for Orientation=" + Orientation);
                    return Owner.AreScrollerInteractionsAllowed;
                }
            }
#endif

#if USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED
            public bool IsInteractionElementRailEnabled
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_IsInteractionElementRailEnabled for Orientation=" + Orientation);
                    return Owner.IsRailing;
                }
            }
#endif

            public bool IsScrolling
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_IsScrolling for Orientation=" + Orientation);
                    return Owner.IsScrolling;
                }
            }

            public UIElement InteractionElement
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionElement for Orientation=" + Orientation);
                    return Owner.GetParentInteractionElement();
                }
            }

            public Orientation InteractionElementScrollOrientation
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionElementScrollOrientation for Orientation=" + Orientation);
                    return Orientation;
                }
            }

            internal double Offset
            {
                get;
                private set;
            }

            internal double ScrollableExtent
            {
                get;
                private set;
            }

            internal double Viewport
            {
                get;
                private set;
            }

            private ScrollingScrollMode ScrollMode
            {
                get;
                set;
            }

            private Orientation Orientation
            {
                get;
                set;
            }

            private BiDirectionalScrollController Owner
            {
                get;
                set;
            }

            private string OffsetPropertyName
            {
                get;
                set;
            }

            private string ScrollableExtentPropertyName
            {
                get;
                set;
            }

            private string MultiplierPropertyName
            {
                get;
                set;
            }

            private CompositionPropertySet ExpressionAnimationSources
            {
                get;
                set;
            }

            private ExpressionAnimation ThumbOffsetAnimation
            {
                get;
                set;
            }

            public void SetExpressionAnimationSources(
                CompositionPropertySet propertySet, string offsetPropertyName, string scrollableExtentPropertyName, string multiplierPropertyName)
            {
                RaiseLogMessage(
                    "UniScrollController: SetExpressionAnimationSources for Orientation=" + Orientation +
                    " with offsetPropertyName=" + offsetPropertyName +
                    ", scrollableExtentPropertyName=" + scrollableExtentPropertyName +
                    ", multiplierPropertyName=" + multiplierPropertyName);
                ExpressionAnimationSources = propertySet;
                if (ExpressionAnimationSources != null)
                {
                    OffsetPropertyName = offsetPropertyName.Trim();
                    ScrollableExtentPropertyName = scrollableExtentPropertyName.Trim();
                    MultiplierPropertyName = multiplierPropertyName.Trim();

                    UpdateInteractionElementScrollMultiplier();

                    if (ThumbOffsetAnimation == null)
                    {
                        EnsureThumbAnimation();
                        StartThumbAnimation();
                    }
                }
                else
                {
                    OffsetPropertyName =
                    ScrollableExtentPropertyName =
                    MultiplierPropertyName = string.Empty;

                    ThumbOffsetAnimation = null;
                    StopThumbAnimation();
                }
            }

            public void SetScrollMode(ScrollingScrollMode scrollMode)
            {
                RaiseLogMessage(
                    "UniScrollController: SetScrollMode for Orientation=" + Orientation +
                    " with scrollMode=" + scrollMode);
                ScrollMode = scrollMode;
#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
                UpdateAreScrollControllerInteractionsAllowed();
#endif
            }

            public void SetDimensions(double offset, double scrollableExtent, double viewport)
            {
                RaiseLogMessage(
                    "UniScrollController: SetDimensions for Orientation=" + Orientation +
                    " with offset=" + offset +
                    ", scrollableExtent=" + scrollableExtent +
                    ", viewport=" + viewport);

                if (scrollableExtent < 0)
                {
                    throw new ArgumentOutOfRangeException("scrollableExtent");
                }

                if (viewport < 0.0)
                {
                    throw new ArgumentOutOfRangeException("viewport");
                }

                offset = Math.Max(0.0, offset);
                offset = Math.Min(scrollableExtent, offset);

                double offsetTarget = 0.0;

                if (Owner.OperationsCount == 0)
                {
                    offsetTarget = offset;
                }
                else
                {
                    offsetTarget = Math.Max(0.0, offsetTarget);
                    offsetTarget = Math.Min(scrollableExtent, offsetTarget);
                }

                if (Orientation == Orientation.Horizontal)
                    Owner.OffsetTarget = new Point(offsetTarget, Owner.OffsetTarget.Y);
                else
                    Owner.OffsetTarget = new Point(Owner.OffsetTarget.X, offsetTarget);

                bool updateThumbSize =
                    ScrollableExtent != scrollableExtent ||
                    Viewport != viewport;

                Offset = offset;
                ScrollableExtent = scrollableExtent;
                Viewport = viewport;

                if (updateThumbSize && !Owner.UpdateThumbSize())
                {
                    UpdateInteractionElementScrollMultiplier();
                }
            }

            public CompositionAnimation GetScrollAnimation(
                int info,
                Vector2 currentPosition,
                CompositionAnimation defaultAnimation)
            {
                RaiseLogMessage(
                    "UniScrollController: GetScrollAnimation for Orientation=" + Orientation +
                    " with OffsetsChangeCorrelationId=" + info + ", currentPosition=" + currentPosition);
                return defaultAnimation;
            }

            public void NotifyScrollCompleted(
                int info)
            {
                RaiseLogMessage(
                    "UniScrollController: NotifyScrollCompleted for Orientation=" + Orientation +
                    " with OffsetsChangeCorrelationId=" + info);

                ScrollCompleted?.Invoke(this, new UniScrollControllerScrollingScrollCompletedEventArgs(info));
            }

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
            internal bool UpdateAreScrollControllerInteractionsAllowed()
            {
                bool oldAreScrollControllerInteractionsAllowed = AreScrollControllerInteractionsAllowed;

                AreScrollControllerInteractionsAllowed = ScrollMode != ScrollingScrollMode.Disabled && Owner.IsEnabled;

                if (oldAreScrollControllerInteractionsAllowed != AreScrollControllerInteractionsAllowed)
                {
                    RaiseInteractionInfoChanged();
                    return true;
                }
                return false;
            }
#endif

            internal void UpdateInteractionElementScrollMultiplier()
            {
                if (ExpressionAnimationSources != null && !string.IsNullOrWhiteSpace(MultiplierPropertyName))
                {
                    float interactionVisualScrollMultiplier = Owner.GetInteractionElementScrollMultiplier(Orientation, ScrollableExtent);

                    RaiseLogMessage("UniScrollController: UpdateInteractionElementScrollMultiplier for Orientation=" + Orientation + ", InteractionElementScrollMultiplier=" + interactionVisualScrollMultiplier);
                    ExpressionAnimationSources.InsertScalar(MultiplierPropertyName, interactionVisualScrollMultiplier);
                }
            }

            internal void RaiseInteractionInfoChanged()
            {
                RaiseLogMessage("UniScrollController: RaiseInteractionInfoChanged for Orientation=" + Orientation);

                if (InteractionInfoChanged != null)
                {
                    InteractionInfoChanged(this, null);
                }
            }

            internal bool RaiseInteractionRequested(PointerPoint pointerPoint)
            {
                RaiseLogMessage("UniScrollController: RaiseInteractionRequested for Orientation=" + Orientation + " with pointerPoint=" + pointerPoint);

                if (InteractionRequested != null)
                {
                    ScrollControllerInteractionRequestedEventArgs args = new ScrollControllerInteractionRequestedEventArgs(pointerPoint);
                    InteractionRequested(this, args);
                    RaiseLogMessage("UniScrollController: RaiseInteractionRequested result Handled=" + args.Handled);
                    return args.Handled;
                }
                return false;
            }

            internal int RaiseScrollToRequested(
                double offset,
                ScrollingAnimationMode animationMode)
            {
                RaiseLogMessage("UniScrollController: RaiseScrollToRequested for Orientation=" + Orientation + " with offset=" + offset + ", animationMode=" + animationMode);

                if (ScrollToRequested != null)
                {
                    ScrollControllerScrollToRequestedEventArgs e =
                        new ScrollControllerScrollToRequestedEventArgs(
                            offset,
                            new ScrollingScrollOptions(animationMode, ScrollingSnapPointsMode.Ignore));
                    ScrollToRequested(this, e);
                    return e.CorrelationId;
                }
                return -1;
            }

            internal int RaiseScrollByRequested(
                double offsetDelta,
                ScrollingAnimationMode animationMode)
            {
                RaiseLogMessage("UniScrollController: RaiseScrollByRequested for Orientation=" + Orientation + " with offsetDelta=" + offsetDelta + ", animationMode=" + animationMode);

                if (ScrollByRequested != null)
                {
                    ScrollControllerScrollByRequestedEventArgs e =
                        new ScrollControllerScrollByRequestedEventArgs(
                            offsetDelta,
                            new ScrollingScrollOptions(animationMode, ScrollingSnapPointsMode.Ignore));
                    ScrollByRequested(this, e);
                    return e.CorrelationId;
                }
                return -1;
            }

            internal int RaiseAddScrollVelocityRequested(
                float offsetVelocity, float? inertiaDecayRate)
            {
                RaiseLogMessage("UniScrollController: RaiseAddScrollVelocityRequested for Orientation=" + Orientation + " with offsetVelocity=" + offsetVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

                if (AddScrollVelocityRequested != null)
                {
                    ScrollControllerAddScrollVelocityRequestedEventArgs e =
                        new ScrollControllerAddScrollVelocityRequestedEventArgs(
                            offsetVelocity,
                            inertiaDecayRate);
                    AddScrollVelocityRequested(this, e);
                    return e.CorrelationId;
                }
                return -1;
            }

            private void EnsureThumbAnimation()
            {
                if (ThumbOffsetAnimation == null && ExpressionAnimationSources != null &&
                    !string.IsNullOrWhiteSpace(MultiplierPropertyName) &&
                    !string.IsNullOrWhiteSpace(OffsetPropertyName) &&
                    !string.IsNullOrWhiteSpace(ScrollableExtentPropertyName))
                {
                    ThumbOffsetAnimation = ExpressionAnimationSources.Compositor.CreateExpressionAnimation();
                    ThumbOffsetAnimation.Expression =
                        "min(eas." + ScrollableExtentPropertyName + ",max(0,eas." + OffsetPropertyName + "))/(-eas." + MultiplierPropertyName + ")";
                    ThumbOffsetAnimation.SetReferenceParameter("eas", ExpressionAnimationSources);
                }
            }

            private void StartThumbAnimation()
            {
                if (Owner.InteractionElement != null && ThumbOffsetAnimation != null)
                {
                    Visual interactionVisual = ElementCompositionPreview.GetElementVisual(Owner.InteractionElement);

                    if (Orientation == Orientation.Horizontal)
                    {
                        interactionVisual.StartAnimation("Translation.X", ThumbOffsetAnimation);
                    }
                    else
                    {
                        interactionVisual.StartAnimation("Translation.Y", ThumbOffsetAnimation);
                    }
                }
            }

            private void StopThumbAnimation()
            {
                if (Owner.InteractionElement != null)
                {
                    Visual interactionVisual = ElementCompositionPreview.GetElementVisual(Owner.InteractionElement);

                    if (Orientation == Orientation.Horizontal)
                    {
                        interactionVisual.StopAnimation("Translation.X");
                    }
                    else
                    {
                        interactionVisual.StopAnimation("Translation.Y");
                    }
                }
            }

            private void RaiseLogMessage(string message)
            {
                Owner.RaiseLogMessage(message);
            }
        }

        private struct OperationInfo
        {
            public int CorrelationId;
            public Point RelativeOffsetChange;
            public Point OffsetTarget;

            public OperationInfo(int correlationId, Point relativeOffsetChange, Point offsetTarget) : this()
            {
                CorrelationId = correlationId;
                RelativeOffsetChange = relativeOffsetChange;
                OffsetTarget = offsetTarget;
            }
        }

        private const float SmallChangeAdditionalVelocity = 144.0f;
        private const float SmallChangeInertiaDecayRate = 0.975f;

        private List<string> lstAsyncEventMessage = new List<string>();
        private List<int> lstViewChangeCorrelationIds = new List<int>();
        private List<int> lstScrollToCorrelationIds = new List<int>();
        private List<int> lstScrollByCorrelationIds = new List<int>();
        private List<int> lstAddScrollVelocityCorrelationIds = new List<int>();
        private Dictionary<int, OperationInfo> operations = new Dictionary<int, OperationInfo>();
        private UniScrollController horizontalScrollController = null;
        private UniScrollController verticalScrollController = null;
        private RepeatButton biDecrementRepeatButton = null;
        private RepeatButton biIncrementRepeatButton = null;
        private RepeatButton horizontalIncrementVerticalDecrementRepeatButton = null;
        private RepeatButton horizontalDecrementVerticalIncrementRepeatButton = null;
        private RepeatButton horizontalDecrementRepeatButton = null;
        private RepeatButton horizontalIncrementRepeatButton = null;
        private RepeatButton verticalDecrementRepeatButton = null;
        private RepeatButton verticalIncrementRepeatButton = null;
#if USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED
        private bool isRailing = true;
#endif
        private Point preManipulationThumbOffset;

        public event TypedEventHandler<BiDirectionalScrollController, string> LogMessage;
        public event TypedEventHandler<BiDirectionalScrollController, BiDirectionalScrollControllerScrollingScrollCompletedEventArgs> ScrollCompleted;

        public BiDirectionalScrollController()
        {
            this.DefaultStyleKey = typeof(BiDirectionalScrollController);
#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
            AreScrollerInteractionsAllowed = true;
#endif
            this.horizontalScrollController = new UniScrollController(this, Orientation.Horizontal);
            this.verticalScrollController = new UniScrollController(this, Orientation.Vertical);

            this.horizontalScrollController.ScrollCompleted += UniScrollController_ScrollCompleted;
            this.verticalScrollController.ScrollCompleted += UniScrollController_ScrollCompleted;

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
            IsEnabledChanged += BiDirectionalScrollController_IsEnabledChanged;
#endif
            SizeChanged += BiDirectionalScrollController_SizeChanged;
        }

#if USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED
        public bool IsRailing
        {
            get
            {
                return isRailing;
            }
            set
            {
                if (isRailing != value)
                {
                    isRailing = value;

                    if (Thumb != null)
                    {
                        Thumb.ManipulationMode = ManipulationModes.TranslateX | ManipulationModes.TranslateY;
                        if (IsRailing)
                        {
                            Thumb.ManipulationMode |= ManipulationModes.TranslateRailsX | ManipulationModes.TranslateRailsY;
                        }
                    }

                    RaiseInteractionInfoChanged();
                }
            }
        }
#endif

        public IScrollController HorizontalScrollController
        {
            get
            {
                return horizontalScrollController;
            }
        }

        public IScrollController VerticalScrollController
        {
            get
            {
                return verticalScrollController;
            }
        }

        public int ScrollTo(double horizontalOffset, double verticalOffset, ScrollingAnimationMode animationMode)
        {
            int viewChangeCorrelationId = RaiseScrollToRequested(
                new Point(horizontalOffset, verticalOffset),
                animationMode, false /*hookupCompletion*/);
            lstViewChangeCorrelationIds.Add(viewChangeCorrelationId);
            return viewChangeCorrelationId;
        }

        public int ScrollBy(double horizontalOffsetDelta, double verticalOffsetDelta, ScrollingAnimationMode animationMode)
        {
            int viewChangeCorrelationId = RaiseScrollByRequested(
                new Point(horizontalOffsetDelta, verticalOffsetDelta),
                animationMode, false /*hookupCompletion*/);
            lstViewChangeCorrelationIds.Add(viewChangeCorrelationId);
            return viewChangeCorrelationId;
        }

        public int AddScrollVelocity(Vector2 offsetVelocity, Vector2? inertiaDecayRate)
        {
            int viewChangeCorrelationId = RaiseAddScrollVelocityRequested(
                offsetVelocity, inertiaDecayRate, false /*hookupCompletion*/);
            lstViewChangeCorrelationIds.Add(viewChangeCorrelationId);
            return viewChangeCorrelationId;
        }

        protected override void OnApplyTemplate()
        {
            UnhookHandlers();

            base.OnApplyTemplate();

            Thumb = GetTemplateChild("Thumb") as FrameworkElement;
            biDecrementRepeatButton = GetTemplateChild("BiDecrementRepeatButton") as RepeatButton;
            biIncrementRepeatButton = GetTemplateChild("BiIncrementRepeatButton") as RepeatButton;
            horizontalIncrementVerticalDecrementRepeatButton = GetTemplateChild("HorizontalIncrementVerticalDecrementRepeatButton") as RepeatButton;
            horizontalDecrementVerticalIncrementRepeatButton = GetTemplateChild("HorizontalDecrementVerticalIncrementRepeatButton") as RepeatButton;
            horizontalDecrementRepeatButton = GetTemplateChild("HorizontalDecrementRepeatButton") as RepeatButton;
            horizontalIncrementRepeatButton = GetTemplateChild("HorizontalIncrementRepeatButton") as RepeatButton;
            verticalDecrementRepeatButton = GetTemplateChild("VerticalDecrementRepeatButton") as RepeatButton;
            verticalIncrementRepeatButton = GetTemplateChild("VerticalIncrementRepeatButton") as RepeatButton;

            if (Thumb != null)
            {
                Thumb.ManipulationMode = ManipulationModes.TranslateX | ManipulationModes.TranslateY;
#if USE_SCROLLCONTROLLER_ISINTERACTIONELEMENTRAILENABLED
                if (IsRailing)
#endif
                {
                    Thumb.ManipulationMode |= ManipulationModes.TranslateRailsX | ManipulationModes.TranslateRailsY;
                }

                InteractionElement = Thumb;
                ElementCompositionPreview.SetIsTranslationEnabled(Thumb, true);
            }
            else
            {
                InteractionElement = null;
            }

            HookHandlers();

            RaiseInteractionInfoChanged();
        }

        internal UIElement GetParentInteractionElement()
        {
            RaiseLogMessage("BiDirectionalScrollController: GetParentInteractionElement");
            return (Thumb != null && Thumb.Parent != null) ? Thumb.Parent as UIElement : null;
        }

#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
        internal bool AreScrollerInteractionsAllowed
        {
            get;
            private set;
        }
#endif

        internal bool IsScrolling
        {
            get;
            private set;
        }

        internal UIElement InteractionElement
        {
            get;
            private set;
        }

        internal Point OffsetTarget
        {
            get;
            set;
        }

        private FrameworkElement Thumb
        {
            get;
            set;
        }

        private ExpressionAnimation HorizontalThumbOffsetAnimation
        {
            get;
            set;
        }

        private ExpressionAnimation VerticalThumbOffsetAnimation
        {
            get;
            set;
        }

        private double HorizontalThumbOffset
        {
            get
            {
                if (Thumb != null)
                {
                    double parentWidth = 0.0;
                    FrameworkElement parent = Thumb.Parent as FrameworkElement;
                    if (parent != null)
                    {
                        parentWidth = parent.ActualWidth;
                    }
                    if (horizontalScrollController.ScrollableExtent != 0.0)
                    {
                        return horizontalScrollController.Offset / horizontalScrollController.ScrollableExtent * (parentWidth - Thumb.Width);
                    }
                }

                return 0.0;
            }
        }

        private double VerticalThumbOffset
        {
            get
            {
                if (Thumb != null)
                {
                    double parentHeight = 0.0;
                    FrameworkElement parent = Thumb.Parent as FrameworkElement;
                    if (parent != null)
                    {
                        parentHeight = parent.ActualHeight;
                    }
                    if (verticalScrollController.ScrollableExtent != 0.0)
                    {
                        return verticalScrollController.Offset / verticalScrollController.ScrollableExtent * (parentHeight - Thumb.Height);
                    }
                }

                return 0.0;
            }
        }

        private void HookHandlers()
        {
            if (biDecrementRepeatButton != null)
            {
                biDecrementRepeatButton.Click += BiDecrementRepeatButton_Click;
            }

            if (biIncrementRepeatButton != null)
            {
                biIncrementRepeatButton.Click += BiIncrementRepeatButton_Click;
            }

            if (horizontalIncrementVerticalDecrementRepeatButton != null)
            {
                horizontalIncrementVerticalDecrementRepeatButton.Click += HorizontalIncrementVerticalDecrementRepeatButton_Click;
            }

            if (horizontalDecrementVerticalIncrementRepeatButton != null)
            {
                horizontalDecrementVerticalIncrementRepeatButton.Click += HorizontalDecrementVerticalIncrementRepeatButton_Click;
            }

            if (horizontalDecrementRepeatButton != null)
            {
                horizontalDecrementRepeatButton.Click += HorizontalDecrementRepeatButton_Click;
            }

            if (horizontalIncrementRepeatButton != null)
            {
                horizontalIncrementRepeatButton.Click += HorizontalIncrementRepeatButton_Click;
            }

            if (verticalDecrementRepeatButton != null)
            {
                verticalDecrementRepeatButton.Click += VerticalDecrementRepeatButton_Click;
            }

            if (verticalIncrementRepeatButton != null)
            {
                verticalIncrementRepeatButton.Click += VerticalIncrementRepeatButton_Click;
            }

            if (Thumb != null)
            {
                Thumb.PointerPressed += Thumb_PointerPressed;
                Thumb.ManipulationStarting += Thumb_ManipulationStarting;
                Thumb.ManipulationDelta += Thumb_ManipulationDelta;
                Thumb.ManipulationCompleted += Thumb_ManipulationCompleted;

                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parent.PointerPressed += Parent_PointerPressed;
                }
            }
        }

        private void UnhookHandlers()
        {
            if (biDecrementRepeatButton != null)
            {
                biDecrementRepeatButton.Click -= BiDecrementRepeatButton_Click;
            }

            if (biIncrementRepeatButton != null)
            {
                biIncrementRepeatButton.Click -= BiIncrementRepeatButton_Click;
            }

            if (horizontalIncrementVerticalDecrementRepeatButton != null)
            {
                horizontalIncrementVerticalDecrementRepeatButton.Click -= HorizontalIncrementVerticalDecrementRepeatButton_Click;
            }

            if (horizontalDecrementVerticalIncrementRepeatButton != null)
            {
                horizontalDecrementVerticalIncrementRepeatButton.Click -= HorizontalDecrementVerticalIncrementRepeatButton_Click;
            }

            if (horizontalDecrementRepeatButton != null)
            {
                horizontalDecrementRepeatButton.Click -= HorizontalDecrementRepeatButton_Click;
            }

            if (horizontalIncrementRepeatButton != null)
            {
                horizontalIncrementRepeatButton.Click -= HorizontalIncrementRepeatButton_Click;
            }

            if (verticalDecrementRepeatButton != null)
            {
                verticalDecrementRepeatButton.Click -= VerticalDecrementRepeatButton_Click;
            }

            if (verticalIncrementRepeatButton != null)
            {
                verticalIncrementRepeatButton.Click -= VerticalIncrementRepeatButton_Click;
            }

            if (Thumb != null)
            {
                Thumb.PointerPressed -= Thumb_PointerPressed;
                Thumb.ManipulationStarting -= Thumb_ManipulationStarting;
                Thumb.ManipulationDelta -= Thumb_ManipulationDelta;
                Thumb.ManipulationCompleted -= Thumb_ManipulationCompleted;

                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parent.PointerPressed -= Parent_PointerPressed;
                }
            }
        }

        private void BiDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: BiDecrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void BiIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: BiIncrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementVerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementVerticalDecrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(SmallChangeAdditionalVelocity, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementVerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementVerticalIncrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalDecrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(0, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalIncrementRepeatButton_Click");

            int viewChangeCorrelationId =
                RaiseAddScrollVelocityRequested(new Vector2(0, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void Thumb_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            Point pt = e.GetCurrentPoint(sender as UIElement).Position;
            RaiseLogMessage("BiDirectionalScrollController: Thumb_PointerPressed with position=" + pt);

            switch (e.Pointer.PointerDeviceType)
            {
                case Windows.Devices.Input.PointerDeviceType.Touch:
                case Windows.Devices.Input.PointerDeviceType.Pen:
                    RaiseInteractionRequested(e.GetCurrentPoint(null));
                    break;
                case Windows.Devices.Input.PointerDeviceType.Mouse:
#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
                    AreScrollerInteractionsAllowed = false;
#endif

                    if (!IsScrolling)
                    {
                        IsScrolling = true;
                        RaiseInteractionInfoChanged();
                    }
                    break;
            }
        }

        private void Parent_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            Point pt = e.GetCurrentPoint(sender as UIElement).Position;
            RaiseLogMessage("BiDirectionalScrollController: Parent_PointerPressed with position=" + pt);

            Point maxThumbOffset = MaxThumbOffset();
            double targetThumbHorizontalOffset = pt.X - Thumb.ActualWidth / 2.0;
            double targetThumbVerticalOffset = pt.Y - Thumb.ActualHeight / 2.0;

            targetThumbHorizontalOffset = Math.Max(targetThumbHorizontalOffset, 0.0);
            targetThumbVerticalOffset = Math.Max(targetThumbVerticalOffset, 0.0);

            targetThumbHorizontalOffset = Math.Min(targetThumbHorizontalOffset, maxThumbOffset.X);
            targetThumbVerticalOffset = Math.Min(targetThumbVerticalOffset, maxThumbOffset.Y);

            Point targetThumbOffset = new Point(targetThumbHorizontalOffset, targetThumbVerticalOffset);
            Point targetScrollPresenterOffset = ScrollPresenterOffsetFromThumbOffset(targetThumbOffset);

            int viewChangeCorrelationId = RaiseScrollToRequested(targetScrollPresenterOffset, ScrollingAnimationMode.Auto, true /*hookupCompletion*/);
            if (viewChangeCorrelationId != -1 && !operations.ContainsKey(viewChangeCorrelationId))
            {
                operations.Add(
                    viewChangeCorrelationId,
                    new OperationInfo(viewChangeCorrelationId, new Point(targetScrollPresenterOffset.X - HorizontalThumbOffset, targetScrollPresenterOffset.Y - VerticalThumbOffset), targetScrollPresenterOffset));
            }
        }

        private void Thumb_ManipulationStarting(object sender, ManipulationStartingRoutedEventArgs e)
        {
            if (IsScrolling)
            {
                preManipulationThumbOffset = new Point(HorizontalThumbOffset, VerticalThumbOffset);
            }
        }

        private void Thumb_ManipulationDelta(object sender, ManipulationDeltaRoutedEventArgs e)
        {
            if (IsScrolling)
            {
                Point targetThumbOffset = new Point(
                    preManipulationThumbOffset.X + e.Cumulative.Translation.X,
                    preManipulationThumbOffset.Y + e.Cumulative.Translation.Y);
                Point scrollPresenterOffset = ScrollPresenterOffsetFromThumbOffset(targetThumbOffset);

                int viewChangeCorrelationId = RaiseScrollToRequested(
                    scrollPresenterOffset, ScrollingAnimationMode.Disabled, true /*hookupCompletion*/);
            }
        }

        private void Thumb_ManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {
#if USE_SCROLLCONTROLLER_ARESCROLLERINTERACTIONSALLOWED
            AreScrollerInteractionsAllowed = true;
#endif

            if (IsScrolling)
            {
                IsScrolling = false;
                RaiseInteractionInfoChanged();
            }
        }

#if USE_SCROLLCONTROLLER_ARESCROLLCONTROLLERINTERACTIONSALLOWED
        private void BiDirectionalScrollController_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: IsEnabledChanged with IsEnabled=" + IsEnabled);
            if (!horizontalScrollController.UpdateAreScrollControllerInteractionsAllowed() ||
                !verticalScrollController.UpdateAreScrollControllerInteractionsAllowed())
            {
                RaiseInteractionInfoChanged();
            }
        }
#endif
        private void BiDirectionalScrollController_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateThumbSize();
        }

        private bool UpdateThumbSize()
        {
            if (Thumb != null)
            {
                Size parentSize;
                FrameworkElement parent = Thumb.Parent as FrameworkElement;

                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }

                Size newSize;
                if (horizontalScrollController.Viewport == 0.0)
                {
                    newSize.Width = 40.0;
                }
                else
                {
                    newSize.Width = Math.Max(Math.Min(40.0, parentSize.Width), horizontalScrollController.Viewport / (horizontalScrollController.ScrollableExtent + horizontalScrollController.Viewport) * parentSize.Width);
                }
                if (verticalScrollController.Viewport == 0.0)
                {
                    newSize.Height = 40.0;
                }
                else
                {
                    newSize.Height = Math.Max(Math.Min(40.0, parentSize.Height), verticalScrollController.Viewport / (verticalScrollController.ScrollableExtent + verticalScrollController.Viewport) * parentSize.Height);
                }
                if (newSize.Width != Thumb.Width)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Horizontal, setting Width=" + newSize.Width);
                    Thumb.Width = newSize.Width;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateHorizontalInteractionElementScrollMultiplier);
                    return true;
                }
                if (newSize.Height != Thumb.Height)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Vertical, setting Height=" + newSize.Height);
                    Thumb.Height = newSize.Height;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateVerticalInteractionElementScrollMultiplier);
                    return true;
                }
            }
            return false;
        }

        private void UpdateHorizontalInteractionElementScrollMultiplier()
        {
            horizontalScrollController.UpdateInteractionElementScrollMultiplier();
        }

        private void UpdateVerticalInteractionElementScrollMultiplier()
        {
            verticalScrollController.UpdateInteractionElementScrollMultiplier();
        }

        private Point ScrollPresenterOffsetFromThumbOffset(Point thumbOffset)
        {
            Point scrollPresenterOffset = new Point();

            if (Thumb != null)
            {
                Size parentSize;
                Size thumbSize = new Size(Thumb.ActualWidth, Thumb.ActualHeight);
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }
                if (parentSize.Width != thumbSize.Width || parentSize.Height != thumbSize.Height)
                {
                    scrollPresenterOffset = new Point(
                        parentSize.Width == thumbSize.Width ? 0 : (thumbOffset.X * horizontalScrollController.ScrollableExtent / (parentSize.Width - thumbSize.Width)),
                        parentSize.Height == thumbSize.Height ? 0 : (thumbOffset.Y * verticalScrollController.ScrollableExtent / (parentSize.Height - thumbSize.Height)));
                }
            }

            return scrollPresenterOffset;
        }

        private Point MaxThumbOffset()
        {
            Point maxThumbOffset = new Point();

            if (Thumb != null)
            {
                Size parentSize;
                Size thumbSize = new Size(Thumb.ActualWidth, Thumb.ActualHeight);
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentSize = new Size(parent.ActualWidth, parent.ActualHeight);
                }
                maxThumbOffset = new Point(
                    Math.Max(0.0, parentSize.Width - thumbSize.Width),
                    Math.Max(0.0, parentSize.Height - thumbSize.Height));
            }

            return maxThumbOffset;
        }

        private void UniScrollController_ScrollCompleted(IScrollController sender, UniScrollControllerScrollingScrollCompletedEventArgs args)
        {
            if (lstScrollToCorrelationIds.Contains(args.OffsetChangeCorrelationId))
            {
                lstScrollToCorrelationIds.Remove(args.OffsetChangeCorrelationId);

                Point relativeOffsetChange;

                if (operations.ContainsKey(args.OffsetChangeCorrelationId))
                {
                    OperationInfo oi = operations[args.OffsetChangeCorrelationId];
                    relativeOffsetChange = oi.RelativeOffsetChange;
                    operations.Remove(args.OffsetChangeCorrelationId);
                }

                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest completed. OffsetChangeCorrelationId=" + args.OffsetChangeCorrelationId);
            }
            else if (lstScrollByCorrelationIds.Contains(args.OffsetChangeCorrelationId))
            {
                lstScrollByCorrelationIds.Remove(args.OffsetChangeCorrelationId);

                Point relativeOffsetChange;

                if (operations.ContainsKey(args.OffsetChangeCorrelationId))
                {
                    OperationInfo oi = operations[args.OffsetChangeCorrelationId];
                    relativeOffsetChange = oi.RelativeOffsetChange;
                    operations.Remove(args.OffsetChangeCorrelationId);
                }

                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest completed. OffsetChangeCorrelationId=" + args.OffsetChangeCorrelationId);
            }
            else if (lstAddScrollVelocityCorrelationIds.Contains(args.OffsetChangeCorrelationId))
            {
                lstAddScrollVelocityCorrelationIds.Remove(args.OffsetChangeCorrelationId);

                RaiseLogMessage("BiDirectionalScrollController: AddScrollVelocityRequest completed. OffsetChangeCorrelationId=" + args.OffsetChangeCorrelationId);
            }

            if (lstViewChangeCorrelationIds.Contains(args.OffsetChangeCorrelationId))
            {
                lstViewChangeCorrelationIds.Remove(args.OffsetChangeCorrelationId);

                RaiseScrollCompleted(args.OffsetChangeCorrelationId);
            }
        }

        private void RaiseInteractionInfoChanged()
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionInfoChanged");
            horizontalScrollController.RaiseInteractionInfoChanged();
            verticalScrollController.RaiseInteractionInfoChanged();
        }

        private void RaiseInteractionRequested(PointerPoint pointerPoint)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionRequested with pointerPoint=" + pointerPoint);
            bool handled = horizontalScrollController.RaiseInteractionRequested(pointerPoint);
            RaiseLogMessage("BiDirectionalScrollController: RaiseInteractionRequested result Handled=" + handled);
        }

        private int RaiseScrollToRequested(
            Point offset,
            ScrollingAnimationMode animationMode,
            bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseScrollToRequested with offset=" + offset + ", animationMode=" + animationMode);

            int horizontalOffsetChangeCorrelationId = horizontalScrollController.RaiseScrollToRequested(
                offset.X, animationMode);

            if (horizontalOffsetChangeCorrelationId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Horizontal ScrollToRequest started. CorrelationId=" + horizontalOffsetChangeCorrelationId);

            int verticalOffsetChangeCorrelationId = verticalScrollController.RaiseScrollToRequested(
                offset.Y, animationMode);

            if (verticalOffsetChangeCorrelationId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Vertical ScrollToRequest started. CorrelationId=" + verticalOffsetChangeCorrelationId);

            int offsetChangeCorrelationId = -1;

            if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId == verticalOffsetChangeCorrelationId)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId != verticalOffsetChangeCorrelationId)
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest CorrelationIds do not match.");
            }
            else if (horizontalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (verticalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = verticalOffsetChangeCorrelationId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollToRequest CorrelationIds are -1.");
            }

            if (hookupCompletion && offsetChangeCorrelationId != -1 && !lstScrollToCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstScrollToCorrelationIds.Add(offsetChangeCorrelationId);
            }

            return offsetChangeCorrelationId;
        }

        private int RaiseScrollByRequested(
            Point offsetDelta,
            ScrollingAnimationMode animationMode,
            bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseScrollByRequested with offsetDelta=" + offsetDelta + ", animationMode=" + animationMode);

            int horizontalOffsetChangeCorrelationId = horizontalScrollController.RaiseScrollByRequested(
                offsetDelta.X, animationMode);

            if (horizontalOffsetChangeCorrelationId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Horizontal ScrollByRequest started. CorrelationId=" + horizontalOffsetChangeCorrelationId);

            int verticalOffsetChangeCorrelationId = verticalScrollController.RaiseScrollByRequested(
                offsetDelta.Y, animationMode);

            if (verticalOffsetChangeCorrelationId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Vertical ScrollByRequest started. CorrelationId=" + verticalOffsetChangeCorrelationId);

            int offsetChangeCorrelationId = -1;

            if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId == verticalOffsetChangeCorrelationId)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId != verticalOffsetChangeCorrelationId)
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest CorrelationIds do not match.");
            }
            else if (horizontalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (verticalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = verticalOffsetChangeCorrelationId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: ScrollByRequest CorrelationIds are -1.");
            }

            if (hookupCompletion && offsetChangeCorrelationId != -1 && !lstScrollByCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstScrollByCorrelationIds.Add(offsetChangeCorrelationId);
            }

            return offsetChangeCorrelationId;
        }

        private int RaiseAddScrollVelocityRequested(
            Vector2 additionalVelocity, Vector2? inertiaDecayRate, bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseAddScrollVelocityRequested with additionalVelocity=" + additionalVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

            int horizontalOffsetChangeCorrelationId = -1;
            int verticalOffsetChangeCorrelationId = -1;

            if (additionalVelocity.X != 0.0f)
            {
                horizontalOffsetChangeCorrelationId = horizontalScrollController.RaiseAddScrollVelocityRequested(
                    additionalVelocity.X, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.X);

                if (horizontalOffsetChangeCorrelationId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: AddScrollVelocityRequest started. CorrelationId=" + horizontalOffsetChangeCorrelationId);
            }

            if (additionalVelocity.Y != 0.0f)
            {
                verticalOffsetChangeCorrelationId = verticalScrollController.RaiseAddScrollVelocityRequested(
                    additionalVelocity.Y, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.Y);

                if (verticalOffsetChangeCorrelationId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: AddScrollVelocityRequest started. CorrelationId=" + verticalOffsetChangeCorrelationId);
            }

            int offsetChangeCorrelationId = -1;

            if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId == verticalOffsetChangeCorrelationId)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (horizontalOffsetChangeCorrelationId != -1 && verticalOffsetChangeCorrelationId != -1 && horizontalOffsetChangeCorrelationId != verticalOffsetChangeCorrelationId)
            {
                RaiseLogMessage("BiDirectionalScrollController: AddScrollVelocityRequest operations do not match.");
            }
            else if (horizontalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = horizontalOffsetChangeCorrelationId;
            }
            else if (verticalOffsetChangeCorrelationId != -1)
            {
                offsetChangeCorrelationId = verticalOffsetChangeCorrelationId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: AddScrollVelocityRequest operations are null.");
            }

            if (hookupCompletion && offsetChangeCorrelationId != -1 && !lstAddScrollVelocityCorrelationIds.Contains(offsetChangeCorrelationId))
            {
                lstAddScrollVelocityCorrelationIds.Add(offsetChangeCorrelationId);
            }

            return offsetChangeCorrelationId;
        }

        internal int OperationsCount
        {
            get
            {
                return operations.Count;
            }
        }

        internal float GetInteractionElementScrollMultiplier(Orientation orientation, double scrollableExtent)
        {
            if (Thumb != null)
            {
                Thumb.UpdateLayout();

                double parentDim = 0.0;
                double thumbDim =
                    orientation == Orientation.Horizontal ? Thumb.ActualWidth : Thumb.ActualHeight;
                FrameworkElement parent = Thumb.Parent as FrameworkElement;
                if (parent != null)
                {
                    parentDim = orientation == Orientation.Horizontal ? parent.ActualWidth : parent.ActualHeight;
                }
                if (parentDim != thumbDim)
                {
                    return (float)(scrollableExtent / (thumbDim - parentDim));
                }
            }

            return 0.0f;
        }

        internal void RaiseLogMessage(string message)
        {
            if (LogMessage != null)
            {
                LogMessage(this, message);
            }
        }

        private void RaiseScrollCompleted(int viewChangeCorrelationId)
        {
            if (ScrollCompleted != null)
            {
                BiDirectionalScrollControllerScrollingScrollCompletedEventArgs args = new BiDirectionalScrollControllerScrollingScrollCompletedEventArgs(viewChangeCorrelationId);

                ScrollCompleted(this, args);
            }
        }
    }
}
