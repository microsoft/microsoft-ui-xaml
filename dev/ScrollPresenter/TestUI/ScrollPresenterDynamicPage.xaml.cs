// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
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

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterDynamicPage : TestPage
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

        private int lastOffsetsChangeCorrelationId = -1;
        private int lastOffsetsChangeWithAdditionalVelocityCorrelationId = -1;
        private int lastZoomFactorChangeCorrelationId = -1;
        private int lastZoomFactorChangeWithAdditionalVelocityCorrelationId = -1;
        private HashSet<int> relativeChangeCorrelationIds = new HashSet<int>();
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<QueuedOperation> lstQueuedOperations = new List<QueuedOperation>();
        private TextBlock textBlock;
        private Viewbox viewbox;
        private TilePanel tilePanel;
        private Image largeImg;
        private Rectangle rectangle;
        private Button button;
        private Border border;
        private Border populatedBorder;
        private StackPanel verticalStackPanel;
        private ScrollPresenter dynamicScrollPresenter = null;
        private ScrollPresenter scrollPresenter = null;

        public ScrollPresenterDynamicPage()
        {
            this.InitializeComponent();

            UseScrollPresenter(markupScrollPresenter);

            CreateChildren();
        }

        private void ScrollPresenter_ExtentChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("StateChanged " + sender.State.ToString());
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollPresenter_ScrollCompleted(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage("ScrollCompleted OffsetsChangeCorrelationId=" + args.CorrelationId + ", Result=" + result);
        }

        private void ScrollPresenter_ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

            AppendAsyncEventMessage("ZoomCompleted ZoomFactorChangeCorrelationId=" + args.CorrelationId + ", Result=" + result);
        }

        private void ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

            AppendAsyncEventMessage("ZoomCompleted ZoomFactorChangeCorrelationId=" + args.CorrelationId + ", Result=" + result);
        }

        private void CreateChildren()
        {
            textBlock = new TextBlock();
            textBlock.TextWrapping = TextWrapping.Wrap;
            string textBlockText = string.Empty;
            for (int i = 0; i < 50; i++)
            {
                textBlockText += "Large TextBlock Text. ";
            }
            textBlock.Text = textBlockText;

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
            Rectangle viewboxChild = new Rectangle()
            {
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
                BorderBrush = chartreuseBrush,
                BorderThickness = new Thickness(5),
                Child = borderChild
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
                BorderBrush = chartreuseBrush,
                BorderThickness = new Thickness(3),
                Margin = new Thickness(15),
                Background = new SolidColorBrush(Colors.Beige),
                Child = populatedBorderChild
            };
            verticalStackPanel = new StackPanel();
            for (int index = 0; index < 10; index++)
            {
                verticalStackPanel.Children.Add(new Rectangle() { Fill = lgb, Height = 95 });
                verticalStackPanel.Children.Add(new Rectangle() { Fill = interBrush, Height = 5 });
            }
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for a particular ScrollPresenter instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scrollPresenter, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for a particular ScrollPresenter instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scrollPresenter, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
            scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
            scrollPresenter.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            scrollPresenter.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            scrollPresenter.ScrollAnimationStarting += ScrollPresenter_ScrollAnimationStarting;
            scrollPresenter.ZoomAnimationStarting += ScrollPresenter_ZoomAnimationStarting;
        }

        private void ChkLogScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged -= ScrollPresenter_StateChanged;
            scrollPresenter.ViewChanged -= ScrollPresenter_ViewChanged;
            scrollPresenter.ScrollCompleted -= ScrollPresenter_ScrollCompleted;
            scrollPresenter.ZoomCompleted -= ScrollPresenter_ZoomCompleted;
            scrollPresenter.ScrollAnimationStarting -= ScrollPresenter_ScrollAnimationStarting;
            scrollPresenter.ZoomAnimationStarting -= ScrollPresenter_ZoomAnimationStarting;
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
            if (scrollPresenter != null)
            {
                FrameworkElement contentAsFE = scrollPresenter.Content as FrameworkElement;
                if (contentAsFE != null)
                {
                    contentAsFE.EffectiveViewportChanged += Content_EffectiveViewportChanged;
                }
            }
        }

        private void UnhookContentEffectiveViewportChanged()
        {
            if (scrollPresenter != null)
            {
                FrameworkElement contentAsFE = scrollPresenter.Content as FrameworkElement;
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

        private void ChkScrollPresenterProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollPresenterProperties != null)
                svScrollPresenterProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollPresenterProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollPresenterProperties != null)
                svScrollPresenterProperties.Visibility = Visibility.Collapsed;
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

        private void ChkScrollPresenterMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollPresenterMethods != null)
                svScrollPresenterMethods.Visibility = Visibility.Visible;
        }

        private void ChkScrollPresenterMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollPresenterMethods != null)
                svScrollPresenterMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollPresenterEvents != null)
                grdScrollPresenterEvents.Visibility = Visibility.Visible;
        }

        private void ChkScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollPresenterEvents != null)
                grdScrollPresenterEvents.Visibility = Visibility.Collapsed;
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

        private void ChkScrollPresenterTestHooks_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollPresenterTestHooks != null)
                grdScrollPresenterTestHooks.Visibility = Visibility.Visible;
        }

        private void ChkScrollPresenterTestHooks_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollPresenterTestHooks != null)
                grdScrollPresenterTestHooks.Visibility = Visibility.Collapsed;
        }

        private void CmbContentOrientation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.ContentOrientation = (ScrollingContentOrientation)cmbContentOrientation.SelectedIndex;
        }

        private void CmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.HorizontalScrollMode = (ScrollingScrollMode)cmbHorizontalScrollMode.SelectedIndex;
        }

        private void UpdateCmbContentOrientation()
        {
            try
            {
                cmbContentOrientation.SelectedIndex = (int)scrollPresenter.ContentOrientation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbHorizontalScrollMode()
        {
            cmbHorizontalScrollMode.SelectedIndex = (int)scrollPresenter.HorizontalScrollMode;
        }

        private void CmbHorizontalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.HorizontalScrollChainMode = (ScrollingChainMode)cmbHorizontalScrollChainMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollChainMode()
        {
            cmbHorizontalScrollChainMode.SelectedIndex = (int)scrollPresenter.HorizontalScrollChainMode;
        }

        private void CmbHorizontalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.HorizontalScrollRailMode = (ScrollingRailMode)cmbHorizontalScrollRailMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollRailMode()
        {
            cmbHorizontalScrollRailMode.SelectedIndex = (int)scrollPresenter.HorizontalScrollRailMode;
        }

        private void CmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.VerticalScrollMode = (ScrollingScrollMode)cmbVerticalScrollMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollMode()
        {
            cmbVerticalScrollMode.SelectedIndex = (int)scrollPresenter.VerticalScrollMode;
        }

        private void CmbVerticalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.VerticalScrollChainMode = (ScrollingChainMode)cmbVerticalScrollChainMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollChainMode()
        {
            cmbVerticalScrollChainMode.SelectedIndex = (int)scrollPresenter.VerticalScrollChainMode;
        }

        private void CmbVerticalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.VerticalScrollRailMode = (ScrollingRailMode)cmbVerticalScrollRailMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollRailMode()
        {
            cmbVerticalScrollRailMode.SelectedIndex = (int)scrollPresenter.VerticalScrollRailMode;
        }

        private void CmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.ZoomMode = (ScrollingZoomMode)cmbZoomMode.SelectedIndex;
        }

        private void UpdateCmbZoomMode()
        {
            cmbZoomMode.SelectedIndex = (int)scrollPresenter.ZoomMode;
        }

        private void CmbZoomChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
                scrollPresenter.ZoomChainMode = (ScrollingChainMode)cmbZoomChainMode.SelectedIndex;
        }

        private void UpdateCmbZoomChainMode()
        {
            cmbZoomChainMode.SelectedIndex = (int)scrollPresenter.ZoomChainMode;
        }

        private void CmbIgnoredInputKinds_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter != null)
            {
                switch (cmbIgnoredInputKinds.SelectedIndex)
                {
                    case 0:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.None;
                        break;
                    case 1:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.Touch;
                        break;
                    case 2:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.Pen;
                        break;
                    case 3:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.MouseWheel;
                        break;
                    case 4:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.Keyboard;
                        break;
                    case 5:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.Gamepad;
                        break;
                    case 6:
                        scrollPresenter.IgnoredInputKinds = ScrollingInputKinds.All;
                        break;
                }
            }
        }

        private void UpdateCmbIgnoredInputKinds()
        {
            switch (scrollPresenter.IgnoredInputKinds)
            {
                case ScrollingInputKinds.None:
                    cmbIgnoredInputKinds.SelectedIndex = 0;
                    break;
                case ScrollingInputKinds.Touch:
                    cmbIgnoredInputKinds.SelectedIndex = 1;
                    break;
                case ScrollingInputKinds.Pen:
                    cmbIgnoredInputKinds.SelectedIndex = 2;
                    break;
                case ScrollingInputKinds.MouseWheel:
                    cmbIgnoredInputKinds.SelectedIndex = 3;
                    break;
                case ScrollingInputKinds.Keyboard:
                    cmbIgnoredInputKinds.SelectedIndex = 4;
                    break;
                case ScrollingInputKinds.Gamepad:
                    cmbIgnoredInputKinds.SelectedIndex = 5;
                    break;
                case ScrollingInputKinds.All:
                    cmbIgnoredInputKinds.SelectedIndex = 6;
                    break;
            }
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollPresenter.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentHorizontalAlignment()
        {
            if (scrollPresenter.Content is FrameworkElement)
            {
                cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollPresenter.Content).HorizontalAlignment;
            }
        }

        private void CmbContentVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollPresenter.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentVerticalAlignment()
        {
            if (scrollPresenter.Content is FrameworkElement)
            {
                cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollPresenter.Content).VerticalAlignment;
            }
        }

        private void CmbScrollPresenterManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbScrollPresenterManipulationMode.SelectedIndex)
            {
                case 0:
                    scrollPresenter.ManipulationMode = ManipulationModes.System;
                    break;
                case 1:
                    scrollPresenter.ManipulationMode = ManipulationModes.None;
                    break;
            }
        }

        private void UpdateCmbScrollPresenterManipulationMode()
        {
            switch (scrollPresenter.ManipulationMode)
            {
                case ManipulationModes.System:
                    cmbScrollPresenterManipulationMode.SelectedIndex = 0;
                    break;
                case ManipulationModes.None:
                    cmbScrollPresenterManipulationMode.SelectedIndex = 1;
                    break;
            }
        }

        private void CmbContentManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollPresenter.Content is FrameworkElement)
            {
                switch (cmbContentManipulationMode.SelectedIndex)
                {
                    case 0:
                        scrollPresenter.Content.ManipulationMode = ManipulationModes.System;
                        break;
                    case 1:
                        scrollPresenter.Content.ManipulationMode = ManipulationModes.None;
                        break;
                }
            }
        }

        private void UpdateCmbContentManipulationMode()
        {
            if (scrollPresenter.Content != null)
            {
                switch (scrollPresenter.Content.ManipulationMode)
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
                    scrollPresenter.Background = null;
                    break;
                case 1:
                    scrollPresenter.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scrollPresenter.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scrollPresenter.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }
        }

        private void UpdateCmbBackground()
        {
            SolidColorBrush bg = scrollPresenter.Background as SolidColorBrush;

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
                FrameworkElement currentContent = scrollPresenter.Content as FrameworkElement;
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
                    case 10:
                        newContent = textBlock;
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

                if (chkLogScrollPresenterEvents.IsChecked == true)
                {
                    UnhookContentEffectiveViewportChanged();
                }

                scrollPresenter.Content = newContent;

                if (chkLogScrollPresenterEvents.IsChecked == true)
                {
                    HookContentEffectiveViewportChanged();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());

                UpdateCmbContent();
            }
        }

        private void UpdateCmbContent()
        {
            if (scrollPresenter.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollPresenter.Content is Image)
            {
                if (((scrollPresenter.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollPresenter.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollPresenter.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollPresenter.Content is Border)
            {
                if ((scrollPresenter.Content as Border).Child is Rectangle)
                {
                    cmbContent.SelectedIndex = 5;
                }
                else
                {
                    cmbContent.SelectedIndex = 6;
                }
            }
            else if (scrollPresenter.Content is StackPanel)
            {
                cmbContent.SelectedIndex = 7;
            }
            else if (scrollPresenter.Content is TilePanel)
            {
                cmbContent.SelectedIndex = 8;
            }
            else if (scrollPresenter.Content is Viewbox)
            {
                cmbContent.SelectedIndex = 9;
            }
            else if (scrollPresenter.Content is TextBox)
            {
                cmbContent.SelectedIndex = 10;
            }
        }

        private void BtnGetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentMinWidth.Text = string.Empty;
            else
                txtContentMinWidth.Text = ((FrameworkElement)(scrollPresenter.Content)).MinWidth.ToString();
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scrollPresenter.Content)).Width.ToString();
        }

        private void BtnGetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentMaxWidth.Text = string.Empty;
            else
                txtContentMaxWidth.Text = ((FrameworkElement)(scrollPresenter.Content)).MaxWidth.ToString();
        }

        private void BtnSetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).MinWidth = Convert.ToDouble(txtContentMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).MaxWidth = Convert.ToDouble(txtContentMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentMinHeight.Text = string.Empty;
            else
                txtContentMinHeight.Text = ((FrameworkElement)(scrollPresenter.Content)).MinHeight.ToString();
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scrollPresenter.Content)).Height.ToString();
        }

        private void BtnGetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentMaxHeight.Text = string.Empty;
            else
                txtContentMaxHeight.Text = ((FrameworkElement)(scrollPresenter.Content)).MaxHeight.ToString();
        }

        private void BtnSetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).MinHeight = Convert.ToDouble(txtContentMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).MaxHeight = Convert.ToDouble(txtContentMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scrollPresenter.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollPresenter.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is Control || scrollPresenter.Content is Border || scrollPresenter.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollPresenter.Content is Control)
                txtContentPadding.Text = ((Control)(scrollPresenter.Content)).Padding.ToString();
            else if (scrollPresenter.Content is Border)
                txtContentPadding.Text = ((Border)(scrollPresenter.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scrollPresenter.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollPresenter.Content is Control)
                {
                    ((Control)(scrollPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollPresenter.Content is Border)
                {
                    ((Border)(scrollPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollPresenter.Content is StackPanel)
                {
                    ((StackPanel)(scrollPresenter.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentActualWidth.Text = string.Empty;
            else
                txtContentActualWidth.Text = (scrollPresenter.Content as FrameworkElement).ActualWidth.ToString();
        }

        private void BtnGetContentActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null || !(scrollPresenter.Content is FrameworkElement))
                txtContentActualHeight.Text = string.Empty;
            else
                txtContentActualHeight.Text = (scrollPresenter.Content as FrameworkElement).ActualHeight.ToString();
        }

        private void BtnGetContentDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null)
                txtContentDesiredSize.Text = string.Empty;
            else
                txtContentDesiredSize.Text = scrollPresenter.Content.DesiredSize.ToString();
        }

        private void BtnGetContentRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollPresenter.Content == null)
                txtContentRenderSize.Text = string.Empty;
            else
                txtContentRenderSize.Text = scrollPresenter.Content.RenderSize.ToString();
        }

        private void UpdateMinZoomFactor()
        {
            txtMinZoomFactor.Text = scrollPresenter.MinZoomFactor.ToString();
        }

        private void UpdateMaxZoomFactor()
        {
            txtMaxZoomFactor.Text = scrollPresenter.MaxZoomFactor.ToString();
        }

        private void BtnGetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMinZoomFactor();
        }

        private void BtnSetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                scrollPresenter.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblHorizontalOffset.Text = scrollPresenter.HorizontalOffset.ToString();
        }

        private void BtnGetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblVerticalOffset.Text = scrollPresenter.VerticalOffset.ToString();
        }

        private void BtnGetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            tblZoomFactor.Text = scrollPresenter.ZoomFactor.ToString();
        }

        private void BtnGetState_Click(object sender, RoutedEventArgs e)
        {
            tblState.Text = scrollPresenter.State.ToString();
        }

        private void BtnGetExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            tblExtentWidth.Text = scrollPresenter.ExtentWidth.ToString();
        }

        private void BtnGetExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            tblExtentHeight.Text = scrollPresenter.ExtentHeight.ToString();
        }

        private void BtnGetViewportWidth_Click(object sender, RoutedEventArgs e)
        {
            tblViewportWidth.Text = scrollPresenter.ViewportWidth.ToString();
        }

        private void BtnGetViewportHeight_Click(object sender, RoutedEventArgs e)
        {
            tblViewportHeight.Text = scrollPresenter.ViewportHeight.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            txtWidth.Text = scrollPresenter.Width.ToString();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            txtMaxWidth.Text = scrollPresenter.MaxWidth.ToString();
        }

        private void BtnSetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.MaxWidth = Convert.ToDouble(txtMaxWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            txtHeight.Text = scrollPresenter.Height.ToString();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            txtMaxHeight.Text = scrollPresenter.MaxHeight.ToString();
        }

        private void BtnSetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.MaxHeight = Convert.ToDouble(txtMaxHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetActualWidth_Click(object sender, RoutedEventArgs e)
        {
            tblActualWidth.Text = scrollPresenter.ActualWidth.ToString();
        }

        private void BtnGetActualHeight_Click(object sender, RoutedEventArgs e)
        {
            tblActualHeight.Text = scrollPresenter.ActualHeight.ToString();
        }

        private void BtnClearScrollPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollPresenterEvents.Items.Clear();
        }

        private void BtnCopyScrollPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstScrollPresenterEvents.Items)
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
                    lastOffsetsChangeCorrelationId = scrollPresenter.ScrollBy(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options);
                    relativeChangeCorrelationIds.Add(lastOffsetsChangeCorrelationId);
                    AppendAsyncEventMessage("Invoked ScrollBy CorrelationId=" + lastOffsetsChangeCorrelationId);
                }
                else
                {
                    lastOffsetsChangeCorrelationId = scrollPresenter.ScrollTo(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options);
                    AppendAsyncEventMessage("Invoked ScrollTo CorrelationId=" + lastOffsetsChangeCorrelationId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeCorrelationId != -1)
                {
                    AppendAsyncEventMessage("Canceling scrollTo/By CorrelationId=" + lastOffsetsChangeCorrelationId);
                    scrollPresenter.ScrollBy(
                        0,
                        0,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollPresenter_ScrollAnimationStarting(ScrollPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ScrollAnimationStarting OffsetsChangeCorrelationId=" + args.CorrelationId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y + ") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y + ")");

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    Vector3KeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex != 0)
                    {
                        bool isRelativeChange = relativeChangeCorrelationIds.Contains(args.CorrelationId);

                        double targetHorizontalOffset = Convert.ToDouble(txtScrollHorizontalOffset.Text);
                        if (isRelativeChange)
                        {
                            targetHorizontalOffset += scrollPresenter.HorizontalOffset;
                        }
                        float targetHorizontalPosition = ComputeHorizontalPositionFromOffset(targetHorizontalOffset);

                        double targetVerticalOffset = Convert.ToDouble(txtScrollVerticalOffset.Text);
                        if (isRelativeChange)
                        {
                            targetVerticalOffset += scrollPresenter.VerticalOffset;
                        }
                        float targetVerticalPosition = ComputeVerticalPositionFromOffset(targetVerticalOffset);

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateVector3KeyFrameAnimation();
                        if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaHorizontalPosition = 0.1f * (float)(targetHorizontalOffset - scrollPresenter.HorizontalOffset);
                            float deltaVerticalPosition = 0.1f * (float)(targetVerticalOffset - scrollPresenter.VerticalOffset);

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
                            float deltaHorizontalPosition = (float)(targetHorizontalOffset - scrollPresenter.HorizontalOffset);
                            float deltaVerticalPosition = (float)(targetVerticalOffset - scrollPresenter.VerticalOffset);

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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnAddScrollVelocity_Click(object sender, RoutedEventArgs e)
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

                lastOffsetsChangeWithAdditionalVelocityCorrelationId = scrollPresenter.AddScrollVelocity(
                    new Vector2(Convert.ToSingle(txtScrollHorizontalVelocity.Text), Convert.ToSingle(txtScrollVerticalVelocity.Text)),
                    inertiaDecayRate);
                AppendAsyncEventMessage("Invoked AddScrollVelocity CorrelationId=" + lastOffsetsChangeWithAdditionalVelocityCorrelationId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelAddScrollVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeWithAdditionalVelocityCorrelationId != -1)
                {
                    AppendAsyncEventMessage("Canceling AddScrollVelocity CorrelationId=" + lastOffsetsChangeWithAdditionalVelocityCorrelationId);
                    scrollPresenter.ScrollBy(
                        0,
                        0,
                        new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private float ComputeHorizontalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinHorizontalPosition(scrollPresenter.ZoomFactor));
        }

        private float ComputeVerticalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinVerticalPosition(scrollPresenter.ZoomFactor));
        }

        private float ComputeMinHorizontalPosition(float zoomFactor)
        {
            UIElement content = scrollPresenter.Content;

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
            Visual scrollPresenterVisual = ElementCompositionPreview.GetElementVisual(scrollPresenter);
            double contentWidth = double.IsNaN(contentAsFE.Width) ? contentAsFE.ActualWidth : contentAsFE.Width;
            float minPosX = 0.0f;
            float extentWidth = Math.Max(0.0f, (float)(contentWidth + contentMargin.Left + contentMargin.Right));

            if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Center ||
                contentAsFE.HorizontalAlignment == HorizontalAlignment.Stretch)
            {
                minPosX = Math.Min(0.0f, (extentWidth * zoomFactor - scrollPresenterVisual.Size.X) / 2.0f);
            }
            else if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Right)
            {
                minPosX = Math.Min(0.0f, extentWidth * zoomFactor - scrollPresenterVisual.Size.X);
            }

            return minPosX;
        }

        private float ComputeMinVerticalPosition(float zoomFactor)
        {
            UIElement content = scrollPresenter.Content;

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
            Visual scrollPresenterVisual = ElementCompositionPreview.GetElementVisual(scrollPresenter);
            double contentHeight = double.IsNaN(contentAsFE.Height) ? contentAsFE.ActualHeight : contentAsFE.Height;
            float minPosY = 0.0f;
            float extentHeight = Math.Max(0.0f, (float)(contentHeight + contentMargin.Top + contentMargin.Bottom));

            if (contentAsFE.VerticalAlignment == VerticalAlignment.Center ||
                contentAsFE.VerticalAlignment == VerticalAlignment.Stretch)
            {
                minPosY = Math.Min(0.0f, (extentHeight * zoomFactor - scrollPresenterVisual.Size.Y) / 2.0f);
            }
            else if (contentAsFE.VerticalAlignment == VerticalAlignment.Bottom)
            {
                minPosY = Math.Min(0.0f, extentHeight * zoomFactor - scrollPresenterVisual.Size.Y);
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
                    lastZoomFactorChangeCorrelationId = scrollPresenter.ZoomBy(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options);
                    relativeChangeCorrelationIds.Add(lastZoomFactorChangeCorrelationId);
                    AppendAsyncEventMessage("Invoked ZoomBy CorrelationId=" + lastZoomFactorChangeCorrelationId);
                }
                else
                {
                    lastZoomFactorChangeCorrelationId = scrollPresenter.ZoomTo(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options);
                    AppendAsyncEventMessage("Invoked ZoomTo CorrelationId=" + lastZoomFactorChangeCorrelationId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelZoom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeCorrelationId != -1)
                {
                    AppendAsyncEventMessage("Canceling ZoomTo/By CorrelationId=" + lastZoomFactorChangeCorrelationId);
                    scrollPresenter.ZoomBy(
                        0,
                        Vector2.Zero,
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollPresenter_ZoomAnimationStarting(ScrollPresenter sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ZoomAnimationStarting ZoomFactorChangeCorrelationId=" + args.CorrelationId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);

                ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    ScalarKeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex != 0)
                    {
                        float targetZoomFactor = Convert.ToSingle(txtZoomZoomFactor.Text);
                        if (relativeChangeCorrelationIds.Contains(args.CorrelationId))
                        {
                            targetZoomFactor += scrollPresenter.ZoomFactor;
                        }

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateScalarKeyFrameAnimation();
                        if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaZoomFactor = 0.1f * (float)(targetZoomFactor - scrollPresenter.ZoomFactor);

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
                            float deltaZoomFactor = (float)(targetZoomFactor - scrollPresenter.ZoomFactor);

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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnAddZoomVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastZoomFactorChangeWithAdditionalVelocityCorrelationId = scrollPresenter.AddZoomVelocity(
                    Convert.ToSingle(txtAddZoomVelocityVelocity.Text),
                    (txtAddZoomVelocityCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtAddZoomVelocityCenterPoint.Text),
                    (txtAddZoomVelocityInertiaDecayRate.Text == "null") ? (float?)null : (float?)Convert.ToSingle(txtAddZoomVelocityInertiaDecayRate.Text));
                AppendAsyncEventMessage("Invoked AddZoomVelocity CorrelationId=" + lastZoomFactorChangeWithAdditionalVelocityCorrelationId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelAddZoomVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeWithAdditionalVelocityCorrelationId != -1)
                {
                    AppendAsyncEventMessage("Canceling AddZoomVelocity CorrelationId=" + lastZoomFactorChangeWithAdditionalVelocityCorrelationId);
                    scrollPresenter.ZoomBy(
                        0,
                        Vector2.Zero,
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                    scrollPresenter.VerticalSnapPoints.Add(snapPoint);
                }
                else if(cmbSnapPointKind.SelectedIndex == 1)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text), ScrollSnapPointsAlignment.Near);
                    scrollPresenter.HorizontalSnapPoints.Add(snapPoint); 
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text));
                    scrollPresenter.ZoomSnapPoints.Add(snapPoint);
                }
#else
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text),
                        (ScrollSnapPointsAlignment)cmbSnapPointAlignment.SelectedIndex);
                    if (cmbSnapPointKind.SelectedIndex == 0)
                    {
                        scrollPresenter.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scrollPresenter.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text));
                    scrollPresenter.ZoomSnapPoints.Add(snapPoint);
                }
