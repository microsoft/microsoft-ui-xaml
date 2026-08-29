// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.Foundation;
using Windows.UI.ViewManagement;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="ScrollViewer", Icon="ScrollView.png")]
    public sealed partial class ScrollViewerPage : TestPage
    {
        private TextBlock textBlock;
        private Viewbox viewbox;
        private TilePanel tilePanel;
        private Image largeImg;
        private Rectangle rectangle;
        private Button button;
        private RepeatButton repeatButton;
        private Border border;
        private Border populatedBorder;
        private StackPanel horizontalStackPanel;
        private StackPanel verticalStackPanel;
        private Rectangle rectangleTopLeftHeader;
        private Rectangle rectangleLeftHeader;
        private Rectangle rectangleTopHeader;

        public ScrollViewerPage()
        {
            this.InitializeComponent();
            this.Loaded += ScrollViewerPage_Loaded;

            UseScrollViewer();
            CreateChildren();
        }

        ~ScrollViewerPage()
        {
        }

        private void UseScrollViewer()
        {
            try
            {
                UpdateCmbHorizontalScrollBarVisibility();
                UpdateCmbHorizontalScrollMode();
                UpdateChkIsHorizontalScrollChainingEnabled();
                UpdateChkIsHorizontalRailEnabled();
                UpdateCmbVerticalScrollBarVisibility();
                UpdateCmbVerticalScrollMode();
                UpdateChkIsVerticalScrollChainingEnabled();
                UpdateChkIsVerticalRailEnabled();
                UpdateCmbZoomMode();
                UpdateChkIsZoomChainingEnabled();
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbScrollViewerManipulationMode();
                UpdateCmbContentManipulationMode();
                UpdateCmbBackground();
                UpdateCmbContent();
                UpdateCmbXYFocusKeyboardNavigation();
                UpdateTxtMinZoomFactor();
                UpdateTxtMaxZoomFactor();
                UpdateTxtMargin();
                UpdateTxtPadding();
                UpdateChkIsEnabled();
                UpdateChkIsTabStop();

                scrollViewer.ViewChanging += ScrollViewer_ViewChanging;
                scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
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
            tilePanel.Background = new SolidColorBrush(Microsoft.UI.Colors.Orange);
            largeImg = new Image() { Source = new BitmapImage(new Uri("ms-appx:/Assets/LargeWisteria.jpg")) };
            SolidColorBrush chartreuseBrush = new SolidColorBrush(Microsoft.UI.Colors.Chartreuse);
            SolidColorBrush blanchedAlmondBrush = new SolidColorBrush(Microsoft.UI.Colors.BlanchedAlmond);
            SolidColorBrush interBrush = new SolidColorBrush(Microsoft.UI.Colors.Yellow);
            LinearGradientBrush lgb = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };
            GradientStop gs = new GradientStop() { Color = Microsoft.UI.Colors.Blue, Offset = 0.0 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Microsoft.UI.Colors.White, Offset = 0.5 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Microsoft.UI.Colors.Red, Offset = 1.0 };
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
                Background = new SolidColorBrush(Microsoft.UI.Colors.Beige),
                Child = populatedBorderChild
            };
            horizontalStackPanel = new StackPanel()
            {
                Orientation = Orientation.Horizontal
            };
            for (int index = 0; index < 10; index++)
            {
                horizontalStackPanel.Children.Add(new Rectangle() { Fill = lgb, Width = 95 });
                horizontalStackPanel.Children.Add(new Rectangle() { Fill = interBrush, Width = 5 });
            }
            verticalStackPanel = new StackPanel();
            for (int index = 0; index < 10; index++)
            {
                verticalStackPanel.Children.Add(new Rectangle() { Fill = lgb, Height = 95 });
                verticalStackPanel.Children.Add(new Rectangle() { Fill = interBrush, Height = 5 });
            }
            repeatButton = new RepeatButton() { Content = "RepeatButton" };
        }

        private void ScrollViewerPage_Loaded(object sender, RoutedEventArgs args)
        {
            rectangleTopLeftHeader = scrollViewer.TopLeftHeader as Rectangle;
            scrollViewer.TopLeftHeader = null;

            rectangleLeftHeader = scrollViewer.LeftHeader as Rectangle;
            scrollViewer.LeftHeader = null;

            rectangleTopHeader = scrollViewer.TopHeader as Rectangle;
            scrollViewer.TopHeader = null;
        }

        private string ScrollViewerViewToString(ScrollViewerView scrollViewerView)
        {
            return "H=" + scrollViewerView.HorizontalOffset.ToString() + ", V=" + scrollViewerView.VerticalOffset + ", ZF=" + scrollViewerView.ZoomFactor;
        }

        private void ScrollViewer_ViewChanging(object sender, ScrollViewerViewChangingEventArgs args)
        {
            AppendEventMessage("ViewChanging IsInertial=" + args.IsInertial + ", NextView=(" + ScrollViewerViewToString(args.NextView) + "), FinalView=(" + ScrollViewerViewToString(args.FinalView) + ")");
        }

        private void ScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            AppendEventMessage("ViewChanged IsIntermediate=" + args.IsIntermediate + ", H=" + scrollViewer.HorizontalOffset.ToString() + ", V=" + scrollViewer.VerticalOffset + ", ZF=" + scrollViewer.ZoomFactor);
        }

        private void ChkScrollViewerProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollViewerProperties != null)
                svScrollViewerProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollViewerProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollViewerProperties != null)
                svScrollViewerProperties.Visibility = Visibility.Collapsed;
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

        private void ChkScrollViewerMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollViewerMethods != null)
                svScrollViewerMethods.Visibility = Visibility.Visible;
        }

        private void ChkScrollViewerMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollViewerMethods != null)
                svScrollViewerMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollViewerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollViewerEvents != null)
                grdScrollViewerEvents.Visibility = Visibility.Visible;
        }

        private void ChkScrollViewerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollViewerEvents != null)
                grdScrollViewerEvents.Visibility = Visibility.Collapsed;
        }

        private void CmbHorizontalScrollBarVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.HorizontalScrollBarVisibility = (ScrollBarVisibility)cmbHorizontalScrollBarVisibility.SelectedIndex;
        }

        private void CmbHorizontalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.HorizontalScrollMode = (ScrollMode)cmbHorizontalScrollMode.SelectedIndex;
        }

        private void UpdateCmbHorizontalScrollBarVisibility()
        {
            cmbHorizontalScrollBarVisibility.SelectedIndex = (int)scrollViewer.HorizontalScrollBarVisibility;
        }

        private void UpdateCmbHorizontalScrollMode()
        {
            cmbHorizontalScrollMode.SelectedIndex = (int)scrollViewer.HorizontalScrollMode;
        }

        private void ChkIsHorizontalScrollChainingEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.IsHorizontalScrollChainingEnabled = (bool)chkIsHorizontalScrollChainingEnabled.IsChecked;
        }

        private void UpdateChkIsHorizontalScrollChainingEnabled()
        {
            chkIsHorizontalScrollChainingEnabled.IsChecked = scrollViewer.IsHorizontalScrollChainingEnabled;
        }

        private void ChkIsHorizontalRailEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.IsHorizontalRailEnabled = (bool)chkIsHorizontalRailEnabled.IsChecked;
        }

        private void UpdateChkIsHorizontalRailEnabled()
        {
            chkIsHorizontalRailEnabled.IsChecked = scrollViewer.IsHorizontalRailEnabled;
        }

        private void CmbVerticalScrollBarVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.VerticalScrollBarVisibility = (ScrollBarVisibility)cmbVerticalScrollBarVisibility.SelectedIndex;
        }

        private void CmbVerticalScrollMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.VerticalScrollMode = (ScrollMode)cmbVerticalScrollMode.SelectedIndex;
        }

        private void UpdateCmbVerticalScrollBarVisibility()
        {
            cmbVerticalScrollBarVisibility.SelectedIndex = (int)scrollViewer.VerticalScrollBarVisibility;
        }

        private void UpdateCmbVerticalScrollMode()
        {
            cmbVerticalScrollMode.SelectedIndex = (int)scrollViewer.VerticalScrollMode;
        }

        private void ChkIsVerticalScrollChainingEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.IsVerticalScrollChainingEnabled = (bool)chkIsVerticalScrollChainingEnabled.IsChecked;
        }

        private void UpdateChkIsVerticalScrollChainingEnabled()
        {
            chkIsVerticalScrollChainingEnabled.IsChecked = scrollViewer.IsVerticalScrollChainingEnabled;
        }

        private void ChkIsVerticalRailEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.IsVerticalRailEnabled = (bool)chkIsVerticalRailEnabled.IsChecked;
        }

        private void UpdateChkIsVerticalRailEnabled()
        {
            chkIsVerticalRailEnabled.IsChecked = scrollViewer.IsVerticalRailEnabled;
        }

        private void CmbZoomMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.ZoomMode = (ZoomMode)cmbZoomMode.SelectedIndex;
        }

        private void UpdateCmbZoomMode()
        {
            cmbZoomMode.SelectedIndex = (int)scrollViewer.ZoomMode;
        }

        private void ChkIsZoomChainingEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
                scrollViewer.IsZoomChainingEnabled = (bool)chkIsZoomChainingEnabled.IsChecked;
        }

        private void UpdateChkIsZoomChainingEnabled()
        {
            chkIsZoomChainingEnabled.IsChecked = scrollViewer.IsZoomChainingEnabled;
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollViewer.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentHorizontalAlignment()
        {
            if (scrollViewer.Content is FrameworkElement)
            {
                cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollViewer.Content).HorizontalAlignment;
            }
        }

        private void CmbContentVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer.Content is FrameworkElement)
            {
                ((FrameworkElement)scrollViewer.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbContentVerticalAlignment()
        {
            if (scrollViewer.Content is FrameworkElement)
            {
                cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollViewer.Content).VerticalAlignment;
            }
        }

        private void CmbScrollViewerManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbScrollViewerManipulationMode.SelectedIndex)
            {
                case 0:
                    scrollViewer.ManipulationMode = ManipulationModes.System;
                    break;
                case 1:
                    scrollViewer.ManipulationMode = ManipulationModes.None;
                    break;
            }
        }

        private void UpdateCmbScrollViewerManipulationMode()
        {
            switch (scrollViewer.ManipulationMode)
            {
                case ManipulationModes.System:
                    cmbScrollViewerManipulationMode.SelectedIndex = 0;
                    break;
                case ManipulationModes.None:
                    cmbScrollViewerManipulationMode.SelectedIndex = 1;
                    break;
            }
        }

        private void CmbContentManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollViewer.Content is UIElement scrollViewerContentAsUIE)
            {
                switch (cmbContentManipulationMode.SelectedIndex)
                {
                    case 0:
                        scrollViewerContentAsUIE.ManipulationMode = ManipulationModes.System;
                        break;
                    case 1:
                        scrollViewerContentAsUIE.ManipulationMode = ManipulationModes.None;
                        break;
                }
            }
        }

        private void UpdateCmbContentManipulationMode()
        {
            if (scrollViewer.Content is UIElement scrollViewerContentAsUIE)
            {
                switch (scrollViewerContentAsUIE.ManipulationMode)
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
                    scrollViewer.Background = null;
                    break;
                case 1:
                    scrollViewer.Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent);
                    break;
                case 2:
                    scrollViewer.Background = new SolidColorBrush(Microsoft.UI.Colors.AliceBlue);
                    break;
                case 3:
                    scrollViewer.Background = new SolidColorBrush(Microsoft.UI.Colors.Aqua);
                    break;
            }
        }

        private void UpdateCmbBackground()
        {
            SolidColorBrush bg = scrollViewer.Background as SolidColorBrush;

            if (bg == null)
            {
                cmbBackground.SelectedIndex = 0;
            }
            else if (bg.Color == Microsoft.UI.Colors.Transparent)
            {
                cmbBackground.SelectedIndex = 1;
            }
            else if (bg.Color == Microsoft.UI.Colors.AliceBlue)
            {
                cmbBackground.SelectedIndex = 2;
            }
            else if (bg.Color == Microsoft.UI.Colors.Aqua)
            {
                cmbBackground.SelectedIndex = 3;
            }
        }

        private void CmbContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentContent = scrollViewer.Content as FrameworkElement;
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
                        newContent = horizontalStackPanel;
                        break;
                    case 8:
                        newContent = verticalStackPanel;
                        break;
                    case 9:
                        newContent = tilePanel;
                        break;
                    case 10:
                        newContent = viewbox;
                        break;
                    case 11:
                        newContent = textBlock;
                        break;
                    case 12:
                        newContent = repeatButton;
                        break;
                }

                if (newContent != null)
                {
                    if (chkPreserveProperties.IsChecked == true && currentContent != null)
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

                    if ((bool)chkUseTopLeftHeader.IsChecked || (bool)chkUseLeftHeader.IsChecked || (bool)chkUseTopHeader.IsChecked)
                    {
                        newContent.HorizontalAlignment = HorizontalAlignment.Left;
                        newContent.VerticalAlignment = VerticalAlignment.Top;
                    }
                }

                scrollViewer.Content = newContent;

                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();

                _ = DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateHeaderSizes);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());

                UpdateCmbContent();
            }
        }

        private void UpdateCmbContent()
        {
            if (scrollViewer.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollViewer.Content is Image)
            {
                if (((scrollViewer.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollViewer.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollViewer.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollViewer.Content is Border)
            {
                if ((scrollViewer.Content as Border).Child is Rectangle)
                {
                    cmbContent.SelectedIndex = 5;
                }
                else
                {
                    cmbContent.SelectedIndex = 6;
                }
            }
            else if (scrollViewer.Content is StackPanel)
            {
                if ((scrollViewer.Content as StackPanel).Orientation == Orientation.Horizontal)
                {
                    cmbContent.SelectedIndex = 7;
                }
                else
                {
                    cmbContent.SelectedIndex = 8;
                }
            }
            else if (scrollViewer.Content is TilePanel)
            {
                cmbContent.SelectedIndex = 9;
            }
            else if (scrollViewer.Content is Viewbox)
            {
                cmbContent.SelectedIndex = 10;
            }
            else if (scrollViewer.Content is TextBlock)
            {
                cmbContent.SelectedIndex = 11;
            }
            else if (scrollViewer.Content is RepeatButton)
            {
                cmbContent.SelectedIndex = 12;
            }
        }

        private void UpdateTxtMargin()
        {
            txtMargin.Text = scrollViewer.Margin.ToString();
        }

        private void UpdateTxtPadding()
        {
            txtPadding.Text = scrollViewer.Padding.ToString();
        }

        private void BtnGetMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateTxtMargin();
        }

        private void BtnSetMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.Margin = GetThicknessFromString(txtMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdateTxtPadding();
        }

        private void BtnSetPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.Padding = GetThicknessFromString(txtPadding.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void UpdateChkIsEnabled()
        {
            chkIsEnabled.IsChecked = scrollViewer.IsEnabled;
        }

        private void ChkIsEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsEnabled = (bool)chkIsEnabled.IsChecked;
        }

        private void UpdateChkIsTabStop()
        {
            chkIsTabStop.IsChecked = scrollViewer.IsTabStop;
        }

        private void ChkIsTabStop_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsTabStop = (bool)chkIsTabStop.IsChecked;
            UpdateHeaderSizes();
        }

        private void ChkUseTopLeftHeader_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            scrollViewer.TopLeftHeader = (bool)chkUseTopLeftHeader.IsChecked ? rectangleTopLeftHeader : null;
            UpdateHeaderSizes();
        }

        private void ChkUseLeftHeader_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            scrollViewer.LeftHeader = (bool)chkUseLeftHeader.IsChecked ? rectangleLeftHeader : null;
            UpdateHeaderSizes();
        }

        private void ChkUseTopHeader_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            scrollViewer.TopHeader = (bool)chkUseTopHeader.IsChecked ? rectangleTopHeader : null;
        }

        private void UpdateHeaderSizes()
        {
            if (scrollViewer.Content == null)
            {
                if (rectangleLeftHeader != null)
                {
                    rectangleLeftHeader.Height = 0;
                }
                if (rectangleTopHeader != null)
                {
                    rectangleTopHeader.Width = 0;
                }
            }
            else
            {
                if (rectangleLeftHeader != null)
                {
                    rectangleLeftHeader.Height = (scrollViewer.Content as FrameworkElement).ActualHeight;
                }
                if (rectangleTopHeader != null)
                {
                    rectangleTopHeader.Width = (scrollViewer.Content as FrameworkElement).ActualWidth;
                }
            }
        }

        private void UpdateCmbXYFocusKeyboardNavigation()
        {
            cmbXYFocusKeyboardNavigation.SelectedIndex = (int)scrollViewer.XYFocusKeyboardNavigation;
        }

        private void CmbXYFocusKeyboardNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollViewer.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbXYFocusKeyboardNavigation.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMinWidth.Text = string.Empty;
            else
                txtContentMinWidth.Text = ((FrameworkElement)(scrollViewer.Content)).MinWidth.ToString();
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scrollViewer.Content)).Width.ToString();
        }

        private void BtnGetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMaxWidth.Text = string.Empty;
            else
                txtContentMaxWidth.Text = ((FrameworkElement)(scrollViewer.Content)).MaxWidth.ToString();
        }

        private void BtnSetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).MinWidth = Convert.ToDouble(txtContentMinWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).MaxWidth = Convert.ToDouble(txtContentMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMinHeight.Text = string.Empty;
            else
                txtContentMinHeight.Text = ((FrameworkElement)(scrollViewer.Content)).MinHeight.ToString();
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scrollViewer.Content)).Height.ToString();
        }

        private void BtnGetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMaxHeight.Text = string.Empty;
            else
                txtContentMaxHeight.Text = ((FrameworkElement)(scrollViewer.Content)).MaxHeight.ToString();
        }

        private void BtnSetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).MinHeight = Convert.ToDouble(txtContentMinHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).MaxHeight = Convert.ToDouble(txtContentMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scrollViewer.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is Control || scrollViewer.Content is Border || scrollViewer.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollViewer.Content is Control)
                txtContentPadding.Text = ((Control)(scrollViewer.Content)).Padding.ToString();
            else if (scrollViewer.Content is Border)
                txtContentPadding.Text = ((Border)(scrollViewer.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scrollViewer.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is Control)
                {
                    ((Control)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollViewer.Content is Border)
                {
                    ((Border)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollViewer.Content is StackPanel)
                {
                    ((StackPanel)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentActualWidth.Text = string.Empty;
            else
                txtContentActualWidth.Text = (scrollViewer.Content as FrameworkElement).ActualWidth.ToString();
        }

        private void BtnGetContentActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentActualHeight.Text = string.Empty;
            else
                txtContentActualHeight.Text = (scrollViewer.Content as FrameworkElement).ActualHeight.ToString();
        }

        private void BtnGetContentDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is UIElement))
                txtContentDesiredSize.Text = string.Empty;
            else
                txtContentDesiredSize.Text = (scrollViewer.Content as UIElement).DesiredSize.ToString();
        }

        private void BtnGetContentRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is UIElement))
                txtContentRenderSize.Text = string.Empty;
            else
                txtContentRenderSize.Text = (scrollViewer.Content as UIElement).RenderSize.ToString();
        }

        private void UpdateTxtMinZoomFactor()
        {
            txtMinZoomFactor.Text = scrollViewer.MinZoomFactor.ToString();
        }

        private void UpdateTxtMaxZoomFactor()
        {
            txtMaxZoomFactor.Text = scrollViewer.MaxZoomFactor.ToString();
        }

        private void BtnGetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateTxtMinZoomFactor();
        }

        private void BtnSetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.MinZoomFactor = Convert.ToSingle(txtMinZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateTxtMaxZoomFactor();
        }

        private void BtnSetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.MaxZoomFactor = Convert.ToSingle(txtMaxZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblHorizontalOffset.Text = scrollViewer.HorizontalOffset.ToString();
        }

        private void BtnGetVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            tblVerticalOffset.Text = scrollViewer.VerticalOffset.ToString();
        }

        private void BtnGetZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            tblZoomFactor.Text = scrollViewer.ZoomFactor.ToString();
        }

        private void BtnGetExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            tblExtentWidth.Text = scrollViewer.ExtentWidth.ToString();
        }

        private void BtnGetExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            tblExtentHeight.Text = scrollViewer.ExtentHeight.ToString();
        }

        private void BtnGetViewportWidth_Click(object sender, RoutedEventArgs e)
        {
            tblViewportWidth.Text = scrollViewer.ViewportWidth.ToString();
        }

        private void BtnGetViewportHeight_Click(object sender, RoutedEventArgs e)
        {
            tblViewportHeight.Text = scrollViewer.ViewportHeight.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            txtWidth.Text = scrollViewer.Width.ToString();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            txtMaxWidth.Text = scrollViewer.MaxWidth.ToString();
        }

        private void BtnSetMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.MaxWidth = Convert.ToDouble(txtMaxWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            txtHeight.Text = scrollViewer.Height.ToString();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            txtMaxHeight.Text = scrollViewer.MaxHeight.ToString();
        }

        private void BtnSetMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.MaxHeight = Convert.ToDouble(txtMaxHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetActualWidth_Click(object sender, RoutedEventArgs e)
        {
            tblActualWidth.Text = scrollViewer.ActualWidth.ToString();
        }

        private void BtnGetActualHeight_Click(object sender, RoutedEventArgs e)
        {
            tblActualHeight.Text = scrollViewer.ActualHeight.ToString();
        }

        private void BtnClearScrollViewerEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollViewerEvents.Items.Clear();
        }

        private void BtnChangeView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double? horizontalOffset = string.IsNullOrWhiteSpace(txtChangeViewHorizontalOffset.Text) ? (double?)null : Convert.ToDouble(txtChangeViewHorizontalOffset.Text);
                double? verticalOffset = string.IsNullOrWhiteSpace(txtChangeViewVerticalOffset.Text) ? (double?)null : Convert.ToDouble(txtChangeViewVerticalOffset.Text);
                float? zoomFactor = string.IsNullOrWhiteSpace(txtChangeViewZoomFactor.Text) ? (float?)null : Convert.ToSingle(txtChangeViewZoomFactor.Text);
                bool disableAnimation = true;

                if (horizontalOffset == null && verticalOffset == null && zoomFactor == null)
                {
                    return;
                }

                switch (cmbChangeViewAnimationMode.SelectedIndex)
                {
                    case 0: // Disabled
                        break;
                    case 1: // Enabled
                        disableAnimation = false;
                        break;
                    default: // Auto
                        disableAnimation = !new UISettings().AnimationsEnabled;
                        break;
                }

                AppendEventMessage($"ChangeView(horizontalOffset:{horizontalOffset}, verticalOffset:{verticalOffset}, zoomFactor:{zoomFactor}, disableAnimation:{disableAnimation}) from horizontalOffset:{scrollViewer.HorizontalOffset}, verticalOffset:{scrollViewer.VerticalOffset}, zoomFactor:{scrollViewer.ZoomFactor}");

                bool result = scrollViewer.ChangeView(horizontalOffset, verticalOffset, zoomFactor, disableAnimation);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollViewerEvents.Items.Add(ex.ToString());
            }
        }

        private void AppendEventMessage(string message)
        {
            lstScrollViewerEvents.Items.Add(message);
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
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
    }
}
