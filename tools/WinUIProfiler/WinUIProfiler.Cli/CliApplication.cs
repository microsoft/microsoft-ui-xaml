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

          processes  Counts the events of each process with XAML events, to find the target PID.
                     A session file shows its process.
          summary    Counts one process's events by provider and event.
          dump       Prints one process's events in order. --name keeps events whose
                     name contains <text>; --max limits the output (default 1000, 0 = all).
          record     Writes one process's events from a trace to a new session file, as the
                     app does while connected. --limit-mb is the size limit (default 500).

          Events are the XAML TraceLogging events and the kernel's GPU memory usage and
          budget events (DxgKrnl) of the process. A trace needs --pid; a session file
          holds one process.
        """;

    private sealed record Options(string Command, string InputPath, int? ProcessId, string? NameFilter, int Max,
        string? OutputPath, long LimitBytes)
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
        if (args.Length < 2 || args[0] is not ("processes" or "summary" or "dump" or "record"))
        {
            throw new ArgumentException("Expected a command and a trace or session file path.");
        }

        int? processId = null;
        string? nameFilter = null;
        string? outputPath = null;
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
                case "--name" when args[0] == "dump":
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
                default:
                    throw new ArgumentException($"Unknown option {args[i]}.");
            }

            i++;
        }

        var options = new Options(args[0], Path.GetFullPath(args[1]), processId, nameFilter, max, outputPath, limitBytes);
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

                var fields = string.Join(" ", profilerEvent.Fields.Select(field => $"{field.Name}={Format(field.Name, field.Value)}"));
                output.WriteLine($"{profilerEvent.RelativeMilliseconds,12:F3} {profilerEvent.ThreadId,6} {ShortName(profilerEvent.Provider),-9} " +
                    $"{profilerEvent.Name} {fields}");
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

    private static void WriteSessionInfo(SessionFileInfo info, TextWriter output)
    {
        output.WriteLine($"Stored: {info.StoredEvents} events; source {info.Source}, coverage {info.Coverage}; " +
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

    // Element IDs, pointers and handles arrive as Int64; print them in hex like a debugger would.
    private static string Format(string name, object? value) => value switch
    {
        null => "null",
        string text => $"\"{text}\"",
        long number when IsIdentity(name) => $"0x{number:X}",
        IFormattable formattable => formattable.ToString(null, CultureInfo.InvariantCulture),
        _ => value.ToString() ?? ""
    };

    private static bool IsIdentity(string name) =>
        name.EndsWith("Id", StringComparison.OrdinalIgnoreCase) ||
        name.EndsWith("Pointer", StringComparison.OrdinalIgnoreCase) ||
        name.EndsWith("Handle", StringComparison.OrdinalIgnoreCase);
}
