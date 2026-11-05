// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using ModelContextProtocol;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Tools;

namespace WinUIProfiler.Mcp.Sessions;

/// <summary>A session file, with when it was last written and its size, both counting its write-ahead log.</summary>
internal sealed record SessionFileEntry(string Path, DateTime LastWrittenUtc, long Bytes);

/// <summary>
/// What a session's processed data holds, for <see cref="SessionCatalog.Analyses"/>: its ticks, regions of interest,
/// interesting rows outside regions, and launch frames. ViewError says why the shared views couldn't be read, if they
/// couldn't.
/// </summary>
internal sealed record SessionContents(int Ticks, int Regions, int Rows, int LaunchFrames, string? ViewError);

/// <summary>
/// Finds the session files the tools read, describes what one holds, and turns the ways reading one can fail into
/// messages for the client. Only <see cref="McpException"/> messages reach the client, so every failure a user can fix
/// becomes one.
/// </summary>
internal static class SessionCatalog
{
    internal const string Extension = ".sqlite";

    // The end reason of a session file whose trace was read to its end, as the app, record_trace and the CLI write it.
    internal const string EndOfTrace = "End of trace";

    internal const string SessionPathDescription =
        "The session file (.sqlite) to read: a full path, or a file name in the WinUI Profiler app's sessions folder. " +
        "Omit it to read the newest session file there.";

    internal const string NoEndReason = "none (still recording, or the recording stopped unexpectedly)";

    /// <summary>Where the WinUI Profiler app and record_trace write session files.</summary>
    internal static string DefaultFolder { get; } = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Sessions");

