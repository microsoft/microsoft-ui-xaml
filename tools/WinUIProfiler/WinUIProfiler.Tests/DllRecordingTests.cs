// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Launch;
using WinUIProfiler.Engine.Storage;

internal static class DllRecordingTests
{
    internal static void Run()
    {
        foreach (var provider in new[] { XamlProvider.KernelProcess, XamlProvider.KernelImage })
        {
            var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Dlls-{Guid.NewGuid():N}.sqlite");
            try
            {
                using (var writer = SessionWriter.Create(path, new(1, "Dlls", null, 1000, 100, SessionCoverage.Connection)))
                {
                    long seq = 0;
                    void Image(long qpc, string name, bool load = true, ulong address = 0x1000) => writer.Process(++seq,
                        new(qpc, 0, 1, 1, provider, false, load ? "ImageLoad" : "ImageUnload",
                            provider == XamlProvider.KernelImage ? load ? 10 : 2 : 0,
                            [new("ImageName", name), new("ImageBase", address), new("ImageSize", 0x1000u)],
                            EventId: provider == XamlProvider.KernelProcess ? load ? 5 : 6 : null));
                    void Fault(long qpc, ulong address) => writer.Process(++seq,
                        new(qpc, 0, 1, 1, XamlProvider.KernelMemory, false, "Memory/HardFault", 32, [new("VirtualAddress", address)]));

                    Image(110, @"C:\Windows\First.DLL");
                    Image(150, "Third.dll", address: 0x3000);
                    Check(SpinWait.SpinUntil(() => SessionFile.ReadDllLoads(path) is { Count: 2 } loads &&
                        loads.All(load => load.FaultCount is null), TimeSpan.FromSeconds(5)), "Live rows before fault data");

                    if (provider == XamlProvider.KernelProcess)
                    {
                        writer.MarkHardFaultCaptureEnabled();
                        Check(SpinWait.SpinUntil(() => SessionFile.ReadDllLoads(path) is { Count: 2 } loads &&
                            loads.All(load => load.FaultCount == 0), TimeSpan.FromSeconds(5)),
                            "Enabled live capture without faults shows zero before the recording closes");
                    }

                    Fault(109, 0x1000);
                    Fault(115, 0x1000);
                    Fault(115, 0x2000);
                    Fault(127, 0x1001);
                    Image(120, @"C:\Windows\First.DLL", load: false);
                    Image(130, "Second.dll");
                    Fault(130, 0x1000);
                    Fault(135, 0x1001);
                    Image(111, "Ignored.exe", address: 0x4000);
                    Fault(140, 0x4100);
                }

                var rows = SessionFile.ReadDllLoads(path);
                Check(rows is [ { DllPath: "Second.dll", LoadTimeMilliseconds: 30, FaultCount: 2 },
                    { DllPath: @"C:\Windows\First.DLL", LoadTimeMilliseconds: 10, FaultCount: 1 },
                    { DllPath: "Third.dll", LoadTimeMilliseconds: 50, FaultCount: 0 } ],
                    "Saved DLL loads sorted by descending fault count, respecting out-of-order arrival, reuse and address limits");

                using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
                connection.Open();
                Sql.Execute(connection, "DROP VIEW DllLoads");
                Check(SessionFile.ReadDllLoads(path) is null, "Older session without the DLL view");
            }
            finally
            {
                File.Delete(path);
            }
        }

        MixedSourcesAndChangeToken();
        LargeRecording();
        SpanEventsExcludeKernelEvents();
        HeldFaults();
        KernelProcessMarker();
        Console.WriteLine("Passed: live and saved DLL loads, hard-fault attribution, and older session compatibility.");
    }

    private static string NewPath() => Path.Combine(Path.GetTempPath(), $"WinUIProfiler-Dlls-{Guid.NewGuid():N}.sqlite");

    private static ProfilerEvent ImageEvent(XamlProvider provider, long qpc, string name, ulong imageBase, bool load = true,
        int threadId = 1) =>
        new(qpc, 0, 1, threadId, provider, false, load ? "ImageLoad" : "ImageUnload",
            provider == XamlProvider.KernelImage ? load ? 10 : 2 : 0,
            [new("ImageName", name), new("ImageBase", imageBase), new("ImageSize", 0x1000u)],
            EventId: provider == XamlProvider.KernelProcess ? load ? 5 : 6 : null);

    private static ProfilerEvent FaultEvent(long qpc, ulong address, int processId = 1, int threadId = 1) =>
        new(qpc, 0, processId, threadId, XamlProvider.KernelMemory, false, "Memory/HardFault", 32, [new("VirtualAddress", address)]);

