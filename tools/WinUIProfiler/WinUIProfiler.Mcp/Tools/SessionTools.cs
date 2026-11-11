// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using ModelContextProtocol;
using ModelContextProtocol.Server;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;

namespace WinUIProfiler.Mcp.Tools;

/// <summary>Finds session files and summarizes one.</summary>
[McpServerToolType]
internal static class SessionTools
{
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
            using var trail = QueryTrail.Start();
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
                "(.etl) into one with the app or the WinUI Profiler CLI's record command.");
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

    private static double Megabytes(long bytes) => EventJson.Round(bytes / 1024.0 / 1024.0, 2);
}
