// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class TeachingTipFocusPage : TestPage
    {
        public TeachingTipFocusPage()
        {
            this.InitializeComponent();
            TeachingTipTestHooks.IdleStatusChanged += TeachingTipTestHooks_IdleStatusChanged;
            TeachingTipTestHooks.OpenedStatusChanged += TeachingTipTestHooks_OpenedStatusChanged;
            TestTeachingTip.Loaded += TestTeachingTip_Loaded;

            this.Loaded += OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            ChangeTestFrameVisibility(Visibility.Collapsed);
        }

        private void ChangeTestFrameVisibility(Visibility visibility)
        {
            var testFrame = Window.Current.Content as TestFrame;
            testFrame.ChangeBarVisibility(visibility);
        }

        private void OpenTeachingTipButton_Click(object sender, RoutedEventArgs e)
        {
            this.TestTeachingTip.IsOpen = true;
        }

        private void CloseTeachingTipButton_Click(object sender, RoutedEventArgs e)
        {
            this.TestTeachingTip.IsOpen = false;
        }

        private void TeachingTipTestHooks_OpenedStatusChanged(TeachingTip sender, object args)
        {
            if (this.TestTeachingTip.IsOpen)
            {
                this.IsOpenCheckBox.IsChecked = true;
            }
            else
            {
                this.IsOpenCheckBox.IsChecked = false;
            }
        }

        private void TeachingTipTestHooks_IdleStatusChanged(TeachingTip sender, object args)
        {
            if (TeachingTipTestHooks.GetIsIdle(this.TestTeachingTip))
            {
                this.IsIdleCheckBox.IsChecked = true;
            }
            else
            {
                this.IsIdleCheckBox.IsChecked = false;
            }
        }

        private void TestTeachingTip_Loaded(object sender, RoutedEventArgs e)
        {
            this.TitleVisibilityTextBlock.Text = TeachingTipTestHooks.GetTitleVisibility(TestTeachingTip).ToString();
            this.SubtitleVisibilityTextBlock.Text = TeachingTipTestHooks.GetSubtitleVisibility(TestTeachingTip).ToString();
        }
    }
}
