// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using Microsoft.Data.Sqlite;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

[SupportedOSPlatform("windows")]
internal static class GraphicsCaptureIntegration
{
    internal static void RunComposition()
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CompositionCapture-{Guid.NewGuid():N}.etl");
        var storedPath = Path.ChangeExtension(path, "sqlite");
        var live = new ConcurrentQueue<ProfilerEvent>();
        try
        {
            var capture = LiveEtwSource.Start(Environment.ProcessId, [], data =>
            {
                if (data.Provider == XamlProvider.DirectComposition) live.Enqueue(data);
            });
            using (capture)
            {
                ExerciseDirectComposition();
                var timeout = Stopwatch.StartNew();
                while (timeout.Elapsed < TimeSpan.FromSeconds(10) && live.Count(data => data.EventId == 30) < 3)
                    Thread.Sleep(100);
            }
            Expect(capture.Failure is null && capture.EventsLost == 0, "Live composition capture must finish without loss or reader failure.");
            VerifyCompositionEvents(live.ToArray(), "live");

            int otherProcess;
            using (var session = new TraceEventSession($"WinUIProfiler-CompositionFilter-{Environment.ProcessId}-{Guid.NewGuid():N}", path))
            {
                Expect(LiveEtwSource.EnableDirectComposition(filtered =>
                    session.EnableProvider(XamlProviders.DirectComposition, TraceEventLevel.Verbose, 0x1, filtered),
                    new() { ProcessIDFilter = [Environment.ProcessId] }),
                    "The installed composition provider must enable with resource-event and PID filters.");
                ExerciseDirectComposition();
                var start = new ProcessStartInfo(Environment.ProcessPath!)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };
                if (string.Equals(Path.GetFileNameWithoutExtension(Environment.ProcessPath), "dotnet", StringComparison.OrdinalIgnoreCase))
                    start.ArgumentList.Add(typeof(GraphicsCaptureIntegration).Assembly.Location);
                start.ArgumentList.Add("--composition-probe");
                using var probe = Process.Start(start) ?? throw new InvalidOperationException("Could not start the composition filter probe.");
                otherProcess = probe.Id;
                if (!probe.WaitForExit(TimeSpan.FromSeconds(30)))
                {
                    probe.Kill(entireProcessTree: true);
                    probe.WaitForExit();
                    throw new TimeoutException("The composition filter probe did not exit.");
                }
                Expect(probe.ExitCode == 0,
                    $"The composition filter probe failed: {probe.StandardOutput.ReadToEnd()} {probe.StandardError.ReadToEnd()}");
                session.Stop();
            }

            var recorded = new List<ProfilerEvent>();
            var read = OfflineEtlSource.Read(path, Environment.ProcessId, [], recorded.Add);
            Expect(read.EventsLost == 0, "Recorded composition capture must have no ETW loss.");
            VerifyCompositionEvents(recorded, "recorded");
            var otherEvents = 0;
            var unrelatedIds = 0;
            using (var source = new ETWTraceEventSource(path))
            {
                source.AllEvents += data =>
                {
                    if (data.ProviderGuid != XamlProviders.DirectComposition) return;
                    if (data.ProcessID == otherProcess) otherEvents++;
                    if ((int)data.ID is not (1 or 30)) unrelatedIds++;
                };
                source.Process();
            }
            Expect(otherEvents == 0 && unrelatedIds == 0, "Composition capture must exclude other processes and unrelated event IDs.");
            Expect(OfflineEtlSource.ListProcesses(path).Processes.Count == 0,
                "Composition-only processes must not be advertised as XAML applications.");

