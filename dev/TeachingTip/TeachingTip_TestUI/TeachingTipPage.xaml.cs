using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;


#if !BUILD_WINDOWS
using TeachingTip = Microsoft.UI.Xaml.Controls.TeachingTip;
using TeachingTipClosedEventArgs = Microsoft.UI.Xaml.Controls.TeachingTipClosedEventArgs;
using TeachingTipClosingEventArgs = Microsoft.UI.Xaml.Controls.TeachingTipClosingEventArgs;
using TeachingTipTestHooks = Microsoft.UI.Private.Controls.TeachingTipTestHooks;
using TeachingTipBleedingImagePlacementMode = Microsoft.UI.Xaml.Controls.TeachingTipBleedingImagePlacementMode;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class TeachingTipPage : TestPage
    {
        Deferral deferral;
        DispatcherTimer timer;
        Popup testWindowBounds;
        public TeachingTipPage()
        {
            this.InitializeComponent();
            TeachingTipTestHooks.IdleStatusChanged += TeachingTipTestHooks_IdleStatusChanged;
            TeachingTipTestHooks.OpenedStatusChanged += TeachingTipTestHooks_OpenedStatusChanged;
            TeachingTipTestHooks.EffectivePlacementChanged += TeachingTipTestHooks_EffectivePlacementChanged;
            TeachingTipTestHooks.EffectiveBleedingPlacementChanged += TeachingTipTestHooks_EffectiveBleedingPlacementChanged;
            TeachingTipTestHooks.OffsetChanged += TeachingTipTestHooks_OffsetChanged;
            this.TeachingTip.SizeChanged += TeachingTip_SizeChanged;
            this.ContentScrollViewer.ViewChanged += ContentScrollViewer_ViewChanged;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            if (this.TeachingTip != null && this.TeachingTip.IsOpen)
            {
                this.TeachingTip.IsOpen = false;
            }
            if(testWindowBounds != null && testWindowBounds.IsOpen)
            {
                testWindowBounds.IsOpen = false;
            }

            base.OnNavigatedFrom(e);
        }

        private void TeachingTip_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.TipHeight.Text = this.TeachingTip.ActualHeight.ToString();
            this.TipWidth.Text = this.TeachingTip.ActualWidth.ToString();
        }

        private void TeachingTipTestHooks_OffsetChanged(TeachingTip sender, object args)
        {
            this.PopupVerticalOffsetTextBlock.Text = TeachingTipTestHooks.GetVerticalOffset(this.TeachingTip).ToString();
            this.PopupHorizontalOffsetTextBlock.Text = TeachingTipTestHooks.GetHorizontalOffset(this.TeachingTip).ToString();
        }

        private void TeachingTipTestHooks_EffectiveBleedingPlacementChanged(TeachingTip sender, object args)
        {
            var placement = TeachingTipTestHooks.GetEffectiveBleedingPlacement(this.TeachingTip);
            this.EffectiveBleedingPlacementTextBlock.Text = placement.ToString();
        }

        private void TeachingTipTestHooks_EffectivePlacementChanged(TeachingTip sender, object args)
        {
            var placement = TeachingTipTestHooks.GetEffectivePlacement(this.TeachingTip);
            this.EffectivePlacementTextBlock.Text = placement.ToString();
        }

        private void ContentScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            this.ScrollViewerOffsetTextBox.Text = this.ContentScrollViewer.VerticalOffset.ToString();
            OnGetTargetBoundsButtonClicked(null, null);
        }

        private void TeachingTipTestHooks_OpenedStatusChanged(TeachingTip sender, object args)
        {
            if(this.TeachingTip.IsOpen)
            {
                this.IsOpenCheckBox.IsChecked= true;
            }
            else
            {
                this.IsOpenCheckBox.IsChecked = false;
            }
        }

        private void TeachingTipTestHooks_IdleStatusChanged(TeachingTip sender, object args)
        {
            if(TeachingTipTestHooks.GetIsIdle(TeachingTip))
            {
                this.IsIdleCheckBox.IsChecked = true;
            }
            else
            {
                this.IsIdleCheckBox.IsChecked = false;
            }
        }

        public void OnSetIconButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.IconComboBox.SelectedIndex == 0)
            {
                SymbolIconSource symbolIconSource = new SymbolIconSource();
                symbolIconSource.Symbol = Symbol.People;
                this.TeachingTip.IconSource = symbolIconSource;
                
            }
            else
            {
                this.TeachingTip.IconSource = null;
            }
        }

        public void OnSetBleedingContentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.BleedingContentComboBox.SelectedIndex == 0)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Red);
                this.TeachingTip.BleedingImageContent = grid;
            }
            else if (this.BleedingContentComboBox.SelectedIndex == 1)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Blue);
                this.TeachingTip.BleedingImageContent = grid;
            }
            else if (this.BleedingContentComboBox.SelectedIndex == 2)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/ingredient1.png");
                image.Source = bitmapImage;
                this.TeachingTip.BleedingImageContent = image;
            }
            else if (this.BleedingContentComboBox.SelectedIndex == 3)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/AutoSave.png");
                image.Source = bitmapImage;
                this.TeachingTip.BleedingImageContent = image;
            }
            else
            {
                this.TeachingTip.BleedingImageContent = null;
            }
        }

        public void OnSetContentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.ContentComboBox.SelectedIndex == 0)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Red);
                this.TeachingTip.Content = grid;
            }
            else if (this.ContentComboBox.SelectedIndex == 1)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Blue);
                this.TeachingTip.Content = grid;
            }
            else if (this.ContentComboBox.SelectedIndex == 2)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/ingredient1.png");
                image.Source = bitmapImage;
                this.TeachingTip.Content = image;
            }
            else if (this.ContentComboBox.SelectedIndex == 3)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "This is shorter content.";
                this.TeachingTip.Content = textBlock;
            }
            else if (this.ContentComboBox.SelectedIndex == 4)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "This is longer content. This is longer content. This is longer content. This is longer content. " +
                    "This is longer content. This is longer content.This is longer content. This is longer content." +
                    "This is longer content.This is longer content.This is longer content. This is longer content." +
                    "This is longer content.This is longer content.This is longer content.This is longer content." +
                    "This is longer content.This is longer content.This is longer content.This is longer content." +
                    "This is longer content.This is longer content.This is longer content.This is longer content." +
                    "This is longer content.This is longer content.This is longer content.This is longer content." +
                    "This is longer content.This is longer content.This is longer content.This is longer content." +
                    "This is longer content.This is longer content.";
                textBlock.TextWrapping = TextWrapping.WrapWholeWords;
                this.TeachingTip.Content = textBlock;
            }
            else if (this.ContentComboBox.SelectedIndex == 5)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/AutoSave.png");
                image.Source = bitmapImage;
                this.TeachingTip.Content = image;
            }
            else
            {
                this.TeachingTip.Content = null;
            }
        }

        public void OnSetTitleButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.TitleComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.Title = "";
            }
            else if (this.TitleComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.Title = "Short title.";
            }
            else
            {
                this.TeachingTip.Title = "This is a much longer title that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetSubtextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.SubtextComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.Subtext = "";
            }
            else if (this.SubtextComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.Subtext = "Short Subtext.";
            }
            else
            {
                this.TeachingTip.Subtext = "This is a much longer subtext that might cause some issues if we don't do the right thing..." +
                    "This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetActionButtonTextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.ActionButtonTextComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.ActionButtonText = "";
            }
            else if (this.ActionButtonTextComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.ActionButtonText = "A:Short Text.";
            }
            else
            {
                this.TeachingTip.ActionButtonText = "A:This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetCloseButtonTextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.CloseButtonTextComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.CloseButtonText = "";
            }
            else if (this.CloseButtonTextComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.CloseButtonText = "C:Short Text.";
            }
            else
            {
                this.TeachingTip.CloseButtonText = "C:This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetBleeingImagePlacementButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.BleedingImagePlacementComboBox.SelectedIndex==0)
            {
                this.TeachingTip.BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Auto;
            }
            else if(this.BleedingImagePlacementComboBox.SelectedIndex==1)
            {
                this.TeachingTip.BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Top;
            }
            else
            {
                this.TeachingTip.BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Bottom;
            }
        }
        public void OnGetTargetBoundsButtonClicked(object sender, RoutedEventArgs args)
        {
            var bounds = this.targetButton.TransformToVisual(null).TransformBounds(new Rect(0.0,
            0.0,
            this.targetButton.ActualWidth,
            this.targetButton.ActualHeight));
            
            this.TargetXOffsetTextBlock.Text = bounds.X.ToString();
            this.TargetYOffsetTextBlock.Text = bounds.Y.ToString();
            this.TargetWidthTextBlock.Text = bounds.Width.ToString();
            this.TargetHeightTextBlock.Text = bounds.Height.ToString();
        }

        public void OnSetScrollViewerOffsetButtonClicked(object sender, RoutedEventArgs args)
        {
            this.ContentScrollViewer.ChangeView(0, double.Parse(this.ScrollViewerOffsetTextBox.Text), 1);
        }

        public void OnBringTargetIntoViewButtonClicked(object sender, RoutedEventArgs args)
        {
            this.targetButton.StartBringIntoView(new BringIntoViewOptions());
        }

        public void OnUseTestWindowBoundsCheckBoxChecked(object sender, RoutedEventArgs args)
        {
            Rect windowRect = new Rect(double.Parse(this.TestWindowBoundsXTextBox.Text),
                                       double.Parse(this.TestWindowBoundsYTextBox.Text),
                                       double.Parse(this.TestWindowBoundsWidthTextBox.Text),
                                       double.Parse(this.TestWindowBoundsHeightTextBox.Text));
            TeachingTipTestHooks.SetUseTestWindowBounds(this.TeachingTip, true);
            TeachingTipTestHooks.SetTestWindowBounds(this.TeachingTip, windowRect);
            if(testWindowBounds == null)
            {
                testWindowBounds = new Popup();
                testWindowBounds.IsHitTestVisible = false;
            }
            Grid windowBounds = new Grid();
            windowBounds.Width = windowRect.Width;
            windowBounds.Height = windowRect.Height;
            windowBounds.Background = new SolidColorBrush(Colors.Transparent);
            windowBounds.BorderBrush = new SolidColorBrush(Colors.Red);
            windowBounds.BorderThickness = new Thickness(1.0);
            testWindowBounds.Child = windowBounds;
            testWindowBounds.HorizontalOffset = windowRect.X;
            testWindowBounds.VerticalOffset = windowRect.Y;
            testWindowBounds.IsOpen = true;
        }

        public void OnUseTestWindowBoundsCheckBoxUnchecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetUseTestWindowBounds(this.TeachingTip, false);
            testWindowBounds.IsOpen = false;
        }

        public void OnSetPlacementButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.PlacementComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.Top;
            }
            else if (this.PlacementComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.Bottom;
            }
            else if (this.PlacementComboBox.SelectedIndex == 2)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.Left;
            }
            else if (this.PlacementComboBox.SelectedIndex == 3)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.Right;
            }
            else if (this.PlacementComboBox.SelectedIndex == 4)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.TopEdgeAlignedRight;
            }
            else if (this.PlacementComboBox.SelectedIndex == 5)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.TopEdgeAlignedLeft;
            }
            else if (this.PlacementComboBox.SelectedIndex == 6)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.BottomEdgeAlignedRight;
            }
            else if (this.PlacementComboBox.SelectedIndex == 7)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.BottomEdgeAlignedLeft;
            }
            else if (this.PlacementComboBox.SelectedIndex == 8)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.LeftEdgeAlignedTop;
            }
            else if (this.PlacementComboBox.SelectedIndex == 9)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.LeftEdgeAlignedBottom;
            }
            else if (this.PlacementComboBox.SelectedIndex == 10)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.RightEdgeAlignedTop;
            }
            else if (this.PlacementComboBox.SelectedIndex == 11)
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.RightEdgeAlignedBottom;
            }
            else
            {
                this.TeachingTip.Placement = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode.Auto;
            }
        }

        public void OnSetCloseButtonKindButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.CloseButtonKindComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.CloseButtonKind = Microsoft.UI.Xaml.Controls.TeachingTipCloseButtonKind.Auto;
            }
            else if (this.CloseButtonKindComboBox.SelectedIndex == 1)
            {
                this.TeachingTip.CloseButtonKind = Microsoft.UI.Xaml.Controls.TeachingTipCloseButtonKind.Header;
            }
            else
            {
                this.TeachingTip.CloseButtonKind = Microsoft.UI.Xaml.Controls.TeachingTipCloseButtonKind.Footer;
            }
        }

        public void OnSetIsLightDismissEnabledButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.IsLightDismissEnabledComboBox.SelectedIndex == 0)
            {
                this.TeachingTip.IsLightDismissEnabled = false;
            }
            else
            {
                this.TeachingTip.IsLightDismissEnabled = true;
            }
        }

        public void OnSetTargetOffsetButtonClicked(object sender, RoutedEventArgs args)
        {
            this.TeachingTip.TargetOffset = new Thickness(Double.Parse(this.TargetOffsetTextBox.Text));
        }

        public void OnSetTargetButtonClicked(object sender, RoutedEventArgs args)
        {
            TeachingTip.SetAttach(this.targetButton, this.TeachingTip);
        }

        public void OnUntargetButtonClicked(object sender, RoutedEventArgs args)
        {
            TeachingTip.SetAttach(null, this.TeachingTip);
        }

        public void OnShowButtonClicked(object sender, RoutedEventArgs args)
        {
            this.TeachingTip.IsOpen = true;
        }

        public void OnCloseButtonClicked(object sender, RoutedEventArgs args)
        {
            this.TeachingTip.IsOpen = false;
        }

        public void OnSetTargetVerticalAlignmentButtonClicked(object sender, RoutedEventArgs args)
        {
            if(TargetVerticalAlignmentComboBox.SelectedIndex == 0)
            {
                this.targetButton.VerticalAlignment = VerticalAlignment.Top;
            }
            else if (TargetVerticalAlignmentComboBox.SelectedIndex == 1)
            {
                this.targetButton.VerticalAlignment = VerticalAlignment.Center;
            }
            else
            {
                this.targetButton.VerticalAlignment = VerticalAlignment.Bottom;
            }
            OnGetTargetBoundsButtonClicked(null, null);
        }
        public void OnSetTargetHorizontalAlignmentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (TargetHorizontalAlignmentComboBox.SelectedIndex == 0)
            {
                this.targetButton.HorizontalAlignment = HorizontalAlignment.Left;
            }
            else if (TargetHorizontalAlignmentComboBox.SelectedIndex == 1)
            {
                this.targetButton.HorizontalAlignment = HorizontalAlignment.Center;
            }
            else
            {
                this.targetButton.HorizontalAlignment = HorizontalAlignment.Right;
            }
            OnGetTargetBoundsButtonClicked(null, null);
        }

        public void OnSetAnimationParametersButtonClicked(object sender, RoutedEventArgs args)
        {
            var expandEasing = Window.Current.Compositor.CreateCubicBezierEasingFunction(
                new System.Numerics.Vector2(float.Parse(this.ExpandControlPoint1X.Text), float.Parse(this.ExpandControlPoint1Y.Text)),
                new System.Numerics.Vector2(float.Parse(this.ExpandControlPoint2X.Text), float.Parse(this.ExpandControlPoint2Y.Text)));

            var contractEasing = Window.Current.Compositor.CreateCubicBezierEasingFunction(
                new System.Numerics.Vector2(float.Parse(this.ExpandControlPoint1X.Text), float.Parse(this.ExpandControlPoint1Y.Text)),
                new System.Numerics.Vector2(float.Parse(this.ExpandControlPoint2X.Text), float.Parse(this.ExpandControlPoint2Y.Text)));

            TeachingTipTestHooks.SetExpandEasingFunction(this.TeachingTip, expandEasing);
            TeachingTipTestHooks.SetContractEasingFunction(this.TeachingTip, contractEasing);
        }

        public void ContentElevationSliderChanged(object sender, RangeBaseValueChangedEventArgs args)
        {
            TeachingTipTestHooks.SetContentElevation(this.TeachingTip, (float)args.NewValue);
        }

        public void BeakElevationSliderChanged(object sender, RangeBaseValueChangedEventArgs args)
        {
            TeachingTipTestHooks.SetBeakElevation(this.TeachingTip, (float)args.NewValue);
        }

        public void OnBeakShadowTargetChecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetBeakShadowTargetsShadowTarget(this.TeachingTip, true);
        }

        public void OnBeakShadowTargetUnchecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetBeakShadowTargetsShadowTarget(this.TeachingTip, false);
        }

        public void OnTeachingTipClosed(object sender, TeachingTipClosedEventArgs args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + args.ToString() + " Reason: " + args.Reason.ToString());
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());
        }

        public void OnTeachingTipClosing(object sender, TeachingTipClosingEventArgs args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + args.ToString() + " Reason: " + args.Reason.ToString());
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());
            if (CancelClosesCheckBox.IsChecked == true)
            {
                deferral = args.GetDeferral();
                args.Cancel = true;
                timer = new DispatcherTimer();
                timer.Tick += Timer_Tick;
                timer.Interval = new TimeSpan(0, 0, 1);
                timer.Start();
            }
        }

        private void Timer_Tick(object sender, object e)
        {
            timer.Stop();
            deferral.Complete();
        }

        public void OnTeachingTipActionButtonClicked(object sender, object args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + "Action Button Clicked Event");
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());
        }

        public void OnTeachingTipCloseButtonClicked(object sender, object args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + "Close Button Clicked Event");
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());
        }

        private void BtnClearTeachingTipEvents_Click(object sender, RoutedEventArgs e)
        {
            this.lstTeachingTipEvents.Items.Clear();
        }
    }
}
