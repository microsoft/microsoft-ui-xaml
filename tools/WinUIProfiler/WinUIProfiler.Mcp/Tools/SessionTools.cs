// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using ModelContextProtocol;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;

namespace WinUIProfiler.Mcp.Tools;

/// <summary>Finds session files, summarizes one, and reads a recorded trace into a new one.</summary>
[McpServerToolType]
internal static class SessionTools
{
    // The most processes a client is asked to choose from; past that, the tool lists them instead.
    private const int MaxChoices = 25;

    // The most processes a result lists.
    private const int MaxListed = 100;

    // The Win32 error a damaged or foreign file gives when it's opened as a trace (ERROR_FILE_CORRUPT).
    private const int FileCorrupt = unchecked((int)0x80070570);

    internal static readonly string[] Views = ["list", "summary"];

    // A session file read from a trace is stamped with the times it was read; the trace's recording time isn't stored.
    internal const string TraceTimes = "startedUtc and endedUtc are when the trace was read into this file, not when the " +
        "trace was recorded, which the file doesn't say; the time between them is how long the reading took. The events' " +
        "times are the trace's own, in milliseconds from the origin session.timeBase gives.";

    internal const string ListTraceTimes = "For a session read from a trace (coverage Trace), startedUtc is when the trace " +
        "was read into the file, not when the trace was recorded, which the file doesn't say.";

    [McpServerTool(Name = "sessions", ReadOnly = true, Idempotent = true, OpenWorld = false), Description(
        "Finds and describes WinUI Profiler session files (.sqlite). Start here to choose the session the other tools read; " +
        "they read the newest one when sessionPath is omitted. Views: \"list\" lists the session files in a folder, newest " +
        "first: the process each one recorded, whether it was recorded live or read from a trace, whether the recording has " +
        "ended, and how many events, ticks, UI elements and visuals it holds. \"summary\" describes one session: when and how " +
        "it was recorded, its UI threads' ticks, its regions of interest and interesting rows, when its first frame drew, " +
        "which analyses have data and which tool and view give each, the time base every tool's times use, and what the " +
        "session is missing. Without view, the tool lists, or summarizes sessionPath when it's given.")]
    public static async Task<object> Sessions(
        [Description("\"list\" or \"summary\". Omit it to list, or to summarize sessionPath when it's given.")]
        string? view = null,
        [Description(SessionCatalog.SessionPathDescription + " Read by the summary view.")] string? sessionPath = null,
        [Description("The folder to list, or to find sessionPath in. Omit it for the WinUI Profiler app's sessions folder.")]
        string? folder = null,
        [Description("The list view: the most session files to describe, from 1 to 500; the newest are described first. " +
            "Omit it for 20.")]
        int? limit = null,
        CancellationToken cancellationToken = default)
    {
        var chosen = ToolArguments.Text(view) is null && ToolArguments.Text(sessionPath) is not null
            ? "summary"
            : ToolArguments.View(view, Views, "sessions");
        ToolArguments.OnlyFor(ToolArguments.Text(sessionPath) is not null, nameof(sessionPath), "sessions", chosen, "summary");
        ToolArguments.OnlyFor(limit is not null, nameof(limit), "sessions", chosen, "list");
        var count = ToolArguments.Range(limit ?? 20, 1, 500, nameof(limit));
        var directory = SessionCatalog.Folder(folder);
        if (File.Exists(directory))
        {
            throw new McpException($"{directory} is a file, not a folder. Pass it as sessionPath to the summary view.");
        }

        return await Task.Run(() =>
        {
            using var trail = QueryTrail.Start("sessions");
            if (chosen == "list")
            {
                return trail.Finish(List(directory, count, cancellationToken));
            }

            var path = SessionCatalog.Resolve(sessionPath, directory);
            return trail.Finish(SessionCatalog.Read(path, () => Summary(path)));
        }, cancellationToken).ConfigureAwait(false);
    }

