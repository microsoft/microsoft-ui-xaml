// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;

namespace TableViewSampleApp;

public partial class App : Application
{
    private Window? _window;
    private bool _tabularMetadataRegistered;

    public App()
    {
        InitializeComponent();

        // SPLIT-BINARY: inject the Tabular DLL's XAML metadata provider into the app provider's
        // OtherProviders so the runtime XamlReader.Load in MergeTabularControlsResources() can
        // resolve adv:TabularControlsResources (a split-only type in
        // Microsoft.UI.Xaml.Controls.Tabular.dll, absent from the build-time projection). Runs
        // AFTER InitializeComponent: registering it earlier corrupts Application.Resources
        // (get_Resources then throws 0x8000FFFF).
        RegisterTabularMetadataProvider();

        // The TabularControlsResources merge is deliberately deferred to OnLaunched. Accessing
        // Application.Resources from the App constructor throws E_UNEXPECTED (0x8000FFFF) in this
        // self-contained split config; by OnLaunched the projection is live and the merge succeeds.

        UnhandledException += OnUnhandledException;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern void OutputDebugStringW(string lpOutputString);

    private static void LogSplit(string message)
    {
        var line = "[TabularSplit] " + message;
        try { OutputDebugStringW(line); } catch { /* best effort */ }
        try
        {
            File.AppendAllLines(
                Path.Combine(AppContext.BaseDirectory, "tabular-split-diag.txt"),
                new[] { $"{DateTimeOffset.UtcNow:O} {line}" });
        }
        catch { /* best effort */ }
    }

    /// <summary>
    /// Appends the Tabular DLL XAML metadata provider to the generated app provider's
    /// OtherProviders list so the runtime XamlReader.Load that merges TabularControlsResources
    /// can resolve types that live only in the split Microsoft.UI.Xaml.Controls.Tabular.dll.
    /// </summary>
    private void RegisterTabularMetadataProvider()
    {
        try
        {
            var provider = TabularMetadataProviderLoader.Create();

            const System.Reflection.BindingFlags flags =
                System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic;

            var appType = typeof(App);
            object? appProvider =
                appType.GetProperty("_AppProvider", flags)?.GetValue(this)
                ?? appType.GetField("__appProvider", flags)?.GetValue(this);
            if (appProvider is null)
            {
                LogSplit("Could not locate the generated app metadata provider (_AppProvider).");
                return;
            }

            var metaType = appProvider.GetType();
            object? typeInfoProvider =
                metaType.GetProperty("Provider", flags)?.GetValue(appProvider)
                ?? metaType.GetField("_provider", flags)?.GetValue(appProvider);
            if (typeInfoProvider is null)
            {
                LogSplit("Could not locate the inner XamlTypeInfoProvider (Provider/_provider).");
                return;
            }

            var tipType = typeInfoProvider.GetType();
            var list =
                (tipType.GetProperty("OtherProviders", flags)?.GetValue(typeInfoProvider)
                 ?? tipType.GetField("_otherProviders", flags)?.GetValue(typeInfoProvider))
                as System.Collections.IList;
            if (list is null)
            {
                LogSplit("Could not locate the OtherProviders list on the XamlTypeInfoProvider.");
                return;
            }

            if (provider is null)
            {
                LogSplit("Tabular metadata provider loader returned null; skipping registration.");
                return;
            }

            list.Add(provider);
            _tabularMetadataRegistered = true;
            LogSplit($"Tabular metadata provider registered; OtherProviders count = {list.Count}.");
        }
        catch (Exception ex)
        {
            LogSplit("RegisterTabularMetadataProvider failed: " + ex);
        }
    }

    private void MergeTabularControlsResources()
    {
        // Wrap TabularControlsResources inside an outer ResourceDictionary parsed by the native
        // XAML reader. XamlReader resolves "adv:TabularControlsResources" through the Application's
        // IXamlMetadataProvider chain, into which the Tabular DLL's provider was injected above.
        const string xaml =
            "<ResourceDictionary " +
            "xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" " +
            "xmlns:adv=\"using:Microsoft.UI.Xaml.Controls.Tabular\">" +
            "<ResourceDictionary.MergedDictionaries>" +
            "<adv:TabularControlsResources/>" +
            "</ResourceDictionary.MergedDictionaries>" +
            "</ResourceDictionary>";

        Microsoft.UI.Xaml.ResourceDictionary? rd;
        try
        {
            var loaded = Microsoft.UI.Xaml.Markup.XamlReader.Load(xaml);
            rd = loaded as Microsoft.UI.Xaml.ResourceDictionary;
            if (rd is null)
            {
                LogSplit("Merge: XamlReader returned " + (loaded?.GetType().FullName ?? "null") + " (not ResourceDictionary).");
                return;
            }
            LogSplit("Merge: XamlReader.Load OK; TabularControlsResources constructed natively.");
        }
        catch (Exception ex)
        {
            LogSplit("Merge: XamlReader.Load THREW: " + ex.GetType().FullName + " :: " + ex.Message);
            return;
        }

        try
        {
            Resources.MergedDictionaries.Add(rd);
            LogSplit($"Merge: TabularControlsResources merged; count = {Resources.MergedDictionaries.Count}.");
        }
        catch (Exception ex)
        {
            LogSplit("Merge: Resources.Add THREW: " + ex.GetType().FullName + " (0x" + ex.HResult.ToString("X8") + ") :: " + ex.Message);
        }
    }

    private void MergeTabularControlStyles()
    {
        // SPLIT-BINARY: TableView's default Style + ControlTemplate live in the Tabular control's
        // generic.xaml (app-compiled here at Microsoft.UI.Xaml\Microsoft.UI.Xaml.Controls.Tabular\
        // Themes\generic.xaml -> ms-appx:///Microsoft.UI.Xaml/Microsoft.UI.Xaml.Controls.Tabular/
        // Themes/generic.xbf, the exact DefaultStyleResourceUri the control computes). Merge it into
        // App.Resources so TableView / TableViewRow get their templates (an implicit App-level Style
        // outranks the default style). Runs after the metadata provider registration so the slice's
        // controls: type names resolve.
        try
        {
            var styles = new Microsoft.UI.Xaml.ResourceDictionary
            {
                Source = new System.Uri("ms-appx:///Microsoft.UI.Xaml/Microsoft.UI.Xaml.Controls.Tabular/Themes/generic.xaml")
            };
            Resources.MergedDictionaries.Add(styles);
            LogSplit($"Merge: Tabular control styles merged; count = {Resources.MergedDictionaries.Count}.");
        }
        catch (Exception ex)
        {
            LogSplit("Merge: Tabular control styles THREW: " + ex.GetType().FullName +
                     " (0x" + ex.HResult.ToString("X8") + ") :: " + ex.Message);
        }
    }

    private void OnUnhandledException(object sender, Microsoft.UI.Xaml.UnhandledExceptionEventArgs e)
    {
        LogSplit($"UnhandledException {e.Exception.GetType().FullName}: {e.Exception.Message}");
        LogSplit(e.Exception.ToString());
    }

    protected override void OnLaunched(LaunchActivatedEventArgs args)
    {
        // SPLIT-BINARY: now that app bootstrap is finished, Application.Resources is reachable.
        // Merge TabularControlsResources (brushes) then the Tabular style slice (Style/Template)
        // before the window inflates a TableView; without them TableView inflates with no template
        // and its native MeasureOverride throws on first layout.
        if (_tabularMetadataRegistered)
        {
            MergeTabularControlsResources();
            MergeTabularControlStyles();
        }
        else
        {
            LogSplit("Skipping explicit Tabular resource merges; provider registration was unavailable.");
        }

        _window = new MainWindow();
        _window.Activate();
    }
}
