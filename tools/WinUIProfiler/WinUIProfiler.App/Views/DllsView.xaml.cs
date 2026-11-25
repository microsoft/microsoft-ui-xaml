// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

public sealed record DllLoadRow(string LoadTime, string FaultCount, string DllPath);

public sealed partial class DllsView : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer timer;
    private SessionSource? source;
    private string? path;
    private IReadOnlyList<SessionDllLoad>? displayed;
    private int revision;
    private bool loaded;
    private bool reading;
    private bool readAgain;

    public DllsView()
    {
        InitializeComponent();
        timer = DispatcherQueue.CreateTimer();
        timer.Interval = TimeSpan.FromSeconds(1);
        timer.Tick += (_, _) => _ = RefreshAsync();
        Loaded += (_, _) =>
        {
            loaded = true;
            Update();
        };
        Unloaded += (_, _) =>
        {
            loaded = false;
            timer.Stop();
        };
    }

    internal void SetSource(SessionSource value)
    {
        if (source is not null) source.Changed -= SourceChanged;
        source = value;
        source.Changed += SourceChanged;
        Update();
    }

    private void SourceChanged(object? sender, EventArgs args) => Update();

    private void Update()
    {
        if (path != source?.Path)
        {
            path = source?.Path;
            revision++;
            displayed = null;
            Status.Text = "";
            DllList.ItemsSource = null;
            DllList.Visibility = Visibility.Collapsed;
            ReadError.IsOpen = false;
        }

        if (!loaded) return;
        if (source?.Live is true)
        {
            if (!timer.IsRunning) timer.Start();
        }
        else
        {
            timer.Stop();
        }

        if (path is null) Status.Text = strings.GetString("DllsNoSession");
        else _ = RefreshAsync();
    }

    private void SearchBox_TextChanged(object sender, TextChangedEventArgs args)
    {
        if (displayed is not null) ShowRows(displayed);
    }

    private void ShowRows(IReadOnlyList<SessionDllLoad> rows)
    {
        var search = SearchBox.Text.Trim();
        var matches = rows.Where(row => row.DllPath.Contains(search, StringComparison.OrdinalIgnoreCase))
            .Select(row => new DllLoadRow(
                row.LoadTimeMilliseconds?.ToString("N3", CultureInfo.CurrentCulture) ?? "-",
                row.FaultCount?.ToString("N0", CultureInfo.CurrentCulture) ?? "-", row.DllPath)).ToArray();
        DllList.ItemsSource = matches;
        DllList.Visibility = matches.Length > 0 ? Visibility.Visible : Visibility.Collapsed;
        Status.Text = rows.Count == 0 ? strings.GetString("DllsNoLoads") :
            matches.Length == 0 ? strings.GetString("DllsNoMatches") : "";
    }

    private async Task RefreshAsync()
    {
        if (!loaded || path is not { } file) return;
        if (reading)
        {
            readAgain = true;
            return;
        }

        reading = true;
        var current = revision;
        try
        {
            var rows = await Task.Run(() => SessionFile.ReadDllLoads(file));
            if (current != revision) return;
            ReadError.IsOpen = false;
            if (rows is null)
            {
                DllList.ItemsSource = null;
                DllList.Visibility = Visibility.Collapsed;
                Status.Text = strings.GetString("DllsUnavailable");
            }
            else if (displayed is null || !rows.SequenceEqual(displayed))
            {
                ShowRows(rows);
            }

            displayed = rows;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SqliteException)
        {
            if (current != revision) return;
            ReadError.Message = error.Message;
            ReadError.IsOpen = true;
        }
        finally
        {
            reading = false;
            if (readAgain)
            {
                readAgain = false;
                _ = RefreshAsync();
            }
        }
    }
}
