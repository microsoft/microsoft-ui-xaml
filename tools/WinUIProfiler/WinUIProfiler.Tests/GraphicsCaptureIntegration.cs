// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Data.Sqlite;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

[SupportedOSPlatform("windows")]
internal static class GraphicsCaptureIntegration
{
    internal static void Run()
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-GraphicsCapture-{Guid.NewGuid():N}.sqlite");
        var counts = new ConcurrentDictionary<int, int>();
        try
        {
            using (var writer = SessionWriter.Create(path,
                new(Environment.ProcessId, "GraphicsCaptureIntegration", null, Stopwatch.Frequency, Stopwatch.GetTimestamp(), SessionCoverage.Connection)))
            {
                var name = $"WinUIProfiler-GraphicsSmoke-{Environment.ProcessId}-{Guid.NewGuid():N}";
                using var session = new TraceEventSession(name) { StopOnDispose = true };
                long sequence = 0;
                var lostByEtw = 0;
                Exception? readerFailure = null;
                XamlEventSubscription.Subscribe(session.Source, Environment.ProcessId, [], data =>
                {
                    if (data.Provider == XamlProvider.Direct3D11 && data.EventId is { } id)
                        counts.AddOrUpdate(id, 1, (_, value) => value + 1);
                    writer.Process(++sequence, data);
                });
                session.EnableProvider(XamlProviders.Direct3D11, TraceEventLevel.Verbose, 0x6);
                session.EnableProvider(XamlProviders.DxgKrnl, TraceEventLevel.Verbose, 0x44);
                var reader = new Thread(() =>
                {
                    try { session.Source.Process(); }
                    catch (Exception error) { readerFailure = error; }
                }) { IsBackground = true, Name = "Graphics integration ETW reader" };
                reader.Start();
                try
                {
                    ExerciseDirect3D();
                    var timeout = Stopwatch.StartNew();
                    while (timeout.Elapsed < TimeSpan.FromSeconds(10))
                    {
                        session.Flush();
                        if (new[] { 4, 7, 10, 13, 16 }.All(counts.ContainsKey)) break;
                        Thread.Sleep(100);
                    }
                }
                finally
                {
                    lostByEtw = session.EventsLost;
                    session.Stop();
                    if (!reader.Join(TimeSpan.FromSeconds(10)))
                    {
                        session.Source.StopProcessing();
                        if (!reader.Join(TimeSpan.FromSeconds(10)))
                            throw new TimeoutException("The graphics ETW consumer did not stop.");
                    }
                }
                if (readerFailure is not null) throw new InvalidOperationException("Graphics ETW decoding failed.", readerFailure);
                writer.Close("Graphics integration complete", lostByEtw, droppedByProfiler: 0);
                if (writer.Failure is { } failure) throw new InvalidOperationException("Graphics ETW recording failed.", failure);
            }
            using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
            connection.Open();
            Expect(Enumerable.Range(3, 15).Where(id => (id - 3) % 3 != 2).All(counts.ContainsKey),
                "The installed provider must deliver real device, buffer and Texture1D/2D/3D Start and Stop events.");
            Expect(Enumerable.Range(20, 6).All(counts.ContainsKey),
                "The installed provider must deliver the D3D11 core, KMD and UMD device-creation phases.");
            Expect(Sql.Scalar(connection, """
                SELECT COUNT(*) FROM GraphicsLifetimes
                WHERE Kind IN ('Device', 'Buffer', 'Texture1D', 'Texture2D', 'Texture3D') AND StartKnown AND StopKnown
                """) >= 5, "Real Direct3D events must produce complete processed lifetimes.");
            var allocations = Sql.Scalar(connection,
                "SELECT COUNT(*) FROM GraphicsObjects WHERE Kind IN ('DeviceAllocation', 'AdapterAllocation')");
            var links = Sql.Scalar(connection,
                "SELECT COUNT(*) FROM GraphicsResourceAllocations WHERE AdapterAllocationId IS NOT NULL");
            if (allocations == 0 || links == 0)
            {
                using var details = connection.CreateCommand();
                details.CommandText = """
                    SELECT Provider, Name, EventId, Fields FROM EventDetails
                    WHERE Provider = 'DxgKrnl' OR (Provider = 'Direct3D11' AND EventId IN (3, 6, 9, 12, 15))
                    ORDER BY Seq LIMIT 24
                    """;
                using var rows = details.ExecuteReader();
                while (rows.Read())
                    Console.WriteLine($"{rows.GetString(0)} {rows.GetString(1)} ({rows.GetInt32(2)}): {rows.GetString(3)}");
            }
            Expect(allocations > 0 && links > 0,
                $"Real DxgKrnl allocation payloads must map to runtime resources and adapter allocations (allocations: {allocations}, links: {links}).");
            Expect(Sql.Scalar(connection, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0,
                "Live graphics events must preserve every database reference.");
            Console.WriteLine($"Passed: live D3D11 device/resource lifetimes; kernel allocations: {allocations}; complete allocation chains: {links}.");
            Console.WriteLine("Captured D3D11 IDs: " + string.Join(", ", counts.OrderBy(pair => pair.Key).Select(pair => $"{pair.Key} ({pair.Value})")));
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }

    private static void ExerciseDirect3D()
    {
        var result = D3D11CreateDevice(0, 1, 0, 0, 0, 0, 7, out var device, out _, out var context);
        if (result < 0)
        {
            Console.WriteLine($"Hardware device unavailable (0x{result:x8}); using WARP.");
            result = D3D11CreateDevice(0, 5, 0, 0, 0, 0, 7, out device, out _, out context);
        }
        Marshal.ThrowExceptionForHR(result);
        List<nint> resources = [];
        try
        {
            // ID3D11Device slots 3-6 create Buffer and Texture1D/2D/3D; descriptors contain 32-bit fields.
            resources.Add(Create(device, 3, 1024, 0, 1, 0, 0, 0));
            resources.Add(Create(device, 4, 1024, 1, 1, 28, 0, 8, 0, 0));
            resources.Add(Create(device, 5, 256, 256, 1, 1, 28, 1, 0, 0, 8, 0, 0));
            resources.Add(Create(device, 6, 16, 16, 16, 1, 28, 0, 8, 0, 0));
            // Private allocations can have no KMT resource handle. A shared texture exercises the explicit bridge.
            resources.Add(Create(device, 5, 256, 256, 1, 1, 28, 1, 0, 0, 8, 0, 2));
        }
        finally
        {
            foreach (var resource in resources) Marshal.Release(resource);
            Marshal.Release(context);
            Marshal.Release(device);
        }
    }

    private static nint Create(nint device, int slot, params int[] description)
    {
        var pointer = Marshal.AllocHGlobal(description.Length * sizeof(int));
        try
        {
            Marshal.Copy(description, 0, pointer, description.Length);
            var create = Marshal.GetDelegateForFunctionPointer<CreateResource>(
                Marshal.ReadIntPtr(Marshal.ReadIntPtr(device), slot * nint.Size));
            Marshal.ThrowExceptionForHR(create(device, pointer, 0, out var resource));
            return resource;
        }
        finally { Marshal.FreeHGlobal(pointer); }
    }

    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int CreateResource(nint device, nint description, nint initialData, out nint resource);

    [DllImport("d3d11.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int D3D11CreateDevice(nint adapter, uint driverType, nint software, uint flags,
        nint featureLevels, uint featureLevelCount, uint sdkVersion, out nint device, out uint selectedLevel, out nint context);
}
