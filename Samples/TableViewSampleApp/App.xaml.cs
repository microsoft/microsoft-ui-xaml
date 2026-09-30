// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;

namespace TableViewSampleApp;

public partial class App : Application
{
    private Window? _window;

    /// <summary>The main window's content root, used by the Settings page to apply the theme.</summary>
    internal UIElement? MainWindowContent => _window?.Content;

    public App()
    {
        InitializeComponent();
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _window = new MainWindow();
        _window.Activate();
    }
}
