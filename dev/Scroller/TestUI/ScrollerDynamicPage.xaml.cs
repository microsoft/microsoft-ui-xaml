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

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using RepeatedScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedScrollSnapPoint;
using ZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint;
using RepeatedZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ZoomAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;

using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerViewChangeResult = Microsoft.UI.Private.Controls.ScrollerViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerDynamicPage : TestPage
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
        private Scroller dynamicScroller = null;
        private Scroller scroller = null;

        public ScrollerDynamicPage()
        {
            this.InitializeComponent();

            UseScroller(markupScroller);

            CreateChildren();
        }

        private void Scroller_ExtentChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("StateChanged " + sender.State.ToString());
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void Scroller_ScrollCompleted(Scroller sender, ScrollCompletedEventArgs args)
        {
            ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage("ScrollCompleted OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
        }

        private void Scroller_ZoomCompleted(Scroller sender, ZoomCompletedEventArgs args)
        {
            ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

            AppendAsyncEventMessage("ZoomCompleted ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
        }

        private void ZoomCompleted(Scroller sender, ZoomCompletedEventArgs args)
        {
            ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

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

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for a particular Scroller instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scroller, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            //To turn on info and verbose logging for the Scroller type:
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for a particular Scroller instance:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(scroller, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging without any filter:
            //MUXControlsTestHooks.SetLoggingLevelForInstance(null, isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            //To turn off info and verbose logging for the Scroller type:
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            scroller.ExtentChanged += Scroller_ExtentChanged;
            scroller.StateChanged += Scroller_StateChanged;
            scroller.ViewChanged += Scroller_ViewChanged;
            scroller.ScrollCompleted += Scroller_ScrollCompleted;
            scroller.ZoomCompleted += Scroller_ZoomCompleted;
            scroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
            scroller.ZoomAnimationStarting += Scroller_ZoomAnimationStarting;
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scroller.ExtentChanged -= Scroller_ExtentChanged;
            scroller.StateChanged -= Scroller_StateChanged;
            scroller.ViewChanged -= Scroller_ViewChanged;
            scroller.ScrollCompleted -= Scroller_ScrollCompleted;
            scroller.ZoomCompleted -= Scroller_ZoomCompleted;
            scroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
            scroller.ZoomAnimationStarting -= Scroller_ZoomAnimationStarting;
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
            if (scroller != null)
            {
                FrameworkElement contentAsFE = scroller.Content as FrameworkElement;
                if (contentAsFE != null)
                {
                    contentAsFE.EffectiveViewportChanged += Content_EffectiveViewportChanged;
                }
            }
        }

        private void UnhookContentEffectiveViewportChanged()
        {
            if (scroller != null)
            {
                FrameworkElement contentAsFE = scroller.Content as FrameworkElement;
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

        private void ChkScrollerProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollerProperties != null)
                svScrollerProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollerProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollerProperties != null)
                svScrollerProperties.Visibility = Visibility.Collapsed;
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

        private void ChkScrollerMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollerMethods != null)
                svScrollerMethods.Visibility = Visibility.Visible;
        }

        private void ChkScrollerMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollerMethods != null)
                svScrollerMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerEvents != null)
                grdScrollerEvents.Visibility = Visibility.Visible;
        }

        private void ChkScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerEvents != null)
                grdScrollerEvents.Visibility = Visibility.Collapsed;
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

        private void ChkScrollerTestHooks_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerTestHooks != null)
                grdScrollerTestHooks.Visibility = Visibility.Visible;
        }

        private void ChkScrollerTestHooks_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerTestHooks != null)
                grdScrollerTestHooks.Visibility = Visibility.Collapsed;
        }

        private void CmbContentOrientation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.ContentOrientation = (ContentOrientation)cmbContentOrientation.SelectedIndex;
        }

        private void CmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.HorizontalScrollMode = (ScrollMode)cmbHorizontalScrollMode.SelectedIndex;
        }

        private void UpdateCmbContentOrientation()
        {
            try
            {
                cmbContentOrientation.SelectedIndex = (int)scroller.ContentOrientation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbHorizontalScrollMode()
        {
            cmbHorizontalScrollMode.SelectedIndex = (int)scroller.HorizontalScrollMode;
        }

        private void CmbHorizontalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.HorizontalScrollChainingMode = (ChainingMode)cmbHorizontalScrollChainingMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollChainingMode()
        {
            cmbHorizontalScrollChainingMode.SelectedIndex = (int)scroller.HorizontalScrollChainingMode;
        }

        private void CmbHorizontalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.HorizontalScrollRailingMode = (RailingMode)cmbHorizontalScrollRailingMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollRailingMode()
        {
            cmbHorizontalScrollRailingMode.SelectedIndex = (int)scroller.HorizontalScrollRailingMode;
        }

        private void CmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollMode = (ScrollMode)cmbVerticalScrollMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollMode()
        {
            cmbVerticalScrollMode.SelectedIndex = (int)scroller.VerticalScrollMode;
        }

        private void CmbVerticalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollChainingMode = (ChainingMode)cmbVerticalScrollChainingMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollChainingMode()
        {
            cmbVerticalScrollChainingMode.SelectedIndex = (int)scroller.VerticalScrollChainingMode;
        }

        private void CmbVerticalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollRailingMode = (RailingMode)cmbVerticalScrollRailingMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollRailingMode()
        {
            cmbVerticalScrollRailingMode.SelectedIndex = (int)scroller.VerticalScrollRailingMode;
        }

        private void CmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.ZoomMode = (ZoomMode)cmbZoomMode.SelectedIndex;
        }

        private void UpdateCmbZoomMode()
        {
            cmbZoomMode.SelectedIndex = (int)scroller.ZoomMode;
        }

        private void CmbZoomChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.ZoomChainingMode = (ChainingMode)cmbZoomChainingMode.SelectedIndex;
        }

        private void UpdateCmbZoomChainingMode()
        {
            cmbZoomChainingMode.SelectedIndex = (int)scroller.ZoomChainingMode;
        }

        private void CmbIgnoredInputKind_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
            {
                switch (cmbIgnoredInputKind.SelectedIndex)
                {
                    case 0:
                        scroller.IgnoredInputKind = InputKind.None;
                        break;
                    case 1:
                        scroller.IgnoredInputKind = InputKind.Touch;
                        break;
                    case 2:
                        scroller.IgnoredInputKind = InputKind.Pen;
                        break;
                    case 3:
                        scroller.IgnoredInputKind = InputKind.MouseWheel;
                        break;
                    case 4:
                        scroller.IgnoredInputKind = InputKind.Keyboard;
                        break;
                    case 5:
                        scroller.IgnoredInputKind = InputKind.Gamepad;
                        break;
                    case 6:
                        scroller.IgnoredInputKind = InputKind.All;
                        break;
                }
            }
        }

        private void UpdateCmbIgnoredInputKind()
        {
            switch (scroller.IgnoredInputKind)
            {
                case InputKind.None:
                    cmbIgnoredInputKind.SelectedIndex = 0;
                    break;
                case InputKind.Touch:
                    cmbIgnoredInputKind.SelectedIndex = 1;
                    break;
                case InputKind.Pen:
                    cmbIgnoredInputKind.SelectedIndex = 2;
                    break;
                case InputKind.MouseWheel:
                    cmbIgnoredInputKind.SelectedIndex = 3;
                    break;
                case InputKind.Keyboard:
                    cmbIgnoredInputKind.SelectedIndex = 4;
                    break;
                case InputKind.Gamepad:
                    cmbIgnoredInputKind.SelectedIndex = 5;
                    break;
                case InputKind.All:
                    cmbIgnoredInputKind.SelectedIndex = 6;
                    break;
            }
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Content is FrameworkElement)
            {
                ((FrameworkElement)scroller.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentHorizontalAlignment()
        {
            if (scroller.Content is FrameworkElement)
            {
                cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scroller.Content).HorizontalAlignment;
            }
        }

        private void CmbContentVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Content is FrameworkElement)
            {
                ((FrameworkElement)scroller.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentVerticalAlignment()
        {
            if (scroller.Content is FrameworkElement)
            {
                cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scroller.Content).VerticalAlignment;
            }
        }

        private void CmbScrollerManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbScrollerManipulationMode.SelectedIndex)
            {
                case 0:
                    scroller.ManipulationMode = ManipulationModes.System;
                    break;
                case 1:
                    scroller.ManipulationMode = ManipulationModes.None;
                    break;
            }
        }

        private void UpdateCmbScrollerManipulationMode()
        {
            switch (scroller.ManipulationMode)
            {
                case ManipulationModes.System:
                    cmbScrollerManipulationMode.SelectedIndex = 0;
                    break;
                case ManipulationModes.None:
                    cmbScrollerManipulationMode.SelectedIndex = 1;
                    break;
            }
        }

        private void CmbContentManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Content is FrameworkElement)
            {
                switch (cmbContentManipulationMode.SelectedIndex)
                {
                    case 0:
                        scroller.Content.ManipulationMode = ManipulationModes.System;
                        break;
                    case 1:
                        scroller.Content.ManipulationMode = ManipulationModes.None;
                        break;
                }
            }
        }

        private void UpdateCmbContentManipulationMode()
        {
            if (scroller.Content != null)
            {
                switch (scroller.Content.ManipulationMode)
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
                    scroller.Background = null;
                    break;
                case 1:
                    scroller.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scroller.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scroller.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }
        }

        private void UpdateCmbBackground()
        {
            SolidColorBrush bg = scroller.Background as SolidColorBrush;

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
                FrameworkElement currentContent = scroller.Content as FrameworkElement;
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

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    UnhookContentEffectiveViewportChanged();
                }

                scroller.Content = newContent;

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    HookContentEffectiveViewportChanged();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());

                UpdateCmbContent();
            }
        }

        private void UpdateCmbContent()
        {
            if (scroller.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scroller.Content is Image)
            {
                if (((scroller.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scroller.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scroller.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scroller.Content is Border)
            {
                if ((scroller.Content as Border).Child is Rectangle)
                {
                    cmbContent.SelectedIndex = 5;
                }
                else
                {
                    cmbContent.SelectedIndex = 6;
                }
            }
            else if (scroller.Content is StackPanel)
            {
                cmbContent.SelectedIndex = 7;
            }
            else if (scroller.Content is TilePanel)
            {
                cmbContent.SelectedIndex = 8;
            }
            else if (scroller.Content is Viewbox)
            {
                cmbContent.SelectedIndex = 9;
            }
        }

        private void BtnGetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentMinWidth.Text = string.Empty;
            else
                txtContentMinWidth.Text = ((FrameworkElement)(scroller.Content)).MinWidth.ToString();
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scroller.Content)).Width.ToString();
        }

        private void BtnGetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentMaxWidth.Text = string.Empty;
            else
                txtContentMaxWidth.Text = ((FrameworkElement)(scroller.Content)).MaxWidth.ToString();
        }

        private void BtnSetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).MinWidth = Convert.ToDouble(txtContentMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).MaxWidth = Convert.ToDouble(txtContentMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentMinHeight.Text = string.Empty;
            else
                txtContentMinHeight.Text = ((FrameworkElement)(scroller.Content)).MinHeight.ToString();
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scroller.Content)).Height.ToString();
        }

        private void BtnGetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentMaxHeight.Text = string.Empty;
            else
                txtContentMaxHeight.Text = ((FrameworkElement)(scroller.Content)).MaxHeight.ToString();
        }

        private void BtnSetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).MinHeight = Convert.ToDouble(txtContentMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).MaxHeight = Convert.ToDouble(txtContentMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scroller.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is Control || scroller.Content is Border || scroller.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scroller.Content is Control)
                txtContentPadding.Text = ((Control)(scroller.Content)).Padding.ToString();
            else if (scroller.Content is Border)
                txtContentPadding.Text = ((Border)(scroller.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scroller.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Content is Control)
                {
                    ((Control)(scroller.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scroller.Content is Border)
                {
                    ((Border)(scroller.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scroller.Content is StackPanel)
                {
                    ((StackPanel)(scroller.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentActualWidth.Text = string.Empty;
            else
                txtContentActualWidth.Text = (scroller.Content as FrameworkElement).ActualWidth.ToString();
        }

        private void BtnGetContentActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null || !(scroller.Content is FrameworkElement))
                txtContentActualHeight.Text = string.Empty;
            else
                txtContentActualHeight.Text = (scroller.Content as FrameworkElement).ActualHeight.ToString();
        }

        private void BtnGetContentDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null)
                txtContentDesiredSize.Text = string.Empty;
            else
                txtContentDesiredSize.Text = scroller.Content.DesiredSize.ToString();
        }

        private void BtnGetContentRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Content == null)
                txtContentRenderSize.Text = string.Empty;
            else
                txtContentRenderSize.Text = scroller.Content.RenderSize.ToString();
        }

        private void UpdateMinZoomFactor()
        {
            txtMinZoomFactor.Text = scroller.MinZoomFactor.ToString();
        }

        private void UpdateMaxZoomFactor()
        {
            txtMaxZoomFactor.Text = scroller.MaxZoomFactor.ToString();
        }

        private void BtnGetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMinZoomFactor();
        }

        private void BtnSetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
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
                scroller.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblHorizontalOffset.Text = scroller.HorizontalOffset.ToString();
        }

        private void BtnGetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblVerticalOffset.Text = scroller.VerticalOffset.ToString();
        }

        private void BtnGetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            tblZoomFactor.Text = scroller.ZoomFactor.ToString();
        }

        private void BtnGetState_Click(object sender, RoutedEventArgs e)
        {
            tblState.Text = scroller.State.ToString();
        }

        private void BtnGetExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            tblExtentWidth.Text = scroller.ExtentWidth.ToString();
        }

        private void BtnGetExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            tblExtentHeight.Text = scroller.ExtentHeight.ToString();
        }

        private void BtnGetViewportWidth_Click(object sender, RoutedEventArgs e)
        {
            tblViewportWidth.Text = scroller.ViewportWidth.ToString();
        }

        private void BtnGetViewportHeight_Click(object sender, RoutedEventArgs e)
        {
            tblViewportHeight.Text = scroller.ViewportHeight.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            txtWidth.Text = scroller.Width.ToString();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            txtMaxWidth.Text = scroller.MaxWidth.ToString();
        }

        private void BtnSetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.MaxWidth = Convert.ToDouble(txtMaxWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            txtHeight.Text = scroller.Height.ToString();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            txtMaxHeight.Text = scroller.MaxHeight.ToString();
        }

        private void BtnSetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scroller.MaxHeight = Convert.ToDouble(txtMaxHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetActualWidth_Click(object sender, RoutedEventArgs e)
        {
            tblActualWidth.Text = scroller.ActualWidth.ToString();
        }

        private void BtnGetActualHeight_Click(object sender, RoutedEventArgs e)
        {
            tblActualHeight.Text = scroller.ActualHeight.ToString();
        }

        private void BtnClearScrollerEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollerEvents.Items.Clear();
        }

        private void BtnCopyScrollerEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstScrollerEvents.Items)
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
                AnimationMode animatioMode = (AnimationMode)cmbScrollAnimationMode.SelectedIndex;
                SnapPointsMode snapPointsMode = (SnapPointsMode)cmbScrollSnapPointsMode.SelectedIndex;
                ScrollOptions options = new ScrollOptions(animatioMode, snapPointsMode);

                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                if (isRelativeChange)
                {
                    lastOffsetsChangeId = scroller.ScrollBy(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options).OffsetsChangeId;
                    relativeChangeIds.Add(lastOffsetsChangeId);
                    AppendAsyncEventMessage("Invoked ScrollBy Id=" + lastOffsetsChangeId);
                }
                else
                {
                    lastOffsetsChangeId = scroller.ScrollTo(
                        Convert.ToDouble(txtScrollHorizontalOffset.Text),
                        Convert.ToDouble(txtScrollVerticalOffset.Text),
                        options).OffsetsChangeId;
                    AppendAsyncEventMessage("Invoked ScrollTo Id=" + lastOffsetsChangeId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeId != -1)
                {
                    AppendAsyncEventMessage("Canceling scrollTo/By Id=" + lastOffsetsChangeId);
                    scroller.ScrollBy(
                        0,
                        0,
                        new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_ScrollAnimationStarting(Scroller sender, ScrollAnimationStartingEventArgs args)
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
                            targetHorizontalOffset += scroller.HorizontalOffset;
                        }
                        float targetHorizontalPosition = ComputeHorizontalPositionFromOffset(targetHorizontalOffset);

                        double targetVerticalOffset = Convert.ToDouble(txtScrollVerticalOffset.Text);
                        if (isRelativeChange)
                        {
                            targetVerticalOffset += scroller.VerticalOffset;
                        }
                        float targetVerticalPosition = ComputeVerticalPositionFromOffset(targetVerticalOffset);

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateVector3KeyFrameAnimation();
                        if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaHorizontalPosition = 0.1f * (float)(targetHorizontalOffset - scroller.HorizontalOffset);
                            float deltaVerticalPosition = 0.1f * (float)(targetVerticalOffset - scroller.VerticalOffset);

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
                            float deltaHorizontalPosition = (float)(targetHorizontalOffset - scroller.HorizontalOffset);
                            float deltaVerticalPosition = (float)(targetVerticalOffset - scroller.VerticalOffset);

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
                lstScrollerEvents.Items.Add(ex.ToString());
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

                lastOffsetsChangeWithAdditionalVelocityId = scroller.ScrollFrom(
                    new Vector2(Convert.ToSingle(txtScrollHorizontalVelocity.Text), Convert.ToSingle(txtScrollVerticalVelocity.Text)),
                    inertiaDecayRate).OffsetsChangeId;
                AppendAsyncEventMessage("Invoked ScrollFrom Id=" + lastOffsetsChangeWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelScrollFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastOffsetsChangeWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ScrollFrom Id=" + lastOffsetsChangeWithAdditionalVelocityId);
                    scroller.ScrollBy(
                        0,
                        0,
                        new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private float ComputeHorizontalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinHorizontalPosition(scroller.ZoomFactor));
        }

        private float ComputeVerticalPositionFromOffset(double offset)
        {
            return (float)(offset + ComputeMinVerticalPosition(scroller.ZoomFactor));
        }

        private float ComputeMinHorizontalPosition(float zoomFactor)
        {
            UIElement content = scroller.Content;

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
            Visual scrollerVisual = ElementCompositionPreview.GetElementVisual(scroller);
            double contentWidth = double.IsNaN(contentAsFE.Width) ? contentAsFE.ActualWidth : contentAsFE.Width;
            float minPosX = 0.0f;
            float extentWidth = Math.Max(0.0f, (float)(contentWidth + contentMargin.Left + contentMargin.Right));

            if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Center ||
                contentAsFE.HorizontalAlignment == HorizontalAlignment.Stretch)
            {
                minPosX = Math.Min(0.0f, (extentWidth * zoomFactor - scrollerVisual.Size.X) / 2.0f);
            }
            else if (contentAsFE.HorizontalAlignment == HorizontalAlignment.Right)
            {
                minPosX = Math.Min(0.0f, extentWidth * zoomFactor - scrollerVisual.Size.X);
            }

            return minPosX;
        }

        private float ComputeMinVerticalPosition(float zoomFactor)
        {
            UIElement content = scroller.Content;

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
            Visual scrollerVisual = ElementCompositionPreview.GetElementVisual(scroller);
            double contentHeight = double.IsNaN(contentAsFE.Height) ? contentAsFE.ActualHeight : contentAsFE.Height;
            float minPosY = 0.0f;
            float extentHeight = Math.Max(0.0f, (float)(contentHeight + contentMargin.Top + contentMargin.Bottom));

            if (contentAsFE.VerticalAlignment == VerticalAlignment.Center ||
                contentAsFE.VerticalAlignment == VerticalAlignment.Stretch)
            {
                minPosY = Math.Min(0.0f, (extentHeight * zoomFactor - scrollerVisual.Size.Y) / 2.0f);
            }
            else if (contentAsFE.VerticalAlignment == VerticalAlignment.Bottom)
            {
                minPosY = Math.Min(0.0f, extentHeight * zoomFactor - scrollerVisual.Size.Y);
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
                AnimationMode animationMode = (AnimationMode)cmbZoomAnimationMode.SelectedIndex;
                SnapPointsMode snapPointsMode = (SnapPointsMode)cmbZoomSnapPointsMode.SelectedIndex;
                ZoomOptions options = new ZoomOptions(animationMode, snapPointsMode);

                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                if (isRelativeChange)
                {
                    lastZoomFactorChangeId = scroller.ZoomBy(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options).ZoomFactorChangeId;
                    relativeChangeIds.Add(lastZoomFactorChangeId);
                    AppendAsyncEventMessage("Invoked ZoomBy Id=" + lastZoomFactorChangeId);
                }
                else
                {
                    lastZoomFactorChangeId = scroller.ZoomTo(
                        Convert.ToSingle(txtZoomZoomFactor.Text),
                        (txtZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomCenterPoint.Text),
                        options).ZoomFactorChangeId;
                    AppendAsyncEventMessage("Invoked ZoomTo Id=" + lastZoomFactorChangeId);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelZoom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeId != -1)
                {
                    AppendAsyncEventMessage("Canceling ZoomTo/By Id=" + lastZoomFactorChangeId);
                    scroller.ZoomBy(
                        0,
                        Vector2.Zero, 
                        new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_ZoomAnimationStarting(Scroller sender, ZoomAnimationStartingEventArgs args)
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
                            targetZoomFactor += scroller.ZoomFactor;
                        }

                        customKeyFrameAnimation = stockKeyFrameAnimation.Compositor.CreateScalarKeyFrameAnimation();
                        if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex == 1)
                        {
                            // Accordion case
                            float deltaZoomFactor = 0.1f * (float)(targetZoomFactor - scroller.ZoomFactor);

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
                            float deltaZoomFactor = (float)(targetZoomFactor - scroller.ZoomFactor);

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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnZoomFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastZoomFactorChangeWithAdditionalVelocityId = scroller.ZoomFrom(
                    Convert.ToSingle(txtZoomFromVelocity.Text),
                    (txtZoomFromCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtZoomFromCenterPoint.Text),
                    (txtZoomFromInertiaDecayRate.Text == "null") ? (float?)null : (float?)Convert.ToSingle(txtZoomFromInertiaDecayRate.Text)).ZoomFactorChangeId;
                AppendAsyncEventMessage("Invoked ZoomFrom Id=" + lastZoomFactorChangeWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelZoomFrom_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastZoomFactorChangeWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ZoomFrom Id=" + lastZoomFactorChangeWithAdditionalVelocityId);
                    scroller.ZoomBy(
                        0,
                        Vector2.Zero,
                        new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                    scroller.VerticalSnapPoints.Add(snapPoint);
                }
                else if(cmbSnapPointKind.SelectedIndex == 1)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text), ScrollSnapPointsAlignment.Near);
                    scroller.HorizontalSnapPoints.Add(snapPoint); 
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text));
                    scroller.ZoomSnapPoints.Add(snapPoint);
                }
