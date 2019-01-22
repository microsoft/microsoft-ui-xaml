// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerChangeOffsetsWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsWithAdditionalVelocityOptions;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using ScrollerChangeZoomFactorWithAdditionalVelocityOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorWithAdditionalVelocityOptions;
using ScrollerSnapPointIrregular = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointIrregular;
using ScrollerSnapPointAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollerSnapPointAlignment;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerChangingOffsetsEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingOffsetsEventArgs;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerChangingZoomFactorEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingZoomFactorEventArgs;
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
#endif

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

        private int lastChangeOffsetsId = -1;
        private int lastChangeOffsetsWithAdditionalVelocityId = -1;
        private int lastChangeZoomFactorId = -1;
        private int lastChangeZoomFactorWithAdditionalVelocityId = -1;
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<QueuedOperation> lstQueuedOperations = new List<QueuedOperation>();
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

        private void Scroller_ViewChangeCompleted(Scroller sender, ScrollerViewChangeCompletedEventArgs args)
        {
            AppendAsyncEventMessage("ViewChangeCompleted ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);
        }

        private void CreateChildren()
        {
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
            Rectangle borderChild = new Rectangle() { Fill = lgb };
            border = new Border() {
                BorderBrush = chartreuseBrush, BorderThickness = new Thickness(5), Child = borderChild };
            StackPanel populatedBorderChild = new StackPanel();
            populatedBorderChild.Margin = new Thickness(30);
            for (int i = 0; i < 16; i++)
            {
                TextBlock textBlock = new TextBlock() {
                    Text = "TB#" + populatedBorderChild.Children.Count,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center };

                Border borderInSP = new Border() {
                    BorderThickness = border.Margin = new Thickness(3),
                    BorderBrush = chartreuseBrush,
                    Background = blanchedAlmondBrush,
                    Width = 170,
                    Height = 120,
                    Child = textBlock };

                populatedBorderChild.Children.Add(borderInSP);
            }
            populatedBorder = new Border() {
                BorderBrush = chartreuseBrush, BorderThickness = new Thickness(3), Margin = new Thickness(15),
                Background = new SolidColorBrush(Colors.Beige), Child = populatedBorderChild };
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
            scroller.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            scroller.ChangingOffsets += Scroller_ChangingOffsets;
            scroller.ChangingZoomFactor += Scroller_ChangingZoomFactor;
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scroller.ExtentChanged -= Scroller_ExtentChanged;
            scroller.StateChanged -= Scroller_StateChanged;
            scroller.ViewChanged -= Scroller_ViewChanged;
            scroller.ViewChangeCompleted -= Scroller_ViewChangeCompleted;
            scroller.ChangingOffsets -= Scroller_ChangingOffsets;
            scroller.ChangingZoomFactor -= Scroller_ChangingZoomFactor;
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

        private void CmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.HorizontalScrollMode = (ScrollMode)cmbHorizontalScrollMode.SelectedIndex;
        }

        private void BtnGetIsChildAvailableWidthConstrained_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIsChildAvailableWidthConstrained();
        }

        private void BtnSetIsChildAvailableWidthConstrained_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller != null)
                    scroller.IsChildAvailableWidthConstrained = cmbIsChildAvailableWidthConstrained.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIsChildAvailableWidthConstrained()
        {
            try
            {
                cmbIsChildAvailableWidthConstrained.SelectedIndex = scroller.IsChildAvailableWidthConstrained ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetIsChildAvailableHeightConstrained_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIsChildAvailableHeightConstrained();
        }

        private void BtnSetIsChildAvailableHeightConstrained_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller != null)
                    scroller.IsChildAvailableHeightConstrained = cmbIsChildAvailableHeightConstrained.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIsChildAvailableHeightConstrained()
        {
            try
            {
                cmbIsChildAvailableHeightConstrained.SelectedIndex = scroller.IsChildAvailableHeightConstrained ? 0 : 1;
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

        private void CmbInputKind_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
            {
                switch (cmbInputKind.SelectedIndex)
                {
                    case 0:
                        scroller.InputKind = InputKind.All;
                        break;
                    case 1:
                        scroller.InputKind = InputKind.Touch;
                        break;
                    case 2:
                        scroller.InputKind = InputKind.Pen;
                        break;
                    case 3:
                        scroller.InputKind = InputKind.MouseWheel;
                        break;
                    case 4:
                        scroller.InputKind = InputKind.Touch | InputKind.MouseWheel;
                        break;
                    case 5:
                        scroller.InputKind = InputKind.Touch | InputKind.Pen;
                        break;
                    case 6:
                        scroller.InputKind = InputKind.Pen | InputKind.MouseWheel;
                        break;
                    case 7:
                        scroller.InputKind = InputKind.Touch | InputKind.Pen | InputKind.MouseWheel;
                        break;
                }
            }
        }

        private void UpdateCmbInputKind()
        {
            switch (scroller.InputKind)
            {
                case InputKind.All:
                    cmbInputKind.SelectedIndex = 0;
                    break;
                case InputKind.Touch:
                    cmbInputKind.SelectedIndex = 1;
                    break;
                case InputKind.Pen:
                    cmbInputKind.SelectedIndex = 2;
                    break;
                case InputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 3;
                    break;
                case InputKind.Touch | InputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 4;
                    break;
                case InputKind.Touch | InputKind.Pen:
                    cmbInputKind.SelectedIndex = 5;
                    break;
                case InputKind.Pen | InputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 6;
                    break;
                case InputKind.Touch | InputKind.Pen | InputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 7;
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

        private void BtnClearScrollerEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollerEvents.Items.Clear();
        }

        private void BtnChangeOffsets_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerViewKind offsetsKind = (ScrollerViewKind)cmbOffsetsKind.SelectedIndex;
                ScrollerViewChangeKind viewChangeKind = (ScrollerViewChangeKind)cmbOffsetsViewChangeKind.SelectedIndex;
                ScrollerViewChangeSnapPointRespect snapPointRespect = (ScrollerViewChangeSnapPointRespect)cmbOffsetSnapPointRespect.SelectedIndex;
                ScrollerChangeOffsetsOptions options = new ScrollerChangeOffsetsOptions(
                    Convert.ToDouble(txtCOAHO.Text),
                    Convert.ToDouble(txtCOAVO.Text),
                    offsetsKind,
                    viewChangeKind,
                    snapPointRespect);

                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastChangeOffsetsId = scroller.ChangeOffsets(options);
                AppendAsyncEventMessage("Invoked ChangeOffsets Id=" + lastChangeOffsetsId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelChangeOffsets_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastChangeOffsetsId != -1)
                {
                    AppendAsyncEventMessage("Canceling ChangeOffsets Id=" + lastChangeOffsetsId);
                    scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0, 0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_ChangingOffsets(Scroller sender, ScrollerChangingOffsetsEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ChangingOffsets ViewChangeId=" + args.ViewChangeId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y + ") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y + ")");

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    Vector3KeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenOffsetsChangeAnimation.SelectedIndex != 0)
                    {
                        double targetHorizontalOffset = Convert.ToDouble(txtCOAHO.Text);
                        if (cmbOffsetsKind.SelectedIndex == 1)
                        {
                            targetHorizontalOffset += scroller.HorizontalOffset;
                        }
                        float targetHorizontalPosition = ComputeHorizontalPositionFromOffset(targetHorizontalOffset);

                        double targetVerticalOffset = Convert.ToDouble(txtCOAVO.Text);
                        if (cmbOffsetsKind.SelectedIndex == 1)
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

        private void BtnChangeOffsetsWithAdditionalVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Vector2? inertiaDecayRate = null;

                if (txtCOWAVAHIDR.Text != "null" && txtCOWAVAVIDR.Text != "null")
                {
                    inertiaDecayRate = new Vector2(Convert.ToSingle(txtCOWAVAHIDR.Text), Convert.ToSingle(txtCOWAVAVIDR.Text));
                }

                ScrollerChangeOffsetsWithAdditionalVelocityOptions options = new ScrollerChangeOffsetsWithAdditionalVelocityOptions(
                    new Vector2(Convert.ToSingle(txtCOWAVAHV.Text), Convert.ToSingle(txtCOWAVAVV.Text)),
                    inertiaDecayRate);

                txtStockOffsetsChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastChangeOffsetsWithAdditionalVelocityId = scroller.ChangeOffsetsWithAdditionalVelocity(options);
                AppendAsyncEventMessage("Invoked ChangeOffsetsWithAdditionalVelocity Id=" + lastChangeOffsetsWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelChangeOffsetsWithAdditionalVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastChangeOffsetsWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ChangeOffsetsWithAdditionalVelocity Id=" + lastChangeOffsetsWithAdditionalVelocityId);
                    scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0, 0, ScrollerViewKind.RelativeToCurrentView, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
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

        private void BtnChangeZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerViewKind zoomFactorKind = (ScrollerViewKind)cmbZoomFactorKind.SelectedIndex;
                ScrollerViewChangeKind viewChangeKind = (ScrollerViewChangeKind)cmbZoomFactorViewChangeKind.SelectedIndex;
                ScrollerViewChangeSnapPointRespect snapPointRespect = (ScrollerViewChangeSnapPointRespect)cmbZoomSnapPointRespect.SelectedIndex;
                ScrollerChangeZoomFactorOptions options = new ScrollerChangeZoomFactorOptions(
                    Convert.ToSingle(txtCZFAZF.Text),
                    zoomFactorKind,
                    ConvertFromStringToVector2(txtCZFACP.Text),
                    viewChangeKind, 
                    snapPointRespect);

                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastChangeZoomFactorId = scroller.ChangeZoomFactor(options);
                AppendAsyncEventMessage("Invoked ChangeZoomFactor Id=" + lastChangeZoomFactorId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelChangeZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastChangeZoomFactorId != -1)
                {
                    AppendAsyncEventMessage("Canceling ChangeZoomFactor Id=" + lastChangeZoomFactorId);
                    scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(0, ScrollerViewKind.RelativeToCurrentView, Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_ChangingZoomFactor(Scroller sender, ScrollerChangingZoomFactorEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("ChangingZoomFactor ViewChangeId=" + args.ViewChangeId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);
                
                ScalarKeyFrameAnimation stockKeyFrameAnimation = args.Animation as ScalarKeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    ScalarKeyFrameAnimation customKeyFrameAnimation = stockKeyFrameAnimation;

                    if (cmbOverriddenZoomFactorChangeAnimation.SelectedIndex != 0)
                    {
                        float targetZoomFactor = Convert.ToSingle(txtCZFAZF.Text);
                        if (cmbZoomFactorKind.SelectedIndex == 1)
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

        private void BtnChangeZoomFactorWithAdditionalVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerChangeZoomFactorWithAdditionalVelocityOptions options = new ScrollerChangeZoomFactorWithAdditionalVelocityOptions(
                    Convert.ToSingle(txtCZFWAVAAV.Text), 
                    (txtCZFWAVAIDR.Text == "null") ? (float?)null : (float?)Convert.ToSingle(txtCZFWAVAIDR.Text),
                    ConvertFromStringToVector2(txtCZFWAVACP.Text));

                txtStockZoomFactorChangeDuration.Text = string.Empty;

                ExecuteQueuedOperations();

                lastChangeZoomFactorWithAdditionalVelocityId = scroller.ChangeZoomFactorWithAdditionalVelocity(options);
                AppendAsyncEventMessage("Invoked ChangeZoomFactorWithAdditionalVelocity Id=" + lastChangeZoomFactorWithAdditionalVelocityId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnCancelChangeZoomFactorWithAdditionalVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (lastChangeZoomFactorWithAdditionalVelocityId != -1)
                {
                    AppendAsyncEventMessage("Canceling ChangeZoomFactorWithAdditionalVelocity Id=" + lastChangeZoomFactorWithAdditionalVelocityId);
                    scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(0, ScrollerViewKind.RelativeToCurrentView, Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerSnapPointIrregular snapPoint = new ScrollerSnapPointIrregular(Convert.ToSingle(txtSnapPointValue.Text), Convert.ToSingle(txtSnapPointValueRange.Text), ScrollerSnapPointAlignment.Near);
                if (cmbSnapPointKind.SelectedIndex == 0)
                {
                    scroller.VerticalSnapPoints.Add(snapPoint);
                }
                else if(cmbSnapPointKind.SelectedIndex == 1)
                {
                    scroller.HorizontalSnapPoints.Add(snapPoint); 
                }
                else if(cmbSnapPointKind.SelectedIndex == 2)
                {
                    scroller.ZoomSnapPoints.Add(snapPoint);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
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
                        scroller.ViewChangeCompleted -= Scroller_ViewChangeCompleted;
                        scroller.ChangingOffsets -= Scroller_ChangingOffsets;
                        scroller.ChangingZoomFactor -= Scroller_ChangingZoomFactor;
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

                UpdateCmbIsChildAvailableWidthConstrained();
                UpdateCmbIsChildAvailableHeightConstrained();
                UpdateCmbHorizontalScrollMode();
                UpdateCmbHorizontalScrollChainingMode();
                UpdateCmbHorizontalScrollRailingMode();
                UpdateCmbVerticalScrollMode();
                UpdateCmbVerticalScrollChainingMode();
                UpdateCmbVerticalScrollRailingMode();
                UpdateCmbZoomMode();
                UpdateCmbZoomChainingMode();
                UpdateCmbInputKind();
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
                        scroller.ViewChangeCompleted += Scroller_ViewChangeCompleted;
                        scroller.ChangingOffsets += Scroller_ChangingOffsets;
                        scroller.ChangingZoomFactor += Scroller_ChangingZoomFactor;
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
