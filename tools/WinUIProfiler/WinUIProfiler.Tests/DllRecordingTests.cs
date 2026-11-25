// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
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

        Console.WriteLine("Passed: live and saved DLL loads, hard-fault attribution, and older session compatibility.");
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
