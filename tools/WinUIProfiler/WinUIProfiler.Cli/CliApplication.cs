// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Cli;

internal static class CliApplication
{
    private const int DefaultDumpLimit = 1000;

    private const string Usage = """
        Usage:
          WinUIProfiler.Cli processes <trace.etl | session.sqlite>
          WinUIProfiler.Cli summary <trace.etl | session.sqlite> [--pid <pid>]
          WinUIProfiler.Cli dump <trace.etl | session.sqlite> [--pid <pid>] [--name <text>] [--max <count>]
          WinUIProfiler.Cli record <trace.etl> --pid <pid> --out <session.sqlite> [--limit-mb <size>]
          WinUIProfiler.Cli objects <session.sqlite> [--type <text>] [--name <text>]
          WinUIProfiler.Cli timeline <session.sqlite> (--object <id> | --address <address>)
          WinUIProfiler.Cli frames <session.sqlite> [--frame <id> [--bucket Xaml|Composition|Other] [--filter <text>]]

          processes  Counts the events of each process with XAML events, to find the target PID.
                     A session file shows its process.
          summary    Counts one process's events by provider and event.
          dump       Prints one process's events in order. --name keeps events whose
                     name contains <text>; --max limits the output (default 1000, 0 = all).
          record     Writes one process's events from a trace to a new session file, as the
                     app does while connected. --limit-mb is the size limit (default 500).
          objects    Lists the UI elements in a session file. --type and --name filter them.
          timeline   Prints the events that mention one UI element, marking the field that holds it.
          frames     Lists the frames per UI thread. --frame prints one frame's events: --bucket keeps
                     one bucket; --filter keeps events whose name or a field value contains <text>
                     (a number matches a field with exactly that value).

          Events are the XAML TraceLogging events and the kernel's GPU memory usage and
          budget events (DxgKrnl) of the process. A trace needs --pid; a session file
          holds one process.
        """;

    private sealed record Options(string Command, string InputPath, int? ProcessId, string? NameFilter, int Max,
        string? OutputPath, long LimitBytes, string? TypeFilter, long? ObjectId, long? Address, long? FrameId, string? Bucket,
        string? EventFilter)
    {
        public bool IsSessionFile => InputPath.EndsWith(".sqlite", StringComparison.OrdinalIgnoreCase);
    }

    private sealed record ReadResult(long EventsDelivered, int EventsLost, TimeSpan Duration);

    internal static int Run(string[] args, TextWriter output, TextWriter error, CancellationToken cancellationToken)
    {
        Options options;
        try
        {
            options = Parse(args);
        }
        catch (ArgumentException exception)
        {
            error.WriteLine(exception.Message);
            error.WriteLine(Usage);
            return 2;
        }

        try
        {
            return options.Command switch
            {
                "processes" => Processes(options, output, cancellationToken),
                "summary" => Summary(options, output, cancellationToken),
                "record" => Record(options, output, error, cancellationToken),
                "objects" => Objects(options, output),
                "timeline" => Timeline(options, output, cancellationToken),
                "frames" => Frames(options, output, cancellationToken),
                _ => Dump(options, output, cancellationToken)
            };
        }
        catch (OperationCanceledException)
        {
            error.WriteLine("Cancelled.");
            return 4;
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            error.WriteLine($"Could not read '{options.InputPath}': {exception.Message}");
            return 3;
        }
    }

