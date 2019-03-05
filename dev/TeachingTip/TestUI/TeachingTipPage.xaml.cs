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
using TeachingTipPlacementMode = Microsoft.UI.Xaml.Controls.TeachingTipPlacementMode;
using TeachingTipCloseButtonKind = Microsoft.UI.Xaml.Controls.TeachingTipCloseButtonKind;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
#endif

namespace MUXControlsTestApp
{
    enum TipLocation
    {
        SetAttach = 0,
        VisualTree = 1,
        Resources = 2
    }
    public sealed partial class TeachingTipPage : TestPage
    {
        Deferral deferral;
        DispatcherTimer timer;
        Popup testWindowBounds;
        TipLocation tipLocation = TipLocation.SetAttach;

        public TeachingTipPage()
        {
            this.InitializeComponent();
            TeachingTipTestHooks.IdleStatusChanged += TeachingTipTestHooks_IdleStatusChanged;
            TeachingTipTestHooks.OpenedStatusChanged += TeachingTipTestHooks_OpenedStatusChanged;
            TeachingTipTestHooks.EffectivePlacementChanged += TeachingTipTestHooks_EffectivePlacementChanged;
            TeachingTipTestHooks.EffectiveBleedingPlacementChanged += TeachingTipTestHooks_EffectiveBleedingPlacementChanged;
            TeachingTipTestHooks.OffsetChanged += TeachingTipTestHooks_OffsetChanged;
            this.TeachingTipInSetAttach.SizeChanged += TeachingTip_SizeChanged;
            this.TeachingTipInVisualTree.SizeChanged += TeachingTip_SizeChanged;
            this.TeachingTipInResources.SizeChanged += TeachingTip_SizeChanged;
            this.ContentScrollViewer.ViewChanged += ContentScrollViewer_ViewChanged;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            if (this.TeachingTipInSetAttach != null && this.TeachingTipInSetAttach.IsOpen)
            {
                this.TeachingTipInSetAttach.IsOpen = false;
            }
            if (this.TeachingTipInResources != null && this.TeachingTipInResources.IsOpen)
            {
                this.TeachingTipInResources.IsOpen = false;
            }
            if (this.TeachingTipInVisualTree != null && this.TeachingTipInVisualTree.IsOpen)
            {
                this.TeachingTipInVisualTree.IsOpen = false;
            }
            if (testWindowBounds != null && testWindowBounds.IsOpen)
            {
                testWindowBounds.IsOpen = false;
            }

            base.OnNavigatedFrom(e);
        }

        private void TeachingTip_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if(sender == getTeachingTip())
            {
                this.TipHeightTextBlock.Text = ((TeachingTip)sender).ActualHeight.ToString();
                this.TipWidthTextBlock.Text = ((TeachingTip)sender).ActualWidth.ToString();
            }
        }

        private void TeachingTipTestHooks_OffsetChanged(TeachingTip sender, object args)
        {
            if (sender == getTeachingTip())
            {
                this.PopupVerticalOffsetTextBlock.Text = TeachingTipTestHooks.GetVerticalOffset(sender).ToString();
                this.PopupHorizontalOffsetTextBlock.Text = TeachingTipTestHooks.GetHorizontalOffset(sender).ToString();
            }
        }

        private void TeachingTipTestHooks_EffectiveBleedingPlacementChanged(TeachingTip sender, object args)
        {
            if (sender == getTeachingTip())
            {
                var placement = TeachingTipTestHooks.GetEffectiveBleedingPlacement(sender);
                this.EffectiveBleedingPlacementTextBlock.Text = placement.ToString();
            }
        }

        private void TeachingTipTestHooks_EffectivePlacementChanged(TeachingTip sender, object args)
        {
            if (sender == getTeachingTip())
            {
                var placement = TeachingTipTestHooks.GetEffectivePlacement(sender);
                this.EffectivePlacementTextBlock.Text = placement.ToString();
            }
        }

