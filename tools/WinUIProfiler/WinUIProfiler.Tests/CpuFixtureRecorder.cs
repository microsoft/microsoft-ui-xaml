// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Principal;
using System.Text;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Parsers;
using Microsoft.Diagnostics.Tracing.Parsers.Kernel;
using Microsoft.Diagnostics.Tracing.Parsers.Symbol;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;

// Records Fixtures\NativeSymbols\CpuProfile.etl, the CPU profile the trace import tests read, as WPR records and merges
// one: the kernel's sampled stacks of this process while a thread of its own runs the fixture DLL's FixtureAlpha, which
// calls FixtureBeta, between two Microsoft.UI.Xaml events the thread writes, with the DLL's PDB identity. Only what an
// import reads of this process is kept: its threads, samples and stacks, the DLL's image events and identity, the
// sampling interval, and the two events. No other process or image is, and no process event, which has the command line
// and the user. Run it elevated after rebuilding the DLL:
//   dotnet run --project WinUIProfiler.Tests -c Release -- --record-cpu-fixture <path of CpuProfile.etl>
internal static class CpuFixtureRecorder
{
    internal const string BeginEvent = "CpuFixtureBegin";
    internal const string EndEvent = "CpuFixtureEnd";
    private static readonly TimeSpan Running = TimeSpan.FromMilliseconds(200);

    private delegate ulong FixtureFunction(ulong iterations);

    [SupportedOSPlatform("windows")]
    internal static void Record(string output)
    {
        if (TraceEventSession.IsElevated() != true)
        {
            throw new UnauthorizedAccessException("Recording the CPU fixture needs an elevated prompt, as the kernel's sampled profile does.");
        }

        var fixture = Path.Combine(AppContext.BaseDirectory, "Fixtures", "NativeSymbols", "NativeSymbols.dll");
        // The trace has the path the DLL was loaded from, and its own file's path, so neither may name the user.
        var directory = Path.Combine(Path.GetPathRoot(Environment.SystemDirectory)!, "WinUIProfilerFixture");
        if (Directory.Exists(directory))
        {
            throw new IOException($"{directory} exists, left by an earlier recording: delete it and record again.");
        }

        Directory.CreateDirectory(directory);
        var library = IntPtr.Zero;
        using var current = Process.GetCurrentProcess();
        var affinity = current.ProcessorAffinity;
        try
        {
            // A trace file has buffers of its own for each processor that logged, so the process runs on one, its last,
            // rather than leaving a part-filled buffer for each processor its threads ran on.
            current.ProcessorAffinity = (nint)(1L << (63 - BitOperations.LeadingZeroCount((ulong)(long)affinity)));
            var dll = Path.Combine(directory, "NativeSymbols.dll");
            File.Copy(fixture, dll);
            var raw = Path.Combine(directory, "Raw.etl");
            var merged = Path.Combine(directory, "Merged.etl");
            var filtered = Path.Combine(directory, "CpuProfile.etl");
            using (var xaml = new EventSource("Microsoft.UI.Xaml", EventSourceSettings.EtwSelfDescribingEventFormat))
            {
                Check(xaml.Guid == XamlProviders.MicrosoftUIXaml, "The event source must be the Microsoft.UI.Xaml provider.");
                LiveEtwSource.StopAbandonedSessions();
                using (var session = new TraceEventSession(
                    $"{LiveEtwSource.SessionPrefix}{Environment.ProcessId}-fixture{Guid.NewGuid():N}", raw) { StopOnDispose = true })
                {
                    // As KernelSampleSession samples, with the XAML provider in the same session.
                    session.EnableKernelProvider(
                        KernelTraceEventParser.Keywords.Process | KernelTraceEventParser.Keywords.Thread |
                        KernelTraceEventParser.Keywords.ImageLoad | KernelTraceEventParser.Keywords.Profile,
                        KernelTraceEventParser.Keywords.Profile);
                    session.EnableProvider(XamlProviders.MicrosoftUIXaml, TraceEventLevel.Verbose);
                    var enabled = Stopwatch.StartNew();
                    while (!xaml.IsEnabled())
                    {
                        Check(enabled.Elapsed < TimeSpan.FromSeconds(10), "The session must enable the Microsoft.UI.Xaml event source.");
                        Thread.Sleep(10);
                    }

                    // Loaded while the session records, so the trace has its load, and run on a thread of its own, so the
                    // trace has the thread's start.
                    library = NativeLibrary.Load(dll);
                    var alpha = Marshal.GetDelegateForFunctionPointer<FixtureFunction>(NativeLibrary.GetExport(library, "FixtureAlpha"));
                    var worker = new Thread(() =>
                    {
                        xaml.Write(BeginEvent);
                        var running = Stopwatch.StartNew();
                        while (running.Elapsed < Running)
                        {
                            alpha(1_000_000);
                        }

                        xaml.Write(EndEvent);
                    })
                    {
                        Name = "CPU fixture"
                    };
                    worker.Start();
                    worker.Join();
                }
            }

            TraceEventSession.Merge([raw], merged, TraceEventMergeOptions.ImageIDsOnly);
            var processId = Environment.ProcessId;
            Filter(merged, filtered, processId);
            CheckPrivate(filtered);
            var samples = CountSamples(filtered, processId);
            Check(OfflineEtlSource.ReadQpcFrequency(filtered) is not null, "The trace must have its clock's frequency.");
            Check(OfflineEtlSource.ListProcesses(filtered) is { Processes: [{ XamlEvents: 2 } process] } && process.ProcessId == processId,
                "The trace must have this process's two XAML events, and no other process's.");
            Check(samples > 0, "The trace must have CPU samples of this process.");
            File.Copy(filtered, output, overwrite: true);
            Console.WriteLine($"Recorded {output}: {new FileInfo(output).Length} bytes, {samples} CPU samples of process {processId}.");
        }
        finally
        {
            current.ProcessorAffinity = affinity;
            if (library != IntPtr.Zero)
            {
                NativeLibrary.Free(library);
            }

            Directory.Delete(directory, recursive: true);
        }
    }

