// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using AnimatedIcon = Microsoft.UI.Xaml.Controls.AnimatedIcon;
using Microsoft.UI.Private.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "AnimatedIcon")]
    public sealed partial class AnimatedIconPage : TestPage
    {
        public AnimatedIconPage()
        {
            this.InitializeComponent();
            this.Loaded += AnimatedIconPage_Loaded;
            AnimatedIconTestHooks.LastAnimationSegmentChanged += AnimatedIconTestHooks_LastAnimationSegmentChanged;
        }

        private void AnimatedIconTestHooks_LastAnimationSegmentChanged(AnimatedIcon sender, object args)
        {
            var rootGrid = VisualTreeHelper.GetChild(LargeAnimatedCheckbox, 0);
            var grid = VisualTreeHelper.GetChild(rootGrid, 0);
            var CheckIcon = VisualTreeHelper.GetChild(grid, 1) as AnimatedIcon;

            if(sender == CheckIcon)
            {
                LastTransitionTextBlock.Text = AnimatedIconTestHooks.GetLastAnimationSegment(CheckIcon);
            }
        }

        private void AnimatedIconPage_Loaded(object sender, RoutedEventArgs e)
        {
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.DropDownIcon_Cut.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.Cut);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.DropDownIcon_Queue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.QueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.DropDownIcon_SpeedUpQueue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.HamburgerIcon_Cut.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.Cut);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.HamburgerIcon_Queue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.QueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.HamburgerIcon_SpeedUpQueue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SettingsIcon_Cut.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.Cut);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SettingsIcon_Queue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.QueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SettingsIcon_SpeedUpQueue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.BackIcon_Cut.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.Cut);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.BackIcon_Queue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.QueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.BackIcon_SpeedUpQueue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SearchIcon_Cut.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.Cut);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SearchIcon_Queue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.QueueOne);
            AnimatedIconTestHooks.SetAnimationQueueBehavior(this.SearchIcon_SpeedUpQueue.GetAnimatedIcon(), AnimatedIconAnimationQueueBehavior.SpeedUpQueueOne);
        }

        private void Slider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (this.DropDownIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.DropDownIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.DropDownIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.DropDownIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.DropDownIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.DropDownIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.HamburgerIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.HamburgerIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.HamburgerIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SettingsIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SettingsIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SettingsIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.BackIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.BackIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.BackIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SearchIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SearchIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetDurationMultiplier(this.SearchIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.AnimatedCheckbox != null)
            {
                var rootGrid = VisualTreeHelper.GetChild(AnimatedCheckbox, 0);
                var grid = VisualTreeHelper.GetChild(rootGrid, 0);
                var CheckIcon = VisualTreeHelper.GetChild(grid, 1) as AnimatedIcon;

                AnimatedIconTestHooks.SetDurationMultiplier(CheckIcon, (float)e.NewValue);
            }
            if (this.LargeAnimatedCheckbox != null)
            {
                var rootGrid = VisualTreeHelper.GetChild(LargeAnimatedCheckbox, 0);
                var grid = VisualTreeHelper.GetChild(rootGrid, 0);
                var CheckIcon = VisualTreeHelper.GetChild(grid, 1) as AnimatedIcon;

                AnimatedIconTestHooks.SetDurationMultiplier(CheckIcon, (float)e.NewValue);
            }
        }

        private void SpeedUpSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (this.DropDownIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.DropDownIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.DropDownIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.DropDownIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.DropDownIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.DropDownIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.HamburgerIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.HamburgerIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.HamburgerIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.HamburgerIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SettingsIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SettingsIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SettingsIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SettingsIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.BackIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.BackIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.BackIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.BackIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_Cut != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SearchIcon_Cut.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_Queue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SearchIcon_Queue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.SearchIcon_SpeedUpQueue != null)
            {
                AnimatedIconTestHooks.SetSpeedUpMultiplier(this.SearchIcon_SpeedUpQueue.GetAnimatedIcon(), (float)e.NewValue);
            }
            if (this.AnimatedCheckbox != null)
            {
                var rootGrid = VisualTreeHelper.GetChild(AnimatedCheckbox, 0);
                var grid = VisualTreeHelper.GetChild(rootGrid, 0);
                var CheckIcon = VisualTreeHelper.GetChild(grid, 1) as AnimatedIcon;

                AnimatedIconTestHooks.SetSpeedUpMultiplier(CheckIcon, (float)e.NewValue);
            }
            if (this.LargeAnimatedCheckbox != null)
            {
                var rootGrid = VisualTreeHelper.GetChild(LargeAnimatedCheckbox, 0);
                var grid = VisualTreeHelper.GetChild(rootGrid, 0);
                var CheckIcon = VisualTreeHelper.GetChild(grid, 1) as AnimatedIcon;

                AnimatedIconTestHooks.SetSpeedUpMultiplier(CheckIcon, (float)e.NewValue);
            }
        }
    }
}
