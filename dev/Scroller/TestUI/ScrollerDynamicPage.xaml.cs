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
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollerChainingMode = Microsoft.UI.Xaml.Controls.ScrollerChainingMode;
using ScrollerRailingMode = Microsoft.UI.Xaml.Controls.ScrollerRailingMode;
using ScrollerScrollMode = Microsoft.UI.Xaml.Controls.ScrollerScrollMode;
using ScrollerZoomMode = Microsoft.UI.Xaml.Controls.ScrollerZoomMode;
using ScrollerInputKind = Microsoft.UI.Xaml.Controls.ScrollerInputKind;
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
            SetChildMargin,
            SetChildPadding,
            SetChildMinWidth,
            SetChildMinHeight,
            SetChildWidth,
            SetChildHeight,
            SetChildMaxWidth,
            SetChildMaxHeight,
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

        private void ChkLogChildEffectiveViewportChangedEvent_Checked(object sender, RoutedEventArgs e)
        {
            HookChildEffectiveViewportChanged();
        }

        private void ChkLogChildEffectiveViewportChangedEvent_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookChildEffectiveViewportChanged();
        }

        private void HookChildEffectiveViewportChanged()
        {
            if (scroller != null)
            {
                FrameworkElement childAsFE = scroller.Child as FrameworkElement;
                if (childAsFE != null)
                {
                    childAsFE.EffectiveViewportChanged += Child_EffectiveViewportChanged;
                }
            }
        }

        private void UnhookChildEffectiveViewportChanged()
        {
            if (scroller != null)
            {
                FrameworkElement childAsFE = scroller.Child as FrameworkElement;
                if (childAsFE != null)
                {
                    childAsFE.EffectiveViewportChanged -= Child_EffectiveViewportChanged;
                }
            }
        }

        private void Child_EffectiveViewportChanged(FrameworkElement sender, EffectiveViewportChangedEventArgs args)
        {
            AppendAsyncEventMessage("Child_EffectiveViewportChanged: BringIntoViewDistance=" + 
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

        private void ChkChildProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdChildProperties != null)
                grdChildProperties.Visibility = Visibility.Visible;
        }

        private void ChkChildProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdChildProperties != null)
                grdChildProperties.Visibility = Visibility.Collapsed;
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
                scroller.HorizontalScrollMode = (ScrollerScrollMode)cmbHorizontalScrollMode.SelectedIndex;
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
                scroller.HorizontalScrollChainingMode = (ScrollerChainingMode)cmbHorizontalScrollChainingMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollChainingMode()
        {
            cmbHorizontalScrollChainingMode.SelectedIndex = (int)scroller.HorizontalScrollChainingMode;
        }

        private void CmbHorizontalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.HorizontalScrollRailingMode = (ScrollerRailingMode)cmbHorizontalScrollRailingMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollRailingMode()
        {
            cmbHorizontalScrollRailingMode.SelectedIndex = (int)scroller.HorizontalScrollRailingMode;
        }

        private void CmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollMode = (ScrollerScrollMode)cmbVerticalScrollMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollMode()
        {
            cmbVerticalScrollMode.SelectedIndex = (int)scroller.VerticalScrollMode;
        }

        private void CmbVerticalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollChainingMode = (ScrollerChainingMode)cmbVerticalScrollChainingMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollChainingMode()
        {
            cmbVerticalScrollChainingMode.SelectedIndex = (int)scroller.VerticalScrollChainingMode;
        }

        private void CmbVerticalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.VerticalScrollRailingMode = (ScrollerRailingMode)cmbVerticalScrollRailingMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollRailingMode()
        {
            cmbVerticalScrollRailingMode.SelectedIndex = (int)scroller.VerticalScrollRailingMode;
        }

        private void CmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.ZoomMode = (ScrollerZoomMode)cmbZoomMode.SelectedIndex;
        }

        private void UpdateCmbZoomMode()
        {
            cmbZoomMode.SelectedIndex = (int)scroller.ZoomMode;
        }

        private void CmbZoomChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller != null)
                scroller.ZoomChainingMode = (ScrollerChainingMode)cmbZoomChainingMode.SelectedIndex;
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
                        scroller.InputKind = ScrollerInputKind.All;
                        break;
                    case 1:
                        scroller.InputKind = ScrollerInputKind.Touch;
                        break;
                    case 2:
                        scroller.InputKind = ScrollerInputKind.Pen;
                        break;
                    case 3:
                        scroller.InputKind = ScrollerInputKind.MouseWheel;
                        break;
                    case 4:
                        scroller.InputKind = ScrollerInputKind.Touch | ScrollerInputKind.MouseWheel;
                        break;
                    case 5:
                        scroller.InputKind = ScrollerInputKind.Touch | ScrollerInputKind.Pen;
                        break;
                    case 6:
                        scroller.InputKind = ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel;
                        break;
                    case 7:
                        scroller.InputKind = ScrollerInputKind.Touch | ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel;
                        break;
                }
            }
        }

        private void UpdateCmbInputKind()
        {
            switch (scroller.InputKind)
            {
                case ScrollerInputKind.All:
                    cmbInputKind.SelectedIndex = 0;
                    break;
                case ScrollerInputKind.Touch:
                    cmbInputKind.SelectedIndex = 1;
                    break;
                case ScrollerInputKind.Pen:
                    cmbInputKind.SelectedIndex = 2;
                    break;
                case ScrollerInputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 3;
                    break;
                case ScrollerInputKind.Touch | ScrollerInputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 4;
                    break;
                case ScrollerInputKind.Touch | ScrollerInputKind.Pen:
                    cmbInputKind.SelectedIndex = 5;
                    break;
                case ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 6;
                    break;
                case ScrollerInputKind.Touch | ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel:
                    cmbInputKind.SelectedIndex = 7;
                    break;
            }
        }

        private void CmbChildHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Child is FrameworkElement)
            {
                ((FrameworkElement)scroller.Child).HorizontalAlignment = (HorizontalAlignment)cmbChildHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbChildHorizontalAlignment()
        {
            if (scroller.Child is FrameworkElement)
            {
                cmbChildHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scroller.Child).HorizontalAlignment;
            }
        }

        private void CmbChildVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Child is FrameworkElement)
            {
                ((FrameworkElement)scroller.Child).VerticalAlignment = (VerticalAlignment)cmbChildVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbChildVerticalAlignment()
        {
            if (scroller.Child is FrameworkElement)
            {
                cmbChildVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scroller.Child).VerticalAlignment;
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

        private void CmbChildManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scroller.Child is FrameworkElement)
            {
                switch (cmbChildManipulationMode.SelectedIndex)
                {
                    case 0:
                        scroller.Child.ManipulationMode = ManipulationModes.System;
                        break;
                    case 1:
                        scroller.Child.ManipulationMode = ManipulationModes.None;
                        break;
                }
            }
        }

        private void UpdateCmbChildManipulationMode()
        {
            if (scroller.Child != null)
            {
                switch (scroller.Child.ManipulationMode)
                {
                    case ManipulationModes.System:
                        cmbChildManipulationMode.SelectedIndex = 0;
                        break;
                    case ManipulationModes.None:
                        cmbChildManipulationMode.SelectedIndex = 1;
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

        private void CmbChild_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentChild = scroller.Child as FrameworkElement;
                FrameworkElement newChild = null;

                switch (cmbChild.SelectedIndex)
                {
                    case 0:
                        newChild = null;
                        break;
                    case 1:
                        newChild = smallImg;
                        break;
                    case 2:
                        newChild = largeImg;
                        break;
                    case 3:
                        newChild = rectangle;
                        break;
                    case 4:
                        newChild = button;
                        break;
                    case 5:
                        newChild = border;
                        break;
                    case 6:
                        newChild = populatedBorder;
                        break;
                    case 7:
                        newChild = verticalStackPanel;
                        break;
                    case 8:
                        newChild = tilePanel;
                        break;
                }

                if (chkPreserveProperties.IsChecked == true && currentChild != null && newChild != null)
                {
                    newChild.Width = currentChild.Width;
                    newChild.Height = currentChild.Height;
                    newChild.Margin = currentChild.Margin;
                    newChild.HorizontalAlignment = currentChild.HorizontalAlignment;
                    newChild.VerticalAlignment = currentChild.VerticalAlignment;

                    if (currentChild is Control && newChild is Control)
                    {
                        ((Control)newChild).Padding = ((Control)currentChild).Padding;
                    }
                }

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    UnhookChildEffectiveViewportChanged();
                }

                scroller.Child = newChild;

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    HookChildEffectiveViewportChanged();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());

                UpdateCmbChild();
            }
        }

        private void UpdateCmbChild()
        {
            if (scroller.Child == null)
            {
                cmbChild.SelectedIndex = 0;
            }
            else if (scroller.Child is Image)
            {
                if (((scroller.Child as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbChild.SelectedIndex = 2;
                }
                else
                {
                    cmbChild.SelectedIndex = 1;
                }
            }
            else if (scroller.Child is Rectangle)
            {
                cmbChild.SelectedIndex = 3;
            }
            else if (scroller.Child is Button)
            {
                cmbChild.SelectedIndex = 4;
            }
            else if (scroller.Child is Border)
            {
                if ((scroller.Child as Border).Child is Rectangle)
                {
                    cmbChild.SelectedIndex = 5;
                }
                else
                {
                    cmbChild.SelectedIndex = 6;
                }
            }
            else if (scroller.Child is StackPanel)
            {
                cmbChild.SelectedIndex = 7;
            }
            else if (scroller.Child is TilePanel)
            {
                cmbChild.SelectedIndex = 8;
            }
        }

        private void BtnGetChildMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildMinWidth.Text = string.Empty;
            else
                txtChildMinWidth.Text = ((FrameworkElement)(scroller.Child)).MinWidth.ToString();
        }

        private void BtnGetChildWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildWidth.Text = string.Empty;
            else
                txtChildWidth.Text = ((FrameworkElement)(scroller.Child)).Width.ToString();
        }

        private void BtnGetChildMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildMaxWidth.Text = string.Empty;
            else
                txtChildMaxWidth.Text = ((FrameworkElement)(scroller.Child)).MaxWidth.ToString();
        }

        private void BtnSetChildMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).MinWidth = Convert.ToDouble(txtChildMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).Width = Convert.ToDouble(txtChildWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).MaxWidth = Convert.ToDouble(txtChildMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildMinWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildMinWidth, value));
                AppendAsyncEventMessage("Queued SetChildMinWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildWidth, value));
                AppendAsyncEventMessage("Queued SetChildWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildMaxWidth.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildMaxWidth, value));
                AppendAsyncEventMessage("Queued SetChildMaxWidth " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetChildMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildMinHeight.Text = string.Empty;
            else
                txtChildMinHeight.Text = ((FrameworkElement)(scroller.Child)).MinHeight.ToString();
        }

        private void BtnGetChildHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildHeight.Text = string.Empty;
            else
                txtChildHeight.Text = ((FrameworkElement)(scroller.Child)).Height.ToString();
        }

        private void BtnGetChildMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildMaxHeight.Text = string.Empty;
            else
                txtChildMaxHeight.Text = ((FrameworkElement)(scroller.Child)).MaxHeight.ToString();
        }

        private void BtnSetChildMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).MinHeight = Convert.ToDouble(txtChildMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).Height = Convert.ToDouble(txtChildHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).MaxHeight = Convert.ToDouble(txtChildMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildMinHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildMinHeight, value));
                AppendAsyncEventMessage("Queued SetChildMinHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildHeight, value));
                AppendAsyncEventMessage("Queued SetChildHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtChildMaxHeight.Text);
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildMaxHeight, value));
                AppendAsyncEventMessage("Queued SetChildMaxHeight " + value);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetChildMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildMargin.Text = string.Empty;
            else
                txtChildMargin.Text = ((FrameworkElement)(scroller.Child)).Margin.ToString();
        }

        private void BtnSetChildMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is FrameworkElement)
                {
                    ((FrameworkElement)(scroller.Child)).Margin = GetThicknessFromString(txtChildMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildMargin, txtChildMargin.Text));
                AppendAsyncEventMessage("Queued SetChildMargin " + txtChildMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetChildPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is Control || scroller.Child is Border || scroller.Child is StackPanel))
                txtChildPadding.Text = string.Empty;
            else if (scroller.Child is Control)
                txtChildPadding.Text = ((Control)(scroller.Child)).Padding.ToString();
            else if (scroller.Child is Border)
                txtChildPadding.Text = ((Border)(scroller.Child)).Padding.ToString();
            else
                txtChildPadding.Text = ((StackPanel)(scroller.Child)).Padding.ToString();
        }

        private void BtnSetChildPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scroller.Child is Control)
                {
                    ((Control)(scroller.Child)).Padding = GetThicknessFromString(txtChildPadding.Text);
                }
                else if (scroller.Child is Border)
                {
                    ((Border)(scroller.Child)).Padding = GetThicknessFromString(txtChildPadding.Text);
                }
                else if (scroller.Child is StackPanel)
                {
                    ((StackPanel)(scroller.Child)).Padding = GetThicknessFromString(txtChildPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnQueueChildPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.SetChildPadding, txtChildPadding.Text));
                AppendAsyncEventMessage("Queued SetChildPadding " + txtChildPadding.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetChildActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildActualWidth.Text = string.Empty;
            else
                txtChildActualWidth.Text = (scroller.Child as FrameworkElement).ActualWidth.ToString();
        }

        private void BtnGetChildActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null || !(scroller.Child is FrameworkElement))
                txtChildActualHeight.Text = string.Empty;
            else
                txtChildActualHeight.Text = (scroller.Child as FrameworkElement).ActualHeight.ToString();
        }

        private void BtnGetChildDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null)
                txtChildDesiredSize.Text = string.Empty;
            else
                txtChildDesiredSize.Text = scroller.Child.DesiredSize.ToString();
        }

        private void BtnGetChildRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scroller.Child == null)
                txtChildRenderSize.Text = string.Empty;
            else
                txtChildRenderSize.Text = scroller.Child.RenderSize.ToString();
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
            UIElement child = scroller.Child;

            if (child == null)
            {
                return 0;
            }

            FrameworkElement childAsFE = child as FrameworkElement;

            if (childAsFE == null)
            {
                return 0;
            }

            Thickness childMargin = childAsFE.Margin;
            Visual scrollerVisual = ElementCompositionPreview.GetElementVisual(scroller);
            double childWidth = double.IsNaN(childAsFE.Width) ? childAsFE.ActualWidth : childAsFE.Width;
            float minPosX = 0.0f;
            float extentWidth = Math.Max(0.0f, (float)(childWidth + childMargin.Left + childMargin.Right));

            if (childAsFE.HorizontalAlignment == HorizontalAlignment.Center ||
                childAsFE.HorizontalAlignment == HorizontalAlignment.Stretch)
            {
                minPosX = Math.Min(0.0f, (extentWidth * zoomFactor - scrollerVisual.Size.X) / 2.0f);
            }
            else if (childAsFE.HorizontalAlignment == HorizontalAlignment.Right)
            {
                minPosX = Math.Min(0.0f, extentWidth * zoomFactor - scrollerVisual.Size.X);
            }

            return minPosX;
        }

        private float ComputeMinVerticalPosition(float zoomFactor)
        {
            UIElement child = scroller.Child;

            if (child == null)
            {
                return 0;
            }

            FrameworkElement childAsFE = child as FrameworkElement;

            if (childAsFE == null)
            {
                return 0;
            }

            Thickness childMargin = childAsFE.Margin;
            Visual scrollerVisual = ElementCompositionPreview.GetElementVisual(scroller);
            double childHeight = double.IsNaN(childAsFE.Height) ? childAsFE.ActualHeight : childAsFE.Height;
            float minPosY = 0.0f;
            float extentHeight = Math.Max(0.0f, (float)(childHeight + childMargin.Top + childMargin.Bottom));

            if (childAsFE.VerticalAlignment == VerticalAlignment.Center ||
                childAsFE.VerticalAlignment == VerticalAlignment.Stretch)
            {
                minPosY = Math.Min(0.0f, (extentHeight * zoomFactor - scrollerVisual.Size.Y) / 2.0f);
            }
            else if (childAsFE.VerticalAlignment == VerticalAlignment.Bottom)
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

        private void BtnGetChildLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float childLayoutOffsetX = 0.0f;

                ScrollerTestHooks.GetChildLayoutOffsetX(scroller, out childLayoutOffsetX);

                txtChildLayoutOffsetX.Text = childLayoutOffsetX.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildLayoutOffsetX_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerTestHooks.SetChildLayoutOffsetX(scroller, Convert.ToSingle(txtChildLayoutOffsetX.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetChildLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float childLayoutOffsetY = 0.0f;

                ScrollerTestHooks.GetChildLayoutOffsetY(scroller, out childLayoutOffsetY);

                txtChildLayoutOffsetY.Text = childLayoutOffsetY.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetChildLayoutOffsetY_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerTestHooks.SetChildLayoutOffsetY(scroller, Convert.ToSingle(txtChildLayoutOffsetY.Text));
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
                        UnhookChildEffectiveViewportChanged();
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
                UpdateCmbChildHorizontalAlignment();
                UpdateCmbChildVerticalAlignment();
                UpdateCmbScrollerManipulationMode();
                UpdateCmbChildManipulationMode();
                UpdateCmbBackground();
                UpdateCmbChild();
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
                        HookChildEffectiveViewportChanged();
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
                        case QueuedOperationType.SetChildWidth:
                            if (scroller.Child is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Child)).Width = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetChildWidth " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetChildHeight:
                            if (scroller.Child is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Child)).Height = qo.DoubleValue;
                            }
                            AppendAsyncEventMessage("Unqueued SetChildHeight " + qo.DoubleValue);
                            break;
                        case QueuedOperationType.SetChildMargin:
                            if (scroller.Child is FrameworkElement)
                            {
                                ((FrameworkElement)(scroller.Child)).Margin = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetChildMargin " + qo.StringValue);
                            break;
                        case QueuedOperationType.SetChildPadding:
                            if (scroller.Child is Control)
                            {
                                ((Control)(scroller.Child)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scroller.Child is Border)
                            {
                                ((Border)(scroller.Child)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            else if (scroller.Child is StackPanel)
                            {
                                ((StackPanel)(scroller.Child)).Padding = GetThicknessFromString(qo.StringValue);
                            }
                            AppendAsyncEventMessage("Unqueued SetChildPadding " + qo.StringValue);
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
