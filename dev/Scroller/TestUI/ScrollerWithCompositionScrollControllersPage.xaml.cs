// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerWithCompositionScrollControllersPage : TestPage
    {
        public ScrollerWithCompositionScrollControllersPage()
        {
            this.InitializeComponent();
            horizontalCompositionScrollController.LogMessage += CompositionScrollController_LogMessage;
            verticalCompositionScrollController.LogMessage += CompositionScrollController_LogMessage;
        }

        private void CompositionScrollController_LogMessage(CompositionScrollController sender, string args)
        {
            LogMessage(args);
        }

        private void CmbHorizontalScrollController_SelectionChanged(object sender, Windows.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            ComboBox cmbHorizontalScrollController = sender as ComboBox;
            Scroller scroller = cmbHorizontalScrollController == cmbHorizontalScrollController1 ? scroller1 : scroller2;
            CompositionScrollController oldCompositionScrollController = scroller.HorizontalScrollController as CompositionScrollController;

            switch (cmbHorizontalScrollController.SelectedIndex)
            {
                case 0:
                    if (scroller.HorizontalScrollController != null)
                    {
                        scroller.HorizontalScrollController = null;
                        LogMessage(scroller.Name + ".HorizontalScrollController reset");
                    }
                    break;
                case 1:
                    scroller.HorizontalScrollController = horizontalCompositionScrollController;
                    LogMessage(scroller.Name + ".HorizontalScrollController set to horizontalScrollController");
                    break;
                case 2:
                    scroller.HorizontalScrollController = verticalCompositionScrollController;
                    LogMessage(scroller.Name + ".HorizontalScrollController set to verticalScrollController");
                    break;
            }

            if (oldCompositionScrollController != null)
                oldCompositionScrollController.IsEnabled = true;
        }

        private void CmbVerticalScrollController_SelectionChanged(object sender, Windows.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            ComboBox cmbVerticalScrollController = sender as ComboBox;
            Scroller scroller = cmbVerticalScrollController == cmbVerticalScrollController1 ? scroller1 : scroller2;
            CompositionScrollController oldCompositionScrollController = scroller.VerticalScrollController as CompositionScrollController;

            switch (cmbVerticalScrollController.SelectedIndex)
            {
                case 0:
                    if (scroller.VerticalScrollController != null)
                    {
                        scroller.VerticalScrollController = null;
                        LogMessage(scroller.Name + ".VerticalScrollController reset");
                    }
                    break;
                case 1:
                    scroller.VerticalScrollController = verticalCompositionScrollController;
                    LogMessage(scroller.Name + ".VerticalScrollController set to verticalScrollController");
                    break;
                case 2:
                    scroller.VerticalScrollController = horizontalCompositionScrollController;
                    LogMessage(scroller.Name + ".VerticalScrollController set to horizontalScrollController");
                    break;
            }

            if (oldCompositionScrollController != null)
                oldCompositionScrollController.IsEnabled = true;
        }

        private void LogMessage(string message)
        {
            if (chkLog.IsChecked.Value)
            {
                lstLog.Items.Add(message);
            }
        }

        private void BtnClearLog_Click(object sender, RoutedEventArgs e)
        {
            lstLog.Items.Clear();
        }

        private void ChkIsHorizontallyScrollable_Checked(object sender, RoutedEventArgs e)
        {
            CheckBox chkIsHorizontallyScrollable = sender as CheckBox;
            ComboBox cmbHorizontalScrollController = chkIsHorizontallyScrollable == chkIsHorizontallyScrollable1 ? cmbHorizontalScrollController1 : cmbHorizontalScrollController2;
            Scroller scroller = chkIsHorizontallyScrollable == chkIsHorizontallyScrollable1 ? scroller1 : scroller2;

            scroller.HorizontalScrollMode = ScrollMode.Enabled;
            switch (cmbHorizontalScrollController.SelectedIndex)
            {
                case 1:
                    horizontalCompositionScrollController.IsEnabled = true;
                    break;
                case 2:
                    verticalCompositionScrollController.IsEnabled = true;
                    break;
            }
        }

        private void ChkIsHorizontallyScrollable_Unchecked(object sender, RoutedEventArgs e)
        {
            CheckBox chkIsHorizontallyScrollable = sender as CheckBox;
            ComboBox cmbHorizontalScrollController = chkIsHorizontallyScrollable == chkIsHorizontallyScrollable1 ? cmbHorizontalScrollController1 : cmbHorizontalScrollController2;
            Scroller scroller = chkIsHorizontallyScrollable == chkIsHorizontallyScrollable1 ? scroller1 : scroller2;

            scroller.HorizontalScrollMode = ScrollMode.Disabled;
            switch (cmbHorizontalScrollController.SelectedIndex)
            {
                case 1:
                    horizontalCompositionScrollController.IsEnabled = false;
                    break;
                case 2:
                    verticalCompositionScrollController.IsEnabled = false;
                    break;
            }
        }

        private void ChkIsVerticallyScrollable_Checked(object sender, RoutedEventArgs e)
        {
            CheckBox chkIsVerticallyScrollable = sender as CheckBox;
            ComboBox cmbVerticalScrollController = chkIsVerticallyScrollable == chkIsVerticallyScrollable1 ? cmbVerticalScrollController1 : cmbVerticalScrollController2;
            Scroller scroller = chkIsVerticallyScrollable == chkIsVerticallyScrollable1 ? scroller1 : scroller2;

            scroller.VerticalScrollMode = ScrollMode.Enabled;
            switch (cmbVerticalScrollController.SelectedIndex)
            {
                case 1:
                    verticalCompositionScrollController.IsEnabled = true;
                    break;
                case 2:
                    horizontalCompositionScrollController.IsEnabled = true;
                    break;
            }
        }

        private void ChkIsVerticallyScrollable_Unchecked(object sender, RoutedEventArgs e)
        {
            CheckBox chkIsVerticallyScrollable = sender as CheckBox;
            ComboBox cmbVerticalScrollController = chkIsVerticallyScrollable == chkIsVerticallyScrollable1 ? cmbVerticalScrollController1 : cmbVerticalScrollController2;
            Scroller scroller = chkIsVerticallyScrollable == chkIsVerticallyScrollable1 ? scroller1 : scroller2;

            scroller.VerticalScrollMode = ScrollMode.Disabled;
            switch (cmbVerticalScrollController.SelectedIndex)
            {
                case 1:
                    verticalCompositionScrollController.IsEnabled = false;
                    break;
                case 2:
                    horizontalCompositionScrollController.IsEnabled = false;
                    break;
            }
        }

        private void ChkIsVisible_Checked(object sender, RoutedEventArgs e)
        {
            scroller2.Visibility = Visibility.Visible;
        }

        private void ChkIsVisible_Unchecked(object sender, RoutedEventArgs e)
        {
            scroller2.Visibility = Visibility.Collapsed;
        }

        private void ChkIsThumbPositionMirrored_Checked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsHorizontalThumbPositionMirrored)
                horizontalCompositionScrollController.IsThumbPositionMirrored = true;
            else
                verticalCompositionScrollController.IsThumbPositionMirrored = true;
        }

        private void ChkIsThumbPositionMirrored_Unchecked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsHorizontalThumbPositionMirrored)
                horizontalCompositionScrollController.IsThumbPositionMirrored = false;
            else
                verticalCompositionScrollController.IsThumbPositionMirrored = false;
        }

        private void ChkIsAnimatingThumbOffset_Checked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsAnimatingHorizontalThumbOffset)
                horizontalCompositionScrollController.IsAnimatingThumbOffset = true;
            else
                verticalCompositionScrollController.IsAnimatingThumbOffset = true;
        }

        private void ChkIsAnimatingThumbOffset_Unchecked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsAnimatingHorizontalThumbOffset)
                horizontalCompositionScrollController.IsAnimatingThumbOffset = false;
            else
                verticalCompositionScrollController.IsAnimatingThumbOffset = false;
        }

        private void ChkIsThumbPannable_Checked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsHorizontalThumbPannable)
                horizontalCompositionScrollController.IsThumbPannable = true;
            else
                verticalCompositionScrollController.IsThumbPannable = true;
        }

        private void ChkIsThumbPannable_Unchecked(object sender, RoutedEventArgs e)
        {
            if (sender == chkIsHorizontalThumbPannable)
                horizontalCompositionScrollController.IsThumbPannable = false;
            else
                verticalCompositionScrollController.IsThumbPannable = false;
        }

        private void txtHorizontalOverriddenOffsetsChangeDuration_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                double durationOverride = Convert.ToDouble(txtHorizontalOverriddenOffsetsChangeDuration.Text);
                horizontalCompositionScrollController.OverriddenOffsetChangeDuration = TimeSpan.FromMilliseconds(durationOverride);
            }
            catch
            {
                horizontalCompositionScrollController.OverriddenOffsetChangeDuration = TimeSpan.MinValue;
            }
        }

        private void txtVerticalOverriddenOffsetsChangeDuration_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                double durationOverride = Convert.ToDouble(txtVerticalOverriddenOffsetsChangeDuration.Text);
                verticalCompositionScrollController.OverriddenOffsetChangeDuration = TimeSpan.FromMilliseconds(durationOverride);
            }
            catch
            {
                verticalCompositionScrollController.OverriddenOffsetChangeDuration = TimeSpan.MinValue;
            }
        }

        private void cmbHorizontalOverriddenOffsetsChangeAnimation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbHorizontalOverriddenOffsetsChangeAnimation.SelectedIndex)
            {
                case 0:
                    horizontalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Default;
                    break;
                case 1:
                    horizontalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Accordion;
                    break;
                case 2:
                    horizontalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Teleportation;
                    break;
            }
        }

        private void cmbVerticalOverriddenOffsetsChangeAnimation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbVerticalOverriddenOffsetsChangeAnimation.SelectedIndex)
            {
                case 0:
                    verticalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Default;
                    break;
                case 1:
                    verticalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Accordion;
                    break;
                case 2:
                    verticalCompositionScrollController.OffsetChangeAnimationType = CompositionScrollController.AnimationType.Teleportation;
                    break;
            }
        }
    }
}