    // A trace with both kinds of image events lists each mapping once, and keeps mappings only one kind saw.
    private static void MixedSourcesAndChangeToken()
    {
        var path = NewPath();
        try
        {
            using (var writer = SessionWriter.Create(path, new(1, "Dlls", null, 1000, 100, SessionCoverage.Connection)))
            {
                long seq = 0;
                writer.Process(++seq, ImageEvent(XamlProvider.KernelImage, 110, "A.dll", 0x1000));
                writer.Process(++seq, ImageEvent(XamlProvider.KernelProcess, 111, "A.dll", 0x1000));
                writer.Process(++seq, ImageEvent(XamlProvider.KernelProcess, 120, "B.dll", 0x5000));
                writer.Process(++seq, FaultEvent(115, 0x1010));
                writer.Process(++seq, FaultEvent(125, 0x5010));
                Check(SpinWait.SpinUntil(() => SessionFile.ReadDllLoads(path) is { Count: 2 }, TimeSpan.FromSeconds(5)) &&
                    SpinWait.SpinUntil(() => SessionFile.ReadDllChangeToken(path)?.Contains(",5,") == true, TimeSpan.FromSeconds(5)),
                    "Mixed-source rows while recording");
                var token = SessionFile.ReadDllChangeToken(path);
                Check(token is not null && SessionFile.ReadDllChangeToken(path) == token, "Change token is stable without new DLL events");
                writer.Process(++seq, FaultEvent(126, 0x5020));
                Check(SpinWait.SpinUntil(() => SessionFile.ReadDllChangeToken(path) != token, TimeSpan.FromSeconds(5)),
                    "Change token changes when a fault is stored");
            }

            var rows = SessionFile.ReadDllLoads(path);
            Check(rows is [{ DllPath: "B.dll", FaultCount: 2 }, { DllPath: "A.dll", LoadTimeMilliseconds: 10, FaultCount: 1 }],
                "Each mapping listed once, from either kind of image event");
        }
        finally
        {
            File.Delete(path);
        }
    }

    // 500 mappings and 100,000 faults: the view counts each mapping's faults through the address index.
    private static void LargeRecording()
    {
        const int Mappings = 500, Faults = 100_000;
        var path = NewPath();
        try
        {
            using (var writer = SessionWriter.Create(path, new(1, "Dlls", null, 1000, 100, SessionCoverage.Trace)))
            {
                long seq = 0;
                for (var i = 0; i < Mappings; i++)
                {
                    writer.Process(++seq, ImageEvent(XamlProvider.KernelProcess, 100 + i, $"D{i}.dll", 0x10000ul * (ulong)(i + 1)));
                }

                for (var i = 0; i < Faults; i++)
                {
                    writer.Process(++seq, FaultEvent(1000 + i, 0x10000ul * (ulong)(i % Mappings + 1) + (ulong)(i % 0x1000)));
                }
            }

            var stopwatch = Stopwatch.StartNew();
            var rows = SessionFile.ReadDllLoads(path);
            stopwatch.Stop();
            Check(rows is { Count: Mappings } && rows.Sum(row => row.FaultCount) == Faults, "Every fault counted once");
            Check(stopwatch.Elapsed < TimeSpan.FromSeconds(2), $"DllLoads read took {stopwatch.Elapsed.TotalMilliseconds:N0} ms");
            Console.WriteLine($"DllLoads: {Mappings} mappings, {Faults} faults read in {stopwatch.Elapsed.TotalMilliseconds:N0} ms.");
        }
        finally
        {
            File.Delete(path);
        }
    }

    // Live hard faults come from another reader, so a late one can be stored inside a later region's events.
    private static void SpanEventsExcludeKernelEvents()
    {
        var path = NewPath();
        try
        {
            using (var writer = SessionWriter.Create(path, new(1, "Dlls", null, 1000, 100, SessionCoverage.Connection)))
            {
                writer.Process(1, new(101, 0, 1, 1, XamlProvider.Xaml, true, "Layout/Start", 1, []));
                writer.Process(2, FaultEvent(50, 0x1000));
                writer.Process(3, ImageEvent(XamlProvider.KernelProcess, 60, "A.dll", 0x1000));
                writer.Process(4, new(102, 0, 1, 1, XamlProvider.Xaml, true, "Layout/Stop", 2, []));
            }

            Check(SessionFile.CountSpanEvents(path, 1, 1, 4, null, null) == 2, "A region's events leave out kernel DLL events");
            using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
            connection.Open();
            Check(Sql.Scalar(connection, "SELECT COUNT(*) FROM HardFaults") == 1, "The fault is still stored for the DLLs tab");
        }
        finally
        {
            File.Delete(path);
        }
    }