    private static Options Parse(string[] args)
    {
        if (args.Length < 2 || args[0] is not ("processes" or "summary" or "dump" or "record" or "objects" or "timeline" or "frames"))
        {
            throw new ArgumentException("Expected a command and a trace or session file path.");
        }

        int? processId = null;
        string? nameFilter = null;
        string? outputPath = null;
        string? typeFilter = null;
        long? objectId = null;
        long? address = null;
        long? frameId = null;
        string? bucket = null;
        string? eventFilter = null;
        var limitBytes = SessionWriter.DefaultLimitBytes;
        var max = DefaultDumpLimit;
        for (var i = 2; i < args.Length; i++)
        {
            var value = i + 1 < args.Length ? args[i + 1] : throw new ArgumentException($"Missing value for {args[i]}.");
            switch (args[i])
            {
                case "--pid":
                    processId = int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var pid) && pid > 0
                        ? pid : throw new ArgumentException("--pid must be a positive process ID.");
                    break;
                case "--name" when args[0] is "dump" or "objects":
                    nameFilter = value;
                    break;
                case "--max" when args[0] == "dump":
                    max = int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var limit)
                        ? limit : throw new ArgumentException("--max must be 0 or a positive count.");
                    break;
                case "--out" when args[0] == "record":
                    outputPath = Path.GetFullPath(value);
                    break;
                case "--limit-mb" when args[0] == "record":
                    limitBytes = int.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var megabytes) && megabytes > 0
                        ? megabytes * 1024L * 1024 : throw new ArgumentException("--limit-mb must be a positive size in MB.");
                    break;
                case "--type" when args[0] == "objects":
                    typeFilter = value;
                    break;
                case "--object" when args[0] == "timeline":
                    objectId = long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var id) && id > 0
                        ? id : throw new ArgumentException("--object must be an element ID from 'objects'.");
                    break;
                case "--address" when args[0] == "timeline":
                    address = ParseAddress(value) ?? throw new ArgumentException("--address must be a hex (0x...) or decimal address.");
                    break;
                case "--frame" when args[0] == "frames":
                    frameId = long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var frame) && frame > 0
                        ? frame : throw new ArgumentException("--frame must be a frame ID from 'frames'.");
                    break;
                case "--bucket" when args[0] == "frames":
                    bucket = Array.Find(["Xaml", "Composition", "Other"], name => name.Equals(value, StringComparison.OrdinalIgnoreCase))
                        ?? throw new ArgumentException("--bucket must be Xaml, Composition or Other.");
                    break;
                case "--filter" when args[0] == "frames":
                    eventFilter = value;
                    break;
                default:
                    throw new ArgumentException($"Unknown option {args[i]}.");
            }

            i++;
        }

        var options = new Options(args[0], Path.GetFullPath(args[1]), processId, nameFilter, max, outputPath, limitBytes,
            typeFilter, objectId, address, frameId, bucket, eventFilter);
        if (options.Command is "objects" or "timeline" or "frames" && !options.IsSessionFile)
        {
            throw new ArgumentException($"{args[0]} reads a session file (.sqlite); use 'record' to make one from a trace.");
        }

        if (options.Command == "timeline" && (objectId is null) == (address is null))
        {
            throw new ArgumentException("timeline requires either --object <id> or --address <address>.");
        }

        if ((bucket ?? eventFilter) is not null && frameId is null)
        {
            throw new ArgumentException("--bucket and --filter need --frame <id>.");
        }

        if (options.Command is "summary" or "dump" or "record" && !options.IsSessionFile && processId is null)
        {
            throw new ArgumentException($"{args[0]} on a trace requires --pid.");
        }

        if (options.Command == "record" && (options.IsSessionFile || outputPath is null))
        {
            throw new ArgumentException("record reads a trace (.etl) and requires --out <session.sqlite>.");
        }

        return options;
    }

    // A session file holds one process; --pid, if given, must be that process.
    private static ReadResult Read(Options options, int? processId, Action<ProfilerEvent> sink, CancellationToken cancellationToken)
    {
        if (!options.IsSessionFile)
        {
            var result = OfflineEtlSource.Read(options.InputPath, processId, sink, cancellationToken);
            return new(result.EventsDelivered, result.EventsLost, result.SessionDuration);
        }

        var info = SessionFile.ReadInfo(options.InputPath);
        if (processId is { } requested && requested != info.ProcessId)
        {
            return new(0, info.LostByEtw ?? 0, TimeSpan.Zero);
        }

        double first = double.MaxValue, last = 0;
        var delivered = SessionFile.Read(options.InputPath, profilerEvent =>
        {
            first = Math.Min(first, profilerEvent.RelativeMilliseconds);
            last = Math.Max(last, profilerEvent.RelativeMilliseconds);
            sink(profilerEvent);
        }, cancellationToken);
        return new(delivered, info.LostByEtw ?? 0, TimeSpan.FromMilliseconds(delivered == 0 ? 0 : last - first));
    }

    private static int Processes(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var counts = new Dictionary<int, long[]>();
        var result = Read(options, null, profilerEvent =>
        {
            if (!counts.TryGetValue(profilerEvent.ProcessId, out var perProvider))
            {
                counts[profilerEvent.ProcessId] = perProvider = new long[Enum.GetValues<XamlProvider>().Length];
            }

            perProvider[(int)profilerEvent.Provider]++;
        }, cancellationToken);

        // In a trace, every process that uses the GPU has GPU memory events; only those with XAML events are listed.
        output.WriteLine($"{"PID",8} {"Events",10} {"XAML",10} {"XAML-Prof",10} {"DxgKrnl",10}");
        foreach (var (processId, perProvider) in counts
            .Where(entry => options.IsSessionFile ||
                entry.Value[(int)XamlProvider.Xaml] + entry.Value[(int)XamlProvider.XamlProfiler] > 0)
            .OrderByDescending(entry => entry.Value.Sum()))
        {
            output.WriteLine($"{processId,8} {perProvider.Sum(),10} {perProvider[(int)XamlProvider.Xaml],10} " +
                $"{perProvider[(int)XamlProvider.XamlProfiler],10} {perProvider[(int)XamlProvider.DxgKrnl],10}");
        }

        output.WriteLine($"{(options.IsSessionFile ? "Session" : "Trace")}: {result.Duration.TotalSeconds:F1} s, {result.EventsLost} events lost.");
        if (options.IsSessionFile)
        {
            WriteSessionInfo(SessionFile.ReadInfo(options.InputPath), output);
        }

        return 0;
    }

    private static int Summary(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        // One event name can have several field layouts (for example a start and a stop form), so they are counted apart.
        var counts = new Dictionary<(XamlProvider Provider, string Name, string Fields), long>();
        double first = double.MaxValue, last = 0;
        var processId = options.ProcessId;
        var result = Read(options, options.ProcessId, profilerEvent =>
        {
            processId = profilerEvent.ProcessId;
            first = Math.Min(first, profilerEvent.RelativeMilliseconds);
            last = Math.Max(last, profilerEvent.RelativeMilliseconds);
            var key = (profilerEvent.Provider, profilerEvent.Name, string.Join(",", profilerEvent.Fields.Select(field => field.Name)));
            counts[key] = counts.GetValueOrDefault(key) + 1;
        }, cancellationToken);

        if (result.EventsDelivered == 0)
        {
            output.WriteLine($"No events from process {options.ProcessId}. Run 'processes' to list the PIDs.");
            return 0;
        }

        var seconds = Math.Max(0.001, (last - first) / 1000);
        output.WriteLine($"Process {processId}: {result.EventsDelivered} events over {seconds:F1} s " +
            $"({result.EventsDelivered / seconds:F0}/s); {result.EventsLost} events lost in the {(options.IsSessionFile ? "session" : "trace")}.");
        output.WriteLine();
        foreach (var (key, count) in counts.OrderByDescending(entry => entry.Value))
        {
            output.WriteLine($"{count,9}  {ShortName(key.Provider),-9} {key.Name}  [{key.Fields}]");
        }

        return 0;
    }

    private static int Dump(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        using var limit = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        long printed = 0;
        try
        {
            Read(options, options.ProcessId, profilerEvent =>
            {
                if (limit.IsCancellationRequested ||
                    options.NameFilter is { } filter && !profilerEvent.Name.Contains(filter, StringComparison.OrdinalIgnoreCase))
                {
                    return;
                }

                output.WriteLine(FormatEvent(profilerEvent, null));
                if (options.Max > 0 && ++printed >= options.Max)
                {
                    limit.Cancel();
                }
            }, limit.Token);
        }
        catch (OperationCanceledException) when (!cancellationToken.IsCancellationRequested)
        {
            output.WriteLine($"... stopped after {options.Max} events (use --max 0 for all).");
        }

        return 0;
    }

    private static int Record(Options options, TextWriter output, TextWriter error, CancellationToken cancellationToken)
    {
        var writer = SessionWriter.Create(options.OutputPath!,
            new(options.ProcessId!.Value, null, null, null, null, SessionCoverage.Trace, options.InputPath), options.LimitBytes);
        long sequence = 0;
        EtlReadResult? result = null;
        try
        {
            result = OfflineEtlSource.Read(options.InputPath, options.ProcessId, profilerEvent => writer.Process(++sequence, profilerEvent),
                cancellationToken);
        }
        finally
        {
            writer.Close(result is null ? "Reading the trace failed" : "End of trace", result?.EventsLost, 0);
        }

        if (writer.Failure is { } failure)
        {
            error.WriteLine($"Writing '{writer.Path}' failed: {failure.Message}");
            return 3;
        }

        output.WriteLine($"Read {sequence} events of process {options.ProcessId} from the trace.");
        WriteSessionInfo(SessionFile.ReadInfo(writer.Path), output);
        output.WriteLine($"Session file: {writer.Path} ({writer.Status.FileBytes / 1024.0 / 1024:F1} MB)");
        return 0;
    }

    private static int Objects(Options options, TextWriter output)
    {
        var all = SessionFile.ReadObjects(options.InputPath);
        var selected = all.Where(element =>
            (options.TypeFilter is not { } type || element.Type?.Contains(type, StringComparison.OrdinalIgnoreCase) == true) &&
            (options.NameFilter is not { } name || element.Name?.Contains(name, StringComparison.OrdinalIgnoreCase) == true)).ToList();
        output.WriteLine($"{"ID",7}  {"Type",-32} {"Name",-24} {"Address",-14} {"First (ms)",12} {"Last (ms)",12} {"Events",7}");
        foreach (var element in selected)
        {
            output.WriteLine($"{element.ObjectId,7}  {ShortType(element.Type),-32} {element.Name ?? "",-24} 0x{element.Address,-12:X} " +
                $"{element.FirstMilliseconds,12:F3} {element.LastMilliseconds,12:F3} {element.Events,7}");
        }

        output.WriteLine($"{selected.Count} of {all.Count} UI elements.");
        return 0;
    }

    private static int Timeline(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var selected = SessionFile.ReadObjects(options.InputPath, options.ObjectId, options.Address);
        if (selected.Count == 0)
        {
            output.WriteLine(options.ObjectId is { } id ? $"No UI element with ID {id}." : $"No UI element at 0x{options.Address:X}.");
            return 0;
        }

        foreach (var element in selected)
        {
            output.WriteLine($"UI element {element.ObjectId}: {element.Type ?? "type not seen"}" +
                (element.Name is null ? "" : $" \"{element.Name}\"") + $" at 0x{element.Address:X}, {element.Events} events");
            SessionFile.ReadTimeline(options.InputPath, element.ObjectId,
                (profilerEvent, field) => output.WriteLine(FormatEvent(profilerEvent, field)), cancellationToken);
            output.WriteLine();
        }

        return 0;
    }

    private static int Frames(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var frames = SessionFile.ReadFrames(options.InputPath, options.FrameId);
        if (options.FrameId is not { } frameId)
        {
            output.WriteLine($"{"ID",7} {"Thread",7} {"Frame",7} {"Ticks",6} {"Start (ms)",12} {"Span (ms)",10} {"Draw (ms)",10} " +
                $"{"Tick time",10} {"Events",7} {"Xaml",6} {"Comp",6} {"Other",6}");
            foreach (var frame in frames.OrderBy(frame => frame.ThreadId).ThenBy(frame => frame.FrameNumber))
            {
                output.WriteLine($"{frame.FrameId,7} {frame.ThreadId,7} {frame.FrameNumber,7} {frame.Ticks,6} " +
                    $"{Milliseconds(frame.StartMilliseconds),12} {Milliseconds(frame.EndMilliseconds - frame.StartMilliseconds),10} " +
                    $"{Milliseconds(frame.EndMilliseconds - frame.LastTickStartMilliseconds),10} {Milliseconds(frame.TickMilliseconds),10} " +
                    $"{frame.Events,7} {frame.Xaml,6} {frame.Composition,6} {frame.Other,6}{(frame.Drawn ? "" : "  (latest; may not have drawn)")}");
            }

            output.WriteLine($"{frames.Count} frames on {frames.Select(frame => frame.ThreadId).Distinct().Count()} UI thread(s).");
            return 0;
        }

        if (frames.Count == 0)
        {
            output.WriteLine($"No frame with ID {frameId}.");
            return 0;
        }

        var selected = frames[0];
        output.WriteLine($"Frame {selected.FrameNumber} on thread {selected.ThreadId}: {selected.Ticks} tick(s), {selected.Events} events " +
            $"({selected.Xaml} Xaml, {selected.Composition} Composition, {selected.Other} Other)");
        SessionFile.ReadFrameEvents(options.InputPath, frameId, options.Bucket, options.EventFilter, 0, -1,
            (profilerEvent, bucket) => output.WriteLine(FormatEvent(profilerEvent, bucket)), cancellationToken);
        return 0;
    }

    private static void WriteSessionInfo(SessionFileInfo info, TextWriter output)
    {
        output.WriteLine($"Stored: {info.StoredEvents} events, {info.UIElements} UI elements, {info.Frames} frames; " +
            $"source {info.Source}, coverage {info.Coverage}; " +
            $"ended: {info.EndReason ?? "not recorded (still recording, or stopped unexpectedly)"}.");
        if (info.CompleteFromSeq is { } completeFrom)
        {
            output.WriteLine($"Events before Seq {completeFrom} were dropped to stay within {info.LimitBytes / 1024 / 1024} MB.");
        }
    }

    private static string ShortName(XamlProvider provider) => provider switch
    {
        XamlProvider.Xaml => "XAML",
        XamlProvider.XamlProfiler => "XAML-Prof",
        _ => "DxgKrnl"
    };

    private static string ShortType(string? type) => type is null ? "?" : type[(type.LastIndexOf('.') + 1)..];

    private static string Milliseconds(double? value) => value is { } milliseconds
        ? milliseconds.ToString("F3", CultureInfo.InvariantCulture) : "-";

    // tag is shown in brackets after the event name: the field that holds a timeline's element, or the event's bucket.
    private static string FormatEvent(ProfilerEvent profilerEvent, string? tag)
    {
        return $"{profilerEvent.RelativeMilliseconds,12:F3} {profilerEvent.ThreadId,6} {ShortName(profilerEvent.Provider),-9} " +
            $"{profilerEvent.Name}{(tag is null ? "" : $" [{tag}]")} {EventFormat.Fields(profilerEvent)}";
    }

    private static long? ParseAddress(string text) =>
        text.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
            ? long.TryParse(text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex) ? hex : null
            : long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var number) ? number : null;
}
