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
    // Shared Loop page for logging TableView samples issues (the preferred triage channel).
    private const string LoopReportUri = "https://loop.cloud.microsoft/p/eyJ1IjoiaHR0cHM6Ly9taWNyb3NvZnQuc2hhcmVwb2ludC1kZi5jb20vdGVhbXMvU2hlbGxGVEw%2FbmF2PWN6MGxNa1owWldGdGN5VXlSbE5vWld4c1JsUk1KbVE5WWlFdFFWUTBiSEZaV0U5clpUZGZTakJ5T0ZGclJrSklabkpCVXpkclNWOTRUbk13WVRob1JXcDVNMkY1U0dvMlVHWkxURmgxVWpkTllURlZhVVJDZUcxMUptWTlNREV5UzFsVlYweEhWRW8wVUVORFRrRmFVbFpJV1ZvMFJWbEdWamMyVFUxV1RTWmpQU1V5UmlabWJIVnBaRDB4Sm1FOVZHVmhiWE1tY0QwbE5EQm1iSFZwWkhnbE1rWnNiMjl3TFhCaFoyVXRZMjl1ZEdGcGJtVnkifQ%3D%3D";
    private MainWindow? _mainWindow;

    public HomePage()
    {
        InitializeComponent();
        ReportLoopButton.Click += async (_, __) =>
        {
            try { await Windows.System.Launcher.LaunchUriAsync(new Uri(LoopReportUri)); } catch { }
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