    // A fault whose process is unknown waits for its thread's process.
    private static void HeldFaults()
    {
        var delivered = new List<ProfilerEvent>();
        var faults = new DllEventSubscription((processId, _) => processId == 7, delivered.Add, capacity: 3);
        faults.OnFault(-1, 10, 1, 0, FaultEvent(1, 0x1000, -1, 10));
        faults.OnFault(-1, 11, 2, 0, FaultEvent(2, 0x1000, -1, 11));
        faults.OnFault(7, 12, 3, 0, FaultEvent(3, 0x1000, 7, 12));
        faults.OnFault(-1, 13, 4, 0, FaultEvent(4, 0x1000, -1, 13));
        faults.OnFault(-1, 14, 5, 0, FaultEvent(5, 0x1000, -1, 14));
        Check(delivered is [{ TimestampQpc: 3 }] && faults.HeldCount == 3 && faults.Unattributed == 1, "Faults held past capacity");

        faults.Learn(10, 7);
        faults.Learn(11, 8);
        // Thread 13 started at 10 ms; its fault at 4 ms was an earlier thread's with the same ID.
        faults.Learn(13, 7, fromMilliseconds: 10);
        Check(delivered is [{ TimestampQpc: 3 }, { TimestampQpc: 1, ProcessId: 7 }] && faults.HeldCount == 0 &&
            faults.Unattributed == 2, "Held faults released to the target once their thread's process is known");
    }

    // A marker configuration can still list Kernel-Process, and the image events are enabled with its settings.
    private static void KernelProcessMarker()
    {
        var config = MarkerConfig.Parse("""
            {
              "version": 1,
              "providers": [{ "name": "Microsoft-Windows-Kernel-Process", "guid": "22fb2cd6-0e7b-422b-a0c7-2fad1fd0e716",
                "level": 5, "keywords": "0x10" }],
              "milestones": [{ "label": "Process started", "provider": "Microsoft-Windows-Kernel-Process", "event": "ProcessStart" }]
            }
            """);
        Check(config.Providers is [{ Provider.Name: "Microsoft-Windows-Kernel-Process" }], "Kernel-Process marker provider");
        Check(LiveEtwSource.ImageSettings(config.Providers) is (5, 0x50, null), "Marker settings merged with the image keyword");
        Check(LiveEtwSource.ImageSettings([]) is (4, 0x40, [5, 6]), "Image events only, without a Kernel-Process marker");
    }

    internal static async Task CaptureAsync()
    {
        var directory = Directory.CreateTempSubdirectory("WinUIProfiler-DllCapture-");
        try
        {
            using var etw = LiveEtwSource.StartBeforeLaunch(["where.exe"], [], []);
            using var kernel = KernelSampleSession.Start();
            var start = Stopwatch.GetTimestamp();
            using var child = Process.Start(new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "where.exe"), "where.exe")
            {
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true
            }) ?? throw new InvalidOperationException("Could not start the DLL capture target.");
            using var group = new ProcessRecordingGroup([new(child.Id, "where", null, Stopwatch.Frequency, start,
                SessionCoverage.ProcessStart)], child.Id, null, directory.FullName);
            var recordingPath = group.Primary.Recording?.Path ?? throw new InvalidOperationException(group.RecordingError);
            try
            {
                etw.StartReading(child.Id, null, group.Post);
                kernel.StartReading(group.IsRecorded, group.PostSample, group.Post);
                group.MarkHardFaultCaptureEnabled();
                await child.StandardOutput.ReadToEndAsync();
                await child.WaitForExitAsync();
                etw.Dispose();
                kernel.Stop();
                group.Close("Target exited", etw.EventsLost);
                Check(etw.Failure is null && kernel.Failure is null && group.Failure is null, "ETW readers and recording pipeline");
                var rows = SessionFile.ReadDllLoads(recordingPath);
                if (rows is null || !rows.Any(row => row.DllPath.EndsWith(@"\ntdll.dll", StringComparison.OrdinalIgnoreCase)))
                {
                    throw new InvalidOperationException("Expected real DLL loads captured before process launch and saved to SQLite.");
                }

                Check(rows.All(row => row.FaultCount >= 0), "Live capture reports counts even when no hard faults occurred");

                Console.WriteLine($"Captured {rows.Count} live DLL loads ({rows.Count(row => row.FaultCount.HasValue)} with hard-fault data).");
            }
            finally
            {
                etw.Dispose();
                kernel.Stop();
                group.Close("Disconnected");
                File.Delete(recordingPath);
                File.Delete(SessionFile.CpuDatabasePath(recordingPath));
            }
        }
        finally
        {
            directory.Delete();
        }
    }

    private static void Check(bool condition, string description)
    {
        if (!condition) throw new InvalidOperationException(description);
    }
}
