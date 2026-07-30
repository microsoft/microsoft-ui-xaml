// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TitleBar", Icon = "DefaultIcon.png")]
    public sealed partial class TitleBarPage : TestPage
    {
        public TitleBarPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();
        }

        private void CmbTitleBarOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "TitleBar",
                cmbTitleBarOutputDebugStringLevel.SelectedIndex == 1 || cmbTitleBarOutputDebugStringLevel.SelectedIndex == 2,
                cmbTitleBarOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void TitleBarWindowingButton_Click(object sender, RoutedEventArgs e)
        {
            var newWindow = new TitleBarPageWindow();
            newWindow.Activate();
        }

        private void GetAutoRefreshValueButton_Click(object sender, RoutedEventArgs e)
        {
            DragRegionStatusTextBlock.Text = "AutoRefresh:" + WindowingTitleBar.AutoRefreshDragRegions.ToString();
        }

        private void SetAutoRefreshTrueButton_Click(object sender, RoutedEventArgs e)
        {
            WindowingTitleBar.AutoRefreshDragRegions = true;
            DragRegionStatusTextBlock.Text = "AutoRefresh:" + WindowingTitleBar.AutoRefreshDragRegions.ToString();
        }

        private void SetAutoRefreshFalseButton_Click(object sender, RoutedEventArgs e)
        {
            WindowingTitleBar.AutoRefreshDragRegions = false;
            DragRegionStatusTextBlock.Text = "AutoRefresh:" + WindowingTitleBar.AutoRefreshDragRegions.ToString();
        }

        private void RecomputeDragRegionsButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                WindowingTitleBar.RecomputeDragRegions();
                DragRegionStatusTextBlock.Text = "RecomputeDragRegions:Success";
            }
            catch (System.Exception ex)
            {
                DragRegionStatusTextBlock.Text = "RecomputeDragRegions:Error:" + ex.Message;
            }
        }

        private void SetIsDragRegionTrueButton_Click(object sender, RoutedEventArgs e)
        {
            TitleBar.SetIsDragRegion(AutoSuggestBoxInTitleBar, true);
            var val = TitleBar.GetIsDragRegion(AutoSuggestBoxInTitleBar);
            DragRegionStatusTextBlock.Text = "IsDragRegion:" + (val.HasValue ? val.Value.ToString() : "null");
        }

        private void SetIsDragRegionFalseButton_Click(object sender, RoutedEventArgs e)
        {
            TitleBar.SetIsDragRegion(AutoSuggestBoxInTitleBar, false);
            var val = TitleBar.GetIsDragRegion(AutoSuggestBoxInTitleBar);
            DragRegionStatusTextBlock.Text = "IsDragRegion:" + (val.HasValue ? val.Value.ToString() : "null");
        }

        private void GetIsDragRegionValueButton_Click(object sender, RoutedEventArgs e)
        {
            var val = TitleBar.GetIsDragRegion(AutoSuggestBoxInTitleBar);
            DragRegionStatusTextBlock.Text = "IsDragRegion:" + (val.HasValue ? val.Value.ToString() : "null");
        }

        private void ClearIsDragRegionButton_Click(object sender, RoutedEventArgs e)
        {
            AutoSuggestBoxInTitleBar.ClearValue(TitleBar.IsDragRegionProperty);
            var val = TitleBar.GetIsDragRegion(AutoSuggestBoxInTitleBar);
            DragRegionStatusTextBlock.Text = "IsDragRegion:" + (val.HasValue ? val.Value.ToString() : "null");
        }
    }
}