    // The trace's CPU samples of the process, counted from its events, not by the profiler's collector.
    internal static long CountSamples(string trace, int processId)
    {
        using var source = new ETWTraceEventSource(trace);
        long samples = 0;
        source.Kernel.PerfInfoSample += data =>
        {
            if (data.ProcessID == processId)
            {
                samples++;
            }
        };
        source.Process();
        return samples;
    }

    // Copies from the merged trace what an import reads of the process. A merged trace has an image's identity in events
    // at the image event's time, before or after it, so the DLL's image events are found first, and the identity events
    // at their times kept.
    private static void Filter(string merged, string output, int processId)
    {
        static bool IsFixture(ImageLoadTraceData data, int processId) => data.ProcessID == processId &&
            string.Equals(Path.GetFileName(data.FileName), "NativeSymbols.dll", StringComparison.OrdinalIgnoreCase);

        var images = new HashSet<(ulong Base, long Qpc)>();
        using (var source = new ETWTraceEventSource(merged))
        {
            source.Kernel.All += data =>
            {
                if (data is ImageLoadTraceData load && IsFixture(load, processId))
                {
                    images.Add((load.ImageBase, Qpc(load)));
                }
            };
            source.Process();
        }

        Check(images.Count > 0, "The trace must have the fixture DLL's load.");
        using var relogger = new ETWReloggerTraceEventSource(merged, output);
        bool Identifies(TraceEvent data, ulong imageBase) => images.Contains((imageBase, Qpc(data)));
        relogger.Kernel.All += data =>
        {
            bool keep;
            switch (data)
            {
                case ThreadTraceData thread:
                    keep = thread.ProcessID == processId;
                    break;
                case ImageLoadTraceData load:
                    keep = IsFixture(load, processId);
                    break;
                case SampledProfileTraceData sample:
                    keep = sample.ProcessID == processId;
                    break;
                case StackWalkStackTraceData stack:
                    keep = stack.ProcessID == processId;
                    break;
                case SampledProfileIntervalTraceData:
                    keep = true;
                    break;
                default:
                    keep = false;
                    break;
            }

            if (keep)
            {
                relogger.WriteEvent(data);
            }
        };
        var symbols = new SymbolTraceEventParser(relogger);
        symbols.ImageID += data =>
        {
            if (Identifies(data, data.ImageBase))
            {
                relogger.WriteEvent(data);
            }
        };
        symbols.ImageIDDbgID_RSDS += data =>
        {
            if (Identifies(data, data.ImageBase))
            {
                relogger.WriteEvent(data);
            }
        };
        relogger.AllEvents += data =>
        {
            if (data.ProviderGuid == XamlProviders.MicrosoftUIXaml && data.ProcessID == processId)
            {
                relogger.WriteEvent(data);
            }
        };
        relogger.Process();
    }

