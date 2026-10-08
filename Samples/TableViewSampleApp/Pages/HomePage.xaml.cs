// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp;
using Windows.ApplicationModel.DataTransfer;

namespace TableViewSampleApp.Pages;

public sealed partial class HomePage : Page
{
    private static readonly Uri ReportIssueUri = new("https://github.com/microsoft/microsoft-ui-xaml/issues/new?template=bug_report.yaml");
    private MainWindow? _mainWindow;

    public HomePage()
    {
        InitializeComponent();
    }

    protected override void OnNavigatedTo(NavigationEventArgs e)
    {
        base.OnNavigatedTo(e);
        _mainWindow = e.Parameter as MainWindow;
    }

    private void OnNavigateButtonClick(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: string tag })
        {
            _mainWindow?.NavigateTo(tag);
        }
    }

    private async void OnReportIssueClick(object sender, RoutedEventArgs e)
    {
        var launched = false;
        try
        {
            launched = await Windows.System.Launcher.LaunchUriAsync(ReportIssueUri);
        }
        catch (Exception)
        {
            // No browser registered, or the launch was blocked: fall through to the copy below.
        }

        if (launched)
        {
            ReportIssueFallbackText.Visibility = Visibility.Collapsed;
            return;
        }

        var package = new DataPackage();
        package.SetText(ReportIssueUri.AbsoluteUri);
        Clipboard.SetContent(package);
        ReportIssueFallbackText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "Could not open a browser. The link was copied to the clipboard: {0}",
            ReportIssueUri.AbsoluteUri);
        ReportIssueFallbackText.Visibility = Visibility.Visible;
    }
}
