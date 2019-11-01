// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Composition.Interactions;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollingContentOrientation = Microsoft.UI.Xaml.Controls.ScrollingContentOrientation;
using ScrollingChainMode = Microsoft.UI.Xaml.Controls.ScrollingChainMode;
using ScrollingRailMode = Microsoft.UI.Xaml.Controls.ScrollingRailMode;
using ScrollingScrollMode = Microsoft.UI.Xaml.Controls.ScrollingScrollMode;
using ScrollingZoomMode = Microsoft.UI.Xaml.Controls.ScrollingZoomMode;
using ScrollingInputKinds = Microsoft.UI.Xaml.Controls.ScrollingInputKinds;
using ScrollingAnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using RepeatedScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint;
using ZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint;
using RepeatedZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ScrollingZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomAnimationStartingEventArgs;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterDynamicPage : TestPage
    {
        private enum QueuedOperationType
        {
            SetMinZoomFactor,
            SetMaxZoomFactor,
            SetWidth,
            SetHeight,
            SetContentMargin,
            SetContentPadding,
            SetContentMinWidth,
            SetContentMinHeight,
            SetContentWidth,
            SetContentHeight,
            SetContentMaxWidth,
            SetContentMaxHeight,
        }

        private class QueuedOperation
        {
            public QueuedOperation(QueuedOperationType type, double value)
            {
                this.Type = type;
                this.DoubleValue = value;
            }
            public QueuedOperation(QueuedOperationType type, string value)
            {
                this.Type = type;
                this.StringValue = value;
            }

            public QueuedOperationType Type { get; set; }
            public double DoubleValue { get; set; }
            public string StringValue { get; set; }
        }

        private int lastOffsetsChangeId = -1;
        private int lastOffsetsChangeWithAdditionalVelocityId = -1;
        private int lastOffsetsChangeWithVelocityId = -1;
        private int lastZoomFactorChangeId = -1;
        private int lastZoomFactorChangeWithAdditionalVelocityId = -1;
        private HashSet<int> relativeChangeIds = new HashSet<int>();
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<QueuedOperation> lstQueuedOperations = new List<QueuedOperation>();
        private Viewbox viewbox;
        private TilePanel tilePanel;
        private Image largeImg;
        private Rectangle rectangle;
        private Button button;
        private Border border;
        private Border populatedBorder;
        private StackPanel verticalStackPanel;
        private ScrollingPresenter dynamicScrollingPresenter = null;
        private ScrollingPresenter scrollingPresenter = null;

        public ScrollingPresenterDynamicPage()
        {
            this.InitializeComponent();

            UseScrollingPresenter(markupScrollingPresenter);

            CreateChildren();
        }

        private void ScrollingPresenter_ExtentChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("StateChanged " + sender.State.ToString());
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollingPresenter_ViewChangedForVelocitySpying(ScrollingPresenter sender, object args)
        {
            Vector3 positionVelocityInPixelsPerSecond;
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
            CompositionGetValueStatus status = CompositionPropertySpy.TryGetVector3(interactionTracker, "PositionVelocityInPixelsPerSecond", out positionVelocityInPixelsPerSecond);
            AppendAsyncEventMessage("InteractionTracker.PositionVelocityInPixelsPerSecond=" + positionVelocityInPixelsPerSecond.X + ", " + positionVelocityInPixelsPerSecond.Y);
            AppendAsyncEventMessage("InteractionTracker.PositionInertiaDecayRate=" + interactionTracker.PositionInertiaDecayRate + ", InteractionTracker.ScaleInertiaDecayRate=" + interactionTracker.ScaleInertiaDecayRate);
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage("ScrollCompleted OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
        }

        private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

            AppendAsyncEventMessage("ZoomCompleted ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
        }

        private void CreateChildren()
        {
            viewbox = new Viewbox();
            tilePanel = new TilePanel();
            tilePanel.TileCount = 100;
            tilePanel.Background = new SolidColorBrush(Colors.Orange);
            largeImg = new Image() { Source = new BitmapImage(new Uri("ms-appx:/Assets/LargeWisteria.jpg")) };
            SolidColorBrush chartreuseBrush = new SolidColorBrush(Colors.Chartreuse);
            SolidColorBrush blanchedAlmondBrush = new SolidColorBrush(Colors.BlanchedAlmond);
            SolidColorBrush interBrush = new SolidColorBrush(Colors.Yellow);
            LinearGradientBrush lgb = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };
            GradientStop gs = new GradientStop() { Color = Colors.Blue, Offset = 0.0 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Colors.White, Offset = 0.5 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Colors.Red, Offset = 1.0 };
            lgb.GradientStops.Add(gs);
            rectangle = new Rectangle() { Fill = lgb };
            button = new Button() { Content = "Button" };
            Rectangle viewboxChild = new Rectangle() {
                Fill = lgb,
                Width = 600,
                Height = 500
            };
            viewbox.Child = viewboxChild;
            Rectangle borderChild = new Rectangle()
            {
                Fill = lgb
            };
            border = new Border()
            {
                BorderBrush = chartreuseBrush, BorderThickness = new Thickness(5), Child = borderChild
            };
            StackPanel populatedBorderChild = new StackPanel();
            populatedBorderChild.Margin = new Thickness(30);
            for (int i = 0; i < 16; i++)
            {
                TextBlock textBlock = new TextBlock()
                {
                    Text = "TB#" + populatedBorderChild.Children.Count,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center
                };

                Border borderInSP = new Border()
                {
                    BorderThickness = border.Margin = new Thickness(3),
                    BorderBrush = chartreuseBrush,
                    Background = blanchedAlmondBrush,
                    Width = 170,
                    Height = 120,
                    Child = textBlock
                };

                populatedBorderChild.Children.Add(borderInSP);
            }
            populatedBorder = new Border()
            {
                BorderBrush = chartreuseBrush, BorderThickness = new Thickness(3), Margin = new Thickness(15),
                Background = new SolidColorBrush(Colors.Beige), Child = populatedBorderChild
            };
            verticalStackPanel = new StackPanel();
            for (int index = 0; index < 10; index++)
            {
                verticalStackPanel.Children.Add(new Rectangle() { Fill = lgb, Height = 95 });
                verticalStackPanel.Children.Add(new Rectangle() { Fill = interBrush, Height = 5 });
            }
        }

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for a particular ScrollingPresenter instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scrollingPresenter, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for a particular ScrollingPresenter instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scrollingPresenter, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            scrollingPresenter.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            scrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
            scrollingPresenter.ZoomAnimationStarting += ScrollingPresenter_ZoomAnimationStarting;
        }

        private void ChkLogScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted -= ScrollingPresenter_ScrollCompleted;
            scrollingPresenter.ZoomCompleted -= ScrollingPresenter_ZoomCompleted;
            scrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
            scrollingPresenter.ZoomAnimationStarting -= ScrollingPresenter_ZoomAnimationStarting;
        }

        private void ChkLogInteractionTrackerInfo_Checked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
            CompositionPropertySpy.StartSpyingVector3Property(interactionTracker, "PositionVelocityInPixelsPerSecond", Vector3.Zero);
            scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChangedForVelocitySpying;
        }

        private void ChkLogInteractionTrackerInfo_Unchecked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
            CompositionPropertySpy.StopSpyingProperty(interactionTracker, "PositionVelocityInPixelsPerSecond");
            scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChangedForVelocitySpying;
        }

        private void ChkLogContentEffectiveViewportChangedEvent_Checked(object sender, RoutedEventArgs e)
        {
            HookContentEffectiveViewportChanged();
        }

        private void ChkLogContentEffectiveViewportChangedEvent_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookContentEffectiveViewportChanged();
        }

        private void HookContentEffectiveViewportChanged()
        {
            if (scrollingPresenter != null)
            {
                FrameworkElement contentAsFE = scrollingPresenter.Content as FrameworkElement;
                if (contentAsFE != null)
                {
                    contentAsFE.EffectiveViewportChanged += Content_EffectiveViewportChanged;
                }
            }
        }

        private void UnhookContentEffectiveViewportChanged()
        {
            if (scrollingPresenter != null)
            {
                FrameworkElement contentAsFE = scrollingPresenter.Content as FrameworkElement;
                if (contentAsFE != null)
                {
                    contentAsFE.EffectiveViewportChanged -= Content_EffectiveViewportChanged;
                }
            }
        }

        private void Content_EffectiveViewportChanged(FrameworkElement sender, EffectiveViewportChangedEventArgs args)
        {
            AppendAsyncEventMessage("Content_EffectiveViewportChanged: BringIntoViewDistance=" + 
                args.BringIntoViewDistanceX + "," + args.BringIntoViewDistanceY + ", EffectiveViewport=" +
                args.EffectiveViewport.ToString() + ", MaxViewport=" + args.MaxViewport.ToString());
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
                AppendAsyncEventMessage("Warning: Failure while accessing sender's Name");
            }

            if (args.IsVerboseLevel)
            {
                AppendAsyncEventMessage("Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                AppendAsyncEventMessage("Info: " + senderName + "m:" + msg);
            }
        }

        private void ChkScrollingPresenterProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollingPresenterProperties != null)
                svScrollingPresenterProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollingPresenterProperties != null)
                svScrollingPresenterProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkContentProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdContentProperties != null)
                grdContentProperties.Visibility = Visibility.Visible;
        }

        private void ChkContentProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdContentProperties != null)
                grdContentProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollingPresenterMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollingPresenterMethods != null)
                svScrollingPresenterMethods.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollingPresenterMethods != null)
                svScrollingPresenterMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterEvents != null)
                grdScrollingPresenterEvents.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterEvents != null)
                grdScrollingPresenterEvents.Visibility = Visibility.Collapsed;
        }

        private void ChkPageMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (grdPageMethods != null)
                grdPageMethods.Visibility = Visibility.Visible;
        }

        private void ChkPageMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdPageMethods != null)
                grdPageMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollingPresenterTestHooks_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterTestHooks != null)
                grdScrollingPresenterTestHooks.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterTestHooks_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterTestHooks != null)
                grdScrollingPresenterTestHooks.Visibility = Visibility.Collapsed;
        }

        private void CmbContentOrientation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.ContentOrientation = (ScrollingContentOrientation)cmbContentOrientation.SelectedIndex;
        }

        private void CmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.HorizontalScrollMode = (ScrollingScrollMode)cmbHorizontalScrollMode.SelectedIndex;
        }

        private void UpdateCmbContentOrientation()
        {
            try
            {
                cmbContentOrientation.SelectedIndex = (int)scrollingPresenter.ContentOrientation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbHorizontalScrollMode()
        {
            cmbHorizontalScrollMode.SelectedIndex = (int)scrollingPresenter.HorizontalScrollMode;
        }

        private void CmbHorizontalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.HorizontalScrollChainMode = (ScrollingChainMode)cmbHorizontalScrollChainMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollChainMode()
        {
            cmbHorizontalScrollChainMode.SelectedIndex = (int)scrollingPresenter.HorizontalScrollChainMode;
        }

        private void CmbHorizontalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.HorizontalScrollRailMode = (ScrollingRailMode)cmbHorizontalScrollRailMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollRailMode()
        {
            cmbHorizontalScrollRailMode.SelectedIndex = (int)scrollingPresenter.HorizontalScrollRailMode;
        }

        private void CmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.VerticalScrollMode = (ScrollingScrollMode)cmbVerticalScrollMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollMode()
        {
            cmbVerticalScrollMode.SelectedIndex = (int)scrollingPresenter.VerticalScrollMode;
        }

        private void CmbVerticalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.VerticalScrollChainMode = (ScrollingChainMode)cmbVerticalScrollChainMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollChainMode()
        {
            cmbVerticalScrollChainMode.SelectedIndex = (int)scrollingPresenter.VerticalScrollChainMode;
        }

        private void CmbVerticalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.VerticalScrollRailMode = (ScrollingRailMode)cmbVerticalScrollRailMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollRailMode()
        {
            cmbVerticalScrollRailMode.SelectedIndex = (int)scrollingPresenter.VerticalScrollRailMode;
        }

        private void CmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.ZoomMode = (ScrollingZoomMode)cmbZoomMode.SelectedIndex;
        }

        private void UpdateCmbZoomMode()
        {
            cmbZoomMode.SelectedIndex = (int)scrollingPresenter.ZoomMode;
        }

        private void CmbZoomChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
                scrollingPresenter.ZoomChainMode = (ScrollingChainMode)cmbZoomChainMode.SelectedIndex;
        }

        private void UpdateCmbZoomChainMode()
        {
            cmbZoomChainMode.SelectedIndex = (int)scrollingPresenter.ZoomChainMode;
        }

        private void CmbIgnoredInputKind_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter != null)
            {
                switch (cmbIgnoredInputKind.SelectedIndex)
                {
                    case 0:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.None;
                        break;
                    case 1:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.Touch;
                        break;
                    case 2:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.Pen;
                        break;
                    case 3:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.MouseWheel;
                        break;
                    case 4:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.Keyboard;
                        break;
                    case 5:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.Gamepad;
                        break;
                    case 6:
                        scrollingPresenter.IgnoredInputKind = ScrollingInputKinds.All;
                        break;
                }
            }
        }

        private void UpdateCmbIgnoredInputKind()
        {
            switch (scrollingPresenter.IgnoredInputKind)
            {
                case ScrollingInputKinds.None:
                    cmbIgnoredInputKind.SelectedIndex = 0;
                    break;
                case ScrollingInputKinds.Touch:
                    cmbIgnoredInputKind.SelectedIndex = 1;
                    break;
                case ScrollingInputKinds.Pen:
                    cmbIgnoredInputKind.SelectedIndex = 2;
                    break;
                case ScrollingInputKinds.MouseWheel:
                    cmbIgnoredInputKind.SelectedIndex = 3;
                    break;
                case ScrollingInputKinds.Keyboard:
                    cmbIgnoredInputKind.SelectedIndex = 4;
                    break;
                case ScrollingInputKinds.Gamepad:
                    cmbIgnoredInputKind.SelectedIndex = 5;
                    break;
                case ScrollingInputKinds.All:
                    cmbIgnoredInputKind.SelectedIndex = 6;
                    break;
            }
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollingPresenter.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentHorizontalAlignment()
        {
            if (scrollingPresenter.Content is FrameworkElement)
            {
                cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollingPresenter.Content).HorizontalAlignment;
            }
        }

        private void CmbContentVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollingPresenter.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentVerticalAlignment()
        {
            if (scrollingPresenter.Content is FrameworkElement)
            {
                cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollingPresenter.Content).VerticalAlignment;
            }
        }

        private void CmbScrollingPresenterManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbScrollingPresenterManipulationMode.SelectedIndex)
            {
                case 0:
                    scrollingPresenter.ManipulationMode = ManipulationModes.System;
                    break;
                case 1:
                    scrollingPresenter.ManipulationMode = ManipulationModes.None;
                    break;
            }
        }

        private void UpdateCmbScrollingPresenterManipulationMode()
        {
            switch (scrollingPresenter.ManipulationMode)
            {
                case ManipulationModes.System:
                    cmbScrollingPresenterManipulationMode.SelectedIndex = 0;
                    break;
                case ManipulationModes.None:
                    cmbScrollingPresenterManipulationMode.SelectedIndex = 1;
                    break;
            }
        }

        private void CmbContentManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollingPresenter.Content is FrameworkElement)
            {
                switch (cmbContentManipulationMode.SelectedIndex)
                {
                    case 0:
                        scrollingPresenter.Content.ManipulationMode = ManipulationModes.System;
                        break;
                    case 1:
                        scrollingPresenter.Content.ManipulationMode = ManipulationModes.None;
                        break;
                }
            }
        }

        private void UpdateCmbContentManipulationMode()
        {
            if (scrollingPresenter.Content != null)
            {
                switch (scrollingPresenter.Content.ManipulationMode)
                {
                    case ManipulationModes.System:
                        cmbContentManipulationMode.SelectedIndex = 0;
                        break;
                    case ManipulationModes.None:
                        cmbContentManipulationMode.SelectedIndex = 1;
                        break;
                }
            }
        }

        private void CmbBackground_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbBackground.SelectedIndex)
            {
                case 0:
                    scrollingPresenter.Background = null;
                    break;
                case 1:
                    scrollingPresenter.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scrollingPresenter.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scrollingPresenter.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }
        }

        private void UpdateCmbBackground()
        {
            SolidColorBrush bg = scrollingPresenter.Background as SolidColorBrush;

            if (bg == null)
            {
                cmbBackground.SelectedIndex = 0;
            }
            else if (bg.Color == Colors.Transparent)
            {
                cmbBackground.SelectedIndex = 1;
            }
            else if (bg.Color == Colors.AliceBlue)
            {
                cmbBackground.SelectedIndex = 2;
            }
            else if (bg.Color == Colors.Aqua)
            {
                cmbBackground.SelectedIndex = 3;
            }
        }

        private void CmbContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentContent = scrollingPresenter.Content as FrameworkElement;
                FrameworkElement newContent = null;

                switch (cmbContent.SelectedIndex)
                {
                    case 0:
                        newContent = null;
                        break;
                    case 1:
                        newContent = smallImg;
                        break;
                    case 2:
                        newContent = largeImg;
                        break;
                    case 3:
                        newContent = rectangle;
                        break;
                    case 4:
                        newContent = button;
                        break;
                    case 5:
                        newContent = border;
                        break;
                    case 6:
                        newContent = populatedBorder;
                        break;
                    case 7:
                        newContent = verticalStackPanel;
                        break;
                    case 8:
                        newContent = tilePanel;
                        break;
                    case 9:
                        newContent = viewbox;
                        break;
                }

                if (chkPreserveProperties.IsChecked == true && currentContent != null && newContent != null)
                {
                    newContent.Width = currentContent.Width;
                    newContent.Height = currentContent.Height;
                    newContent.Margin = currentContent.Margin;
                    newContent.HorizontalAlignment = currentContent.HorizontalAlignment;
                    newContent.VerticalAlignment = currentContent.VerticalAlignment;

                    if (currentContent is Control && newContent is Control)
                    {
                        ((Control)newContent).Padding = ((Control)currentContent).Padding;
                    }
                }

                if (chkLogScrollingPresenterEvents.IsChecked == true)
                {
                    UnhookContentEffectiveViewportChanged();
                }

                scrollingPresenter.Content = newContent;

                if (chkLogScrollingPresenterEvents.IsChecked == true)
                {
                    HookContentEffectiveViewportChanged();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());

                UpdateCmbContent();
            }
        }

        private void UpdateCmbContent()
        {
            if (scrollingPresenter.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollingPresenter.Content is Image)
            {
                if (((scrollingPresenter.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollingPresenter.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollingPresenter.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollingPresenter.Content is Border)
            {
                if ((scrollingPresenter.Content as Border).Child is Rectangle)
                {
                    cmbContent.SelectedIndex = 5;
                }
                else
                {
                    cmbContent.SelectedIndex = 6;
                }
            }
            else if (scrollingPresenter.Content is StackPanel)
            {
                cmbContent.SelectedIndex = 7;
            }
            else if (scrollingPresenter.Content is TilePanel)
            {
                cmbContent.SelectedIndex = 8;
            }
            else if (scrollingPresenter.Content is Viewbox)
            {
                cmbContent.SelectedIndex = 9;
            }
        }

        private void BtnGetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentMinWidth.Text = string.Empty;
            else
                txtContentMinWidth.Text = ((FrameworkElement)(scrollingPresenter.Content)).MinWidth.ToString();
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scrollingPresenter.Content)).Width.ToString();
        }

        private void BtnGetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentMaxWidth.Text = string.Empty;
            else
                txtContentMaxWidth.Text = ((FrameworkElement)(scrollingPresenter.Content)).MaxWidth.ToString();
        }

        private void BtnSetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).MinWidth = Convert.ToDouble(txtContentMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).MaxWidth = Convert.ToDouble(txtContentMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentMinWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentMinWidth, value));
                AppendAsyncEventMessage("Queued SetContentMinWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentWidth, value));
                AppendAsyncEventMessage("Queued SetContentWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentMaxWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentMaxWidth, value));
                AppendAsyncEventMessage("Queued SetContentMaxWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentMinHeight.Text = string.Empty;
            else
                txtContentMinHeight.Text = ((FrameworkElement)(scrollingPresenter.Content)).MinHeight.ToString();
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scrollingPresenter.Content)).Height.ToString();
        }

        private void BtnGetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentMaxHeight.Text = string.Empty;
            else
                txtContentMaxHeight.Text = ((FrameworkElement)(scrollingPresenter.Content)).MaxHeight.ToString();
        }

        private void BtnSetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).MinHeight = Convert.ToDouble(txtContentMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).MaxHeight = Convert.ToDouble(txtContentMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentMinHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentMinHeight, value));
                AppendAsyncEventMessage("Queued SetContentMinHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentHeight, value));
                AppendAsyncEventMessage("Queued SetContentHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtContentMaxHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentMaxHeight, value));
                AppendAsyncEventMessage("Queued SetContentMaxHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scrollingPresenter.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollingPresenter.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentMargin, txtContentMargin.Text));
                AppendAsyncEventMessage("Queued SetContentMargin " + txtContentMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is Control || scrollingPresenter.Content is Border || scrollingPresenter.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollingPresenter.Content is Control)
                txtContentPadding.Text = ((Control)(scrollingPresenter.Content)).Padding.ToString();
            else if (scrollingPresenter.Content is Border)
                txtContentPadding.Text = ((Border)(scrollingPresenter.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scrollingPresenter.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollingPresenter.Content is Control)
                {
                    ((Control)(scrollingPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollingPresenter.Content is Border)
                {
                    ((Border)(scrollingPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollingPresenter.Content is StackPanel)
                {
                    ((StackPanel)(scrollingPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetContentPadding, txtContentPadding.Text));
                AppendAsyncEventMessage("Queued SetContentPadding " + txtContentPadding.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentActualWidth.Text = string.Empty;
            else
                txtContentActualWidth.Text = (scrollingPresenter.Content as FrameworkElement).ActualWidth.ToString();
        }

        private void BtnGetContentActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null || !(scrollingPresenter.Content is FrameworkElement))
                txtContentActualHeight.Text = string.Empty;
            else
                txtContentActualHeight.Text = (scrollingPresenter.Content as FrameworkElement).ActualHeight.ToString();
        }

        private void BtnGetContentDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null)
                txtContentDesiredSize.Text = string.Empty;
            else
                txtContentDesiredSize.Text = scrollingPresenter.Content.DesiredSize.ToString();
        }

        private void BtnGetContentRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingPresenter.Content == null)
                txtContentRenderSize.Text = string.Empty;
            else
                txtContentRenderSize.Text = scrollingPresenter.Content.RenderSize.ToString();
        }

        private void UpdateMinZoomFactor()
        {
            txtMinZoomFactor.Text = scrollingPresenter.MinZoomFactor.ToString();
        }

        private void UpdateMaxZoomFactor()
        {
            txtMaxZoomFactor.Text = scrollingPresenter.MaxZoomFactor.ToString();
        }

        private void BtnGetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMinZoomFactor();
        }

        private void BtnSetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtMinZoomFactor.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetMinZoomFactor, value));
                AppendAsyncEventMessage("Queued SetMinZoomFactor " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMaxZoomFactor();
        }

        private void BtnSetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtMaxZoomFactor.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetMaxZoomFactor, value));
                AppendAsyncEventMessage("Queued SetMaxZoomFactor " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblHorizontalOffset.Text = scrollingPresenter.HorizontalOffset.ToString();
        }

        private void BtnGetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblVerticalOffset.Text = scrollingPresenter.VerticalOffset.ToString();
        }

        private void BtnGetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            tblZoomFactor.Text = scrollingPresenter.ZoomFactor.ToString();
        }

        private void BtnGetState_Click(object sender, RoutedEventArgs e)
        {
            tblState.Text = scrollingPresenter.State.ToString();
        }

        private void BtnGetExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            tblExtentWidth.Text = scrollingPresenter.ExtentWidth.ToString();
        }

        private void BtnGetExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            tblExtentHeight.Text = scrollingPresenter.ExtentHeight.ToString();
        }

        private void BtnGetViewportWidth_Click(object sender, RoutedEventArgs e)
        {
            tblViewportWidth.Text = scrollingPresenter.ViewportWidth.ToString();
        }

        private void BtnGetViewportHeight_Click(object sender, RoutedEventArgs e)
        {
            tblViewportHeight.Text = scrollingPresenter.ViewportHeight.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            txtWidth.Text = scrollingPresenter.Width.ToString();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetWidth, value));
                AppendAsyncEventMessage("Queued SetWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            txtMaxWidth.Text = scrollingPresenter.MaxWidth.ToString();
        }

        private void BtnSetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.MaxWidth = Convert.ToDouble(txtMaxWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            txtHeight.Text = scrollingPresenter.Height.ToString();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetHeight, value));
                AppendAsyncEventMessage("Queued SetHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            txtMaxHeight.Text = scrollingPresenter.MaxHeight.ToString();
        }

        private void BtnSetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingPresenter.MaxHeight = Convert.ToDouble(txtMaxHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetActualWidth_Click(object sender, RoutedEventArgs e)
        {
            tblActualWidth.Text = scrollingPresenter.ActualWidth.ToString();
        }

        private void BtnGetActualHeight_Click(object sender, RoutedEventArgs e)
        {
            tblActualHeight.Text = scrollingPresenter.ActualHeight.ToString();
        }

        private void BtnClearScrollingPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollingPresenterEvents.Items.Clear();
        }

        private void BtnCopyScrollingPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstScrollingPresenterEvents.Items)
            {
                logs += log.ToString() + "\n";
            }

            var dataPackage = new Windows.ApplicationModel.DataTransfer.DataPackage();
            dataPackage.SetText(logs);
            Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(dataPackage);
        }

        private void BtnScrollTo_Click(object sender, RoutedEventArgs e)
        {
            Scroll(isRelativeChange: false);
        }

        private void BtnScrollBy_Click(object sender, RoutedEventArgs e)
        {
            Scroll(isRelativeChange: true);
        }

        private void Scroll(bool isRelativeChange)
        {
            try
            {
                ScrollingAnimationMode animatioMode = (ScrollingAnimationMode)cmbScrollAnimationMode.SelectedIndex;
                ScrollingSnapPointsMode snapPointsMode = (ScrollingSnapPointsMode)cmbScrollSnapPointsMode.SelectedIndex;
                ScrollingScrollOptions options = new ScrollingScrollOptions(animatioMode, snapPointsMode);

                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                if (isRelativeChange)
                {
                    lastOffsetsChangeId = scrollingPresenter.ScrollBy(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options).OffsetsChangeId;
                    relativeChangeIds.Add(lastOffsetsChangeId);
                    AppendAsyncEventMessage("Invoked ScrollBy Id=" + lastOffsetsChangeId);
                }
                else
                {
                    lastOffsetsChangeId = scrollingPresenter.ScrollTo(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options).OffsetsChangeId;
                    AppendAsyncEventMessage("Invoked ScrollTo Id=" + lastOffsetsChangeId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeId != -1)
                {
                    AppendAsyncEventMessage("Canceling ScrollTo/By Id=" + lastOffsetsChangeId);
                    int cancelId = scrollingPresenter.ScrollBy(
                        0,
                        0,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore)).OffsetsChangeId;
                    AppendAsyncEventMessage("Cancel Id=" + cancelId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollingPresenter_ScrollAnimationStarting(ScrollingPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y + ") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y + ")");

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    Vector3KeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex != 0)
                    {
                        bool isRelativeChange = relativeChangeIds.Contains(args.ScrollInfo.OffsetsChangeId);

                        double targetHorizontalOffset = Convert.ToDouble(txtScrollHorizontalOffset.Text);
                        if (isRelativeChange)
                        {
                            targetHorizontalOffset += scrollingPresenter.HorizontalOffset;
                        }
                        float targetHorizontalPosition = ComputeHorizontalPositionFromOffset(targetHorizontalOffset);

                        double targetVerticalOffset = Convert.ToDouble(txtScrollVerticalOffset.Text);
                        if (isRelativeChange)
                        {
                            targetVerticalOffset += scrollingPresenter.VerticalOffset;
                        }
                        float targetVerticalPosition = ComputeVerticalPositionFromOffset(targetVerticalOffset);

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateVector3KeyFrameAnimation();
                        if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaHorizontalPosition = 0.1f * (float)(targetHorizontalOffset - scrollingPresenter.HorizontalOffset);
                            float deltaVerticalPosition = 0.1f * (float)(targetVerticalOffset - scrollingPresenter.VerticalOffset);

                            for (int step = 0; step < 3; step++)
                            {
                                customKeyFrameAnimation.InsertKeyFrame(
                                    1.0f - (0.4f / (float)Math.Pow(2, step)),
                                    new Vector3(targetHorizontalPosition + deltaHorizontalPosition, targetVerticalPosition + deltaVerticalPosition, 0.0f));
                                deltaHorizontalPosition /= -2.0f;
                                deltaVerticalPosition /= -2.0f;
                            }

                            customKeyFrameAnimation.InsertKeyFrame(1.0f, new Vector3(targetHorizontalPosition, targetVerticalPosition, 0.0f));
                        }
                        else
                        {
                            // Teleportation case
                            float deltaHorizontalPosition = (float)(targetHorizontalOffset - scrollingPresenter.HorizontalOffset);
                            float deltaVerticalPosition = (float)(targetVerticalOffset - scrollingPresenter.VerticalOffset);

                            CubicBezierEasingFunction cubicBezierStart = stockKeyFrameAnimation.Compositor.CreateCubicBezierEasingFunction(
                                new Vector2(1.0f, 0.0f),
                                new Vector2(1.0f, 0.0f));

                            StepEasingFunction step = stockKeyFrameAnimation.Compositor.CreateStepEasingFunction(1);

                            CubicBezierEasingFunction cubicBezierEnd = stockKeyFrameAnimation.Compositor.CreateCubicBezierEasingFunction(
                                new Vector2(0.0f, 1.0f),
                                new Vector2(0.0f, 1.0f));

                            customKeyFrameAnimation.InsertKeyFrame(
                                0.499999f,
                                new Vector3(targetHorizontalPosition - 0.9f * deltaHorizontalPosition, targetVerticalPosition - 0.9f * deltaVerticalPosition, 0.0f),
                                cubicBezierStart);
                            customKeyFrameAnimation.InsertKeyFrame(
                                0.5f,
                                new Vector3(targetHorizontalPosition - 0.1f * deltaHorizontalPosition, targetVerticalPosition - 0.1f * deltaVerticalPosition, 0.0f),
                                step);
                            customKeyFrameAnimation.InsertKeyFrame(
                                1.0f,
                                new Vector3(targetHorizontalPosition, targetVerticalPosition, 0.0f),
                                cubicBezierEnd);
                        }
                        customKeyFrameAnimation.Duration = stockKeyFrameAnimation.Duration;
                        args.Animation = customKeyFrameAnimation;
                    }

                    if (!string.IsNullOrWhiteSpace(txtOverriddenOffsetsChangeDuration.Text))
                    {
                        txtStockOffsetsChangeDuration.Text = stockKeyFrameAnimation.Duration.TotalMilliseconds.ToString();
                        double durationOverride = Convert.ToDouble(txtOverriddenOffsetsChangeDuration.Text);
                        customKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnScrollFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Vector2? inertiaDecayRate = null;

                if (txtScrollHorizontalInertiaDecayRate.Text != "null" && txtScrollVerticalInertiaDecayRate.Text != "null")
                {
                    inertiaDecayRate = new Vector2(Convert.ToSingle(txtScrollHorizontalInertiaDecayRate.Text), Convert.ToSingle(txtScrollVerticalInertiaDecayRate.Text));
                }

                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastOffsetsChangeWithAdditionalVelocityId = scrollingPresenter.ScrollFrom(
                    new Vector2(Convert.ToSingle(txtScrollHorizontalAdditionalVelocity.Text), Convert.ToSingle(txtScrollVerticalAdditionalVelocity.Text)),
                    inertiaDecayRate).OffsetsChangeId;
                AppendAsyncEventMessage("Invoked ScrollFrom Id=" + lastOffsetsChangeWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScrollFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ScrollFrom Id=" + lastOffsetsChangeWithAdditionalVelocityId);
                    int cancelId = scrollingPresenter.ScrollBy(
                        0,
                        0,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore)).OffsetsChangeId;
                    AppendAsyncEventMessage("Cancel Id=" + cancelId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnScrollWith_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastOffsetsChangeWithVelocityId = scrollingPresenter.ScrollWith(
                    new Vector2(Convert.ToSingle(txtScrollHorizontalVelocity.Text), Convert.ToSingle(txtScrollVerticalVelocity.Text))).OffsetsChangeId;
                AppendAsyncEventMessage("Invoked ScrollWith Id=" + lastOffsetsChangeWithVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScrollWith_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeWithVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ScrollWith Id=" + lastOffsetsChangeWithVelocityId);
                    int cancelId = scrollingPresenter.ScrollBy(
                        0,
                        0,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore)).OffsetsChangeId;
                    AppendAsyncEventMessage("Cancel Id=" + cancelId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private float ComputeHorizontalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinHorizontalPosition(scrollingPresenter.ZoomFactor));
        }

        private float ComputeVerticalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinVerticalPosition(scrollingPresenter.ZoomFactor));
        }

        private float ComputeMinHorizontalPosition(float zoomFactor)
        {
            UIElement content = scrollingPresenter.Content;

            if (content == null)
            {
                return 0;
            }

            FrameworkElement contentAsFE = content as FrameworkElement;

            if (contentAsFE == null)
            {
                return 0;
            }

            Thickness contentMargin = contentAsFE.Margin;
            Visual scrollingPresenterVisual = ElementCompositionPreview.GetElementVisual(scrollingPresenter);
            double contentWidth = double.IsNaN(contentAsFE.Width) ? contentAsFE.ActualWidth : contentAsFE.Width;
            float minPosX = 0.0f;
            float extentWidth = Math.Max(0.0f, (float)(contentWidth + contentMargin.Left + contentMargin.Right));

            if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Center ||
                contentAsFE.HorizontalAlignment == HorizontalAlignment.Stretch)
            {
                minPosX = Math.Min(0.0f, (extentWidth * zoomFactor - scrollingPresenterVisual.Size.X) / 2.0f);
            }
            else if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Right)
            {
                minPosX = Math.Min(0.0f, extentWidth * zoomFactor - scrollingPresenterVisual.Size.X);
            }

            return minPosX;
        }

        private float ComputeMinVerticalPosition(float zoomFactor)
        {
            UIElement content = scrollingPresenter.Content;

            if (content == null)
            {
                return 0;
            }

            FrameworkElement contentAsFE = content as FrameworkElement;

            if (contentAsFE == null)
            {
                return 0;
            }

            Thickness contentMargin = contentAsFE.Margin;
            Visual scrollingPresenterVisual = ElementCompositionPreview.GetElementVisual(scrollingPresenter);
            double contentHeight = double.IsNaN(contentAsFE.Height) ? contentAsFE.ActualHeight : contentAsFE.Height;
            float minPosY = 0.0f;
            float extentHeight = Math.Max(0.0f, (float)(contentHeight + contentMargin.Top + contentMargin.Bottom));

            if (contentAsFE.VerticalAlignment == VerticalAlignment.Center ||
                contentAsFE.VerticalAlignment == VerticalAlignment.Stretch)
            {
                minPosY = Math.Min(0.0f, (extentHeight * zoomFactor - scrollingPresenterVisual.Size.Y) / 2.0f);
            }
            else if (contentAsFE.VerticalAlignment == VerticalAlignment.Bottom)
            {
                minPosY = Math.Min(0.0f, extentHeight * zoomFactor - scrollingPresenterVisual.Size.Y);
            }

            return minPosY;
        }

        private void BtnZoomTo_Click(object sender, RoutedEventArgs e)
        {
            Zoom(isRelativeChange: false);
        }

        private void BtnZoomBy_Click(object sender, RoutedEventArgs e)
        {
            Zoom(isRelativeChange: true);
        }

        private void Zoom(bool isRelativeChange)
        {
            try
            {
                ScrollingAnimationMode animationMode = (ScrollingAnimationMode)cmbZoomAnimationMode.SelectedIndex;
                ScrollingSnapPointsMode snapPointsMode = (ScrollingSnapPointsMode)cmbZoomSnapPointsMode.SelectedIndex;
                ScrollingZoomOptions options = new ScrollingZoomOptions(animationMode, snapPointsMode);

                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                if (isRelativeChange)
                {
                    lastZoomFactorChangeId = scrollingPresenter.ZoomBy(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options).ZoomFactorChangeId;
                    relativeChangeIds.Add(lastZoomFactorChangeId);
                    AppendAsyncEventMessage("Invoked ZoomBy Id=" + lastZoomFactorChangeId);
                }
                else
                {
                    lastZoomFactorChangeId = scrollingPresenter.ZoomTo(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options).ZoomFactorChangeId;
                    AppendAsyncEventMessage("Invoked ZoomTo Id=" + lastZoomFactorChangeId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelZoom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeId != -1)
                {
                    AppendAsyncEventMessage("Canceling ZoomTo/By Id=" + lastZoomFactorChangeId);
                    int cancelId = scrollingPresenter.ZoomBy(
                        0,
                        Vector2.Zero, 
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore)).ZoomFactorChangeId;
                    AppendAsyncEventMessage("Cancel Id=" + cancelId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollingPresenter_ZoomAnimationStarting(ScrollingPresenter sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ZoomAnimationStarting ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);
                
                ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    ScalarKeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex != 0)
                    {
                        float targetZoomFactor = Convert.ToSingle(txtZoomZoomFactor.Text);
                        if (relativeChangeIds.Contains(args.ZoomInfo.ZoomFactorChangeId))
                        {
                            targetZoomFactor += scrollingPresenter.ZoomFactor;
                        }

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateScalarKeyFrameAnimation();
                        if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaZoomFactor = 0.1f * (float)(targetZoomFactor - scrollingPresenter.ZoomFactor);

                            for (int step = 0; step < 3; step++)
                            {
                                customKeyFrameAnimation.InsertKeyFrame(
                                    1.0f - (0.4f / (float)Math.Pow(2, step)),
                                    targetZoomFactor + deltaZoomFactor);
                                deltaZoomFactor /= -2.0f;
                            }

                            customKeyFrameAnimation.InsertKeyFrame(1.0f, targetZoomFactor);
                        }
                        else
                        {
                            // Teleportation case
                            float deltaZoomFactor = (float)(targetZoomFactor - scrollingPresenter.ZoomFactor);

                            CubicBezierEasingFunction cubicBezierStart = stockKeyFrameAnimation.Compositor.CreateCubicBezierEasingFunction(
                                new Vector2(1.0f, 0.0f),
                                new Vector2(1.0f, 0.0f));

                            StepEasingFunction step = stockKeyFrameAnimation.Compositor.CreateStepEasingFunction(1);

                            CubicBezierEasingFunction cubicBezierEnd = stockKeyFrameAnimation.Compositor.CreateCubicBezierEasingFunction(
                                new Vector2(0.0f, 1.0f),
                                new Vector2(0.0f, 1.0f));

                            customKeyFrameAnimation.InsertKeyFrame(
                                0.499999f,
                                targetZoomFactor - 0.9f * deltaZoomFactor,
                                cubicBezierStart);
                            customKeyFrameAnimation.InsertKeyFrame(
                                0.5f,
                                targetZoomFactor - 0.1f * deltaZoomFactor,
                                step);
                            customKeyFrameAnimation.InsertKeyFrame(
                                1.0f,
                                targetZoomFactor,
                                cubicBezierEnd);
                        }
                        customKeyFrameAnimation.Duration = stockKeyFrameAnimation.Duration;
                        args.Animation = customKeyFrameAnimation;
                    }

                    if (!string.IsNullOrWhiteSpace(txtOverriddenZoomFactorChangeDuration.Text))
                    {
                        txtStockZoomFactorChangeDuration.Text = stockKeyFrameAnimation.Duration.TotalMilliseconds.ToString();
                        double durationOverride = Convert.ToDouble(txtOverriddenZoomFactorChangeDuration.Text);
                        customKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnZoomFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastZoomFactorChangeWithAdditionalVelocityId = scrollingPresenter.ZoomFrom(
                    Convert.ToSingle(txtZoomFromVelocity.Text),
                    (txtZoomFromCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomFromCenterPoint.Text),
                    (txtZoomFromInertiaDecayRate.Text == "null") ? (float?)null : (float?)Convert.ToSingle(txtZoomFromInertiaDecayRate.Text)).ZoomFactorChangeId;
                AppendAsyncEventMessage("Invoked ZoomFrom Id=" + lastZoomFactorChangeWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelZoomFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ZoomFrom Id=" + lastZoomFactorChangeWithAdditionalVelocityId);
                    int cancelId = scrollingPresenter.ZoomBy(
                        0,
                        Vector2.Zero,
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore)).ZoomFactorChangeId;
                    AppendAsyncEventMessage("Cancel Id=" + cancelId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbSnapPointKind_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbSnapPointAlignment != null)
            {
                cmbSnapPointAlignment.IsEnabled = cmbSnapPointKind.SelectedIndex != 2;
            }
        }

        private void BtnAddIrregularSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
#if ApplicableRangeType
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text), ScrollSnapPointsAlignment.Near);
                    scrollingPresenter.VerticalSnapPoints.Add(snapPoint);
                }
                else if(cmbSnapPointKind.SelectedIndex == 1)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text), ScrollSnapPointsAlignment.Near);
                    scrollingPresenter.HorizontalSnapPoints.Add(snapPoint); 
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text));
                    scrollingPresenter.ZoomSnapPoints.Add(snapPoint);
                }
