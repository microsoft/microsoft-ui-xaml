// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.ApplicationModel.Core;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

using NavigationViewDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewDisplayModeChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewDisplayModeChangedEventArgs;
using NavigationViewBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewTitleBarIntegrationPage : TestPage
    {
        public NavigationViewTitleBarIntegrationPage()
        {
            this.InitializeComponent();

            this.Loaded += NavigationViewTitleBarIntegration_Loaded;
        }

        private void NavigationViewTitleBarIntegration_Loaded(object sender, RoutedEventArgs e)
        {
            chkIsTitleBarAutoPaddingEnabled.IsChecked = NavView.IsTitleBarAutoPaddingEnabled;
            chkIsPaneToggleButtonVisible.IsChecked = NavView.IsPaneToggleButtonVisible;
            chkAlwaysShowHeader.IsChecked = NavView.AlwaysShowHeader;
            chkExtendViewIntoTitleBar.IsChecked = CoreApplication.GetCurrentView().TitleBar.ExtendViewIntoTitleBar;
            chkIsSettingsVisible.IsChecked = NavView.IsSettingsVisible;
            BtnGetCoreApplicationViewTitleBarHeight_Click(null, null);
            BtnGetCoreApplicationViewTitleBarIsVisible_Click(null, null);
            BtnGetCoreApplicationViewTitleBarSystemOverlayLeftInset_Click(null, null);
            BtnGetCoreApplicationViewTitleBarSystemOverlayRightInset_Click(null, null);
            BtnGetAplicationViewIsFullScreenMode_Click(null, null);
            BtnGetUIViewSettingsUserInteractionMode_Click(null, null);
        }

        private void NavView_DisplayModeChanged(NavigationView sender, NavigationViewDisplayModeChangedEventArgs args)
        {
            switch (args.DisplayMode)
            {
                case NavigationViewDisplayMode.Minimal:
                    tblDisplayMode.Text = "Minimal";
                    tblDisplayMode.Foreground = new SolidColorBrush(Colors.DodgerBlue);
                    break;
                case NavigationViewDisplayMode.Compact:
                    tblDisplayMode.Text = "Compact";
                    tblDisplayMode.Foreground = new SolidColorBrush(Colors.DarkMagenta);
                    break;
                case NavigationViewDisplayMode.Expanded:
                    tblDisplayMode.Text = "Expanded";
                    tblDisplayMode.Foreground = new SolidColorBrush(Colors.DarkRed);
                    break;
            }
        }

        private void ChkExtendViewIntoTitleBar_Checked(object sender, RoutedEventArgs e)
        {
            CoreApplication.GetCurrentView().TitleBar.ExtendViewIntoTitleBar = true;
        }

        private void ChkExtendViewIntoTitleBar_Unchecked(object sender, RoutedEventArgs e)
        {
            CoreApplication.GetCurrentView().TitleBar.ExtendViewIntoTitleBar = false;
        }

        private void ChkIsSettingsVisible_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsSettingsVisible = true;
        }

        private void ChkIsSettingsVisible_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsSettingsVisible = false;
        }

        private void ChkIsPaneToggleButtonVisible_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsPaneToggleButtonVisible = true;
        }

        private void ChkIsPaneToggleButtonVisible_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsPaneToggleButtonVisible = false;
        }

        private void ChkIsBackButtonVisible_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
        }

        private void ChkIsBackButtonVisible_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed;
        }

        private void ChkIsBackEnabled_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackEnabled = true;
        }

        private void ChkIsBackEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackEnabled = false;
        }

        private void ChkIsTitleBarAutoPaddingEnabled_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsTitleBarAutoPaddingEnabled = true;
        }

        private void ChkIsTitleBarAutoPaddingEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsTitleBarAutoPaddingEnabled = false;
        }

        private void ChkAlwaysShowHeader_Checked(object sender, RoutedEventArgs e)
        {
            NavView.AlwaysShowHeader = true;
        }

        private void ChkAlwaysShowHeader_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.AlwaysShowHeader = false;
        }

        private void BtnChangePaneTitle_Click(object sender, RoutedEventArgs e)
        {
            if (NavView.PaneTitle == "")
                NavView.PaneTitle = "Custom PaneTitle";
            else
                NavView.PaneTitle = "";
        }

        private void BtnChangeHeader_Click(object sender, RoutedEventArgs args)
        {
            if (NavView.Header == null)
                NavView.Header = new TextBlock() { Text = "Custom Header" };
            else
                NavView.Header = null;
        }

        private void BtnChangePaneDisplayMode_Click(object sender, RoutedEventArgs e)
        {
            NavView.PaneDisplayMode = NavView.PaneDisplayMode == NavigationViewPaneDisplayMode.Top ? NavigationViewPaneDisplayMode.Auto : NavigationViewPaneDisplayMode.Top;
        }

        private void BtnGetCoreApplicationViewTitleBarHeight_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblCoreApplicationViewTitleBarHeight.Text = CoreApplication.GetCurrentView().TitleBar.Height.ToString();
        }

        private void BtnGetCoreApplicationViewTitleBarIsVisible_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblCoreApplicationViewTitleBarIsVisible.Text = CoreApplication.GetCurrentView().TitleBar.IsVisible.ToString();
        }

        private void BtnGetCoreApplicationViewTitleBarSystemOverlayLeftInset_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblCoreApplicationViewTitleBarSystemOverlayLeftInset.Text = CoreApplication.GetCurrentView().TitleBar.SystemOverlayLeftInset.ToString();
        }

        private void BtnGetCoreApplicationViewTitleBarSystemOverlayRightInset_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblCoreApplicationViewTitleBarSystemOverlayRightInset.Text = CoreApplication.GetCurrentView().TitleBar.SystemOverlayRightInset.ToString();
        }

        private void BtnGetAplicationViewIsFullScreenMode_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblAplicationViewIsFullScreenMode.Text = Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().IsFullScreenMode.ToString();
        }

        private void BtnGetUIViewSettingsUserInteractionMode_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            tblUIViewSettingsUserInteractionMode.Text = Windows.UI.ViewManagement.UIViewSettings.GetForCurrentView().UserInteractionMode.ToString();
        }
    }
}