#else
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    ScrollSnapPoint snapPoint = new ScrollSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text),
                        (ScrollSnapPointsAlignment)cmbSnapPointAlignment.SelectedIndex);
                    if (cmbSnapPointKind.SelectedIndex == 0)
                    {
                        scroller.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scroller.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    ZoomSnapPoint snapPoint = new ZoomSnapPoint(
                        Convert.ToSingle(txtIrregularSnapPointValue.Text));
                    scroller.ZoomSnapPoints.Add(snapPoint);
                }
#endif
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                        scroller.VerticalSnapPoints.Add(snapPoint);
                    }
                    else
                    {
                        scroller.HorizontalSnapPoints.Add(snapPoint);
                    }
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    RepeatedZoomSnapPoint snapPoint = new RepeatedZoomSnapPoint(
                        Convert.ToDouble(txtRepeatedSnapPointOffset.Text),
                        Convert.ToDouble(txtRepeatedSnapPointInterval.Text),
                        Convert.ToDouble(txtRepeatedSnapPointStart.Text),
                        Convert.ToDouble(txtRepeatedSnapPointEnd.Text));
                    scroller.ZoomSnapPoints.Add(snapPoint);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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

                ComboBox cmbAnimationMode = (zoomFactor == null || zoomFactor == scroller.ZoomFactor) ? 
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

                ChangeView(scroller, horizontalOffset, verticalOffset, zoomFactor, disableAnimation);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private bool ChangeView(Scroller scroller, double? horizontalOffset, double? verticalOffset, float? zoomFactor, bool disableAnimation)
        {
            AppendAsyncEventMessage($"ChangeView(horizontalOffset:{horizontalOffset}, verticalOffset:{verticalOffset}, zoomFactor:{zoomFactor}, disableAnimation:{disableAnimation}) from horizontalOffset:{scroller.HorizontalOffset}, verticalOffset:{scroller.VerticalOffset}, zoomFactor:{scroller.ZoomFactor}");

            double targetHorizontalOffset = horizontalOffset == null ? scroller.HorizontalOffset : (double)horizontalOffset;
            double targetVerticalOffset = verticalOffset == null ? scroller.VerticalOffset : (double)verticalOffset;
            float targetZoomFactor = zoomFactor == null ? scroller.ZoomFactor : (float)Math.Max(Math.Min((double)zoomFactor, scroller.MaxZoomFactor), scroller.MinZoomFactor);
            float deltaZoomFactor = targetZoomFactor - scroller.ZoomFactor;

            if (disableAnimation)
            {
                targetHorizontalOffset = Math.Max(Math.Min(targetHorizontalOffset, scroller.ExtentWidth * targetZoomFactor - scroller.ViewportWidth), 0.0);
                targetVerticalOffset = Math.Max(Math.Min(targetVerticalOffset, scroller.ExtentHeight * targetZoomFactor - scroller.ViewportHeight), 0.0);
            }
            // During an animation, out-of-bounds offsets may become valid as the extents are dynamically updated, so no clamping is performed for animated view changes.

            if (deltaZoomFactor == 0.0f)
            {
                if (targetHorizontalOffset == scroller.HorizontalOffset && targetVerticalOffset == scroller.VerticalOffset)
                {
                    AppendAsyncEventMessage("ChangeView no-op");
                    return false;
                }

                lastOffsetsChangeId = scroller.ScrollTo(
                    targetHorizontalOffset,
                    targetVerticalOffset,
                    new ScrollOptions(
                        disableAnimation ? AnimationMode.Disabled : AnimationMode.Enabled,
                        disableAnimation ? SnapPointsMode.Ignore : SnapPointsMode.Default)).OffsetsChangeId;
                AppendAsyncEventMessage($"ChangeView invoked ScrollTo(horizontalOffset:{targetHorizontalOffset}, verticalOffset:{targetVerticalOffset}) Id={lastOffsetsChangeId}");
            }
            else
            {
                FrameworkElement contentAsFE = scroller.Content as FrameworkElement;
                HorizontalAlignment contentHorizontalAlignment = contentAsFE == null ? HorizontalAlignment.Stretch : contentAsFE.HorizontalAlignment;
                VerticalAlignment contentVerticalAlignment = contentAsFE == null ? VerticalAlignment.Stretch : contentAsFE.VerticalAlignment;
                double currentPositionX = scroller.HorizontalOffset;
                double currentPositionY = scroller.VerticalOffset;
                double currentViewportExcessX = scroller.ViewportWidth - scroller.ExtentWidth * scroller.ZoomFactor;
                double targetViewportExcessX = scroller.ViewportWidth - scroller.ExtentWidth * targetZoomFactor;
                double currentViewportExcessY = scroller.ViewportHeight - scroller.ExtentHeight * scroller.ZoomFactor;
                double targetViewportExcessY = scroller.ViewportHeight - scroller.ExtentHeight * targetZoomFactor;

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
                    (float)(targetHorizontalOffset * scroller.ZoomFactor - currentPositionX * targetZoomFactor) / deltaZoomFactor,
                    (float)(targetVerticalOffset * scroller.ZoomFactor - currentPositionY * targetZoomFactor) / deltaZoomFactor);


                lastZoomFactorChangeId = scroller.ZoomTo(
                    targetZoomFactor,
                    centerPoint,
                    new ZoomOptions(
                        disableAnimation ? AnimationMode.Disabled : AnimationMode.Enabled,
                        disableAnimation ? SnapPointsMode.Ignore : SnapPointsMode.Default)).ZoomFactorChangeId;
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
                    scroller.VerticalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 1)
                {
                    scroller.HorizontalSnapPoints.Clear();
                }
                else if (cmbSnapPointKind.SelectedIndex == 2)
                {
                    scroller.ZoomSnapPoints.Clear();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                    lstScrollerEvents.Items.Add(asyncEventMessage);
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

        private void BtnParentMarkupScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(markupScroller);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentMarkupScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScroller);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentReparentMarkupScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(markupScroller);
                root.Children.Add(markupScroller);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCreateDynamicScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                dynamicScroller = new Scroller();
                dynamicScroller.Name = "dynamicScroller";
                dynamicScroller.Width = 300.0;
                dynamicScroller.Height = 400.0;
                dynamicScroller.Margin = new Thickness(1);
                dynamicScroller.Background = new SolidColorBrush(Colors.HotPink);
                dynamicScroller.VerticalAlignment = VerticalAlignment.Top;
                Grid.SetRow(dynamicScroller, 1);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnParentDynamicScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Add(dynamicScroller);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUnparentDynamicScroller_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                root.Children.Remove(dynamicScroller);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnUseMarkupScroller_Click(object sender, RoutedEventArgs e)
        {
            UseScroller(markupScroller);
        }

        private void BtnUseDynamicScroller_Click(object sender, RoutedEventArgs e)
        {
            UseScroller(dynamicScroller);
        }

        private void BtnReleaseDynamicScroller_Click(object sender, RoutedEventArgs e)
        {
            dynamicScroller = null;

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

                ScrollerTestHooks.GetContentLayoutOffsetX(scroller, out contentLayoutOffsetX);

                txtContentLayoutOffsetX.Text = contentLayoutOffsetX.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerTestHooks.SetContentLayoutOffsetX(scroller, Convert.ToSingle(txtContentLayoutOffsetX.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float contentLayoutOffsetY = 0.0f;

                ScrollerTestHooks.GetContentLayoutOffsetY(scroller, out contentLayoutOffsetY);

                txtContentLayoutOffsetY.Text = contentLayoutOffsetY.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerTestHooks.SetContentLayoutOffsetY(scroller, Convert.ToSingle(txtContentLayoutOffsetY.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetArrangeRenderSizesDelta_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtArrangeRenderSizesDelta.Text = ScrollerTestHooks.GetArrangeRenderSizesDelta(scroller).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMinPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMinPosition.Text = ScrollerTestHooks.GetMinPosition(scroller).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxPosition_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtMaxPosition.Text = ScrollerTestHooks.GetMaxPosition(scroller).ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void UseScroller(Scroller s)
        {
            if (scroller == s || s == null)
                return;

            try
            {
                if (scroller != null)
                {
                    if (chkLogScrollerMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
                    }

                    if (chkLogScrollerEvents.IsChecked == true)
                    {
                        scroller.ExtentChanged -= Scroller_ExtentChanged;
                        scroller.StateChanged -= Scroller_StateChanged;
                        scroller.ViewChanged -= Scroller_ViewChanged;
                        scroller.ScrollCompleted -= Scroller_ScrollCompleted;
                        scroller.ZoomCompleted -= Scroller_ZoomCompleted;
                        scroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                        scroller.ZoomAnimationStarting -= Scroller_ZoomAnimationStarting;
                    }

                    if (chkLogScrollerEvents.IsChecked == true)
                    {
                        UnhookContentEffectiveViewportChanged();
                    }
                }

                scroller = s;

                if (chkLogScrollerMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                UpdateCmbContentOrientation();
                UpdateCmbHorizontalScrollMode();
                UpdateCmbHorizontalScrollChainingMode();
                UpdateCmbHorizontalScrollRailingMode();
                UpdateCmbVerticalScrollMode();
                UpdateCmbVerticalScrollChainingMode();
                UpdateCmbVerticalScrollRailingMode();
                UpdateCmbZoomMode();
                UpdateCmbZoomChainingMode();
                UpdateCmbIgnoredInputKind();
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbScrollerManipulationMode();
                UpdateCmbContentManipulationMode();
                UpdateCmbBackground();
                UpdateCmbContent();
                UpdateMinZoomFactor();
                UpdateMaxZoomFactor();

                txtWidth.Text = string.Empty;
                txtHeight.Text = string.Empty;

                if (scroller != null)
                {
                    if (chkLogScrollerEvents.IsChecked == true)
                    {
                        scroller.ExtentChanged += Scroller_ExtentChanged;
                        scroller.StateChanged += Scroller_StateChanged;
                        scroller.ViewChanged += Scroller_ViewChanged;
                        scroller.ScrollCompleted += Scroller_ScrollCompleted;
                        scroller.ZoomCompleted += Scroller_ZoomCompleted;
                        scroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                        scroller.ZoomAnimationStarting += Scroller_ZoomAnimationStarting;
                    }

                    if (chkLogScrollerEvents.IsChecked == true)
                    {
                        HookContentEffectiveViewportChanged();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
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
                            scroller.MinZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMinZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetMaxZoomFactor:
                            scroller.MaxZoomFactor = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetMaxZoomFactor " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetWidth:
                            scroller.Width = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetHeight:
                            scroller.Height = qo.DoubleValue;
                            AppendAsyncEventMessage("Unqueued SetHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentWidth:
                            if (scroller.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Content)).Width = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentHeight:
                            if (scroller.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Content)).Height = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetContentHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetContentMargin:
                            if (scroller.Content is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Content)).Margin = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetContentMargin " + qo.StringValue);
                            break;
                        case QueuedOperationType.SetContentPadding:
                            if (scroller.Content is Control)
                            {
                                ((Control)(scroller.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scroller.Content is Border)
                            {
                                ((Border)(scroller.Content)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scroller.Content is StackPanel)
                            {
                                ((StackPanel)(scroller.Content)).Padding = GetThicknessFromString(qo.StringValue);
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
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }
    }
}
