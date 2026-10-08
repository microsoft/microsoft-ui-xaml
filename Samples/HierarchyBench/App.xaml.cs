// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Linq;
using Microsoft.UI.Xaml;

namespace HierarchyBench;

public partial class App : Application
{
    private Window? _window;
    private BenchmarkOptions? _options;

    public App()
    {
        InitializeComponent();
        UnhandledException += (_, e) =>
        {
            try { File.AppendAllText(_options?.LogFile ?? Path.Combine(AppContext.BaseDirectory, "bench.log"), $"[{DateTime.Now:HH:mm:ss.fff}] UnhandledException {e.Exception}{Environment.NewLine}"); } catch { }
        };
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        _options = BenchmarkOptions.Parse(Environment.GetCommandLineArgs().Skip(1).ToArray(), AppContext.BaseDirectory);
        _window = new Window
        {
            Title = "HierarchyBench (" + BenchmarkHost.Api + ")",
            Content = new BenchmarkHost(_options),
        };
        _window.AppWindow.Resize(new Windows.Graphics.SizeInt32(1400, 900));
        _window.Activate();
    }
}
