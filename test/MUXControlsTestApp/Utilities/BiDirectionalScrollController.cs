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

#if !BUILD_WINDOWS
using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeResult = Microsoft.UI.Xaml.Controls.ScrollerViewChangeResult;
using ScrollControllerInteractionRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerInteractionRequestedEventArgs;
using ScrollControllerOffsetChangeRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeRequestedEventArgs;
using ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerChangeOffsetsWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsWithAdditionalVelocityOptions;
#endif

namespace MUXControlsTestApp.Utilities
{
    public class BiDirectionalScrollControllerViewChangeCompletedEventArgs
    {
        internal BiDirectionalScrollControllerViewChangeCompletedEventArgs(int viewChangeId, ScrollerViewChangeResult result)
        {
            ViewChangeId = viewChangeId;
            Result = result;
        }

        public int ViewChangeId
        {
            get;
            set;
        }

        public ScrollerViewChangeResult Result
        {
            get;
            set;
        }
    }

    public sealed class BiDirectionalScrollController : ContentControl
    {
        private class UniScrollControllerOffsetChangeCompletedEventArgs
        {
            public UniScrollControllerOffsetChangeCompletedEventArgs(int offsetChangeId, ScrollerViewChangeResult result)
            {
                OffsetChangeId = offsetChangeId;
                Result = result;
            }

            public int OffsetChangeId
            {
                get;
                set;
            }

            public ScrollerViewChangeResult Result
            {
                get;
                set;
            }
        }

        private class UniScrollController : IScrollController
        {
            public event TypedEventHandler<IScrollController, object> InteractionInfoChanged;
            public event TypedEventHandler<IScrollController, ScrollControllerInteractionRequestedEventArgs> InteractionRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerOffsetChangeRequestedEventArgs> OffsetChangeRequested;
            public event TypedEventHandler<IScrollController, ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs> OffsetChangeWithAdditionalVelocityRequested;
            public event TypedEventHandler<IScrollController, UniScrollControllerOffsetChangeCompletedEventArgs> OffsetChangeCompleted;

            public UniScrollController(BiDirectionalScrollController owner, Orientation orientation)
            {
                Owner = owner;
                Orientation = orientation;
                MinOffset = 0.0;
                MaxOffset = 0.0;
                Offset = 0.0;
                Viewport = 0.0;
            }

            public bool AreScrollerInteractionsAllowed
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_AreScrollerInteractionsAllowed for Orientation=" + Orientation);
                    return Owner.AreScrollerInteractionsAllowed;
                }
            }

