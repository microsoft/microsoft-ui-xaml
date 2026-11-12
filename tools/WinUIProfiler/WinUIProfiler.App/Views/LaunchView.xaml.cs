// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Launch;

namespace WinUIProfiler.App.Views;

// The Launch tab: the launch timeline of the session file the other tabs show, with the milestones of the marker
// configuration. It reads the file, not the live pipeline, so it follows a session being recorded and shows a saved one
// the same way.
public sealed partial class LaunchView : UserControl
{
    // The waterfall's width in the item template, and its narrowest bar, so a milestone right after the one before shows.
    private const double TrackWidth = 300;
    private const double MinimumBarWidth = 2;

    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer refreshTimer;
    private SessionSource? source;
    private string? path;
    private MarkerConfig config = MarkerConfig.Default;
    private LaunchTimelineReader? reader;
    // Counts the files shown, so a read of an earlier one, or of this one with an earlier configuration, is dropped.
    private int shown;
    private bool reading;
    private bool readAgain;
    // The read under way, cancelled when another file, or this one with another configuration, is shown.
    private CancellationTokenSource? timelineRead;

    public LaunchView()
    {
        InitializeComponent();
        refreshTimer = DispatcherQueue.CreateTimer();
        refreshTimer.Interval = TimeSpan.FromSeconds(1);
        refreshTimer.Tick += (_, _) => _ = RefreshAsync();
        Reload();
    }

    public ObservableCollection<LaunchMilestoneRow> Milestones { get; } = [];

    // The file to show, re-read every second while the connection writes it.
    public SessionSource? Source
    {
        get => source;
        set
        {
            if (source is not null)
            {
                source.Changed -= Source_Changed;
            }

            source = value;
            if (source is not null)
            {
                source.Changed += Source_Changed;
                Source_Changed(source, EventArgs.Empty);
            }
        }
    }

    // Reads the marker configuration again: the user's file when it exists, otherwise the default, which is also used when
    // the user's file is not valid, with the error shown, so a mistake in it never stops a connection. Returns it, so a new
    // connection records its providers; the timeline shown stays until the connection's file is shown, with it.
    public MarkerConfig LoadConfig()
    {
        try
        {
            config = MarkerConfig.Load();
            ConfigError.IsOpen = false;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            config = MarkerConfig.Default;
            ShowConfigError("MarkersInvalidTitle", error);
        }

        SetLine(ConfigSummary, config.FilePath is { } file ? Format("MarkersFileFormat", file) : Text("MarkersDefault"));
        return config;
    }

    // Reads the marker configuration again and shows the file's timeline with it.
    private void Reload()
    {
        LoadConfig();
        Show(path);
    }

