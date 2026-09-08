// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using Microsoft.UI.Xaml;

namespace TableViewSampleApp;

public partial class App : Application
{
    private const string VerifyGroupsArgument = "--verify-groups";
    private const string VerifySelectionArgument = "--verify-selection";

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

    // ----- Opt-in verification logging -----
    //
    // The gallery doubles as a manual verification harness: launched with --verify-groups or
    // --verify-selection it appends what it observed to a file next to the executable. Compiled
    // out unless TABLEVIEW_SAMPLE_VERIFY is defined, so a normal build carries none of it.

    internal static bool IsGroupsVerificationEnabled() =>
        Environment.GetCommandLineArgs().Any(static a => string.Equals(a, VerifyGroupsArgument, StringComparison.OrdinalIgnoreCase));

    internal static bool IsSelectionVerificationEnabled() =>
        Environment.GetCommandLineArgs().Any(static a => string.Equals(a, VerifySelectionArgument, StringComparison.OrdinalIgnoreCase));

    internal static string GetVerificationLaunchLogPath() =>
        Path.Combine(AppContext.BaseDirectory, "verification-groups-launch.txt");

    internal static string GetSelectionVerificationLogPath() =>
        Path.Combine(AppContext.BaseDirectory, "verification-selection-gutter.txt");

    [Conditional("TABLEVIEW_SAMPLE_VERIFY")]
    internal static void AppendVerificationLog(string message)
    {
        if (!IsGroupsVerificationEnabled())
        {
            return;
        }

        File.AppendAllLines(GetVerificationLaunchLogPath(), new[] { message });
    }

    [Conditional("TABLEVIEW_SAMPLE_VERIFY")]
    internal static void AppendSelectionVerificationLog(string message)
    {
        if (!IsSelectionVerificationEnabled())
        {
            return;
        }

        File.AppendAllLines(GetSelectionVerificationLogPath(), new[] { message });
    }

    internal static void AppendNavigationErrorLog(string message)
    {
        try
        {
            File.AppendAllLines(
                Path.Combine(AppContext.BaseDirectory, "sample-navigation-errors.txt"),
                new[] { $"{DateTimeOffset.Now:O} {message}" });
        }
        catch
        {
            // Best effort: a diagnostic that cannot be written must not take the app down.
        }
    }
}
