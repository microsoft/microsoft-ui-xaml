// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Cli;

internal static class CliApplication
{
    private const int DefaultDumpLimit = 1000;

    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true, Converters = { new JsonStringEnumConverter() } };

    private const string Usage = """
        Usage:
          WinUIProfiler.Cli processes <trace.etl | session.sqlite> [--markers <file>]
          WinUIProfiler.Cli summary <trace.etl | session.sqlite> [--pid <pid>] [--markers <file>]
          WinUIProfiler.Cli dump <trace.etl | session.sqlite> [--pid <pid>] [--name <text>] [--max <count>] [--markers <file>]
          WinUIProfiler.Cli record <trace.etl> --pid <pid> --out <session.sqlite> [--limit-mb <size>] [--markers <file>]
          WinUIProfiler.Cli launch <trace.etl | session.sqlite> [--pid <pid>] [--markers <file>]
          WinUIProfiler.Cli objects <session.sqlite> [--kind element|visual] [--type <text>] [--name <text>]
          WinUIProfiler.Cli timeline <session.sqlite> (--object <id> | --address <address>)
          WinUIProfiler.Cli frames <session.sqlite> [--frame <id> [--bucket Xaml|Composition|Other] [--filter <text>]]
                                                    [--list [--show interesting|slow]] [--json]
          WinUIProfiler.Cli interesting <session.sqlite> [--region <id>] [--json]
          WinUIProfiler.Cli composition <session.sqlite> [--view summary|lifetimes|links] [--json]

          processes  Counts the events of each process with XAML events, to find the target PID.
                     A session file shows its process.
          summary    Counts one process's events by provider and event.
          dump       Prints one process's events in order. --name keeps events whose
                     name contains <text>; --max limits the output (default 1000, 0 = all).
          record     Writes one process's events from a trace to a new session file, as the
                     app does while connected. --limit-mb is the size limit (default 500).
          launch     Prints the process's launch timeline: the milestones of the marker configuration
                     it reached, in milliseconds from its creation, or from an earlier milestone of the
                     process that launched it, and those it did not reach.
          objects    Lists the UI elements and composition visuals in a session file. --kind keeps one kind;
                     --type and --name filter them (a visual's name is its owner element's).
          timeline   Prints the events that mention one object, marking the field that holds it.
          frames     Lists the ticks per UI thread, one row each (the Ticks view). --frame prints one tick's events:
                     --bucket keeps one bucket; --filter keeps events whose name or a field value contains <text>
                     (a number matches a field with exactly that value). --list prints the Frames tab's list instead
                     (the FrameList view); its ID is a tick's ID, a region's ID or another row's start Seq. --show
                     keeps what the tab's Show filter keeps.
          interesting  Lists the regions of interest (see the README) in file order, with what started each
                     one, and the ticks, layout passes, operations, navigations and focus changes in them (the
                     InterestingRows view). --region keeps one region (an ID from 'frames --list') and its rows.

          composition  Reports retained native Composition lifetime GUIDs, independently of WinUI factory counters.
                     The default is summary with capture health; --view lifetimes lists GUID endpoints and
                     --view links lists WinUI public-interface observations and their correlation status.
                     Outstanding observed creations are not total live objects or proof of leaks.

          --json prints the rows of frames, interesting and composition as JSON, with the views' values.
          The views are in the session file: any SQLite reader gets the same rows (see the README).

          Events are the events of the XAML providers (Microsoft-Windows-XAML, -XAML-Diagnostics
          and -XAML-Profiler, Microsoft.UI.Xaml and Microsoft.UI.Xaml.Controls), the kernel's
          GPU memory usage and budget events (DxgKrnl), its window focus events (Win32k),
          CompositionObjectCreated/Destroyed (DComp) when the runtime emits them, the
          marker providers' events of the process, and the launcher providers' events of the
          process that launched it that are part of its launch, which the commands mark as the
          launcher's. A trace needs --pid; a session file holds one process.

          --markers names the marker configuration file: the marker providers read from a trace,
          and the launch milestones; the command stops when it is not valid. Without it,
          %LOCALAPPDATA%\WinUIProfiler\markers.json is used when it exists and is valid (otherwise a
          warning says why), or else the default configuration. See "Launch timeline" in the README.
        """;

    // Markers is the marker configuration, read for the commands that need it.
    private sealed record Options(string Command, string InputPath, int? ProcessId, string? NameFilter, int Max,
        string? OutputPath, long LimitBytes, string? TypeFilter, long? ObjectId, long? Address, long? FrameId, string? Bucket,
        string? EventFilter, ObjectKind? Kind, long? RegionId, bool Json, bool List, string? Show, string? MarkersPath,
        MarkerConfig? Markers = null, string? CompositionView = null)
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
            // A trace's marker providers are read from it; launch also needs the milestones. Read before any file is
            // written, so a configuration error leaves none behind.
            if (options.Command == "launch" || !options.IsSessionFile)
            {
                options = options with { Markers = LoadMarkers(options.MarkersPath, error) };
            }

            return options.Command switch
            {
                "processes" => Processes(options, output, cancellationToken),
                "summary" => Summary(options, output, cancellationToken),
                "record" => Record(options, output, error, cancellationToken),
                "launch" => Launch(options, output, error, cancellationToken),
                "objects" => Objects(options, output),
                "timeline" => Timeline(options, output, cancellationToken),
                "frames" => Frames(options, output, cancellationToken),
                "interesting" => Interesting(options, output),
                "composition" => Composition(options, output),
                _ => Dump(options, output, cancellationToken)
            };
        }
        catch (OperationCanceledException)
        {
            error.WriteLine("Cancelled.");
            return 4;
        }
        catch (MarkerConfigException exception)
        {
            error.WriteLine(exception.Message);
            return 3;
        }
        catch (Exception exception) when (exception is not OutOfMemoryException)
        {
            error.WriteLine($"Could not read '{options.InputPath}': {exception.Message}");
            return 3;
        }
    }

    private static Options Parse(string[] args)
    {
        if (args.Length < 2 || args[0] is not ("processes" or "summary" or "dump" or "record" or "launch" or "objects" or "timeline" or
            "frames" or "interesting" or "composition"))
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
        ObjectKind? kind = null;
        long? regionId = null;
        var json = false;
        var list = false;
        string? show = null;
        string? markersPath = null;
        string? compositionView = null;
        var limitBytes = SessionWriter.DefaultLimitBytes;
        var max = DefaultDumpLimit;
        for (var i = 2; i < args.Length; i++)
        {
            // Flags, without a value.
            if (args[i] == "--json" && args[0] is "frames" or "interesting" or "composition")
            {
                json = true;
                continue;
            }

            if (args[i] == "--list" && args[0] == "frames")
            {
                list = true;
                continue;
            }

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
                case "--kind" when args[0] == "objects":
                    kind = Enum.TryParse<ObjectKind>(value, ignoreCase: true, out var parsed) && Enum.IsDefined(parsed)
                        ? parsed : throw new ArgumentException("--kind must be element or visual.");
                    break;
                case "--object" when args[0] == "timeline":
                    objectId = long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var id) && id > 0
                        ? id : throw new ArgumentException("--object must be an object ID from 'objects'.");
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
                case "--show" when args[0] == "frames":
                    show = Array.Find(["interesting", "slow"], name => name.Equals(value, StringComparison.OrdinalIgnoreCase))
                        ?? throw new ArgumentException("--show must be interesting or slow.");
                    break;
                case "--region" when args[0] == "interesting":
                    regionId = long.TryParse(value, NumberStyles.None, CultureInfo.InvariantCulture, out var region) && region > 0
                        ? region : throw new ArgumentException("--region must be a region ID from 'frames --list'.");
                    break;
                case "--markers" when args[0] is "processes" or "summary" or "dump" or "record" or "launch":
                    markersPath = Path.GetFullPath(value);
                    break;
                case "--view" when args[0] == "composition":
                    compositionView = Array.Find(["summary", "lifetimes", "links"],
                        name => name.Equals(value, StringComparison.OrdinalIgnoreCase))
                        ?? throw new ArgumentException("--view must be summary, lifetimes or links.");
                    break;
                default:
                    throw new ArgumentException($"Unknown option {args[i]}.");
            }

            i++;
        }

        var options = new Options(args[0], Path.GetFullPath(args[1]), processId, nameFilter, max, outputPath, limitBytes,
            typeFilter, objectId, address, frameId, bucket, eventFilter, kind, regionId, json, list, show, markersPath,
            CompositionView: compositionView);
        if (options.Command is "objects" or "timeline" or "frames" or "interesting" or "composition" && !options.IsSessionFile)
        {
            throw new ArgumentException($"{args[0]} reads a session file (.sqlite); use 'record' to make one from a trace.");
        }

        if (markersPath is not null && options.IsSessionFile && options.Command != "launch")
        {
            throw new ArgumentException("--markers chooses the marker providers read from a trace; a session file holds the events recorded with it.");
        }

        if (options.Command == "timeline" && (objectId is null) == (address is null))
        {
            throw new ArgumentException("timeline requires either --object <id> or --address <address>.");
        }

        if ((bucket ?? eventFilter) is not null && frameId is null)
        {
            throw new ArgumentException("--bucket and --filter need --frame <id>.");
        }

        if (show is not null && !list)
        {
            throw new ArgumentException("--show needs --list.");
        }

        if (frameId is not null && (list || json))
        {
            throw new ArgumentException("--list and --json can't be used with --frame.");
        }

        if (options.Command is "summary" or "dump" or "record" or "launch" && !options.IsSessionFile && processId is null)
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
            var result = OfflineEtlSource.Read(options.InputPath, processId, options.Markers!.Providers,
                sink, null, cancellationToken);
            return new(result.EventsDelivered, result.EventsLost, result.SessionDuration);
        }

        var info = SessionFile.ReadInfo(options.InputPath);
        if (processId is { } requested && requested != info.ProcessId)
        {
            return new(0, info.LostByEtw ?? 0, TimeSpan.Zero);
        }

        // The session's duration is its process's: its launcher's events can be earlier than the process's first.
        double first = double.MaxValue, last = double.MinValue;
        var delivered = SessionFile.Read(options.InputPath, profilerEvent =>
        {
            if (profilerEvent.ProcessId == info.ProcessId)
            {
                first = Math.Min(first, profilerEvent.RelativeMilliseconds);
                last = Math.Max(last, profilerEvent.RelativeMilliseconds);
            }

            sink(profilerEvent);
        }, cancellationToken);
        return new(delivered, info.LostByEtw ?? 0, TimeSpan.FromMilliseconds(first > last ? 0 : last - first));
    }

    private static int Processes(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var providers = Enum.GetValues<XamlProvider>();
        var counts = new Dictionary<int, long[]>();
        var result = Read(options, null, profilerEvent =>
        {
            if (!counts.TryGetValue(profilerEvent.ProcessId, out var perProvider))
            {
                counts[profilerEvent.ProcessId] = perProvider = new long[providers.Length];
            }

            perProvider[(int)profilerEvent.Provider]++;
        }, cancellationToken);

        // In a trace, every process that uses the GPU has GPU memory events, every process with windows has focus events,
        // and any process can write a marker provider's events; only those with XAML events are listed. A session file
        // has its process's launcher's events too.
        var launcherProcessId = options.IsSessionFile ? SessionFile.ReadInfo(options.InputPath).LauncherProcessId : null;
        output.WriteLine($"{"PID",8} {"Events",10} {string.Join(" ", providers.Select(provider => $"{ShortName(provider),10}"))}");
        foreach (var (processId, perProvider) in counts
            .Where(entry => options.IsSessionFile || entry.Value.Sum() > entry.Value[(int)XamlProvider.DxgKrnl] +
                entry.Value[(int)XamlProvider.Win32k] + entry.Value[(int)XamlProvider.DComp] + entry.Value[(int)XamlProvider.Marker])
            .OrderByDescending(entry => entry.Value.Sum()))
        {
            output.WriteLine($"{processId,8} {perProvider.Sum(),10} " +
                string.Join(" ", providers.Select(provider => $"{perProvider[(int)provider],10}")) +
                (processId == launcherProcessId ? "  (launcher)" : ""));
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
        // So are providers, by ID: a marker provider can have a built-in provider's short name.
        // The launcher's events are counted apart from the process's too.
        var counts = new Dictionary<(Guid ProviderId, string Name, string Fields, bool Launcher), (string Label, long Count)>();
        double first = double.MaxValue, last = 0;
        long launcherEvents = 0;
        var processId = TargetProcessId(options);
        var result = Read(options, options.ProcessId, profilerEvent =>
        {
            var launcher = profilerEvent.ProcessId != processId;
            if (launcher)
            {
                launcherEvents++;
            }
            else
            {
                first = Math.Min(first, profilerEvent.RelativeMilliseconds);
                last = Math.Max(last, profilerEvent.RelativeMilliseconds);
            }

            var key = (profilerEvent.ProviderId, profilerEvent.Name, string.Join(",", profilerEvent.Fields.Select(field => field.Name)),
                launcher);
            counts[key] = (ShortName(profilerEvent), counts.GetValueOrDefault(key).Count + 1);
        }, cancellationToken);

        if (result.EventsDelivered == launcherEvents)
        {
            output.WriteLine($"No events from process {options.ProcessId}. Run 'processes' to list the PIDs.");
            return 0;
        }

        var events = result.EventsDelivered - launcherEvents;
        var seconds = Math.Max(0.001, (last - first) / 1000);
        output.WriteLine($"Process {processId}: {events} events over {seconds:F1} s " +
            $"({events / seconds:F0}/s); {result.EventsLost} events lost in the {(options.IsSessionFile ? "session" : "trace")}." +
            (launcherEvents == 0 ? "" : $" Its launcher's events: {launcherEvents}, marked (launcher)."));
        output.WriteLine();
        foreach (var (key, (label, count)) in counts.OrderByDescending(entry => entry.Value.Count))
        {
            output.WriteLine($"{count,9}  {label,-9} {key.Name}  [{key.Fields}]{(key.Launcher ? "  (launcher)" : "")}");
        }

        return 0;
    }

    private static int Dump(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        using var limit = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        long printed = 0;
        var processId = TargetProcessId(options);
        try
        {
            Read(options, options.ProcessId, profilerEvent =>
            {
                if (limit.IsCancellationRequested ||
                    options.NameFilter is { } filter && !profilerEvent.Name.Contains(filter, StringComparison.OrdinalIgnoreCase))
                {
                    return;
                }

                output.WriteLine(FormatEvent(profilerEvent,
                    profilerEvent.ProcessId == processId ? null : $"launcher {profilerEvent.ProcessId}"));
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

    // The process summary and dump read: a trace's needs --pid, and a session file holds one.
    private static int TargetProcessId(Options options) =>
        options.IsSessionFile ? SessionFile.ReadInfo(options.InputPath).ProcessId : options.ProcessId!.Value;

    private static int Record(Options options, TextWriter output, TextWriter error, CancellationToken cancellationToken)
    {
        // The trace's clock frequency is stored before any event, as the app stores it, so the file's times never depend
        // on how long the process ran.
        var writer = SessionWriter.Create(options.OutputPath!,
            new(options.ProcessId!.Value, null, null, OfflineEtlSource.ReadQpcFrequency(options.InputPath), null,
                SessionCoverage.Trace, options.InputPath), options.LimitBytes);
        var (events, launcherEvents) = RecordTrace(options, options.Markers!.Providers, writer, cancellationToken);
        if (writer.Failure is { } failure)
        {
            error.WriteLine($"Writing '{writer.Path}' failed: {failure.Message}");
            return 3;
        }

        output.WriteLine($"Read {events} events of process {options.ProcessId}" +
            (launcherEvents == 0 ? "" : $" and {launcherEvents} of its launcher") + " from the trace.");
        WriteSessionInfo(SessionFile.ReadInfo(writer.Path), output);
        output.WriteLine($"Session file: {writer.Path} ({writer.Status.FileBytes / 1024.0 / 1024:F1} MB)");
        return 0;
    }

    // Stores the process's events from the trace, and when and by which process it was created, with that launcher's
    // events, and closes the writer. Returns how many events of the process, and of its launcher, were stored.
    private static (long Events, long LauncherEvents) RecordTrace(Options options, IReadOnlyCollection<MarkerProviderSettings> markers,
        SessionWriter writer, CancellationToken cancellationToken)
    {
        long sequence = 0, launcherEvents = 0;
        EtlReadResult? result = null;
        try
        {
            result = OfflineEtlSource.Read(options.InputPath, options.ProcessId, markers, profilerEvent =>
            {
                if (profilerEvent.ProcessId != options.ProcessId)
                {
                    launcherEvents++;
                }

                writer.Process(++sequence, profilerEvent);
            }, writer.SetProcessStart, cancellationToken);
        }
        finally
        {
            writer.Close(result is null ? "Reading the trace failed" : "End of trace", result?.EventsLost, 0);
        }

        return (sequence - launcherEvents, launcherEvents);
    }

    private static int Launch(Options options, TextWriter output, TextWriter error, CancellationToken cancellationToken)
    {
        var config = options.Markers!;
        if (options.IsSessionFile)
        {
            var info = SessionFile.ReadInfo(options.InputPath);
            if (options.ProcessId is { } requested && requested != info.ProcessId)
            {
                output.WriteLine($"The session file holds process {info.ProcessId}, not {requested}.");
                return 0;
            }

            WriteLaunch(new LaunchTimelineReader(options.InputPath, config).Read(cancellationToken), config, output);
            return 0;
        }

        // The events are stored as 'record' stores them, so the timeline is the one the app shows for the trace. The file
        // is temporary, so it has no size limit: trimming would delete the launch's events first.
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-launch-{Guid.NewGuid():N}.sqlite");
        try
        {
            var writer = SessionWriter.Create(path,
                new(options.ProcessId!.Value, null, null, OfflineEtlSource.ReadQpcFrequency(options.InputPath), null,
                    SessionCoverage.Trace, options.InputPath), long.MaxValue);
            var (events, _) = RecordTrace(options, config.Providers, writer, cancellationToken);
            if (writer.Failure is { } failure)
            {
                error.WriteLine($"Writing the temporary session file '{path}' failed: {failure.Message}");
                return 3;
            }

            if (events == 0)
            {
                output.WriteLine($"No events from process {options.ProcessId}. Run 'processes' to list the PIDs.");
                return 0;
            }

            WriteLaunch(new LaunchTimelineReader(path, config).Read(cancellationToken), config, output);
            return 0;
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" })
            {
                try
                {
                    File.Delete(file);
                }
                catch (Exception exception) when (exception is IOException or UnauthorizedAccessException)
                {
                    error.WriteLine($"Could not delete the temporary file '{file}': {exception.Message}");
                }
            }
        }
    }

    // The same in any culture, as the other commands' numbers.
    private static void WriteLaunch(LaunchTimeline timeline, MarkerConfig config, TextWriter output)
    {
        var session = timeline.Session;
        var origin = timeline.Origin switch
        {
            LaunchOrigin.ProcessCreated => "in ms from the process's creation",
            LaunchOrigin.LauncherMilestone => $"in ms from \"{timeline.OriginLabel}\", which its launcher reached before creating it",
            _ => null
        };
        output.WriteLine(string.Create(CultureInfo.InvariantCulture,
            $"Launch of {session.ProcessName ?? "the process"} (process {session.ProcessId}), ") + (origin is not null
            ? origin + (timeline.RecordingStartMilliseconds is { } recordingStart
                ? string.Create(CultureInfo.InvariantCulture,
                    $"; the recording started {Milliseconds(Math.Abs(recordingStart))} ms {(recordingStart < 0 ? "before" : "after")} it.")
                : ".")
            : "in ms from the start of the recording, since when the process was created is not known."));
        if (session.LauncherProcessId is { } launcher)
        {
            output.WriteLine($"Launched by {session.LauncherName ?? "a process"} (process {launcher}).");
        }

        output.WriteLine($"Marker configuration: {config.FilePath ?? "the default"}.");
        output.WriteLine();
        if (timeline.Milestones.Count + timeline.NotReached.Count == 0)
        {
            output.WriteLine("The marker configuration has no milestones for this process.");
        }

        var width = Math.Max("Milestone".Length, timeline.Milestones.Select(milestone => milestone.Label.Length)
            .Concat(timeline.NotReached.Select(definition => MilestoneName(definition).Length)).DefaultIfEmpty().Max());
        if (timeline.Milestones.Count > 0)
        {
            output.WriteLine($"{"Milestone".PadRight(width)}  {"Time (ms)",12} {"+Prev (ms)",12} {"Thread",7} {"Count",6}  Event");
            foreach (var milestone in timeline.Milestones)
            {
                output.WriteLine(string.Create(CultureInfo.InvariantCulture,
                    $"{milestone.Label.PadRight(width)}  {Milliseconds(milestone.Milliseconds),12} " +
                    $"{Milliseconds(milestone.SincePreviousMilliseconds),12} {milestone.ThreadId?.ToString(CultureInfo.InvariantCulture) ?? "-",7} " +
                    $"{milestone.Count,6}  {MilestoneEvent(milestone.Definition)}"));
            }
        }

        if (timeline.NotReached.Count > 0)
        {
            output.WriteLine();
            output.WriteLine("Not reached:");
            foreach (var definition in timeline.NotReached)
            {
                output.WriteLine($"{MilestoneName(definition).PadRight(width)}  {MilestoneEvent(definition)}");
            }
        }

        foreach (var note in Enum.GetValues<LaunchNotes>().Where(note => note != LaunchNotes.None && timeline.Notes.HasFlag(note)))
        {
            output.WriteLine();
            output.WriteLine("Note: " + note switch
            {
                LaunchNotes.Attached => "the profiler connected to the running process, so the milestones before the recording started are missing, as are the first frame drawn and the milestones whose event repeats, such as First frame start.",
                LaunchNotes.NoProcessStart => "when the process was created is not known; a trace has it when it has the kernel's process events.",
                _ => "the session file deleted its oldest events to stay within its size limit before they were read, so milestones among them may be missing or show a later event."
            });
        }
    }

    // The configuration a path names, which must be valid. Without one, the user's, unless it is not valid: then, as in the
    // app, a warning says why and the default is used, so a mistake in it never stops a command.
    private static MarkerConfig LoadMarkers(string? path, TextWriter error)
    {
        if (path is not null)
        {
            return MarkerConfig.Load(path);
        }

        try
        {
            return MarkerConfig.Load();
        }
        catch (MarkerConfigException exception)
        {
            error.WriteLine($"Using the default marker configuration instead of the user's: {exception.Message}");
            return MarkerConfig.Default;
        }
    }

    // A milestone named by a field's values is named by the field.
    private static string MilestoneName(MilestoneDefinition definition) => definition.Label ?? $"<{definition.LabelFrom}>";

    // A launcher milestone's event is the launcher's.
    private static string MilestoneEvent(MilestoneDefinition definition) =>
        (definition.EventDescription ?? "built in") + (definition.From == MilestoneSource.Launcher ? " (launcher)" : "");

    private static int Composition(Options options, TextWriter output)
    {
        object report = options.CompositionView switch
        {
            "lifetimes" => SessionFile.ReadCompositionLifetimes(options.InputPath),
            "links" => SessionFile.ReadCompositionReferences(options.InputPath),
            _ => SessionFile.ReadCompositionSummary(options.InputPath)
        };
        if (options.Json)
        {
            output.WriteLine(JsonSerializer.Serialize(report, JsonOptions));
            return 0;
        }

        switch (report)
        {
            case CompositionLifetimeSummary summary:
                WriteSessionInfo(summary.Session, output);
                output.WriteLine($"Capture loss: ETW {summary.Session.LostByEtw?.ToString(CultureInfo.InvariantCulture) ?? "unknown"}; " +
                    $"profiler drops {summary.Session.DroppedByProfiler?.ToString(CultureInfo.InvariantCulture) ?? "unknown"}.");
                output.WriteLine($"Created GUIDs: {summary.CreatedObjects}; destroyed GUIDs: {summary.DestroyedObjects}; " +
                    $"matched destructions: {summary.MatchedDestroyed}; outstanding observed: {summary.OutstandingObserved}.");
                output.WriteLine($"Unmatched destructions: {summary.UnmatchedDestructions}; ambiguous lifetimes: {summary.AmbiguousLifetimes}; " +
                    $"invalid timestamp order: {summary.InvalidOrderLifetimes}; malformed events: {summary.MalformedEvents}.");
                output.WriteLine($"WinUI-linked GUIDs: {summary.WinUILinkedObjects}; unmatched references: {summary.UnmatchedReferences}; " +
                    $"ambiguous references: {summary.AmbiguousReferences}.");
                if (summary.CreatedObjects + summary.DestroyedObjects == 0)
                {
                    output.WriteLine("No native lifetime events observed; provider support and total live objects are unknown.");
                }
                break;
            case IReadOnlyList<CompositionLifetime> lifetimes:
                output.WriteLine($"{"Lifetime GUID",-36} {"Status",-25} {"Created ms",12} {"Destroyed ms",12} {"Public pointer",-18} Type");
                foreach (var lifetime in lifetimes)
                {
                    output.WriteLine($"{lifetime.ObjectId} {lifetime.Status,-25} {Milliseconds(lifetime.CreatedMilliseconds),12} " +
                        $"{Milliseconds(lifetime.DestroyedMilliseconds),12} {lifetime.PublicInterfacePointer ?? "-",-18} {lifetime.TypeName}");
                }
                break;
            case IReadOnlyList<CompositionWinUIReference> references:
                output.WriteLine($"{"Seq",8} {"Field",-18} {"Public pointer",-18} {"Status",-10} {"Candidates",10} Lifetime GUID");
                foreach (var reference in references)
                {
                    output.WriteLine($"{reference.Sequence,8} {reference.Field,-18} {reference.Address,-18} " +
                        $"{reference.Status,-10} {reference.Candidates,10} {reference.ObjectId}");
                }
                break;
        }

        output.WriteLine("Retained observations only, not a rundown, ownership, GPU retirement or proof of leaks. " +
            "Missing events, trimming and process exit can leave incomplete lifetimes.");
        return 0;
    }

    private static int Objects(Options options, TextWriter output)
    {
        var all = SessionFile.ReadObjects(options.InputPath, options.Kind);
        var selected = all.Where(tracked =>
            (options.TypeFilter is not { } type || tracked.Type?.Contains(type, StringComparison.OrdinalIgnoreCase) == true) &&
            (options.NameFilter is not { } name || tracked.Name?.Contains(name, StringComparison.OrdinalIgnoreCase) == true)).ToList();
        output.WriteLine($"{"ID",7}  {"Kind",-7} {"Type",-32} {"Name",-24} {"Address",-14} {"First (ms)",12} {"Last (ms)",12} {"Events",7}");
        foreach (var tracked in selected)
        {
            output.WriteLine($"{tracked.ObjectId,7}  {tracked.Kind,-7} {ShortType(tracked.Type),-32} {tracked.Name ?? "",-24} " +
                $"0x{tracked.Address,-12:X} {tracked.FirstMilliseconds,12:F3} {tracked.LastMilliseconds,12:F3} {tracked.Events,7}");
        }

        output.WriteLine($"{selected.Count} of {all.Count} objects.");
        return 0;
    }

    private static int Timeline(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        var selected = SessionFile.ReadObjects(options.InputPath, objectId: options.ObjectId, address: options.Address);
        if (selected.Count == 0)
        {
            output.WriteLine(options.ObjectId is { } id ? $"No object with ID {id}." : $"No object at 0x{options.Address:X}.");
            return 0;
        }

        foreach (var tracked in selected)
        {
            output.WriteLine($"{(tracked.Kind == ObjectKind.Visual ? "Visual" : "UI element")} {tracked.ObjectId}: " +
                $"{tracked.Type ?? "type not seen"}" + (tracked.Name is null ? "" : $" \"{tracked.Name}\"") +
                $" at 0x{tracked.Address:X}, {tracked.Events} events");
            SessionFile.ReadTimeline(options.InputPath, tracked.ObjectId, 0, -1,
                (profilerEvent, fields, frame) => output.WriteLine(FormatEvent(profilerEvent,
                    string.Join(", ", fields) + (frame is null ? "" : $", frame {frame}"))), cancellationToken);
            output.WriteLine();
        }

        return 0;
    }

    private static int Frames(Options options, TextWriter output, CancellationToken cancellationToken)
    {
        if (options.List)
        {
            return FrameList(options, output);
        }

        var frames = SessionFile.ReadFrames(options.InputPath, options.FrameId);
        if (options.Json)
        {
            output.WriteLine(JsonSerializer.Serialize(frames, JsonOptions));
            return 0;
        }

        if (options.FrameId is not { } frameId)
        {
            output.WriteLine($"{"ID",7} {"Thread",7} {"Frame",7} {"Start (ms)",12} {"End (ms)",12} {"Tick (ms)",10} {"Drew",5} " +
                $"{"Events",7} {"Xaml",6} {"Comp",6} {"Other",6}");
            foreach (var frame in frames.OrderBy(frame => frame.ThreadId).ThenBy(frame => frame.FrameId))
            {
                output.WriteLine($"{frame.FrameId,7} {frame.ThreadId,7} {frame.FrameNumber,7} {Milliseconds(frame.StartMilliseconds),12} " +
                    $"{Milliseconds(frame.EndMilliseconds),12} {Milliseconds(frame.TickMilliseconds),10} {Drew(frame.Drawn),5} " +
                    $"{frame.Events,7} {frame.Xaml,6} {frame.Composition,6} {frame.Other,6}");
            }

            output.WriteLine($"{frames.Count} ticks on {frames.Select(frame => frame.ThreadId).Distinct().Count()} UI thread(s); " +
                $"{frames.Count(frame => frame.Drawn == true)} drew a frame. Drew is ? for a thread's latest tick.");
            return 0;
        }

        if (frames.Count == 0)
        {
            output.WriteLine($"No tick with ID {frameId}.");
            return 0;
        }

        var selected = frames[0];
        output.WriteLine($"Frame {selected.FrameNumber} on thread {selected.ThreadId}, tick from {Milliseconds(selected.StartMilliseconds)} ms " +
            $"to {Milliseconds(selected.EndMilliseconds)} ms, drew: {Drew(selected.Drawn)}. {selected.Events} events " +
            $"({selected.Xaml} Xaml, {selected.Composition} Composition, {selected.Other} Other)");
        SessionFile.ReadFrameEvents(options.InputPath, frameId, options.Bucket, options.EventFilter, 0, -1,
            (profilerEvent, bucket, _) => output.WriteLine(FormatEvent(profilerEvent, bucket)), cancellationToken);
        return 0;
    }

    // The Frames tab's list, as the Show filter keeps it. ID is a tick's ID (frames --frame) or a region's (interesting
    // --region).
    private static int FrameList(Options options, TextWriter output)
    {
        var items = SessionFile.ReadFrameList(options.InputPath)
            .Where(item => options.Show switch { "interesting" => item.InInteresting, "slow" => item.InSlow, _ => true }).ToList();
        if (options.Json)
        {
            output.WriteLine(JsonSerializer.Serialize(items, JsonOptions));
            return 0;
        }

        output.WriteLine($"{"Item",6} {"Kind",-6} {"ID",7} {"Thread",7} {"Frame",7} {"Start (ms)",12} {"Stop (ms)",12} {"Duration",10} " +
            $"{"Drew",5} {"Slow",5}  Name");
        foreach (var item in items)
        {
            var (id, start, stop, duration, name) = item.Tick is { } tick
                ? (tick.FrameId, tick.StartMilliseconds, tick.EndMilliseconds, tick.TickMilliseconds,
                    item.RegionId is { } region ? $"ends region {region}" : "")
                : (item.Row!.Type == InterestingRowType.Region ? item.RegionId ?? 0 : item.Row.StartSeq, item.Row.StartMilliseconds,
                    item.Row.StopMilliseconds, item.Row.DurationMilliseconds,
                    $"{item.Row.Name}{(item.Row.Trigger is null ? "" : $" ({item.Row.Trigger})")}");
            output.WriteLine($"{item.ListIndex,6} {item.ItemType,-6} {id,7} {item.Tick?.ThreadId ?? item.Row!.ThreadId,7} " +
                $"{item.Tick?.FrameNumber.ToString(CultureInfo.InvariantCulture) ?? "",7} {Milliseconds(start),12} {Milliseconds(stop),12} " +
                $"{Milliseconds(duration),10} {(item.Tick is { } t ? Drew(t.Drawn) : ""),5} {(item.Tick?.Slow == true ? "yes" : ""),5}  {name}");
        }

        output.WriteLine($"{items.Count} items: {items.Count(item => item.ItemType == FrameListItemType.Tick)} ticks, " +
            $"{items.Count(item => item.ItemType == FrameListItemType.Region)} regions, " +
            $"{items.Count(item => item.ItemType == FrameListItemType.Row)} other interesting rows.");
        return 0;
    }

    private static string Drew(bool? drawn) => drawn switch
    {
        true => "yes",
        false => "no",
        null => "?"
    };

    private static int Interesting(Options options, TextWriter output)
    {
        // In file order, a region before the rows that start with it.
        var rows = SessionFile.ReadInterestingRows(options.InputPath, options.RegionId);
        if (options.Json)
        {
            output.WriteLine(JsonSerializer.Serialize(rows, JsonOptions));
            return 0;
        }

        output.WriteLine($"{"Start (ms)",12} {"Stop (ms)",12} {"Duration",10} {"Thread",7} {"Frame",7}  Type");
        foreach (var row in rows)
        {
            output.WriteLine($"{Milliseconds(row.StartMilliseconds),12} {Milliseconds(row.StopMilliseconds),12} " +
                $"{Milliseconds(row.DurationMilliseconds),10} {row.ThreadId,7} {row.FrameNumber?.ToString(CultureInfo.InvariantCulture) ?? "",7}  " +
                $"{row.Name}{(row.Trigger is null ? "" : $" ({row.Trigger})")}");
        }

        output.WriteLine($"{rows.Count} interesting rows, {rows.Count(row => row.Type == InterestingRowType.Region)} region(s) of interest.");
        return 0;
    }

    private static void WriteSessionInfo(SessionFileInfo info, TextWriter output)
    {
        output.WriteLine($"Stored: {info.StoredEvents} events, {info.UIElements} UI elements, {info.Visuals} visuals, {info.Frames} ticks; " +
            $"source {info.Source}, coverage {info.Coverage}; " +
            $"ended: {info.EndReason ?? "not recorded (still recording, or stopped unexpectedly)"}.");
        if (info.LauncherProcessId is { } launcher)
        {
            output.WriteLine($"Launched by {info.LauncherName ?? "a process"} (process {launcher}).");
        }

        if (info.CompleteFromSeq is { } completeFrom)
        {
            output.WriteLine($"Events before Seq {completeFrom} were dropped to stay within {info.LimitBytes / 1024 / 1024} MB.");
        }
    }

    private static string ShortName(XamlProvider provider) => provider switch
    {
        XamlProvider.Xaml => "XAML",
        XamlProvider.XamlDiagnostics => "XAML-Diag",
        XamlProvider.XamlProfiler => "XAML-Prof",
        XamlProvider.MicrosoftUIXaml => "MUX",
        XamlProvider.MicrosoftUIXamlControls => "MUXC",
        XamlProvider.Win32k => "Win32k",
        XamlProvider.DComp => "DComp",
        XamlProvider.Marker => "Marker",
        _ => "DxgKrnl"
    };

    // A marker event shows its provider's name.
    private static string ShortName(ProfilerEvent profilerEvent) => profilerEvent.Marker?.Name ?? ShortName(profilerEvent.Provider);

    private static string ShortType(string? type) => type is null ? "?" : type[(type.LastIndexOf('.') + 1)..];

    private static string Milliseconds(double? value) => value is { } milliseconds
        ? milliseconds.ToString("F3", CultureInfo.InvariantCulture) : "-";

    // tag is shown in brackets after the event name: the field that holds a timeline's element, or the event's bucket.
    private static string FormatEvent(ProfilerEvent profilerEvent, string? tag)
    {
        return $"{profilerEvent.RelativeMilliseconds,12:F3} {profilerEvent.ThreadId,6} {ShortName(profilerEvent),-9} " +
            $"{profilerEvent.Name}{(tag is null ? "" : $" [{tag}]")} {EventFormat.Fields(profilerEvent)}";
    }

    private static long? ParseAddress(string text) =>
        text.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
            ? long.TryParse(text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out var hex) ? hex : null
            : long.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out var number) ? number : null;
}
