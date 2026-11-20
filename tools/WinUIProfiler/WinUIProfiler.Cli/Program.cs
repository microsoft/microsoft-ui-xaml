// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Reflection;
using WinUITools.Telemetry;

namespace WinUIProfiler.Cli;

internal static class Program
{
    // The commands CliApplication runs. Usage events name only these, never what was typed.
    private static readonly string[] Commands =
    [
        "processes", "summary", "dump", "record", "launch", "objects", "timeline", "frames", "interesting", "symbolize", "cpu",
        "events", "info", "live", "targets", "snapshot"
    ];

    private static int Main(string[] args)
    {
        UsageTelemetry.Initialize("profiler", "cli",
            typeof(Program).Assembly.GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion ?? "");
        if (UsageTelemetry.TakeNotice() is { } notice)
        {
            Console.Error.WriteLine(notice);
        }

        var started = Stopwatch.GetTimestamp();
        int? exitCode = null;
        using var cancellation = new CancellationTokenSource();
        ConsoleCancelEventHandler handler = (_, e) =>
        {
            e.Cancel = true;
            cancellation.Cancel();
        };
        Console.CancelKeyPress += handler;
        try
        {
            exitCode = CliApplication.Run(args, Console.Out, Console.Error, cancellation.Token);
            return exitCode.Value;
        }
        finally
        {
            Console.CancelKeyPress -= handler;
            UsageTelemetry.Track("Profiler.CliCommand", new Dictionary<string, string>
            {
                ["command"] = args.Length > 0 && Commands.Contains(args[0]) ? args[0] : "unknown",
                ["outcome"] = exitCode switch { 0 => "success", 4 => "cancelled", _ => "failed" },
            }, new Dictionary<string, double> { ["durationSec"] = Stopwatch.GetElapsedTime(started).TotalSeconds });
            UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(2)).GetAwaiter().GetResult();
        }
    }
}
