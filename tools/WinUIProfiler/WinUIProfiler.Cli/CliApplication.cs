// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Cli;

internal static class CliApplication
{
    private const int DefaultDumpLimit = 1000;

    private const string Usage = """
        Usage:
          WinUIProfiler.Cli processes <trace.etl>
          WinUIProfiler.Cli summary <trace.etl> --pid <pid>
          WinUIProfiler.Cli dump <trace.etl> --pid <pid> [--name <text>] [--max <count>]

          processes  Counts XAML events per process, to find the target PID.
          summary    Counts one process's XAML events by provider and event.
          dump       Prints one process's XAML events in order. --name keeps events whose
                     name contains <text>; --max limits the output (default 1000, 0 = all).
        """;

    private sealed record Options(string Command, string TracePath, int? ProcessId, string? NameFilter, int Max);

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
            error.WriteLine($"Could not read '{options.TracePath}': {exception.Message}");
            return 3;
        }
    }

    private static Options Parse(string[] args)
    {
        if (args.Length < 2 || args[0] is not ("processes" or "summary" or "dump"))
        {
            throw new ArgumentException("Expected a command and a trace path.");
        }

        int? processId = null;
        string? nameFilter = null;
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
                default:
                    throw new ArgumentException($"Unknown option {args[i]}.");
            }

            i++;
        }

        if (args[0] != "processes" && processId is null)
        {
            throw new ArgumentException($"{args[0]} requires --pid.");
        }

        return new(args[0], Path.GetFullPath(args[1]), processId, nameFilter, max);
    }

    private static int Processes(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var counts = new Dictionary<int, long[]>();
        var result = OfflineEtlSource.Read(options.TracePath, null, profilerEvent =>
        {
            if (!counts.TryGetValue(profilerEvent.ProcessId, out var perProvider))
            {
                counts[profilerEvent.ProcessId] = perProvider = new long[2];
            }

            perProvider[(int)profilerEvent.Provider]++;
        }, cancellationToken);

        output.WriteLine($"{"PID",8} {"Events",10} {"XAML",10} {"XAML-Prof",10}");
        foreach (var (processId, perProvider) in counts.OrderByDescending(entry => entry.Value.Sum()))
        {
            output.WriteLine($"{processId,8} {perProvider.Sum(),10} {perProvider[0],10} {perProvider[1],10}");
        }

        output.WriteLine($"Trace: {result.SessionDuration.TotalSeconds:F1} s, {result.EventsLost} events lost.");
        return 0;
    }

    private static int Summary(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        // One event name can have several field layouts (for example a start and a stop form), so they are counted apart.
        var counts = new Dictionary<(XamlProvider Provider, string Name, string Fields), long>();
        double first = double.MaxValue, last = 0;
        var result = OfflineEtlSource.Read(options.TracePath, options.ProcessId, profilerEvent =>
        {
            first = Math.Min(first, profilerEvent.RelativeMilliseconds);
            last = Math.Max(last, profilerEvent.RelativeMilliseconds);
            var key = (profilerEvent.Provider, profilerEvent.Name, string.Join(",", profilerEvent.Fields.Select(field => field.Name)));
            counts[key] = counts.GetValueOrDefault(key) + 1;
        }, cancellationToken);

        if (result.EventsDelivered == 0)
        {
            output.WriteLine($"No XAML events from process {options.ProcessId}. Run 'processes' to list the PIDs in this trace.");
            return 0;
        }

        var seconds = Math.Max(0.001, (last - first) / 1000);
        output.WriteLine($"Process {options.ProcessId}: {result.EventsDelivered} XAML events over {seconds:F1} s " +
            $"({result.EventsDelivered / seconds:F0}/s); {result.EventsLost} events lost in the trace.");
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
            OfflineEtlSource.Read(options.TracePath, options.ProcessId, profilerEvent =>
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

    private static string ShortName(XamlProvider provider) => provider == XamlProvider.Xaml ? "XAML" : "XAML-Prof";

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