#endif
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                        scrollPresenter.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scrollPresenter.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    RepeatedZoomSnapPoint snapPoint = new RepeatedZoomSnapPoint(
                        Convert.ToDouble(txtRepeatedSnapPointOffset.Text),
                        Convert.ToDouble(txtRepeatedSnapPointInterval.Text),
                        Convert.ToDouble(txtRepeatedSnapPointStart.Text),
                        Convert.ToDouble(txtRepeatedSnapPointEnd.Text));
                    scrollPresenter.ZoomSnapPoints.Add(snapPoint);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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

                ComboBox cmbAnimationMode = (zoomFactor == null || zoomFactor == scrollPresenter.ZoomFactor) ?
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

                ChangeView(scrollPresenter, horizontalOffset, verticalOffset, zoomFactor, disableAnimation);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private bool ChangeView(ScrollPresenter scrollPresenter, double? horizontalOffset, double? verticalOffset, float? zoomFactor, bool disableAnimation)
        {
            AppendAsyncEventMessage($"ChangeView(horizontalOffset:{horizontalOffset}, verticalOffset:{verticalOffset}, zoomFactor:{zoomFactor}, disableAnimation:{disableAnimation}) from horizontalOffset:{scrollPresenter.HorizontalOffset}, verticalOffset:{scrollPresenter.VerticalOffset}, zoomFactor:{scrollPresenter.ZoomFactor}");

            double targetHorizontalOffset = horizontalOffset == null ? scrollPresenter.HorizontalOffset : (double)horizontalOffset;
            double targetVerticalOffset = verticalOffset == null ? scrollPresenter.VerticalOffset : (double)verticalOffset;
            float targetZoomFactor = zoomFactor == null ? scrollPresenter.ZoomFactor : (float)Math.Max(Math.Min((double)zoomFactor, scrollPresenter.MaxZoomFactor), scrollPresenter.MinZoomFactor);
            float deltaZoomFactor = targetZoomFactor - scrollPresenter.ZoomFactor;

            if (disableAnimation)
            {
                targetHorizontalOffset = Math.Max(Math.Min(targetHorizontalOffset, scrollPresenter.ExtentWidth * targetZoomFactor - scrollPresenter.ViewportWidth), 0.0);
                targetVerticalOffset = Math.Max(Math.Min(targetVerticalOffset, scrollPresenter.ExtentHeight * targetZoomFactor - scrollPresenter.ViewportHeight), 0.0);
            }
            // During an animation, out-of-bounds offsets may become valid as the extents are dynamically updated, so no clamping is performed for animated view changes.

            if (deltaZoomFactor == 0.0f)
            {
                if (targetHorizontalOffset == scrollPresenter.HorizontalOffset && targetVerticalOffset == scrollPresenter.VerticalOffset)
                {
                    AppendAsyncEventMessage("ChangeView no-op");
                    return false;
                }

                lastOffsetsChangeCorrelationId = scrollPresenter.ScrollTo(
                    targetHorizontalOffset,
                    targetVerticalOffset,
                    new ScrollingScrollOptions(
                        disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                        disableAnimation ? ScrollingSnapPointsMode.Ignore : ScrollingSnapPointsMode.Default));
                AppendAsyncEventMessage($"ChangeView invoked ScrollTo(horizontalOffset:{targetHorizontalOffset}, verticalOffset:{targetVerticalOffset}) CorrelationId={lastOffsetsChangeCorrelationId}");
            }
            else
            {
                FrameworkElement contentAsFE = scrollPresenter.Content as FrameworkElement;
                HorizontalAlignment contentHorizontalAlignment = contentAsFE == null ? HorizontalAlignment.Stretch : contentAsFE.HorizontalAlignment;
                VerticalAlignment contentVerticalAlignment = contentAsFE == null ? VerticalAlignment.Stretch : contentAsFE.VerticalAlignment;
                double currentPositionX = scrollPresenter.HorizontalOffset;
                double currentPositionY = scrollPresenter.VerticalOffset;
                double currentViewportExcessX = scrollPresenter.ViewportWidth - scrollPresenter.ExtentWidth * scrollPresenter.ZoomFactor;
                double targetViewportExcessX = scrollPresenter.ViewportWidth - scrollPresenter.ExtentWidth * targetZoomFactor;
                double currentViewportExcessY = scrollPresenter.ViewportHeight - scrollPresenter.ExtentHeight * scrollPresenter.ZoomFactor;
                double targetViewportExcessY = scrollPresenter.ViewportHeight - scrollPresenter.ExtentHeight * targetZoomFactor;

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
                    (float)(targetHorizontalOffset * scrollPresenter.ZoomFactor - currentPositionX * targetZoomFactor) / deltaZoomFactor,
                    (float)(targetVerticalOffset * scrollPresenter.ZoomFactor - currentPositionY * targetZoomFactor) / deltaZoomFactor);


                lastZoomFactorChangeCorrelationId = scrollPresenter.ZoomTo(
                    targetZoomFactor,
                    centerPoint,
                    new ScrollingZoomOptions(
                        disableAnimation ? ScrollingAnimationMode.Disabled : ScrollingAnimationMode.Enabled,
                        disableAnimation ? ScrollingSnapPointsMode.Ignore : ScrollingSnapPointsMode.Default));
                AppendAsyncEventMessage($"ChangeView invoked ZoomBy(zoomFactor:{targetZoomFactor}, centerPoint:{centerPoint}) targetting horizontalOffset:{targetHorizontalOffset}, verticalOffset:{targetVerticalOffset} CorrelationId={lastZoomFactorChangeCorrelationId}");
            }

            return true;
        }

        private void BtnClearSnapPoints_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    scrollPresenter.VerticalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 1)
                {
                    scrollPresenter.HorizontalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    scrollPresenter.ZoomSnapPoints.Clear();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                    lstScrollPresenterEvents.Items.Add(asyncEventMessage);
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

        private void BtnParentMarkupScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(markupScrollPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentMarkupScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScrollPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentReparentMarkupScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScrollPresenter);
                root.Children.Add(markupScrollPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCreateDynamicScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                dynamicScrollPresenter = new ScrollPresenter();
                dynamicScrollPresenter.Name = "dynamicScrollPresenter";
                dynamicScrollPresenter.Width = 300.0;
                dynamicScrollPresenter.Height = 400.0;
                dynamicScrollPresenter.Margin = new Thickness(1);
                dynamicScrollPresenter.Background = new SolidColorBrush(Colors.HotPink);
                dynamicScrollPresenter.VerticalAlignment = VerticalAlignment.Top;
                Grid.SetRow(dynamicScrollPresenter, 1);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnParentDynamicScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(dynamicScrollPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentDynamicScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(dynamicScrollPresenter);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUseMarkupScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            UseScrollPresenter(markupScrollPresenter);
        }

        private void BtnUseDynamicScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            UseScrollPresenter(dynamicScrollPresenter);
        }

        private void BtnReleaseDynamicScrollPresenter_Click(object sender, RoutedEventArgs e)
        {
            dynamicScrollPresenter = null;

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

                ScrollPresenterTestHooks.GetContentLayoutOffsetX(scrollPresenter, out contentLayoutOffsetX);

                txtContentLayoutOffsetX.Text = contentLayoutOffsetX.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollPresenterTestHooks.SetContentLayoutOffsetX(scrollPresenter, Convert.ToSingle(txtContentLayoutOffsetX.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float contentLayoutOffsetY = 0.0f;

                ScrollPresenterTestHooks.GetContentLayoutOffsetY(scrollPresenter, out contentLayoutOffsetY);

                txtContentLayoutOffsetY.Text = contentLayoutOffsetY.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollPresenterTestHooks.SetContentLayoutOffsetY(scrollPresenter, Convert.ToSingle(txtContentLayoutOffsetY.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetArrangeRenderSizesDelta_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtArrangeRenderSizesDelta.Text = ScrollPresenterTestHooks.GetArrangeRenderSizesDelta(scrollPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMinPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMinPosition.Text = ScrollPresenterTestHooks.GetMinPosition(scrollPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMaxPosition.Text = ScrollPresenterTestHooks.GetMaxPosition(scrollPresenter).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void UseScrollPresenter(ScrollPresenter s)
        {
            if (scrollPresenter == s || s == null)
                return;

            try
            {
                if (scrollPresenter != null)
                {
                    if (chkLogScrollPresenterMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
                    }

                    if (chkLogScrollPresenterEvents.IsChecked == true)
                    {
                        scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChanged;
                        scrollPresenter.StateChanged -= ScrollPresenter_StateChanged;
                        scrollPresenter.ViewChanged -= ScrollPresenter_ViewChanged;
                        scrollPresenter.ScrollCompleted -= ScrollPresenter_ScrollCompleted;
                        scrollPresenter.ZoomCompleted -= ScrollPresenter_ZoomCompleted;
                        scrollPresenter.ScrollAnimationStarting -= ScrollPresenter_ScrollAnimationStarting;
                        scrollPresenter.ZoomAnimationStarting -= ScrollPresenter_ZoomAnimationStarting;
                    }

                    if (chkLogScrollPresenterEvents.IsChecked == true)
                    {
                        UnhookContentEffectiveViewportChanged();
                    }
                }

                scrollPresenter = s;

                if (chkLogScrollPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
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
                UpdateCmbIgnoredInputKinds();
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbScrollPresenterManipulationMode();
                UpdateCmbContentManipulationMode();
                UpdateCmbBackground();
                UpdateCmbContent();
                UpdateMinZoomFactor();
                UpdateMaxZoomFactor();

                txtWidth.Text = string.Empty;
                txtHeight.Text = string.Empty;

                if (scrollPresenter != null)
                {
                    if (chkLogScrollPresenterEvents.IsChecked == true)
                    {
                        scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
                        scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
                        scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
                        scrollPresenter.ScrollCompleted += ScrollPresenter_ScrollCompleted;
                        scrollPresenter.ZoomCompleted += ScrollPresenter_ZoomCompleted;
                        scrollPresenter.ScrollAnimationStarting += ScrollPresenter_ScrollAnimationStarting;
                        scrollPresenter.ZoomAnimationStarting += ScrollPresenter_ZoomAnimationStarting;
                    }

                    if (chkLogScrollPresenterEvents.IsChecked == true)
                    {
                        HookContentEffectiveViewportChanged();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
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
                            scrollPresenter.MinZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMinZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetMaxZoomFactor:
                            scrollPresenter.MaxZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMaxZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetWidth:
                            scrollPresenter.Width = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetHeight:
                            scrollPresenter.Height = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentWidth:
                            if (scrollPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollPresenter.Content)).Width = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentHeight:
                            if (scrollPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollPresenter.Content)).Height = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentMargin:
                            if (scrollPresenter.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scrollPresenter.Content)).Margin = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetContentMargin " + qo.StringValue);
                            break;
                        case QueuedOperationType.SetContentPadding:
                            if (scrollPresenter.Content is Control)
                            {
                                ((Control)(scrollPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scrollPresenter.Content is Border)
                            {
                                ((Border)(scrollPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scrollPresenter.Content is StackPanel)
                            {
                                ((StackPanel)(scrollPresenter.Content)).Padding = GetThicknessFromString(qo.StringValue);
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
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }
    }
}