    // The fixture is checked in, so it must not name who recorded it, or where.
    [SupportedOSPlatform("windows")]
    private static void CheckPrivate(string trace)
    {
        var patterns = PrivatePatterns();
        var found = new List<string>();
        if (FindPrivate(File.ReadAllBytes(trace), patterns) is { } inFile)
        {
            found.Add($"{inFile} in its bytes");
        }

        // The relogger compresses the file's buffers, which hides the events' text from the search above, so each event's
        // payload is searched as well.
        using (var source = new ETWTraceEventSource(trace))
        {
            source.AllEvents += data =>
            {
                if (FindPrivate(data.EventData(), patterns) is { } inEvent)
                {
                    found.Add($"{inEvent} in a {data.ProviderName}/{data.EventName} event");
                }
            };
            source.Process();
        }

        Check(found.Count == 0, $"The trace must not contain {string.Join("; ", found.Distinct())}.");
    }

    // The first of the user's name, domain and SID, the machine's name, a path under a user profile and the
    // enlistment's path that the bytes hold, in UTF-8 or UTF-16 and any ASCII case, or null. The checked-in fixtures,
    // the trace and the DLL and PDB it records, must hold none of them.
    [SupportedOSPlatform("windows")]
    internal static string? FindPrivate(byte[] bytes) => FindPrivate(bytes, PrivatePatterns());

    private static string? FindPrivate(byte[] bytes, (string Name, byte[] Bytes)[] patterns)
    {
        var contents = Lower(bytes);
        return patterns.FirstOrDefault(pattern => contents.AsSpan().IndexOf(pattern.Bytes) >= 0).Name;
    }

    [SupportedOSPlatform("windows")]
    private static (string Name, byte[] Bytes)[] PrivatePatterns()
    {
        using var identity = WindowsIdentity.GetCurrent();
        var user = identity.User ?? throw new InvalidOperationException("The user has no SID.");
        var sid = new byte[user.BinaryLength];
        user.GetBinaryForm(sid, 0);
        string[] names = [Environment.UserName, Environment.MachineName, Environment.UserDomainName, @"\Users\", "S-1-5-21", EnlistmentRoot()];
        return [.. names.SelectMany(name => new (string Name, byte[] Bytes)[] { (name, Encoding.UTF8.GetBytes(name)), (name, Encoding.Unicode.GetBytes(name)) })
            .Append((Name: "the user's SID", Bytes: sid)).Select(pattern => (pattern.Name, Lower(pattern.Bytes)))];
    }

    // The enlistment the tests run from: the nearest directory above them with a .git, a directory, or a file in a
    // worktree. Out of one, the tool's directory, which holds the tests' project.
    private static string EnlistmentRoot()
    {
        for (var directory = new DirectoryInfo(AppContext.BaseDirectory); directory is not null; directory = directory.Parent)
        {
            var git = Path.Combine(directory.FullName, ".git");
            if (Directory.Exists(git) || File.Exists(git))
            {
                return directory.FullName;
            }
        }

        return Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", ".."));
    }

    private static byte[] Lower(byte[] bytes) => [.. bytes.Select(value => value is >= (byte)'A' and <= (byte)'Z' ? (byte)(value + 32) : value)];

    private static long Qpc(TraceEvent data)
    {
#pragma warning disable CS0618 // The raw QPC timestamp is what the merge gives an image's identity events.
        return data.TimeStampQPC;
#pragma warning restore CS0618
    }

    private static void Check(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }
}