            using (var writer = SessionWriter.Create(storedPath,
                new(Environment.ProcessId, "CompositionCaptureIntegration", null, Stopwatch.Frequency, null, SessionCoverage.Trace)))
            {
                for (var i = 0; i < recorded.Count; i++) writer.Process(i + 1, recorded[i]);
                writer.Close("Composition integration complete", read.EventsLost, droppedByProfiler: 0);
                Expect(writer.Failure is null, "Real composition events must be recorded without a writer failure.");
            }
            var restored = new List<ProfilerEvent>();
            SessionFile.Read(storedPath, restored.Add);
            Expect(restored.Count == recorded.Count && restored.Zip(recorded).All(pair =>
                pair.First.TimestampQpc == pair.Second.TimestampQpc && pair.First.ThreadId == pair.Second.ThreadId &&
                pair.First.ProcessId == pair.Second.ProcessId && pair.First.ProviderId == pair.Second.ProviderId &&
                pair.First.EventId == pair.Second.EventId && pair.First.Version == pair.Second.Version &&
                pair.First.Fields.SequenceEqual(pair.Second.Fields)),
                "Native identities, timestamps, threads and all original payloads must round-trip in recording order.");
            Expect(SessionFile.ReadObjects(storedPath).Count == 0, "Native resource pointers must not be indexed as XAML elements.");
            var lifetimes = SessionFile.ReadComposition(storedPath);
            Expect(lifetimes.MatchedReleaseObservations >= 3 &&
                lifetimes.NativeLifetimes.Count(row => row.Status == "ReleaseObserved") >= 3 &&
                lifetimes.WinUICreations.Count == 0,
                "Real native proxy releases must close their identity generations without inventing WinUI ownership.");
            using var output = new StringWriter();
            using var error = new StringWriter();
            Expect(CliApplication.Run(["dump", storedPath, "--max", "100"], output, error, CancellationToken.None) == 0 &&
                output.ToString().Contains("DComp", StringComparison.Ordinal) &&
                !output.ToString().Contains("DxgKrnl", StringComparison.Ordinal),
                $"Composition events must have their own CLI provider label: {error}");
            Console.WriteLine($"Passed: real live/recorded composition creates/releases, native payload round-trip, PID/event filtering " +
                $"and XAML-only discovery (live: {live.Count}; recorded: {recorded.Count}).");
        }
        finally
        {
            foreach (var file in new[] { path, storedPath, storedPath + "-wal", storedPath + "-shm" }) File.Delete(file);
        }
    }

    private static void VerifyCompositionEvents(IReadOnlyList<ProfilerEvent> events, string source)
    {
        Expect(events.All(data => data.ProviderId == XamlProviders.DirectComposition && data.EventId is 1 or 30 &&
            data.ProcessId == Environment.ProcessId), $"{source} capture must preserve the native event identity and owner process.");
        var creations = events.Where(data => data.EventId == 1).Select(ResourceKey).ToHashSet();
        var released = events.Where(data => data.EventId == 30).Select(ResourceKey).Count(creations.Contains);
        Expect(creations.Count >= 3 && released >= 3,
            $"{source} capture must decode at least three real proxy creation/release pairs (creates: {creations.Count}; releases: {released}).");
    }

    private static (ulong Pointer, ulong Handle) ResourceKey(ProfilerEvent data) =>
        (UnsignedField(data, "resourcePointer"), UnsignedField(data, "resourceHandle"));

    private static ulong UnsignedField(ProfilerEvent data, string name) =>
        Convert.ToUInt64(data.Fields.Single(field => string.Equals(field.Name, name, StringComparison.OrdinalIgnoreCase)).Value);

    internal static void ExerciseDirectComposition()
    {
        var initialized = CoInitializeEx(0, 0);
        if (initialized < 0 && initialized != unchecked((int)0x80010106)) Marshal.ThrowExceptionForHR(initialized);
        nint device = 0;
        List<nint> visuals = [];
        try
        {
            var iid = new Guid("c37ea93a-e7aa-450d-b16f-9746cb0407f3");
            Marshal.ThrowExceptionForHR(DCompositionCreateDevice(0, in iid, out device));
            // IDCompositionDevice::CreateVisual is slot 7, after IUnknown and the four preceding device methods.
            var create = Marshal.GetDelegateForFunctionPointer<CreateCompositionVisual>(
                Marshal.ReadIntPtr(Marshal.ReadIntPtr(device), 7 * nint.Size));
            for (var i = 0; i < 3; i++)
            {
                Marshal.ThrowExceptionForHR(create(device, out var visual));
                visuals.Add(visual);
            }
        }
        finally
        {
            foreach (var visual in visuals) Marshal.Release(visual);
            if (device != 0) Marshal.Release(device);
            if (initialized >= 0) CoUninitialize();
        }
    }

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
                Expect(LiveEtwSource.EnableGpuMemory(options =>
                    session.EnableProvider(XamlProviders.DxgKrnl, TraceEventLevel.Verbose, 0x44, options)),
                    "The live kernel integration requires allocation and memory event-ID filtering.");
                var reader = new Thread(() =>
                {
                    try { session.Source.Process(); }
                    catch (Exception error) { readerFailure = error; }
                })
                { IsBackground = true, Name = "Graphics integration ETW reader" };
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
        CaptureStateFilters(failure: false);
        CaptureStateFilters(failure: true);
    }

    private static void CaptureStateFilters(bool failure)
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-GraphicsFilter-{Guid.NewGuid():N}.etl");
        try
        {
            int otherProcess;
            long filteredFrom;
            using (var session = new TraceEventSession($"WinUIProfiler-GraphicsFilter-{Environment.ProcessId}-{Guid.NewGuid():N}", path))
            {
                var options = new TraceEventProviderOptions { ProcessIDFilter = [Environment.ProcessId] };
                session.EnableProvider(XamlProviders.Direct3D11, TraceEventLevel.Verbose, 0x6, options);
                // Establish a real unfiltered control before verifying capture-state restores the kernel ID filter.
                session.EnableProvider(XamlProviders.DxgKrnl, TraceEventLevel.Verbose, 0x44);
                ExerciseDirect3D();
                LiveEtwSource.CaptureGraphicsState((provider, keywords) =>
                {
                    session.CaptureState(provider, keywords);
                    if (failure) throw new InvalidOperationException("Capture state failed after enabling.");
                }, () => session.EnableProvider(XamlProviders.Direct3D11, TraceEventLevel.Verbose, 0x6, options),
                    () => LiveEtwSource.EnableGpuMemory(kernelOptions =>
                        session.EnableProvider(XamlProviders.DxgKrnl, TraceEventLevel.Verbose, 0x44, kernelOptions),
                        previouslyEnabled: true));
                filteredFrom = Stopwatch.GetTimestamp();
                var start = new ProcessStartInfo(Environment.ProcessPath!)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    CreateNoWindow = true
                };
                if (string.Equals(Path.GetFileNameWithoutExtension(Environment.ProcessPath), "dotnet", StringComparison.OrdinalIgnoreCase))
                    start.ArgumentList.Add(typeof(GraphicsCaptureIntegration).Assembly.Location);
                start.ArgumentList.Add("--graphics-probe");
                using var probe = Process.Start(start) ?? throw new InvalidOperationException("Could not start the graphics filter probe.");
                otherProcess = probe.Id;
                if (!probe.WaitForExit(TimeSpan.FromSeconds(30)))
                {
                    probe.Kill(entireProcessTree: true);
                    probe.WaitForExit();
                    throw new TimeoutException("The graphics filter probe did not exit.");
                }
                Expect(probe.ExitCode == 0,
                    $"The graphics filter probe failed: {probe.StandardOutput.ReadToEnd()} {probe.StandardError.ReadToEnd()}");
                ExerciseDirect3D();
                session.Stop();
            }
            var selectedEvents = 0;
            var otherEvents = 0;
            var kernelEvents = 0;
            var unrelatedKernelEvents = 0;
            var unfilteredKernelEvents = 0;
            using (var source = new ETWTraceEventSource(path))
            {
                source.AllEvents += data =>
                {
#pragma warning disable CS0618 // Compare the event's native timestamp with the filter-restoration boundary.
                    var afterRestore = data.TimeStampQPC >= filteredFrom;
#pragma warning restore CS0618
                    if (data.ProviderGuid == XamlProviders.DxgKrnl)
                    {
                        var selected = (int)data.ID is >= 33 and <= 38 or 366 or 367;
                        if (!afterRestore && !selected) unfilteredKernelEvents++;
                        if (afterRestore && selected) kernelEvents++;
                        if (afterRestore && !selected) unrelatedKernelEvents++;
                    }
                    if (data.ProviderGuid != XamlProviders.Direct3D11 || !afterRestore) return;
                    if (data.ProcessID == Environment.ProcessId) selectedEvents++;
                    if (data.ProcessID == otherProcess) otherEvents++;
                };
                source.Process();
            }
            Expect(selectedEvents > 0 && otherEvents == 0,
                $"Capture state must retain the selected PID filter (selected: {selectedEvents}; unrelated: {otherEvents}; failure: {failure}).");
            Expect(unfilteredKernelEvents > 0 && kernelEvents > 0 && unrelatedKernelEvents == 0,
                $"Capture state must restore the kernel event-ID filter (unfiltered control: {unfilteredKernelEvents}; " +
                $"selected: {kernelEvents}; unrelated: {unrelatedKernelEvents}; failure: {failure}).");
            using var output = new StringWriter();
            using var error = new StringWriter();
            Expect(CliApplication.Run(["processes", path], output, error, CancellationToken.None) == 0,
                $"The CLI must read the graphics-only trace: {error}");
            Expect(!output.ToString().Split('\n').Any(line =>
                int.TryParse(line.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries).FirstOrDefault(), out _)),
                "The CLI must not list graphics-only processes as XAML processes.");
            Console.WriteLine($"Passed: live PID/kernel event-ID filtering after {(failure ? "failed" : "successful")} capture state; " +
                $"graphics-only CLI discovery stays empty (unfiltered kernel: {unfilteredKernelEvents}; filtered kernel: {kernelEvents}).");
        }
        finally { File.Delete(path); }
    }

    internal static void ExerciseDirect3D()
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

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    private delegate int CreateCompositionVisual(nint device, out nint visual);

    [DllImport("dcomp.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int DCompositionCreateDevice(nint dxgiDevice, in Guid iid, out nint device);

    [DllImport("ole32.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int CoInitializeEx(nint reserved, uint flags);

    [DllImport("ole32.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern void CoUninitialize();

    [DllImport("d3d11.dll", ExactSpelling = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    private static extern int D3D11CreateDevice(nint adapter, uint driverType, nint software, uint flags,
        nint featureLevels, uint featureLevelCount, uint sdkVersion, out nint device, out uint selectedLevel, out nint context);
}
