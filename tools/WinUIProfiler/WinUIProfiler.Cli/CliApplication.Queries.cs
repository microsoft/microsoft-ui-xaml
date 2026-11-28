// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.Cli;

internal static partial class CliApplication
{
    private sealed record ScopedEvent(ProfilerEvent Event, string Bucket, IReadOnlyList<ObjectMention> Objects);

    private sealed record EventQuery(string Path, bool Json, long? FrameId, long? RegionId, long? RowStart,
        int? ThreadId, long? FirstSeq, long? LastSeq, string? Bucket, string? Filter, int Offset, int Count);

    private static int QueryCommands(string[] args, TextWriter output, TextWriter error, CancellationToken token,
        CommandUsage? usage)
    {
        EventQuery query;
        try
        {
            if (args is [_, "--help"])
            {
                output.WriteLine(Usage);
                return 0;
            }

            var options = args[0] == "info"
                ? CommandArguments.Parse(args.Skip(1), [], ["--json"])
                : CommandArguments.Parse(args.Skip(1),
                    ["--frame", "--region", "--row", "--thread", "--first-seq", "--last-seq", "--bucket", "--filter", "--offset", "--count"],
                    ["--json"]);
            options.RequirePositionals(1);
            var path = Path.GetFullPath(options.Positionals[0]);
            if (usage is not null)
            {
                usage.Input = "session";
                usage.App = () => TelemetryTarget.ForSessionFile(path);
            }
            var frame = PositiveLong(options, "--frame");
            var region = PositiveLong(options, "--region");
            var row = PositiveLong(options, "--row");
            int? thread = options.Has("--thread") ? options.Integer("--thread", 0, min: 1) : null;
            var first = PositiveLong(options, "--first-seq");
            var last = PositiveLong(options, "--last-seq");
            if (args[0] == "events")
            {
                var selectors = (frame is null ? 0 : 1) + (region is null ? 0 : 1) + (row is null ? 0 : 1) +
                    (first is null && last is null ? 0 : 1);
                if (selectors != 1)
                {
                    throw new ArgumentException("events requires exactly one of --frame, --region, --row, or --first-seq with --last-seq.");
                }

                if ((row is not null || first is not null || last is not null) != (thread is not null))
                {
                    throw new ArgumentException("--row and sequence ranges require --thread; --frame and --region already identify their thread.");
                }

                if ((first is null) != (last is null) || first > last)
                {
                    throw new ArgumentException("A sequence range requires --first-seq <= --last-seq.");
                }
            }

            var bucket = options.Get("--bucket") is { } text
                ? Array.Find(["Xaml", "Composition", "Other"], value => value.Equals(text, StringComparison.OrdinalIgnoreCase))
                    ?? throw new ArgumentException("--bucket must be Xaml, Composition or Other.")
                : null;
            var count = options.Integer("--count", 1000);
            query = new(path, options.Has("--json"), frame, region, row, thread, first, last, bucket, options.Get("--filter"),
                options.Integer("--offset", 0), count == 0 ? -1 : count);
        }
        catch (ArgumentException exception)
        {
            error.WriteLine(exception.Message);
            error.WriteLine(Usage);
            return 2;
        }

        try
        {
            token.ThrowIfCancellationRequested();
            if (args[0] == "info")
            {
                var info = SessionFile.ReadInfo(query.Path);
                if (query.Json)
                {
                    WriteJson(info, output);
                }
                else
                {
                    output.WriteLine($"Session: {query.Path}");
                    output.WriteLine($"Process: {info.ProcessId} ({info.ProcessName ?? "unknown"})");
                    output.WriteLine($"Started: {info.StartedUtc:O}; ended: {info.EndedUtc?.ToString("O", CultureInfo.InvariantCulture) ?? "unknown"}");
                    WriteSessionInfo(info, output);
                    output.WriteLine($"Lost by ETW: {info.LostByEtw?.ToString(CultureInfo.InvariantCulture) ?? "unknown"}; " +
                        $"dropped by profiler: {info.DroppedByProfiler?.ToString(CultureInfo.InvariantCulture) ?? "unknown"}.");
                }

                return 0;
            }

            return Events(query, output, token);
        }
        catch (OperationCanceledException)
        {
            error.WriteLine("Cancelled.");
            return 4;
        }
        catch (Exception exception) when (exception is IOException or InvalidDataException or UnauthorizedAccessException or
            Microsoft.Data.Sqlite.SqliteException or InvalidOperationException or JsonException)
        {
            error.WriteLine($"Could not query '{query.Path}': {exception.Message}");
            return 3;
        }
    }