#else
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text),
                        (ScrollSnapPointsAlignment)cmbSnapPointAlignment.SelectedIndex);
                    if (cmbSnapPointKind.SelectedIndex == 0)
                    {
                        scrollingPresenter.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scrollingPresenter.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text));
                    scrollingPresenter.ZoomSnapPoints.Add(snapPoint);
                }
#endif
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnAddRepeatedSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbSnapPointKind.SelectedIndex == 0 || cmbSnapPointKind.SelectedIndex == 1)
                {
                    RepeatedScrollSnapPoint snapPoint = new RepeatedScrollSnapPoint(
                        Convert.ToDouble(txtRepeatedSnapPointOffset.Text),
                        Convert.ToDouble(txtRepeatedSnapPointInterval.Text),
                        Convert.ToDouble(txtRepeatedSnapPointStart.Text),
                        Convert.ToDouble(txtRepeatedSnapPointEnd.Text),
                        (ScrollSnapPointsAlignment)cmbSnapPointAlignment.SelectedIndex);
                    if (cmbSnapPointKind.SelectedIndex == 0)
                    {
                        scrollingPresenter.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scrollingPresenter.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    RepeatedZoomSnapPoint snapPoint = new RepeatedZoomSnapPoint(
                        Convert.ToDouble(txtRepeatedSnapPointOffset.Text),
                        Convert.ToDouble(txtRepeatedSnapPointInterval.Text),
                        Convert.ToDouble(txtRepeatedSnapPointStart.Text),
                        Convert.ToDouble(txtRepeatedSnapPointEnd.Text));
                    scrollingPresenter.ZoomSnapPoints.Add(snapPoint);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnChangeView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double? horizontalOffset = string.IsNullOrWhiteSpace(txtScrollHorizontalOffset.Text) ? (double?)null : Convert.ToDouble(txtScrollHorizontalOffset.Text);
                double? verticalOffset = string.IsNullOrWhiteSpace(txtScrollVerticalOffset.Text) ? (double?)null : Convert.ToDouble(txtScrollVerticalOffset.Text);
                float? zoomFactor = string.IsNullOrWhiteSpace(txtZoomZoomFactor.Text) ? (float?)null : Convert.ToSingle(txtZoomZoomFactor.Text);
                bool disableAnimation = true;

                if (horizontalOffset == null && verticalOffset == null && zoomFactor == null)
                {
                    return;
                }

                ComboBox cmbAnimationMode = (zoomFactor == null || zoomFactor == scrollingPresenter.ZoomFactor) ? 
                    cmbScrollAnimationMode : cmbZoomAnimationMode;

                switch (((ContentControl)cmbAnimationMode.SelectedItem).Content)
                {
                    case "Disabled":
                        break;
                    case "Enabled":
                        disableAnimation = false;
                        break;
                    default: // Auto
                        disableAnimation = !new UISettings().AnimationsEnabled;
                        break;
                }

                ChangeView(scrollingPresenter, horizontalOffset, verticalOffset, zoomFactor, disableAnimation);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private bool ChangeView(ScrollingPresenter scrollingPresenter, double? horizontalOffset, double? verticalOffset, float? zoomFactor, bool disableAnimation)
        {
            AppendAsyncEventMessage($"ChangeView(horizontalOffset:{horizontalOffset}, verticalOffset:{verticalOffset}, zoomFactor:{zoomFactor}, disableAnimation:{disableAnimation}) from horizontalOffset:{scrollingPresenter.HorizontalOffset}, verticalOffset:{scrollingPresenter.VerticalOffset}, zoomFactor:{scrollingPresenter.ZoomFactor}");

            double targetHorizontalOffset = horizontalOffset == null ? scrollingPresenter.HorizontalOffset : (double)horizontalOffset;
            double targetVerticalOffset = verticalOffset == null ? scrollingPresenter.VerticalOffset : (double)verticalOffset;
            float targetZoomFactor = zoomFactor == null ? scrollingPresenter.ZoomFactor : (float)Math.Max(Math.Min((double)zoomFactor, scrollingPresenter.MaxZoomFactor), scrollingPresenter.MinZoomFactor);
            float deltaZoomFactor = targetZoomFactor - scrollingPresenter.ZoomFactor;

            if (disableAnimation)
            {
                targetHorizontalOffset = Math.Max(Math.Min(targetHorizontalOffset, scrollingPresenter.ExtentWidth * targetZoomFactor - scrollingPresenter.ViewportWidth), 0.0);
                targetVerticalOffset = Math.Max(Math.Min(targetVerticalOffset, scrollingPresenter.ExtentHeight * targetZoomFactor - scrollingPresenter.ViewportHeight), 0.0);
            }
            // During an animation, out-of-bounds offsets may become valid as the extents are dynamically updated, so no clamping is performed for animated view changes.

            if (deltaZoomFactor == 0.0f)
            {
                if (targetHorizontalOffset == scrollingPresenter.HorizontalOffset && targetVerticalOffset == scrollingPresenter.VerticalOffset)
                {
                    AppendAsyncEventMessage("ChangeView no-op");
                    return false;
                }

                lastOffsetsChangeId = scrollingPresenter.ScrollTo(
                    targetHorizontalOffset,
                    targetVerticalOffset,
                    new ScrollingScrollOptions(
                        disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                        disableAnimation ? ScrollingSnapPointsMode.Ignore : ScrollingSnapPointsMode.Default)).OffsetsChangeId;
                AppendAsyncEventMessage($"ChangeView invoked ScrollTo(horizontalOffset:{targetHorizontalOffset}, verticalOffset:{targetVerticalOffset}) Id={lastOffsetsChangeId}");
            }
            else
            {
                FrameworkElement contentAsFE = scrollingPresenter.Content as FrameworkElement;
                HorizontalAlignment contentHorizontalAlignment = contentAsFE == null ? HorizontalAlignment.Stretch : contentAsFE.HorizontalAlignment;
                VerticalAlignment contentVerticalAlignment = contentAsFE == null ? VerticalAlignment.Stretch : contentAsFE.VerticalAlignment;
                double currentPositionX = scrollingPresenter.HorizontalOffset;
                double currentPositionY = scrollingPresenter.VerticalOffset;
                double currentViewportExcessX = scrollingPresenter.ViewportWidth - scrollingPresenter.ExtentWidth * scrollingPresenter.ZoomFactor;
                double targetViewportExcessX = scrollingPresenter.ViewportWidth - scrollingPresenter.ExtentWidth * targetZoomFactor;
                double currentViewportExcessY = scrollingPresenter.ViewportHeight - scrollingPresenter.ExtentHeight * scrollingPresenter.ZoomFactor;
                double targetViewportExcessY = scrollingPresenter.ViewportHeight - scrollingPresenter.ExtentHeight * targetZoomFactor;

                switch (contentHorizontalAlignment)
                {
                    case HorizontalAlignment.Center:
                    case HorizontalAlignment.Stretch:
                        if (currentViewportExcessX > 0) currentPositionX -= currentViewportExcessX / 2.0;
                        if (targetViewportExcessX > 0) targetHorizontalOffset -= targetViewportExcessX / 2.0;
                        break;
                    case HorizontalAlignment.Right:
                        if (currentViewportExcessX > 0) currentPositionX -= currentViewportExcessX;
                        if (targetViewportExcessX > 0) targetHorizontalOffset -= targetViewportExcessX;
                        break;
                }

                switch (contentVerticalAlignment)
                {
                    case VerticalAlignment.Center:
                    case VerticalAlignment.Stretch:
                        if (currentViewportExcessY > 0) currentPositionY -= currentViewportExcessY / 2.0;
                        if (targetViewportExcessY > 0) targetVerticalOffset -= targetViewportExcessY / 2.0;
                        break;
                    case VerticalAlignment.Bottom:
                        if (currentViewportExcessY > 0) currentPositionY -= currentViewportExcessY;
                        if (targetViewportExcessY > 0) targetVerticalOffset -= targetViewportExcessY;
                        break;
                }

                Vector2 centerPoint = new Vector2(
                    (float)(targetHorizontalOffset * scrollingPresenter.ZoomFactor - currentPositionX * targetZoomFactor) / deltaZoomFactor,
                    (float)(targetVerticalOffset * scrollingPresenter.ZoomFactor - currentPositionY * targetZoomFactor) / deltaZoomFactor);


                lastZoomFactorChangeId = scrollingPresenter.ZoomTo(
                    targetZoomFactor,
                    centerPoint,
                    new ScrollingZoomOptions(
                        disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                        disableAnimation ? ScrollingSnapPointsMode.Ignore : ScrollingSnapPointsMode.Default)).ZoomFactorChangeId;
                AppendAsyncEventMessage($"ChangeView invoked ZoomBy(zoomFactor:{targetZoomFactor}, centerPoint:{centerPoint}) targetting horizontalOffset:{targetHorizontalOffset}, verticalOffset:{targetVerticalOffset} Id={lastZoomFactorChangeId}");
            }

            return true;
        }

    private void BtnClearSnapPoints_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    scrollingPresenter.VerticalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 1)
                {
                    scrollingPresenter.HorizontalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    scrollingPresenter.ZoomSnapPoints.Clear();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                while (asyncEventMessage.Length > 0)
                {
                    string msgHead = asyncEventMessage;

                    if (asyncEventMessage.Length > 110)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 110);
                        if (commaIndex != -1)
                        {
                            msgHead = asyncEventMessage.Substring(0, commaIndex);
                            asyncEventMessage = asyncEventMessage.Substring(commaIndex + 1);
                        }
                        else
                        {
                            asyncEventMessage = string.Empty;
                        }
                    }
                    else
                    {
                        asyncEventMessage = string.Empty;
                    }

                    lstAsyncEventMessage.Add(msgHead);
                }

                var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, AppendAsyncEventMessage);
            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in lstAsyncEventMessage)
                {
                    lstScrollingPresenterEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private Vector2 ConvertFromStringToVector2(string value)
        {
            float x = 0, y = 0;

            int commaIndex = value.IndexOf(',');
            if (commaIndex != -1)
            {
                x = Convert.ToSingle(value.Substring(0, commaIndex));
                y = Convert.ToSingle(value.Substring(commaIndex + 1));
            }

            return new Vector2(x, y);
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void BtnParentMarkupScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(markupScrollingPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentMarkupScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScrollingPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentReparentMarkupScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScrollingPresenter);
                root.Children.Add(markupScrollingPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCreateDynamicScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                dynamicScrollingPresenter = new ScrollingPresenter();
                dynamicScrollingPresenter.Name = "dynamicScrollingPresenter";
                dynamicScrollingPresenter.Width = 300.0;
                dynamicScrollingPresenter.Height = 400.0;
                dynamicScrollingPresenter.Margin = new Thickness(1);
                dynamicScrollingPresenter.Background = new SolidColorBrush(Colors.HotPink);
                dynamicScrollingPresenter.VerticalAlignment = VerticalAlignment.Top;
                Grid.SetRow(dynamicScrollingPresenter, 1);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnParentDynamicScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(dynamicScrollingPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentDynamicScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(dynamicScrollingPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUseMarkupScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            UseScrollingPresenter(markupScrollingPresenter);
        }

        private void BtnUseDynamicScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            UseScrollingPresenter(dynamicScrollingPresenter);
        }

        private void BtnReleaseDynamicScrollingPresenter_Click(object sender, RoutedEventArgs e)
        {
            dynamicScrollingPresenter = null;

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        private void BtnExecuteQueuedOperations_Click(object sender, RoutedEventArgs e)
        {
            ExecuteQueuedOperations();
        }

        private void BtnDiscardQueuedOperations_Click(object sender, RoutedEventArgs e)
        {
            lstQueuedOperations.Clear();
            AppendAsyncEventMessage("Queued operations cleared.");
        }

        private void BtnGetContentLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float contentLayoutOffsetX = 0.0f;

                ScrollingPresenterTestHooks.GetContentLayoutOffsetX(scrollingPresenter, out contentLayoutOffsetX);

                txtContentLayoutOffsetX.Text = contentLayoutOffsetX.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.SetContentLayoutOffsetX(scrollingPresenter, Convert.ToSingle(txtContentLayoutOffsetX.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float contentLayoutOffsetY = 0.0f;

                ScrollingPresenterTestHooks.GetContentLayoutOffsetY(scrollingPresenter, out contentLayoutOffsetY);

                txtContentLayoutOffsetY.Text = contentLayoutOffsetY.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.SetContentLayoutOffsetY(scrollingPresenter, Convert.ToSingle(txtContentLayoutOffsetY.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetArrangeRenderSizesDelta_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtArrangeRenderSizesDelta.Text = ScrollingPresenterTestHooks.GetArrangeRenderSizesDelta(scrollingPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMinPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMinPosition.Text = ScrollingPresenterTestHooks.GetMinPosition(scrollingPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMaxPosition.Text = ScrollingPresenterTestHooks.GetMaxPosition(scrollingPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void UseScrollingPresenter(ScrollingPresenter s)
        {
            if (scrollingPresenter == s || s == null)
                return;

            try
            {
                if (scrollingPresenter != null)
                {
                    if (chkLogScrollingPresenterMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
                    }

                    if (chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
                        scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
                        scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
                        scrollingPresenter.ScrollCompleted -= ScrollingPresenter_ScrollCompleted;
                        scrollingPresenter.ZoomCompleted -= ScrollingPresenter_ZoomCompleted;
                        scrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                        scrollingPresenter.ZoomAnimationStarting -= ScrollingPresenter_ZoomAnimationStarting;
                    }

                    if (chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        UnhookContentEffectiveViewportChanged();
                    }
                }

                scrollingPresenter = s;

                if (chkLogScrollingPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                UpdateCmbContentOrientation();
                UpdateCmbHorizontalScrollMode();
                UpdateCmbHorizontalScrollChainMode();
                UpdateCmbHorizontalScrollRailMode();
                UpdateCmbVerticalScrollMode();
                UpdateCmbVerticalScrollChainMode();
                UpdateCmbVerticalScrollRailMode();
                UpdateCmbZoomMode();
                UpdateCmbZoomChainMode();
                UpdateCmbIgnoredInputKind();
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbScrollingPresenterManipulationMode();
                UpdateCmbContentManipulationMode();
                UpdateCmbBackground();
                UpdateCmbContent();
                UpdateMinZoomFactor();
                UpdateMaxZoomFactor();

                txtWidth.Text = string.Empty;
                txtHeight.Text = string.Empty;

                if (scrollingPresenter != null)
                {
                    if (chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
                        scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
                        scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
                        scrollingPresenter.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
                        scrollingPresenter.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
                        scrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                        scrollingPresenter.ZoomAnimationStarting += ScrollingPresenter_ZoomAnimationStarting;
                    }

                    if (chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        HookContentEffectiveViewportChanged();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private Thickness GetThicknessFromString(string thickness)
        {
            string[] lengths = thickness.Split(',');
            if (lengths.Length < 4)
                return new Thickness(Convert.ToDouble(lengths[0]));
            else
                return new Thickness(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
        }

        private void ExecuteQueuedOperations()
        {
            try
            {
                while (lstQueuedOperations.Count > 0)
                {
                    QueuedOperation qo = lstQueuedOperations[0];

                    switch (qo.Type)
                    {
                        case QueuedOperationType.SetMinZoomFactor:
                            scrollingPresenter.MinZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMinZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetMaxZoomFactor:
                            scrollingPresenter.MaxZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMaxZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetWidth:
                            scrollingPresenter.Width = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetHeight:
                            scrollingPresenter.Height = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentWidth:
                            if (scrollingPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollingPresenter.Content)).Width = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentHeight:
                            if (scrollingPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollingPresenter.Content)).Height = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentMargin:
                            if (scrollingPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollingPresenter.Content)).Margin = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetContentMargin " + qo.StringValue);
                            break;
                        case QueuedOperationType.SetContentPadding:
                            if (scrollingPresenter.Content is Control)
                            {
                                ((Control)(scrollingPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scrollingPresenter.Content is Border)
                            {
                                ((Border)(scrollingPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scrollingPresenter.Content is StackPanel)
                            {
                                ((StackPanel)(scrollingPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetContentPadding " + qo.StringValue);
                            break;
                    }

                    lstQueuedOperations.RemoveAt(0);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }
    }
}