        private void ContentScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            this.ScrollViewerOffsetTextBox.Text = this.ContentScrollViewer.VerticalOffset.ToString();
            if(e.IsIntermediate)
            {
                this.ScrollViewerStateTextBox.Text = "Scrolling";
            }
            else
            {
                this.ScrollViewerStateTextBox.Text = "Idle";
            }
            OnGetTargetBoundsButtonClicked(null, null);
        }

        private void TeachingTipTestHooks_OpenedStatusChanged(TeachingTip sender, object args)
        {
            if (sender == getTeachingTip())
            {
                if (sender.IsOpen)
                {
                    this.IsOpenCheckBox.IsChecked = true;
                }
                else
                {
                    this.IsOpenCheckBox.IsChecked = false;
                }
            }
        }

        private void TeachingTipTestHooks_IdleStatusChanged(TeachingTip sender, object args)
        {
            if (sender == getTeachingTip())
            {
                if (TeachingTipTestHooks.GetIsIdle(sender))
                {
                    this.IsIdleCheckBox.IsChecked = true;
                }
                else
                {
                    this.IsIdleCheckBox.IsChecked = false;
                }
            }
        }

        public void OnSetIconButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.IconComboBox.SelectedItem == IconPeople)
            {
                SymbolIconSource symbolIconSource = new SymbolIconSource();
                symbolIconSource.Symbol = Symbol.People;
                getTeachingTip().IconSource = symbolIconSource;
                
            }
            else
            {
                getTeachingTip().IconSource = null;
            }
        }

        public void OnSetTipLocationButton(object sender, RoutedEventArgs args)
        {
            if(this.TipLocationComboBox.SelectedItem == TipInSetAttach)
            {
                if(tipLocation != TipLocation.SetAttach)
                {
                    TeachingTipInResources.IsOpen = false;
                    TeachingTipInVisualTree.IsOpen = false;
                    tipLocation = TipLocation.SetAttach;
                }
            }
            else if (this.TipLocationComboBox.SelectedItem == TipInVisualTree)
            {
                if (tipLocation != TipLocation.VisualTree)
                {
                    TeachingTipInResources.IsOpen = false;
                    TeachingTipInSetAttach.IsOpen = false;
                    tipLocation = TipLocation.VisualTree;
                }
                tipLocation = TipLocation.VisualTree;
            }
            else
            {
                if (tipLocation != TipLocation.Resources)
                {
                    TeachingTipInVisualTree.IsOpen = false;
                    TeachingTipInSetAttach.IsOpen = false;
                    tipLocation = TipLocation.Resources;
                }
                tipLocation = TipLocation.Resources;
            }
        }

        public void OnSetBleedingContentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.BleedingContentComboBox.SelectedItem == BleedingRedSquare)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Red);
                getTeachingTip().BleedingImageContent = grid;
            }
            else if (this.BleedingContentComboBox.SelectedItem == BleedingBlueSquare)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Blue);
                getTeachingTip().BleedingImageContent = grid;
            }
            else if (this.BleedingContentComboBox.SelectedItem == BleedingImage)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/ingredient1.png");
                image.Source = bitmapImage;
                getTeachingTip().BleedingImageContent = image;
            }
            else if (this.BleedingContentComboBox.SelectedItem == BleedingAutoSave)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/AutoSave.png");
                image.Source = bitmapImage;
                getTeachingTip().BleedingImageContent = image;
            }
            else
            {
                getTeachingTip().BleedingImageContent = null;
            }
        }

        public void OnSetContentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.ContentComboBox.SelectedItem == ContentRedSquare)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Red);
                getTeachingTip().Content = grid;
            }
            else if (this.ContentComboBox.SelectedItem == ContentBlueSquare)
            {
                Grid grid = new Grid();
                grid.Background = new SolidColorBrush(Colors.Blue);
                getTeachingTip().Content = grid;
            }
            else if (this.ContentComboBox.SelectedItem == ContentImage)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/ingredient1.png");
                image.Source = bitmapImage;
                getTeachingTip().Content = image;
            }
            else if (this.ContentComboBox.SelectedItem == ContentShort)
            {
                TextBlock textBlock = new TextBlock();
                textBlock.Text = "This is shorter content.";
                getTeachingTip().Content = textBlock;
            }
            else if (this.ContentComboBox.SelectedItem == ContentLong)
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
                getTeachingTip().Content = textBlock;
            }
            else if (this.ContentComboBox.SelectedItem == ContentAutoSave)
            {
                Image image = new Image();
                BitmapImage bitmapImage = new BitmapImage();
                bitmapImage.UriSource = new Uri("ms-appx:///Assets/AutoSave.png");
                image.Source = bitmapImage;
                getTeachingTip().Content = image;
            }
            else
            {
                getTeachingTip().Content = null;
            }
        }

        public void OnSetTitleButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.TitleComboBox.SelectedItem == TitleNo)
            {
                getTeachingTip().Title = "";
            }
            else if (this.TitleComboBox.SelectedItem == TitleSmall)
            {
                getTeachingTip().Title = "Short title.";
            }
            else
            {
                getTeachingTip().Title = "This is a much longer title that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetSubtextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.SubtextComboBox.SelectedItem == SubtextNo)
            {
                getTeachingTip().Subtext = "";
            }
            else if (this.SubtextComboBox.SelectedItem == SubtextSmall)
            {
                getTeachingTip().Subtext = "Short Subtext.";
            }
            else
            {
                getTeachingTip().Subtext = "This is a much longer subtext that might cause some issues if we don't do the right thing..." +
                    "This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetActionButtonTextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.ActionButtonTextComboBox.SelectedItem == ActionButtonTextNo)
            {
                getTeachingTip().ActionButtonText = "";
            }
            else if (this.ActionButtonTextComboBox.SelectedItem == ActionButtonTextSmall)
            {
                getTeachingTip().ActionButtonText = "A:Short Text.";
            }
            else
            {
                getTeachingTip().ActionButtonText = "A:This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetCloseButtonTextButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.CloseButtonTextComboBox.SelectedItem == CloseButtonTextNo)
            {
                getTeachingTip().CloseButtonText = "";
            }
            else if (this.CloseButtonTextComboBox.SelectedItem == CloseButtonTextSmall)
            {
                getTeachingTip().CloseButtonText = "C:Short Text.";
            }
            else
            {
                getTeachingTip().CloseButtonText = "C:This is a much longer subtext that might cause some issues if we don't do the right thing...";
            }
        }

        public void OnSetBleeingImagePlacementButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.BleedingImagePlacementComboBox.SelectedItem == BleedingPlacementAuto)
            {
                getTeachingTip().BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Auto;
            }
            else if(this.BleedingImagePlacementComboBox.SelectedItem == BleedingPlacementTop)
            {
                getTeachingTip().BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Top;
            }
            else
            {
                getTeachingTip().BleedingImagePlacement = TeachingTipBleedingImagePlacementMode.Bottom;
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
            var tip = getTeachingTip();
            Rect windowRect = new Rect(double.Parse(this.TestWindowBoundsXTextBox.Text),
                                       double.Parse(this.TestWindowBoundsYTextBox.Text),
                                       double.Parse(this.TestWindowBoundsWidthTextBox.Text),
                                       double.Parse(this.TestWindowBoundsHeightTextBox.Text));
            TeachingTipTestHooks.SetUseTestWindowBounds(tip, true);
            TeachingTipTestHooks.SetTestWindowBounds(tip, windowRect);
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
            TeachingTipTestHooks.SetUseTestWindowBounds(getTeachingTip(), false);
            testWindowBounds.IsOpen = false;
        }

        public void OnTipFollowsTargetCheckBoxChecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetTipFollowsTarget(getTeachingTip(), true);
        }

        public void OnTipFollowsTargetCheckBoxUnchecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetTipFollowsTarget(getTeachingTip(), false);
        }

        public void OnSetPlacementButtonClicked(object sender, RoutedEventArgs args)
        {
            if (this.PlacementComboBox.SelectedItem == PlacementTop)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.Top;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementBottom)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.Bottom;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementLeft)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.Left;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementRight)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.Right;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementTopEdgeRight)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.TopEdgeAlignedRight;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementTopEdgeLeft)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.TopEdgeAlignedLeft;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementBottomEdgeRight)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.BottomEdgeAlignedRight;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementBottomEdgeLeft)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.BottomEdgeAlignedLeft;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementLeftEdgeTop)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.LeftEdgeAlignedTop;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementLeftEdgeBottom)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.LeftEdgeAlignedBottom;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementRightEdgeTop)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.RightEdgeAlignedTop;
            }
            else if (this.PlacementComboBox.SelectedItem == PlacementRightEdgeBottom)
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.RightEdgeAlignedBottom;
            }
            else
            {
                getTeachingTip().Placement = TeachingTipPlacementMode.Auto;
            }
        }

        public void OnSetCloseButtonKindButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.CloseButtonKindComboBox.SelectedItem == CloseButtonKindAuto)
            {
                getTeachingTip().CloseButtonKind = TeachingTipCloseButtonKind.Auto;
            }
            else if (this.CloseButtonKindComboBox.SelectedItem == CloseButtonKindHeader)
            {
                getTeachingTip().CloseButtonKind = TeachingTipCloseButtonKind.Header;
            }
            else
            {
                getTeachingTip().CloseButtonKind = TeachingTipCloseButtonKind.Footer;
            }
        }

        public void OnSetIsLightDismissEnabledButtonClicked(object sender, RoutedEventArgs args)
        {
            if(this.IsLightDismissEnabledComboBox.SelectedItem == IsLightDismissFalse)
            {
                getTeachingTip().IsLightDismissEnabled = false;
            }
            else
            {
                getTeachingTip().IsLightDismissEnabled = true;
            }
        }

        public void OnSetTargetOffsetButtonClicked(object sender, RoutedEventArgs args)
        {
            getTeachingTip().TargetOffset = new Thickness(Double.Parse(this.TargetOffsetTextBox.Text));
        }

        public void OnSetTargetButtonClicked(object sender, RoutedEventArgs args)
        {
            TeachingTip.SetAttach(this.targetButton, getTeachingTip());
        }

        public void OnUntargetButtonClicked(object sender, RoutedEventArgs args)
        {
            TeachingTip.SetAttach(null, getTeachingTip());
        }

        public void OnShowButtonClicked(object sender, RoutedEventArgs args)
        {
            getTeachingTip().IsOpen = true;
        }

        public void OnCloseButtonClicked(object sender, RoutedEventArgs args)
        {
            getTeachingTip().IsOpen = false;
        }

        public void OnSetTargetVerticalAlignmentButtonClicked(object sender, RoutedEventArgs args)
        {
            if(TargetVerticalAlignmentComboBox.SelectedItem == TargetVerticalAlignmentTop)
            {
                getTeachingTip().VerticalAlignment = VerticalAlignment.Top;
            }
            else if (TargetVerticalAlignmentComboBox.SelectedItem == TargetVerticalAlignmentCenter)
            {
                getTeachingTip().VerticalAlignment = VerticalAlignment.Center;
            }
            else
            {
                getTeachingTip().VerticalAlignment = VerticalAlignment.Bottom;
            }
            OnGetTargetBoundsButtonClicked(null, null);
        }
        public void OnSetTargetHorizontalAlignmentButtonClicked(object sender, RoutedEventArgs args)
        {
            if (TargetHorizontalAlignmentComboBox.SelectedItem == TargetHorizontalAlignmentLeft)
            {
                getTeachingTip().HorizontalAlignment = HorizontalAlignment.Left;
            }
            else if (TargetHorizontalAlignmentComboBox.SelectedItem == TargetHorizontalAlignmentCenter)
            {
                getTeachingTip().HorizontalAlignment = HorizontalAlignment.Center;
            }
            else
            {
                getTeachingTip().HorizontalAlignment = HorizontalAlignment.Right;
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

            var tip = getTeachingTip();
            TeachingTipTestHooks.SetExpandEasingFunction(tip, expandEasing);
            TeachingTipTestHooks.SetContractEasingFunction(tip, contractEasing);
        }

        public void OnSetAnimationDurationsButtonClicked(object sender, RoutedEventArgs args)
        {
            var expandDuration = new TimeSpan(0, 0, 0, 0, int.Parse(ExpandAnimationDuration.Text));
            var contractDuration = new TimeSpan(0, 0, 0, 0, int.Parse(ContractAnimationDuration.Text));
            TeachingTipTestHooks.SetExpandAnimationDuration(this.TeachingTip, expandDuration);
            TeachingTipTestHooks.SetContractAnimationDuration(this.TeachingTip, contractDuration);
        }

        public void ContentElevationSliderChanged(object sender, RangeBaseValueChangedEventArgs args)
        {
            TeachingTipTestHooks.SetContentElevation(getTeachingTip(), (float)args.NewValue);
        }

        public void BeakElevationSliderChanged(object sender, RangeBaseValueChangedEventArgs args)
        {
            TeachingTipTestHooks.SetBeakElevation(getTeachingTip(), (float)args.NewValue);
        }

        public void OnTipShadowChecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetTipShouldHaveShadow(getTeachingTip(), true);
        }

        public void OnTipShadowUnchecked(object sender, RoutedEventArgs args)
        {
            TeachingTipTestHooks.SetTipShouldHaveShadow(getTeachingTip(), false);
        }

        public void OnTeachingTipClosed(object sender, TeachingTipClosedEventArgs args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + args.ToString() + " Reason: " + args.Reason.ToString());
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());
        }

        public void OnTeachingTipClosing(TeachingTip sender, TeachingTipClosingEventArgs args)
        {
            lstTeachingTipEvents.Items.Add(lstTeachingTipEvents.Items.Count.ToString() + ") " + args.ToString() + " Reason: " + args.Reason.ToString());
            lstTeachingTipEvents.ScrollIntoView(lstTeachingTipEvents.Items.Last<object>());

            CheckBox cancelClosesCheckBox = null;
            if (sender == TeachingTipInSetAttach)
            {
                cancelClosesCheckBox = CancelClosesCheckBoxInSetAttach;
            }
            else if (sender == TeachingTipInResources)
            {
                cancelClosesCheckBox = CancelClosesCheckBoxInResources;
            }
            else
            {
                cancelClosesCheckBox = CancelClosesCheckBoxInVisualTree;
            }

            if (cancelClosesCheckBox.IsChecked == true)
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

        private TeachingTip getTeachingTip()
        {
            switch(tipLocation)
            {
                case TipLocation.SetAttach:
                    return this.TeachingTipInSetAttach;
                case TipLocation.VisualTree:
                    return this.TeachingTipInVisualTree;
                default:
                    return this.TeachingTipInResources;
            }
        }
    }
}
