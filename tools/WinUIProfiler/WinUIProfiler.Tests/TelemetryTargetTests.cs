// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

// The app the usage events name: a trace's process or a live app's executable by its name only when it is a known
// Microsoft app, any other as a short hash; and the session file and app a CLI command tells its usage event it read.
internal static class TelemetryTargetTests
{
    internal static void Run()
    {
        var other = TelemetryTarget.ForProcessName("ContosoApp");
        Expect(TelemetryTarget.ForProcessName("calculatorapp.exe") == "CalculatorApp" && other.Length == 12 &&
            other.All(char.IsAsciiHexDigitLower) && TelemetryTarget.ForProcessName(" ") == "unknown",
            "A process must be named only when it has the name of a known Microsoft app.");

        // A live app's executable that Microsoft's version information doesn't name, such as a local build, follows the
        // same rule.
        var folder = Directory.CreateTempSubdirectory("WinUIProfiler-UsageTarget-").FullName;
        try
        {
            var known = Path.Combine(folder, "VoiceRecorder.exe");
            var unknown = Path.Combine(folder, "ContosoApp.exe");
            File.WriteAllBytes(known, []);
            File.WriteAllBytes(unknown, []);
            Expect(TelemetryTarget.ForTarget(TargetLaunchKind.Unpackaged, known) == "VoiceRecorder" &&
                TelemetryTarget.ForTarget(TargetLaunchKind.Unpackaged, unknown) == other,
                "A live app's executable must be named by the same rule as a trace's process.");
        }
        finally
        {
            Directory.Delete(folder, recursive: true);
        }

        foreach (var (process, app) in new[] { ("WinUIGallery", "WinUIGallery"), ("ContosoApp", other) })
        {
            var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-UsageTarget-{Guid.NewGuid():N}.sqlite");
            try
            {
                using (var writer = SessionWriter.Create(path, new(42, process, DateTime.FromFileTimeUtc(100), 1000, 0,
                    SessionCoverage.Trace)))
                {
                    writer.Process(1, new(1, 1, 42, 7, XamlProvider.Xaml, true, "Event", 0, []));
                }

                foreach (string[] command in new[] { new[] { "info", path }, ["dump", path] })
                {
                    var usage = new CommandUsage();
                    using var output = new StringWriter();
                    using var error = new StringWriter();
                    Expect(CliApplication.Run(command, output, error, default, usage) == 0 && usage.Input == "session" &&
                        usage.App?.Invoke() == app, $"{command[0]} must report a session file and its app {app}: {error}");
                }
            }
            finally
            {
                foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
            }
        }

        Console.WriteLine("Passed: usage events name a trace's process or a live app's executable only when it is a known " +
            "Microsoft app, and CLI commands report the session file they read and its app.");
    }

    private static void Expect(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }
}