            public bool IsInteracting
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_IsInteracting for Orientation=" + Orientation);
                    return Owner.IsInteracting;
                }
            }

            public Visual InteractionVisual
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionVisual for Orientation=" + Orientation);
                    return Owner.GetParentInteractionVisual();
                }
            }

            public Orientation InteractionVisualScrollOrientation
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionVisualScrollOrientation for Orientation=" + Orientation);
                    return Orientation;
                }
            }

            public RailingMode InteractionVisualScrollRailingMode
            {
                get
                {
                    RaiseLogMessage("UniScrollController: get_InteractionVisualScrollRailingMode for Orientation=" + Orientation);
                    return Owner.IsRailing ? RailingMode.Enabled : RailingMode.Disabled;
                }
            }

            internal double MinOffset
            {
                get;
                private set;
            }

            internal double MaxOffset
            {
                get;
                private set;
            }

            internal double Offset
            {
                get;
                private set;
            }

            internal double Viewport
            {
                get;
                private set;
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

            private string MinOffsetPropertyName
            {
                get;
                set;
            }

            private string MaxOffsetPropertyName
            {
                get;
                set;
            }

            private string OffsetPropertyName
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
                CompositionPropertySet propertySet, string minOffsetPropertyName, string maxOffsetPropertyName, string offsetPropertyName, string multiplierPropertyName)
            {
                RaiseLogMessage(
                    "UniScrollController: SetExpressionAnimationSources for Orientation=" + Orientation +
                    " with minOffsetPropertyName=" + minOffsetPropertyName +
                    ", maxOffsetPropertyName=" + maxOffsetPropertyName +
                    ", offsetPropertyName=" + offsetPropertyName +
                    ", multiplierPropertyName=" + multiplierPropertyName);
                ExpressionAnimationSources = propertySet;
                if (ExpressionAnimationSources != null)
                {
                    MinOffsetPropertyName = minOffsetPropertyName.Trim();
                    MaxOffsetPropertyName = maxOffsetPropertyName.Trim();
                    OffsetPropertyName = offsetPropertyName.Trim();
                    MultiplierPropertyName = multiplierPropertyName.Trim();

                    UpdateInteractionVisualScrollMultiplier();

                    if (ThumbOffsetAnimation == null)
                    {
                        EnsureThumbAnimation();
                        StartThumbAnimation();
                    }
                }
                else
                {
                    MinOffsetPropertyName =
                    MaxOffsetPropertyName =
                    OffsetPropertyName =
                    MultiplierPropertyName = string.Empty;

                    ThumbOffsetAnimation = null;
                    StopThumbAnimation();
                }
            }

            public void SetValues(double minOffset, double maxOffset, double offset, double viewport)
            {
                RaiseLogMessage(
                    "UniScrollController: SetValues for Orientation=" + Orientation +
                    " with minOffset=" + minOffset +
                    ", maxOffset=" + maxOffset +
                    ", offset=" + offset +
                    ", viewport=" + viewport);

                if (maxOffset < minOffset)
                {
                    throw new ArgumentOutOfRangeException("maxOffset");
                }

                if (viewport < 0.0)
                {
                    throw new ArgumentOutOfRangeException("viewport");
                }

                offset = Math.Max(minOffset, offset);
                offset = Math.Min(maxOffset, offset);

                double offsetTarget = 0.0;

                if (Owner.OperationsCount == 0)
                {
                    offsetTarget = offset;
                }
                else
                {
                    offsetTarget = Math.Max(minOffset, offsetTarget);
                    offsetTarget = Math.Min(maxOffset, offsetTarget);
                }

                if (Orientation == Orientation.Horizontal)
                    Owner.OffsetTarget = new Point(offsetTarget, Owner.OffsetTarget.Y);
                else
                    Owner.OffsetTarget = new Point(Owner.OffsetTarget.X, offsetTarget);

                bool updateThumbSize =
                    MinOffset != minOffset ||
                    MaxOffset != maxOffset ||
                    Viewport != viewport;

                MinOffset = minOffset;
                Offset = offset;
                MaxOffset = maxOffset;
                Viewport = viewport;

                if (updateThumbSize)
                {
                    if (!Owner.UpdateThumbSize())
                        UpdateInteractionVisualScrollMultiplier();
                }
            }

            public CompositionAnimation GetOffsetChangeAnimation(
                Int32 offsetChangeId,
                Vector2 currentPosition,
                CompositionAnimation defaultAnimation)
            {
                RaiseLogMessage(
                    "UniScrollController: GetOffsetChangeAnimation for Orientation=" + Orientation +
                    " with offsetChangeId=" + offsetChangeId + ", currentPosition=" + currentPosition);
                return defaultAnimation;
            }

            public void OnOffsetChangeCompleted(
                Int32 offsetChangeId,
                ScrollerViewChangeResult result)
            {
                RaiseLogMessage(
                    "UniScrollController: OnOffsetChangeCompleted for Orientation=" + Orientation +
                    " with offsetChangeId=" + offsetChangeId + ", result=" + result);

                if (OffsetChangeCompleted != null)
                {
                    OffsetChangeCompleted(this, new UniScrollControllerOffsetChangeCompletedEventArgs(offsetChangeId, result));
                }
            }

            internal void UpdateInteractionVisualScrollMultiplier()
            {
                if (ExpressionAnimationSources != null && !string.IsNullOrWhiteSpace(MultiplierPropertyName))
                {
                    float interactionVisualScrollMultiplier = Owner.GetInteractionVisualScrollMultiplier(Orientation, MaxOffset, MinOffset);

                    RaiseLogMessage("UniScrollController: UpdateInteractionVisualScrollMultiplier for Orientation=" + Orientation + ", InteractionVisualScrollMultiplier=" + interactionVisualScrollMultiplier);
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

            internal int RaiseOffsetChangeRequested(
                double offset,
                ScrollerViewKind offsetKind,
                ScrollerViewChangeKind offsetChangeKind)
            {
                RaiseLogMessage("UniScrollController: RaiseOffsetChangeRequested for Orientation=" + Orientation + " with offset=" + offset + ", offsetKind=" + offsetKind + ", offsetChangeKind=" + offsetChangeKind);

                if (OffsetChangeRequested != null)
                {
                    ScrollControllerOffsetChangeRequestedEventArgs e =
                        new ScrollControllerOffsetChangeRequestedEventArgs(
                            offset,
                            offsetKind,
                            offsetChangeKind);
                    OffsetChangeRequested(this, e);
                    return e.ViewChangeId;
                }
                return -1;
            }

            internal int RaiseOffsetChangeWithAdditionalVelocityRequested(
                float additionalVelocity, float? inertiaDecayRate)
            {
                RaiseLogMessage("UniScrollController: RaiseOffsetChangeWithAdditionalVelocityRequested for Orientation=" + Orientation + " with additionalVelocity=" + additionalVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

                if (OffsetChangeWithAdditionalVelocityRequested != null)
                {
                    ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs e =
                        new ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs(
                            additionalVelocity,
                            inertiaDecayRate);
                    OffsetChangeWithAdditionalVelocityRequested(this, e);
                    return e.ViewChangeId;
                }
                return -1;
            }

            private void EnsureThumbAnimation()
            {
                if (ThumbOffsetAnimation == null && ExpressionAnimationSources != null &&
                    !string.IsNullOrWhiteSpace(MultiplierPropertyName) &&
                    !string.IsNullOrWhiteSpace(OffsetPropertyName) &&
                    !string.IsNullOrWhiteSpace(MinOffsetPropertyName) &&
                    !string.IsNullOrWhiteSpace(MaxOffsetPropertyName))
                {
                    ThumbOffsetAnimation = ExpressionAnimationSources.Compositor.CreateExpressionAnimation();
                    ThumbOffsetAnimation.Expression =
                        "min(eas." + MaxOffsetPropertyName + ",max(eas." + MinOffsetPropertyName + ",eas." + OffsetPropertyName + "))/(-eas." + MultiplierPropertyName + ")";
                    ThumbOffsetAnimation.SetReferenceParameter("eas", ExpressionAnimationSources);
                }
            }

            private void StartThumbAnimation()
            {
                if (Owner.InteractionVisual != null && ThumbOffsetAnimation != null)
                {
                    if (Orientation == Orientation.Horizontal)
                    {
                        Owner.InteractionVisual.StartAnimation("Translation.X", ThumbOffsetAnimation);
                    }
                    else
                    {
                        Owner.InteractionVisual.StartAnimation("Translation.Y", ThumbOffsetAnimation);
                    }
                }
            }

            private void StopThumbAnimation()
            {
                if (Owner.InteractionVisual != null)
                {
                    if (Orientation == Orientation.Horizontal)
                    {
                        Owner.InteractionVisual.StopAnimation("Translation.X");
                    }
                    else
                    {
                        Owner.InteractionVisual.StopAnimation("Translation.Y");
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
            public int Id;
            public Point RelativeOffsetChange;
            public Point OffsetTarget;

            public OperationInfo(int id, Point relativeOffsetChange, Point offsetTarget) : this()
            {
                Id = id;
                RelativeOffsetChange = relativeOffsetChange;
                OffsetTarget = offsetTarget;
            }
        }

        private const float SmallChangeAdditionalVelocity = 144.0f;
        private const float SmallChangeInertiaDecayRate = 0.975f;

        private List<string> lstAsyncEventMessage = new List<string>();
        private List<int> lstViewChangeIds = new List<int>();
        private List<int> lstOffsetChangeIds = new List<int>();
        private List<int> lstOffsetChangeWithAdditionalVelocityIds = new List<int>();
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
        private bool isThumbDragged = false;
        private bool isRailing = true;
        private Point preManipulationThumbOffset;

        public event TypedEventHandler<BiDirectionalScrollController, string> LogMessage;
        public event TypedEventHandler<BiDirectionalScrollController, BiDirectionalScrollControllerViewChangeCompletedEventArgs> ViewChangeCompleted;

        public BiDirectionalScrollController()
        {
            this.DefaultStyleKey = typeof(BiDirectionalScrollController);
            AreScrollerInteractionsAllowed = true;
            this.horizontalScrollController = new UniScrollController(this, Orientation.Horizontal);
            this.verticalScrollController = new UniScrollController(this, Orientation.Vertical);

            this.horizontalScrollController.OffsetChangeCompleted += UniScrollController_OffsetChangeCompleted;
            this.verticalScrollController.OffsetChangeCompleted += UniScrollController_OffsetChangeCompleted;

            IsEnabledChanged += BiDirectionalScrollController_IsEnabledChanged;
            SizeChanged += BiDirectionalScrollController_SizeChanged;
        }

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

        public int ChangeOffsets(ScrollerChangeOffsetsOptions options)
        {
            int viewChangeId = RaiseOffsetChangeRequested(
                new Point(options.HorizontalOffset, options.VerticalOffset),
                options.OffsetsKind,
                options.ViewChangeKind, false /*hookupCompletion*/);
            lstViewChangeIds.Add(viewChangeId);
            return viewChangeId;
        }

        public int ChangeOffsetsWithAdditionalVelocity(ScrollerChangeOffsetsWithAdditionalVelocityOptions options)
        {
            int viewChangeId = RaiseOffsetChangeWithAdditionalVelocityRequested(
                options.AdditionalVelocity, options.InertiaDecayRate, false /*hookupCompletion*/);
            lstViewChangeIds.Add(viewChangeId);
            return viewChangeId;
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
                if (IsRailing)
                {
                    Thumb.ManipulationMode |= ManipulationModes.TranslateRailsX | ManipulationModes.TranslateRailsY;
                }

                InteractionVisual = ElementCompositionPreview.GetElementVisual(Thumb);
                ElementCompositionPreview.SetIsTranslationEnabled(Thumb, true);
            }
            else
            {
                InteractionVisual = null;
            }

            HookHandlers();

            RaiseInteractionInfoChanged();
        }

        internal Visual GetParentInteractionVisual()
        {
            RaiseLogMessage("BiDirectionalScrollController: GetParentInteractionVisual");
            return (Thumb != null && Thumb.Parent != null) ?
                ElementCompositionPreview.GetElementVisual(Thumb.Parent as UIElement) : null;
        }

        internal bool AreScrollerInteractionsAllowed
        {
            get;
            private set;
        }

        internal bool IsInteracting
        {
            get;
            private set;
        }

        internal Visual InteractionVisual
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
                    if (horizontalScrollController.MaxOffset != horizontalScrollController.MinOffset)
                    {
                        return horizontalScrollController.Offset / (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset) * (parentWidth - Thumb.Width);
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
                    if (verticalScrollController.MaxOffset != verticalScrollController.MinOffset)
                    {
                        return verticalScrollController.Offset / (verticalScrollController.MaxOffset - verticalScrollController.MinOffset) * (parentHeight - Thumb.Height);
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

            int viewChangeId = 
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void BiIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: BiIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementVerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementVerticalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(SmallChangeAdditionalVelocity, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementVerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementVerticalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(-SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void HorizontalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: HorizontalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(SmallChangeAdditionalVelocity, 0), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalDecrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalDecrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(0, -SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
        }

        private void VerticalIncrementRepeatButton_Click(object sender, RoutedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: VerticalIncrementRepeatButton_Click");

            int viewChangeId =
                RaiseOffsetChangeWithAdditionalVelocityRequested(new Vector2(0, SmallChangeAdditionalVelocity), new Vector2(SmallChangeInertiaDecayRate), true /*hookupCompletion*/);
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
                    isThumbDragged = true;
                    AreScrollerInteractionsAllowed = false;
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
            Point targetScrollerOffset = ScrollerOffsetFromThumbOffset(targetThumbOffset);

            int viewChangeId = RaiseOffsetChangeRequested(targetScrollerOffset, ScrollerViewKind.Absolute, ScrollerViewChangeKind.AllowAnimation, true /*hookupCompletion*/);
            if (viewChangeId != -1)
            {
                operations.Add(
                    viewChangeId, 
                    new OperationInfo(viewChangeId, new Point(targetScrollerOffset.X - HorizontalThumbOffset, targetScrollerOffset.Y - VerticalThumbOffset), targetScrollerOffset));
            }
        }

        private void Thumb_ManipulationStarting(object sender, ManipulationStartingRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                preManipulationThumbOffset = new Point(HorizontalThumbOffset, VerticalThumbOffset);
            }
        }

        private void Thumb_ManipulationDelta(object sender, ManipulationDeltaRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                Point targetThumbOffset = new Point(
                    preManipulationThumbOffset.X + e.Cumulative.Translation.X,
                    preManipulationThumbOffset.Y + e.Cumulative.Translation.Y);
                Point scrollerOffset = ScrollerOffsetFromThumbOffset(targetThumbOffset);

                int viewChangeId = RaiseOffsetChangeRequested(
                    scrollerOffset, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, true /*hookupCompletion*/);
            }
        }

        private void Thumb_ManipulationCompleted(object sender, ManipulationCompletedRoutedEventArgs e)
        {
            if (isThumbDragged)
            {
                isThumbDragged = false;
                AreScrollerInteractionsAllowed = true;
            }
        }

        private void BiDirectionalScrollController_IsEnabledChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            RaiseLogMessage("BiDirectionalScrollController: IsEnabledChanged with IsEnabled=" + IsEnabled);
            RaiseInteractionInfoChanged();
        }

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
                    newSize.Width = Math.Max(Math.Min(40.0, parentSize.Width), horizontalScrollController.Viewport / (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset + horizontalScrollController.Viewport) * parentSize.Width);
                }
                if (verticalScrollController.Viewport == 0.0)
                {
                    newSize.Height = 40.0;
                }
                else
                {
                    newSize.Height = Math.Max(Math.Min(40.0, parentSize.Height), verticalScrollController.Viewport / (verticalScrollController.MaxOffset - verticalScrollController.MinOffset + verticalScrollController.Viewport) * parentSize.Height);
                }
                if (newSize.Width != Thumb.Width)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Horizontal, setting Width=" + newSize.Width);
                    Thumb.Width = newSize.Width;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateHorizontalInteractionVisualScrollMultiplier);
                    return true;
                }
                if (newSize.Height != Thumb.Height)
                {
                    RaiseLogMessage("BiDirectionalScrollController: UpdateThumbSize for Orientation=Vertical, setting Height=" + newSize.Height);
                    Thumb.Height = newSize.Height;
                    var ignored = Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Low, UpdateVerticalInteractionVisualScrollMultiplier);
                    return true;
                }
            }
            return false;
        }

        private void UpdateHorizontalInteractionVisualScrollMultiplier()
        {
            horizontalScrollController.UpdateInteractionVisualScrollMultiplier();
        }

        private void UpdateVerticalInteractionVisualScrollMultiplier()
        {
            verticalScrollController.UpdateInteractionVisualScrollMultiplier();
        }

        private Point ScrollerOffsetFromThumbOffset(Point thumbOffset)
        {
            Point scrollerOffset = new Point();

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
                    scrollerOffset = new Point(
                        parentSize.Width == thumbSize.Width ? 0 : (thumbOffset.X * (horizontalScrollController.MaxOffset - horizontalScrollController.MinOffset) / (parentSize.Width - thumbSize.Width)),
                        parentSize.Height == thumbSize.Height ? 0 : (thumbOffset.Y * (verticalScrollController.MaxOffset - verticalScrollController.MinOffset) / (parentSize.Height - thumbSize.Height)));
                }
            }

            return scrollerOffset;
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

        private void UniScrollController_OffsetChangeCompleted(IScrollController sender, UniScrollControllerOffsetChangeCompletedEventArgs args)
        {
            if (lstOffsetChangeIds.Contains(args.OffsetChangeId))
            {
                lstOffsetChangeIds.Remove(args.OffsetChangeId);

                Point relativeOffsetChange;

                if (operations.ContainsKey(args.OffsetChangeId))
                {
                    OperationInfo oi = operations[args.OffsetChangeId];
                    relativeOffsetChange = oi.RelativeOffsetChange;
                    operations.Remove(args.OffsetChangeId);
                }

                if (args.Result == ScrollerViewChangeResult.Ignored)
                {
                    OffsetTarget = new Point(OffsetTarget.X - relativeOffsetChange.X, OffsetTarget.Y - relativeOffsetChange.Y);
                }

                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeRequest completed. OffsetChangeId=" + args.OffsetChangeId + ", Result=" + args.Result);
            }
            else if (lstOffsetChangeWithAdditionalVelocityIds.Contains(args.OffsetChangeId))
            {
                lstOffsetChangeWithAdditionalVelocityIds.Remove(args.OffsetChangeId);

                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeWithAdditionalVelocityRequest completed. OffsetChangeId=" + args.OffsetChangeId + ", Result=" + args.Result);
            }

            if (lstViewChangeIds.Contains(args.OffsetChangeId))
            {
                lstViewChangeIds.Remove(args.OffsetChangeId);

                RaiseViewChangeCompleted(args.OffsetChangeId, args.Result);
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

        private int RaiseOffsetChangeRequested(
            Point offset,
            ScrollerViewKind offsetKind,
            ScrollerViewChangeKind offsetChangeKind,
            bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseOffsetChangeRequested with offset=" + offset + ", offsetKind=" + offsetKind + ", offsetChangeKind=" + offsetChangeKind);

            int horizontalOffsetChangeId = horizontalScrollController.RaiseOffsetChangeRequested(
                offset.X, offsetKind, offsetChangeKind);

            if (horizontalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Horizontal OffsetChangeRequest started. Id=" + horizontalOffsetChangeId);

            int verticalOffsetChangeId = verticalScrollController.RaiseOffsetChangeRequested(
                offset.Y, offsetKind, offsetChangeKind);

            if (verticalOffsetChangeId != -1)
                RaiseLogMessage("BiDirectionalScrollController: Vertical OffsetChangeRequest started. Id=" + verticalOffsetChangeId);

            int offsetChangeId = -1;

            if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId == verticalOffsetChangeId)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId != verticalOffsetChangeId)
            {
                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeRequest Ids do not match.");
            }
            else if (horizontalOffsetChangeId != -1)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (verticalOffsetChangeId != -1)
            {
                offsetChangeId = verticalOffsetChangeId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeRequest Ids are -1.");
            }

            if (hookupCompletion && offsetChangeId != -1 && !lstOffsetChangeIds.Contains(offsetChangeId))
            {
                lstOffsetChangeIds.Add(offsetChangeId);
            }

            return offsetChangeId;
        }

        private int RaiseOffsetChangeWithAdditionalVelocityRequested(
            Vector2 additionalVelocity, Vector2? inertiaDecayRate, bool hookupCompletion)
        {
            RaiseLogMessage("BiDirectionalScrollController: RaiseOffsetChangeWithAdditionalVelocityRequested with additionalVelocity=" + additionalVelocity + ", inertiaDecayRate=" + inertiaDecayRate);

            int horizontalOffsetChangeId = -1;
            int verticalOffsetChangeId = -1;

            if (additionalVelocity.X != 0.0f)
            {
                horizontalOffsetChangeId = horizontalScrollController.RaiseOffsetChangeWithAdditionalVelocityRequested(
                    additionalVelocity.X, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.X);

                if (horizontalOffsetChangeId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: OffsetChangeWithAdditionalVelocityRequest started. Id=" + horizontalOffsetChangeId);
            }

            if (additionalVelocity.Y != 0.0f)
            {
                verticalOffsetChangeId = verticalScrollController.RaiseOffsetChangeWithAdditionalVelocityRequested(
                    additionalVelocity.Y, inertiaDecayRate == null ? (float?)null : inertiaDecayRate.Value.Y);

                if (verticalOffsetChangeId != -1)
                    RaiseLogMessage("BiDirectionalScrollController: OffsetChangeWithAdditionalVelocityRequest started. Id=" + verticalOffsetChangeId);
            }

            int offsetChangeId = -1;

            if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId == verticalOffsetChangeId)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (horizontalOffsetChangeId != -1 && verticalOffsetChangeId != -1 && horizontalOffsetChangeId != verticalOffsetChangeId)
            {
                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeWithAdditionalVelocityRequest operations do not match.");
            }
            else if (horizontalOffsetChangeId != -1)
            {
                offsetChangeId = horizontalOffsetChangeId;
            }
            else if (verticalOffsetChangeId != -1)
            {
                offsetChangeId = verticalOffsetChangeId;
            }
            else
            {
                RaiseLogMessage("BiDirectionalScrollController: OffsetChangeWithAdditionalVelocityRequest operations are null.");
            }

            if (hookupCompletion && offsetChangeId != -1 && !lstOffsetChangeWithAdditionalVelocityIds.Contains(offsetChangeId))
            {
                lstOffsetChangeWithAdditionalVelocityIds.Add(offsetChangeId);
            }

            return offsetChangeId;
        }

        internal int OperationsCount
        {
            get
            {
                return operations.Count;
            }
        }

        internal float GetInteractionVisualScrollMultiplier(Orientation orientation, double maxOffset, double minOffset)
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
                    return (float)((maxOffset - minOffset) / (thumbDim - parentDim));
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

        private void RaiseViewChangeCompleted(int viewChangeId, ScrollerViewChangeResult result)
        {
            if (ViewChangeCompleted != null)
            {
                BiDirectionalScrollControllerViewChangeCompletedEventArgs args = new BiDirectionalScrollControllerViewChangeCompletedEventArgs(viewChangeId, result);

                ViewChangeCompleted(this, args);
            }
        }
    }
}
