// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using TableViewSampleApp;

namespace TableViewSampleApp.Pages;

public sealed partial class HomePage : Page
{
    private static readonly Uri ReportIssueUri = new("https://github.com/microsoft/microsoft-ui-xaml/issues/new?template=bug_report.yaml");
    private MainWindow? _mainWindow;

    public HomePage()
    {
        InitializeComponent();
        ReportIssueButton.Click += async (_, __) =>
        {
            try { await Windows.System.Launcher.LaunchUriAsync(ReportIssueUri); } catch { }
        };
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
}