    private static ToolAnswer List(string directory, int limit, CancellationToken cancellationToken)
    {
        var files = SessionCatalog.Files(directory);
        var sessions = new List<object>();
        var traces = false;
        var unread = 0;
        foreach (var file in files.Take(limit))
        {
            cancellationToken.ThrowIfCancellationRequested();
            var (row, trace, read) = DescribeFile(file);
            sessions.Add(row);
            traces |= trace;
            unread += read ? 0 : 1;
        }

        var notes = new List<string>();
        if (files.Count == 0)
        {
            notes.Add($"There are no session files in {directory}. Record one with the WinUI Profiler app, or read a trace " +
                "(.etl) into one with record_trace.");
        }
        else if (files.Count > sessions.Count)
        {
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"Only the newest {sessions.Count} of {files.Count} session files are described; raise limit for more."));
        }

        if (traces)
        {
            notes.Add(ListTraceTimes);
        }

        var computed = files.Count == 0
            ? "Found no session files in the folder, so read none."
            : $"Listed the {EventJson.Count(files.Count, "session file", "session files")} in the folder, newest first, and " +
                "read the session record of " + (sessions.Count < files.Count
                    ? string.Create(CultureInfo.InvariantCulture, $"the newest {sessions.Count}")
                    : files.Count == 1 ? "it" : "each") +
                (unread == 0 ? "." : $"; {EventJson.Count(unread, "file", "files")} couldn't be read, so " +
                    $"{(unread == 1 ? "its row has" : "their rows have")} an error instead.");
        return new(new
        {
            view = "list",
            folder = directory,
            total = files.Count,
            returned = sessions.Count,
            sessions,
            notes,
        }, computed, [.. files.Take(sessions.Count).Select(file => file.Path)]);
    }

    private static ToolAnswer Summary(string path)
    {
        var info = SessionFile.ReadInfo(path);
        var (ticks, rows, launch, error) = SessionCatalog.ReadViews(path);
        var regions = rows.Count(row => row.Type == InterestingRowType.Region);
        var contents = new SessionContents(ticks.Count, regions, rows.Count - regions, launch.Count,
            LaunchTools.ObservedMilestones(info, launch, ticks, rows), error);
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (SessionCatalog.IsTrace(info))
        {
            notes.Add(TraceTimes);
        }

        if (error is not null)
        {
            notes.Add(error);
        }

        var first = launch.Count == 0 ? null : launch[0];
        const string Record = "Took the counts of stored events, ticks, UI elements and visuals from the session record. ";
        var computed = error is null
            ? Record + $"Grouped the {EventJson.Count(ticks.Count, "tick", "ticks")} read by UI thread, counting those that drew " +
                "and the slow ones, with the first start, the first drawn end and the last end; counted the " +
                $"{EventJson.Count(regions, "region", "regions")} and, by type, the other " +
                $"{EventJson.Count(rows.Count - regions, "interesting row", "interesting rows")}; took the first of the " +
                $"{EventJson.Count(launch.Count, "launch frame", "launch frames")}; and from those, said which analyses have data."
            : Record + "The frame views couldn't all be read (see the notes), so the summary gives no UI threads, regions or " +
                "launch frames, and the analyses that need them say why.";
        return new(new
        {
            session = SessionCatalog.Header(path, info),
            view = "summary",
            startedUtc = EventJson.Utc(info.StartedUtc),
            endedUtc = EventJson.Utc(info.EndedUtc),
            processStartMs = EventJson.Ms(info.ProcessStartMilliseconds),
            storedEvents = info.StoredEvents,
            ticks = info.Frames,
            uiElements = info.UIElements,
            visuals = info.Visuals,
            limitMB = info.LimitBytes / 1024 / 1024,
            trimmedBeforeSeq = info.CompleteFromSeq,
            lostByEtw = info.LostByEtw,
            droppedByProfiler = info.DroppedByProfiler,
            uiThreads = ticks.GroupBy(tick => tick.ThreadId).Select(thread => new
            {
                threadId = thread.Key,
                ticks = thread.Count(),
                drew = thread.Count(tick => tick.Drawn == true),
                slow = thread.Count(tick => tick.Slow),
                firstStartMs = EventJson.Ms(thread.Min(tick => tick.StartMilliseconds)),
                firstDrawnEndMs = EventJson.Ms(thread.FirstOrDefault(tick => tick.Drawn == true && tick.EndMilliseconds is not null)
                    ?.EndMilliseconds),
                lastEndMs = EventJson.Ms(thread.Max(tick => tick.EndMilliseconds)),
            }).ToList(),
            regions,
            interestingRows = rows.Where(row => row.Type != InterestingRowType.Region).GroupBy(row => row.Type)
                .OrderBy(group => group.Key).ToDictionary(group => group.Key.ToString(), group => group.Count()),
            firstFrame = first is null ? null : new
            {
                frameNumber = first.FrameNumber,
                ms = EventJson.Ms(first.Milliseconds),
                measuredFrom = first.FromProcessStart ? "ProcessStart" : "RecordingStart",
            },
            analyses = SessionCatalog.Analyses(info, contents),
            notes,
        }, computed);
    }

    [McpServerTool(Name = "record_trace", ReadOnly = false, Destructive = false, Idempotent = false, OpenWorld = false), Description(
        "Reads one process's events from a recorded trace (.etl) into a new session file, as the WinUI Profiler app's " +
        "\"Recorded trace (.etl)\" mode does, so the other tools can read it: the XAML providers' events, and those of the " +
        "marker configuration's providers, the user's (%LOCALAPPDATA%\\WinUIProfiler\\markers.json) when it exists and is " +
        "valid, else the default's. A session file holds one process: when the trace has XAML events from several, pass " +
        "processId, or the tool asks (when the client supports it) or lists them. A finished session file this tool read " +
        "from the same process of the same trace, with the same marker providers, is reused while the trace is unchanged, " +
        "unless reread is true. Returns the session file's path.")]
    public static Task<object> RecordTrace(
        McpServer server,
        IProgress<ProgressNotificationValue> progress,
        [Description("The full path of the trace (.etl) to read.")] string etlPath,
        [Description("The ID of the process to read. Omit it to read the only process with XAML events, or to choose one.")]
        int? processId = null,
        [Description("The folder to write the session file to. Omit it for the WinUI Profiler app's sessions folder.")]
        string? folder = null,
        [Description("Read the trace again even when a finished session file this tool read from the same process of the " +
            "same, unchanged trace exists.")]
        bool reread = false,
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(server);
        return RecordTraceCore(etlPath, processId, folder, reread, TraceMarkers.Load(),
            (processes, message, token) => ChooseProcessAsync(server, processes, message, token), progress, cancellationToken);
    }

    /// <summary>
    /// record_trace without the server: markers are the marker providers to read, choose asks the client for a process, or
    /// returns null when it can't, and progress may be null.
    /// </summary>
    internal static async Task<object> RecordTraceCore(string etlPath, int? processId, string? folder, bool reread,
        TraceMarkers markers, Func<IReadOnlyList<TraceProcess>, string, CancellationToken, Task<int?>>? choose,
        IProgress<ProgressNotificationValue>? progress, CancellationToken cancellationToken)
    {
        // Started in this flow, so the reads of the Task.Runs below report to it.
        using var trail = QueryTrail.Start("record_trace");
        var trace = TracePath(etlPath);
        if (processId is { } requested)
        {
            ToolArguments.NotNegative(requested, nameof(processId));
        }

        var directory = SessionCatalog.Folder(folder);
        if (File.Exists(directory))
        {
            throw new McpException($"{directory} is a file, not a folder.");
        }

        if (processId is { } wanted && !reread &&
            await Task.Run(() => FindFinishedSession(directory, trace, wanted, markers), cancellationToken).ConfigureAwait(false) is { } found)
        {
            return trail.Finish(Reused(found, trace, wanted, markers));
        }

        var contents = await Task.Run(() => ListProcesses(trace, cancellationToken), cancellationToken).ConfigureAwait(false);
        var processes = contents.Processes;
        TraceProcess chosen;
        if (processId is { } id)
        {
            chosen = processes.FirstOrDefault(process => process.ProcessId == id) ?? throw new McpException(processes.Count == 0
                ? $"No process in {trace} has XAML events, so there's nothing to read."
                : string.Create(CultureInfo.InvariantCulture, $"Process {id} has no XAML events in {trace}. The processes that do: ") +
                    string.Join("; ", processes.Take(MaxChoices).Select(Label)) + ".");
        }
        else if (processes.Count == 0)
        {
            throw new McpException($"No process in {trace} has XAML events, so there's nothing to read. A trace needs the " +
                "Microsoft-Windows-XAML or Microsoft-Windows-XAML-Profiler provider enabled while the app ran.");
        }
        else if (processes.Count == 1)
        {
            chosen = processes[0];
        }
        else
        {
            var message = string.Create(CultureInfo.InvariantCulture,
                $"{trace} has XAML events from {processes.Count} processes. Which one should be read into a session file?");
            int? picked = choose is not null && processes.Count <= MaxChoices
                ? await choose(processes, message, cancellationToken).ConfigureAwait(false)
                : null;
            if (picked is not { } pick || processes.FirstOrDefault(process => process.ProcessId == pick) is not { } match)
            {
                return trail.Finish(new(new
                {
                    needsProcessId = true,
                    etlPath = trace,
                    message = message + " Call record_trace again with its processId.",
                    total = processes.Count,
                    processes = processes.Take(MaxListed).Select(DescribeProcess).ToList(),
                }, $"Listed the trace's processes with ETW: {EventJson.Count(processes.Count, "process has", "processes have")} " +
                    "XAML events, and none was chosen, so no session file was read and no SQL ran."));
            }

            chosen = match;
        }

        if (processId is null && !reread &&
            await Task.Run(() => FindFinishedSession(directory, trace, chosen.ProcessId, markers), cancellationToken).ConfigureAwait(false) is { } existing)
        {
            return trail.Finish(Reused(existing, trace, chosen.ProcessId, markers));
        }

        // Taken again after the read, so a trace that changed while it was read isn't recorded as read.
        var version = await Task.Run(() => TraceVersion.Read(trace), cancellationToken).ConfigureAwait(false);
        var replay = await Task.Run(() => StartReplay(trace, chosen, directory, markers), cancellationToken).ConfigureAwait(false);
        TraceReplayStatus status;
        string sessionPath;
        string? failure;
        long stored;
        await using (replay.ConfigureAwait(false))
        {
            var gate = new Lock();
            var reported = -1f;
            void Report(object? sender, EventArgs e)
            {
                if (progress is null)
                {
                    return;
                }

                var current = replay.Status;
                var events = current.XamlEvents + current.ProfilerEvents + current.GpuMemoryEvents;
                var value = current.Progress is { } fraction ? (float)EventJson.Round(fraction * 100, 1) : events;
                lock (gate)
                {
                    if (value <= reported)
                    {
                        return;
                    }

                    reported = value;
                }

                progress.Report(new ProgressNotificationValue
                {
                    Progress = value,
                    Total = current.Progress is null ? null : 100,
                    Message = string.Create(CultureInfo.InvariantCulture,
                        $"Read {current.ReadMilliseconds / 1000:F1} s of the trace; {EventJson.Count(events, "event", "events")} " +
                        $"of process {chosen.ProcessId}."),
                });
            }

            replay.Changed += Report;
            try
            {
                await replay.Completion.WaitAsync(cancellationToken).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                await replay.StopAsync().ConfigureAwait(false);

                // The client gave up on the call, so it never learns of the part of the trace that was read. Left, that
                // file would be the newest session, which the other tools read when no session is named.
                DeleteSession(replay.Recording.Path);
                throw;
            }
            finally
            {
                replay.Changed -= Report;
            }

            status = replay.Status;
            var recording = replay.Recording;
            sessionPath = recording.Path;
            stored = recording.StoredEvents;
            failure = replay.RecordingError ?? replay.Failure?.Message;
        }

        if (status.State == TraceReplayState.Failed && stored == 0)
        {
            DeleteSession(sessionPath);
            throw new McpException($"Reading process {chosen.ProcessId} from {trace} failed before any of its events were " +
                $"stored: {failure ?? "the reason isn't known"}");
        }

        var info = SessionCatalog.Read(sessionPath, () => SessionFile.ReadInfo(sessionPath));
        if (status.State == TraceReplayState.Finished && version is not null && version == TraceVersion.Read(trace))
        {
            version.Keep(sessionPath, info, trace, markers);
        }

        var notes = TraceNotes(info, markers);
        if (status.OutOfOrderEvents > 0)
        {
            notes.Add((status.OutOfOrderEvents == 1
                    ? "1 of the process's events has"
                    : $"{EventJson.Count(status.OutOfOrderEvents, "event", "events")} of the process's events each have") +
                " an earlier timestamp than the event before it; the session keeps the trace's order.");
        }

        var read = status.XamlEvents + status.ProfilerEvents + status.GpuMemoryEvents;
        var events = EventJson.Count(read, "event", "events");
        var name = chosen.Name ?? "the process";
        var outcome = status.State switch
        {
            TraceReplayState.Finished => string.Create(CultureInfo.InvariantCulture,
                $"Read {events} of {name} (PID {chosen.ProcessId}) from the trace into {sessionPath}."),
            TraceReplayState.Failed => string.Create(CultureInfo.InvariantCulture,
                $"Reading the trace failed after {events} of {name} (PID {chosen.ProcessId}): " +
                $"{failure ?? "the reason isn't known"}. {sessionPath} holds the events stored before the failure."),
            _ => string.Create(CultureInfo.InvariantCulture,
                $"Reading stopped after {events} of {name} (PID {chosen.ProcessId}); {sessionPath} holds " +
                $"{(read == 1 ? "it" : "them")}."),
        };

        // Without reread, a session file to reuse was looked for first, by FindFinishedSession: before the processes were
        // listed when processId was given, and after one was chosen when it wasn't.
        var computed = string.Create(CultureInfo.InvariantCulture,
            $"Listed the trace's processes that have XAML events and read process {chosen.ProcessId}'s events of the XAML " +
            $"providers and the marker configuration's {EventJson.Count(markers.Providers.Count, "provider", "providers")} " +
            $"from the trace with ETW, into a new session file (those writes aren't listed); then read the new file's session " +
            $"record.") +
            (reread ? "" : string.Create(CultureInfo.InvariantCulture,
                $" Before reading the events, looked among the folder's closed session files named for process " +
                $"{chosen.ProcessId} for one read from this unchanged trace with the same marker providers to reuse, and " +
                $"found none."));
        return trail.Finish(new(new
        {
            session = SessionCatalog.Header(sessionPath, info),
            reused = false,
            state = status.State.ToString(),
            storedEvents = info.StoredEvents,
            xamlEvents = status.XamlEvents,
            profilerEvents = status.ProfilerEvents,
            gpuMemoryEvents = status.GpuMemoryEvents,
            lostEvents = status.LostEvents,
            outOfOrderEvents = status.OutOfOrderEvents,
            message = outcome,
            otherProcesses = processes.Where(process => process.ProcessId != chosen.ProcessId).Take(MaxListed).Select(DescribeProcess).ToList(),
            notes,
        }, computed));
    }

    /// <summary>
    /// A session file record_trace read from the same process of this version of the trace, with these marker providers
    /// (see <see cref="TraceVersion"/>), read to the trace's end and closed since the trace was last written; null when
    /// there's none. The app keeps no record of the version it read, so its session files aren't reused.
    /// </summary>
    internal static string? FindFinishedSession(string directory, string trace, int processId, TraceMarkers markers)
    {
        DateTime traceWritten;
        try
        {
            traceWritten = File.GetLastWriteTimeUtc(trace);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            return null;
        }

        // Read once, and only if a session file was read from the trace.
        var version = new Lazy<TraceVersion?>(() => TraceVersion.Read(trace));

        // Session files are named <date>-<time>-<name>-<pid>[-n].sqlite.
        var marker = string.Create(CultureInfo.InvariantCulture, $"-{processId}");
        foreach (var file in SessionCatalog.Files(directory))
        {
            if (!Path.GetFileNameWithoutExtension(file.Path).Contains(marker, StringComparison.Ordinal) ||
                file.LastWrittenUtc < traceWritten || File.Exists(file.Path + "-wal"))
            {
                continue;
            }

            try
            {
                var info = SessionFile.ReadInfo(file.Path);
                if (SessionCatalog.IsTrace(info) && info.ProcessId == processId && info.EndReason == SessionCatalog.EndOfTrace &&
                    string.Equals(info.EtlPath, trace, StringComparison.OrdinalIgnoreCase) &&
                    TraceVersion.Recorded(file.Path, info, trace, markers) is { } recorded && recorded == version.Value)
                {
                    return file.Path;
                }
            }
            catch (Exception error) when (SessionCatalog.ReadError(file.Path, error) is not null)
            {
                // Not a session file this version reads.
            }
        }

        return null;
    }

    /// <summary>How the app's trace picker names a process.</summary>
    internal static string Label(TraceProcess process) => string.Create(CultureInfo.InvariantCulture,
        $"{process.Name ?? "Unnamed process"} (PID {process.ProcessId}) | {process.Events:N0} events | " +
        $"{process.FirstMilliseconds / 1000:F1} s to {process.LastMilliseconds / 1000:F1} s");

    /// <summary>How the app reports a trace it can't read.</summary>
    internal static string TraceError(string trace, Exception error) => error switch
    {
        FileNotFoundException or DirectoryNotFoundException => $"The trace {trace} was not found.",
        BadImageFormatException => $"{trace} isn't an ETW trace, or it is damaged.",
        _ when error.HResult == FileCorrupt => $"{trace} isn't an ETW trace, or it is damaged.",
        _ => $"{trace} couldn't be read: {error.Message}",
    };

    private static (object Row, bool IsTrace, bool Read) DescribeFile(SessionFileEntry file)
    {
        try
        {
            var info = SessionFile.ReadInfo(file.Path);
            return (new
            {
                file = Path.GetFileName(file.Path),
                path = file.Path,
                processId = info.ProcessId,
                processName = info.ProcessName,
                source = info.Source,
                coverage = info.Coverage.ToString(),
                startedUtc = EventJson.Utc(info.StartedUtc),
                lastWrittenUtc = EventJson.Utc(file.LastWrittenUtc),
                endReason = info.EndReason ?? SessionCatalog.NoEndReason,
                storedEvents = info.StoredEvents,
                ticks = info.Frames,
                uiElements = info.UIElements,
                visuals = info.Visuals,
                sizeMB = Megabytes(file.Bytes),
                trimmedBeforeSeq = info.CompleteFromSeq,
                lostByEtw = info.LostByEtw > 0 ? info.LostByEtw : null,
                droppedByProfiler = info.DroppedByProfiler > 0 ? info.DroppedByProfiler : null,
                etlPath = info.EtlPath,
            }, SessionCatalog.IsTrace(info), true);
        }
        catch (Exception error) when (SessionCatalog.ReadError(file.Path, error) is { } message)
        {
            return (new
            {
                file = Path.GetFileName(file.Path),
                path = file.Path,
                lastWrittenUtc = EventJson.Utc(file.LastWrittenUtc),
                sizeMB = Megabytes(file.Bytes),
                error = message,
            }, false, false);
        }
    }

    private static object DescribeProcess(TraceProcess process) => new
    {
        processId = process.ProcessId,
        name = process.Name,
        events = process.Events,
        xamlEvents = process.XamlEvents,
        profilerEvents = process.ProfilerEvents,
        gpuMemoryEvents = process.GpuMemoryEvents,
        firstMs = EventJson.Ms(process.FirstMilliseconds),
        lastMs = EventJson.Ms(process.LastMilliseconds),
        label = Label(process),
    };

    private static ToolAnswer Reused(string sessionPath, string trace, int processId, TraceMarkers markers)
    {
        var info = SessionCatalog.Read(sessionPath, () => SessionFile.ReadInfo(sessionPath));
        return new(new
        {
            session = SessionCatalog.Header(sessionPath, info),
            reused = true,
            state = TraceReplayState.Finished.ToString(),
            storedEvents = info.StoredEvents,
            message = $"{sessionPath} already holds this process's events from {trace}, read to the trace's end with the same " +
                "marker providers, and the trace hasn't changed since, so it was reused. Pass reread=true to read the trace " +
                "again into a new session file.",
            notes = TraceNotes(info, markers),
        }, string.Create(CultureInfo.InvariantCulture,
            $"Read the session records of the folder's closed session files named for process {processId} and written since " +
            $"the trace was, newest first, until one had been read from this trace to its end with the same marker providers " +
            $"and the trace hadn't changed since. Reused that file instead of reading the trace again, and read its session " +
            $"record."));
    }

    /// <summary>A trace's session's notes, and why the default marker providers were read, when the user's weren't.</summary>
    private static List<string> TraceNotes(SessionFileInfo info, TraceMarkers markers)
    {
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (markers.Note is { } note)
        {
            notes.Add(note);
        }

        return notes;
    }

    private static string TracePath(string etlPath)
    {
        var text = SessionCatalog.Clean(etlPath ?? "");
        if (text.Length == 0)
        {
            throw new McpException("etlPath is required: the full path of a trace (.etl).");
        }

        if (!Path.IsPathFullyQualified(text))
        {
            throw new McpException($"'{etlPath}' isn't a full path. Give the trace's full path, such as C:\\traces\\app.etl.");
        }

        var path = SessionCatalog.FullPath(text, etlPath!);
        if (Directory.Exists(path))
        {
            throw new McpException($"{path} is a folder. Give the full path of a trace (.etl).");
        }

        if (!File.Exists(path))
        {
            throw new McpException($"The trace {path} was not found.");
        }

        if (string.Equals(Path.GetExtension(path), SessionCatalog.Extension, StringComparison.OrdinalIgnoreCase))
        {
            throw new McpException($"{path} is a session file already. Pass it as sessionPath to the other tools.");
        }

        return path;
    }

    private static TraceContents ListProcesses(string trace, CancellationToken cancellationToken)
    {
        try
        {
            return OfflineEtlSource.ListProcesses(trace, cancellationToken);
        }
        catch (OperationCanceledException)
        {
            throw;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            throw new McpException(TraceError(trace, error), error);
        }
    }

    private static TraceReplay StartReplay(string trace, TraceProcess process, string directory, TraceMarkers markers)
    {
        try
        {
            return TraceReplay.Start(trace, process.ProcessId, process.Name, directory, markers: markers.Providers);
        }
        catch (FileNotFoundException error)
        {
            throw new McpException($"The trace {trace} was not found.", error);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SqliteException)
        {
            throw new McpException($"Couldn't create the session file in {directory}: {error.Message}", error);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            throw new McpException(TraceError(trace, error), error);
        }
    }

    private static async Task<int?> ChooseProcessAsync(McpServer server, IReadOnlyList<TraceProcess> processes, string message,
        CancellationToken cancellationToken)
    {
        if (server.ClientCapabilities?.Elicitation is null)
        {
            return null;
        }

        var options = processes.Take(MaxChoices).Select(process => new ElicitRequestParams.EnumSchemaOption
        {
            Const = process.ProcessId.ToString(CultureInfo.InvariantCulture),
            Title = Label(process),
        }).ToList();
        ElicitResult result;
        try
        {
            result = await server.ElicitAsync(new ElicitRequestParams
            {
                Message = message,
                RequestedSchema = new ElicitRequestParams.RequestSchema
                {
                    Properties =
                    {
                        ["processId"] = new ElicitRequestParams.TitledSingleSelectEnumSchema { Title = "Process", OneOf = options },
                    },
                    Required = ["processId"],
                },
            }, cancellationToken).ConfigureAwait(false);
        }
        catch (InvalidOperationException)
        {
            return null;
        }
        catch (McpException)
        {
            // The client answered the request with an error; the tool lists the processes instead.
            return null;
        }

        if (!result.IsAccepted || result.Content is not { } content || !content.TryGetValue("processId", out var element))
        {
            return null;
        }

        return element.ValueKind switch
        {
            JsonValueKind.String when int.TryParse(element.GetString(), NumberStyles.None, CultureInfo.InvariantCulture, out var id) => id,
            JsonValueKind.Number when element.TryGetInt32(out var id) => id,
            _ => null,
        };
    }

    private static void DeleteSession(string path)
    {
        foreach (var file in new[] { path, path + "-wal", path + "-shm" })
        {
            try
            {
                File.Delete(file);
            }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException)
            {
                // Left in the folder for the user to delete.
            }
        }
    }

    private static double Megabytes(long bytes) => EventJson.Round(bytes / 1024.0 / 1024.0, 2);
}