    /// <summary>The session files in a folder, newest first; none when the folder doesn't exist.</summary>
    internal static IReadOnlyList<SessionFileEntry> Files(string folder)
    {
        if (!Directory.Exists(folder))
        {
            return [];
        }

        var entries = new List<SessionFileEntry>();
        try
        {
            foreach (var path in Directory.EnumerateFiles(folder, "*" + Extension))
            {
                // The pattern alone can match longer extensions.
                if (!string.Equals(Path.GetExtension(path), Extension, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                try
                {
                    var file = new FileInfo(path);
                    var written = file.LastWriteTimeUtc;
                    var bytes = file.Length;
                    var log = new FileInfo(path + "-wal");
                    if (log.Exists)
                    {
                        written = log.LastWriteTimeUtc > written ? log.LastWriteTimeUtc : written;
                        bytes += log.Length;
                    }

                    entries.Add(new(path, written, bytes));
                }
                catch (Exception error) when (error is IOException or UnauthorizedAccessException)
                {
                    // The file was deleted, or can't be read, while the folder was listed.
                }
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            throw new McpException($"The folder {folder} couldn't be listed: {error.Message}", error);
        }

        return entries
            .OrderByDescending(entry => entry.LastWrittenUtc)
            .ThenByDescending(entry => entry.Path, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    /// <summary>A path as a user may type it: trimmed, without surrounding quotes, with environment variables expanded.</summary>
    internal static string Clean(string text)
    {
        var value = text.Trim();
        if (value.Length >= 2 && value[0] is '"' or '\'' && value[^1] == value[0])
        {
            value = value[1..^1].Trim();
        }

        return Environment.ExpandEnvironmentVariables(value);
    }

    /// <summary>A folder argument as a full path, or the default folder when it's empty.</summary>
    internal static string Folder(string? folder)
    {
        var text = folder is null ? "" : Clean(folder);
        return text.Length == 0 ? DefaultFolder : FullPath(text, folder!);
    }

    /// <summary>A path argument as a full path, or a message that it isn't a valid path.</summary>
    internal static string FullPath(string text, string argument)
    {
        try
        {
            return Path.GetFullPath(text);
        }
        catch (Exception error) when (error is ArgumentException or NotSupportedException or PathTooLongException)
        {
            throw new McpException($"'{argument}' isn't a valid path.", error);
        }
    }

    /// <summary>
    /// The session file a tool reads: the newest in the folder when sessionPath is empty, else sessionPath as a full
    /// path, or as a file name in the folder, with or without its extension.
    /// </summary>
    internal static string Resolve(string? sessionPath, string? folder = null)
    {
        var directory = folder ?? DefaultFolder;
        var text = sessionPath is null ? "" : Clean(sessionPath);
        if (text.Length == 0)
        {
            return Files(directory).FirstOrDefault()?.Path ?? throw new McpException(
                $"There are no session files in {directory}. Record one with the WinUI Profiler app, or read a trace (.etl) " +
                "into one with record_trace.");
        }

        string path;
        if (Path.IsPathRooted(text))
        {
            path = FullPath(text, sessionPath!);
        }
        else
        {
            path = FullPath(Path.Combine(directory, text), sessionPath!);
            if (!File.Exists(path))
            {
                if (File.Exists(path + Extension))
                {
                    path += Extension;
                }
                else if (FullPath(text, sessionPath!) is var relative && File.Exists(relative))
                {
                    path = relative;
                }
            }
        }

        if (File.Exists(path))
        {
            if (string.Equals(Path.GetExtension(path), ".etl", StringComparison.OrdinalIgnoreCase))
            {
                throw new McpException($"{path} is a trace, not a session file. Read it into a session file with record_trace, " +
                    "then pass the session file it returns.");
            }

            return path;
        }

        if (Directory.Exists(path))
        {
            throw new McpException($"{path} is a folder. The sessions tool's list view lists the session files in a folder.");
        }

        throw new McpException(
            $"The session file {path} was not found. The sessions tool's list view lists the session files in {directory}.");
    }

    /// <summary>Runs a read of a session file, turning the ways it can fail into messages for the client.</summary>
    internal static T Read<T>(string path, Func<T> read)
    {
        try
        {
            return read();
        }
        catch (Exception error) when (ReadError(path, error) is { } message)
        {
            throw new McpException(message, error);
        }
    }

    /// <summary>
    /// The message for a way reading a session file can fail; null for any other error, which isn't the file's.
    /// </summary>
    internal static string? ReadError(string path, Exception error) => error switch
    {
        FileNotFoundException or DirectoryNotFoundException => $"The session file {path} was not found.",
        InvalidDataException => error.Message.Contains(path, StringComparison.OrdinalIgnoreCase)
            ? error.Message
            : $"{path}: {error.Message}",
        SqliteException => $"{path} couldn't be read as a session file: {error.Message}",

        // Enum.Parse's error for a stored name, such as a provider or a row type, that this version doesn't know.
        ArgumentException when error.GetType() == typeof(ArgumentException) =>
            $"{path} holds a value this version of the WinUI Profiler plugin doesn't know; a newer WinUI Profiler " +
            $"probably wrote it. {error.Message}",

        // A stored value, such as a time or a process ID, that isn't in the form WinUI Profiler writes.
        FormatException or OverflowException =>
            $"{path} holds a value that isn't in the form WinUI Profiler writes, so the file is damaged or another " +
            $"program wrote it: {error.Message}",
        IOException or UnauthorizedAccessException => $"{path} couldn't be read: {error.Message}",
        _ => null,
    };

    /// <summary>
    /// Reads what the summary needs from the shared views. A file whose views can't be read, such as one written by
    /// another version of the app, still gets a summary, with the reason.
    /// </summary>
    internal static (IReadOnlyList<SessionFrame> Ticks, IReadOnlyList<InterestingRow> Rows, IReadOnlyList<LaunchFrame> Launch,
        string? Error) ReadViews(string path)
    {
        try
        {
            return (SessionFile.ReadFrames(path), SessionFile.ReadInterestingRows(path), SessionFile.ReadLaunchSummary(path), null);
        }
        catch (SqliteException error)
        {
            return ([], [], [], $"The file's frame views couldn't be read: {error.Message}");
        }
        catch (ArgumentException error) when (error.GetType() == typeof(ArgumentException))
        {
            return ([], [], [], "The file's frame views hold a value this version of the WinUI Profiler plugin doesn't " +
                $"know; a newer WinUI Profiler probably wrote it. {error.Message}");
        }
    }

    /// <summary>Finds the session file a tool reads, then reads it off the calling thread.</summary>
    internal static Task<object> RunAsync(string? sessionPath, Func<string, object> body, CancellationToken cancellationToken) =>
        Task.Run(() =>
        {
            var path = Resolve(sessionPath);
            return Read(path, () => body(path));
        }, cancellationToken);

    internal static bool IsTrace(SessionFileInfo info) => info.Coverage == SessionCoverage.Trace;

    /// <summary>What every tool result says about the session it read.</summary>
    internal static object Header(string path, SessionFileInfo info) => new
    {
        sessionPath = path,
        processId = info.ProcessId,
        processName = info.ProcessName,
        source = info.Source,
        coverage = info.Coverage.ToString(),
        endReason = info.EndReason ?? NoEndReason,
        timeBase = TimeBase(info),
    };

    /// <summary>What the times in a session's results count from.</summary>
    internal static string TimeBase(SessionFileInfo info)
    {
        if (IsTrace(info))
        {
            return "Milliseconds from the first of the process's events in the trace; the trace doesn't say when the process was created.";
        }

        if (info.ProcessStartMilliseconds is { } start)
        {
            var distance = Math.Abs(start).ToString("0.###", CultureInfo.InvariantCulture);
            return start >= 0
                ? $"Milliseconds from the start of the recording; the process was created {distance} ms after it."
                : $"Milliseconds from the start of the recording; the process was created {distance} ms before it.";
        }

        return "Milliseconds from the start of the recording; when the process was created isn't known.";
    }

    /// <summary>Which analyses have data in a session, and which tool and views give them.</summary>
    internal static IReadOnlyList<SessionAnalysis> Analyses(SessionFileInfo info, SessionContents contents) =>
    [
        Analysis("frames", contents.Ticks > 0, "frames", ["summary", "slowest", "tick"], contents.ViewError ?? FrameTools.NoTicks),
        Analysis("regions", contents.Regions + contents.Rows > 0, "frames", ["regions", "rows"],
            contents.ViewError ?? FrameTools.NoRegions),
        Analysis("launch", contents.LaunchFrames > 0, "launch", ["milestones", "frames"],
            contents.ViewError ?? LaunchTools.NoLaunch(info, contents.Ticks)),
        Analysis("objects", info.UIElements + info.Visuals > 0, "objects", ["types", "list", "history"], ObjectTools.NoObjects),
    ];

    /// <summary>What a reader of a session's results should know: what's missing from it, and why.</summary>
    internal static IReadOnlyList<string> Notes(SessionFileInfo info)
    {
        var notes = new List<string>();
        if (info.EndReason is null)
        {
            notes.Add("The session has no end reason: it's still being recorded, or the recording stopped unexpectedly. " +
                "Results cover the events stored so far.");
        }

        // Reading stopped or failed partway, in the app or here.
        if (IsTrace(info) && info.EndReason is { } reason && reason != EndOfTrace)
        {
            notes.Add($"The trace wasn't read to its end (the session's end reason is \"{reason}\"); results cover only " +
                "the events read before then.");
        }

        if (info.CompleteFromSeq is { } completeFrom)
        {
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"The oldest events, before Seq {completeFrom}, were deleted to keep the file within {info.LimitBytes / 1024 / 1024} MB; " +
                $"results cover only the events after them."));
        }

        if (info.LostByEtw is { } lost && lost > 0)
        {
            var events = EventJson.Count(lost, "event", "events");
            var some = lost == 1 ? "it" : "some";
            notes.Add(IsTrace(info)
                ? $"The trace lost {events} while it was recorded, of any process; results may miss {some}."
                : $"ETW lost {events} while recording; results may miss {some}.");
        }

        if (info.DroppedByProfiler is { } dropped && dropped > 0)
        {
            notes.Add($"The profiler dropped {EventJson.Count(dropped, "event", "events")} because its queue was full; " +
                $"results miss {(dropped == 1 ? "it" : "them")}.");
        }

        if (info.Coverage == SessionCoverage.Connection)
        {
            notes.Add("The recording began when the profiler connected to the running process, so it has nothing from before " +
                "then, such as most of the launch.");
        }

        if (IsTrace(info))
        {
            notes.Add($"Read from the trace {info.EtlPath}.");
        }

        return notes;
    }

    private static SessionAnalysis Analysis(string name, bool available, string tool, string[] views, string why) =>
        new(name, available, tool, views, available ? null : why);
}

/// <summary>An analysis the summary lists: whether the session has data for it, the tool and views that give it, and why not.</summary>
internal sealed record SessionAnalysis(string Name, bool Available, string Tool, IReadOnlyList<string> Views, string? Why);