    private static int Events(EventQuery query, TextWriter output, CancellationToken token)
    {
        var info = SessionFile.ReadInfo(query.Path);
        InterestingRow? row = null;
        if (query.RegionId is { } region)
        {
            row = SessionFile.ReadInterestingRows(query.Path, region).SingleOrDefault(item =>
                item.Type == InterestingRowType.Region && item.RegionId == region)
                ?? throw new InvalidDataException($"Region {region} was not found.");
        }
        else if (query.RowStart is { } start)
        {
            var matches = SessionFile.ReadInterestingRows(query.Path)
                .Where(item => item.Type != InterestingRowType.Region && item.StartSeq == start && item.ThreadId == query.ThreadId)
                .DistinctBy(item => item.StopSeq).ToList();
            row = matches.Count == 1 ? matches[0]
                : throw new InvalidDataException(matches.Count == 0 ? $"No interesting row starts at Seq {start} on thread {query.ThreadId}."
                    : "That row has multiple spans; select one using --thread, --first-seq and --last-seq.");
        }

        if (query.FrameId is { } frame && SessionFile.ReadFrames(query.Path, frame).Count == 0)
        {
            throw new InvalidDataException($"Frame {frame} was not found.");
        }

        var thread = row?.ThreadId ?? query.ThreadId;
        var first = row?.StartSeq ?? query.FirstSeq;
        var last = row?.StopSeq ?? query.LastSeq;
        var events = query.Json ? new List<ScopedEvent>() : null;
        void WriteEvent(ProfilerEvent item, string bucket, IReadOnlyList<ObjectMention> objects)
        {
            if (events is not null)
            {
                events.Add(new(item, bucket, objects));
            }
            else
            {
                output.WriteLine(FormatEvent(item, bucket));
                foreach (var mention in objects)
                {
                    output.WriteLine($"    {mention.Field}: {mention.Kind} {mention.ObjectId} {mention.Type} {mention.Name}");
                }
            }
        }

        var total = query.FrameId is { } id
            ? SessionFile.CountFrameEvents(query.Path, id, query.Bucket, query.Filter)
            : SessionFile.CountSpanEvents(query.Path, thread!.Value, first!.Value, last!.Value, query.Bucket, query.Filter);
        var returned = query.FrameId is { } frameId
            ? SessionFile.ReadFrameEvents(query.Path, frameId, query.Bucket, query.Filter, query.Offset, query.Count, WriteEvent, token)
            : SessionFile.ReadSpanEvents(query.Path, thread!.Value, first!.Value, last!.Value, query.Bucket, query.Filter,
                query.Offset, query.Count, WriteEvent, token);
        if (events is not null)
        {
            WriteJson(new
            {
                Session = info,
                query.FrameId,
                query.RegionId,
                Row = row,
                ThreadId = thread,
                FirstSeq = first,
                LastSeq = last,
                query.Offset,
                Returned = returned,
                Total = total,
                NextOffset = query.Offset + returned < total ? (long?)(query.Offset + returned) : null,
                Events = events
            }, output);
        }
        else
        {
            output.WriteLine($"{returned} of {total} events, from offset {query.Offset}.");
        }

        return 0;
    }

    private static long? PositiveLong(CommandArguments options, string name) =>
        options.Get(name) is not { } text ? null
            : long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var value) && value > 0
                ? value : throw new ArgumentException($"{name} must be a positive integer.");

    private static void WriteJson<T>(T value, TextWriter output) => output.WriteLine(JsonSerializer.Serialize(value, JsonOptions));
}