    private void Source_Changed(object? sender, EventArgs args)
    {
        if (source!.Path != path)
        {
            Show(source.Path);
        }

        if (source.Live)
        {
            if (!refreshTimer.IsRunning)
            {
                refreshTimer.Start();
            }
        }
        else if (refreshTimer.IsRunning)
        {
            // One last read for the events saved when the connection ended.
            refreshTimer.Stop();
            _ = RefreshAsync();
        }
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private void Show(string? sessionPath)
    {
        path = sessionPath;
        shown++;
        timelineRead?.Cancel();
        reader = path is null ? null : new LaunchTimelineReader(path, config);
        Milestones.Clear();
        ReadError.IsOpen = false;
        NotesBar.IsOpen = false;
        NotReachedText.Text = "";
        SetLine(LaunchSummary, path is null ? Text("LaunchNoFile") : Path.GetFileName(path));
        _ = RefreshAsync();
    }

    private async Task RefreshAsync()
    {
        if (reading)
        {
            readAgain = true;
            return;
        }

        if (reader is not { } current)
        {
            return;
        }

        reading = true;
        var generation = shown;
        using var cancellation = new CancellationTokenSource();
        timelineRead = cancellation;
        try
        {
            var timeline = await Task.Run(() => current.Read(cancellation.Token), cancellation.Token);
            if (generation == shown)
            {
                ShowTimeline(timeline);
                ReadError.IsOpen = false;
            }
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            if (generation == shown)
            {
                ReadError.Message = error.Message;
                ReadError.IsOpen = true;
            }
        }
        finally
        {
            timelineRead = null;
            reading = false;
            if (readAgain)
            {
                readAgain = false;
                _ = RefreshAsync();
            }
        }
    }

    private void ShowTimeline(LaunchTimeline timeline)
    {
        var session = timeline.Session;
        var process = ProcessText(session.ProcessName, session.ProcessId);
        if (session.LauncherProcessId is { } launcher)
        {
            process = Format("LaunchProcessLauncherFormat", process, ProcessText(session.LauncherName, launcher));
        }

        SetLine(LaunchSummary, timeline.Origin switch
        {
            LaunchOrigin.RecordingStart => Format("LaunchFromRecordingStartFormat", process),
            LaunchOrigin.LauncherMilestone => Format("LaunchFromLauncherFormat", process, timeline.OriginLabel),
            _ when timeline.RecordingStartMilliseconds is > 0 and var late => Format("LaunchFromCreationLateFormat", process, late),
            _ => Format("LaunchFromCreationFormat", process)
        });

        // Each bar runs from the milestone before, or the origin for the first, to its milestone, scaled to the last.
        var end = Math.Max(1, timeline.Milestones.Select(milestone => milestone.Milliseconds).DefaultIfEmpty().Max());
        var previous = 0.0;
        var index = 0;
        foreach (var milestone in timeline.Milestones)
        {
            var row = Row(milestone, previous, end);
            previous = milestone.Milliseconds;
            if (index == Milestones.Count)
            {
                Milestones.Add(row);
            }
            else if (Milestones[index] != row)
            {
                Milestones[index] = row;
            }

            index++;
        }

        while (Milestones.Count > index)
        {
            Milestones.RemoveAt(Milestones.Count - 1);
        }

        NotReachedText.Text = timeline.NotReached.Count == 0 ? ""
            : Format("MilestonesNotReachedFormat", string.Join(", ", timeline.NotReached.Select(NotReachedName)));
        var notes = string.Join(" ", Enum.GetValues<LaunchNotes>()
            .Where(note => note != LaunchNotes.None && timeline.Notes.HasFlag(note))
            .Select(note => Text($"LaunchNote{note}")));
        NotesBar.Message = notes;
        NotesBar.IsOpen = notes.Length > 0;
    }

    private LaunchMilestoneRow Row(LaunchMilestone milestone, double previous, double end)
    {
        var start = Math.Clamp(previous, 0, end) / end * TrackWidth;
        var width = Math.Max(MinimumBarWidth, Math.Clamp(milestone.Milliseconds, 0, end) / end * TrackWidth - start);
        start = Math.Max(0, Math.Min(start, TrackWidth - width));
        var definition = milestone.Definition;
        var time = milestone.Milliseconds.ToString("N1", CultureInfo.CurrentCulture);
        var since = milestone.SincePreviousMilliseconds is { } value ? Format("MilestoneSincePreviousFormat", value) : "";
        var eventText = EventText(definition);
        var thread = milestone.ThreadId?.ToString(CultureInfo.CurrentCulture) ?? "";
        var count = milestone.Count.ToString("N0", CultureInfo.CurrentCulture);
        var accessibleName = string.Join(". ", new[]
        {
            Format("MilestoneAccessibleFormat", milestone.Label, milestone.Milliseconds),
            milestone.SincePreviousMilliseconds is { } after ? Format("MilestoneAccessibleSinceFormat", after) : "",
            eventText,
            milestone.ThreadId is { } id ? Format("MilestoneAccessibleThreadFormat", id) : "",
            milestone.Count > 1 ? Format("MilestoneAccessibleCountFormat", milestone.Count) : ""
        }.Where(part => part.Length > 0));
        return new(milestone.Label, time, since, eventText, thread, count, new Thickness(start, 0, 0, 0), width, accessibleName);
    }

    private string NotReachedName(MilestoneDefinition definition)
    {
        var name = definition.Label ?? Format("MilestoneLabelFromFormat", definition.LabelFrom);
        return definition.EventDescription is null ? name : Format("MilestoneNotReachedItemFormat", name, EventText(definition));
    }

    // The provider and event that mark the milestone, and whether they are the launcher's.
    private string EventText(MilestoneDefinition definition)
    {
        var text = definition.EventDescription ?? Text(definition.BuiltIn == BuiltInMilestone.ProcessCreated
            ? "MilestoneBuiltInProcessCreated" : "MilestoneBuiltInFirstFrame");
        return definition.From == MilestoneSource.Launcher ? Format("MilestoneLauncherEventFormat", text) : text;
    }

    private string ProcessText(string? name, int processId) =>
        name is null ? Format("MilestonePidFormat", processId) : Format("MilestoneProcessFormat", name, processId);

    // A one-line text that may be cut off; its tooltip shows all of it.
    private static void SetLine(TextBlock line, string text)
    {
        line.Text = text;
        ToolTipService.SetToolTip(line, text.Length == 0 ? null : text);
    }

    private void ShowConfigError(string titleKey, Exception error)
    {
        ConfigError.Title = Text(titleKey);
        ConfigError.Message = error.Message;
        ConfigError.IsOpen = true;
    }

    private void Reload_Click(object sender, RoutedEventArgs args) => Reload();

    // Opens the user's configuration, created from the default the first time, in Notepad, or when Notepad can't start,
    // in the app that opens JSON files. Reload reads it again once it is saved.
    private void Edit_Click(object sender, RoutedEventArgs args)
    {
        string file;
        try
        {
            file = MarkerConfig.CreateUserFile();
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowConfigError("MarkersEditFailedTitle", error);
            return;
        }

        Reload();
        try
        {
            try
            {
                // System32's, not one found first on the search path.
                Process.Start(new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "notepad.exe")) { ArgumentList = { file } })
                    ?.Dispose();
            }
            catch (Win32Exception)
            {
                Process.Start(new ProcessStartInfo(file) { UseShellExecute = true })?.Dispose();
            }
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowConfigError("MarkersEditFailedTitle", error);
        }
    }
}
