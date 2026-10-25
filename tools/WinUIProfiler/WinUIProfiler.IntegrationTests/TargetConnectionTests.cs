using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;
using WinUIProfiler.Engine.Processing;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

[SupportedOSPlatform("windows")]
[Collection("Native collector ETW")]
public sealed class TargetConnectionTests
{
    private static string Root => Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\.."));
    private static string AppDirectory => Path.Combine(Root,
        @"tools\WinUIProfiler\WinUIProfiler.App\bin\x64\Release\net10.0-windows10.0.26100.0\win-x64");
    private static string Executable => Path.Combine(AppDirectory, "WinUIProfiler.App.exe");
    private static string Sessions => Path.Combine(Root, @"BuildOutput\LaunchConnectionSessions");

    [Fact]
    public async Task Live_metrics_are_recorded_after_hello_and_survive_disconnect()
    {
        await using var connection = await TargetConnection.ConnectAsync(new(TargetLaunchKind.Unpackaged,
            Executable, "", AppDirectory, Sessions, Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, default);
        using var owned = Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
        try
        {
            await Task.Delay(TimeSpan.FromSeconds(6));
            var live = await connection.ReadRecentMetricsAsync().WaitAsync(TimeSpan.FromSeconds(5));
            Assert.Equal(connection.Scope, live.Context.Scope);
            Assert.Equal(5, live.Items.Select(row => row.Metric).Distinct().Count());
            var readers = Enumerable.Range(0, 4).Select(_ => connection.ReadRecentMetricsAsync()).ToArray();
            await connection.StopAsync().WaitAsync(TimeSpan.FromSeconds(25));
            await Task.WhenAll(readers);
            var final = await connection.ReadRecentMetricsAsync();
            Assert.False(owned.HasExited);
            using var saved = SessionStore.OpenReadOnly(connection.DatabasePath);
            var rows = await saved.Queries.GetMetrics(new(saved.Scope, new(0, long.MaxValue), new(512)), default);
            Assert.Equal(5, rows.Items.Select(row => row.Metric).Distinct().Count());
            Assert.Equal(rows.Context.Watermark, final.Context.Watermark);
            Assert.Equal(rows.Items.Count, final.Items.Count);
            Assert.All(rows.Items.GroupBy(row => row.Metric), group => Assert.True(group.Count() >= 3));
            Assert.All(rows.Items.Where(row => row.Metric == MetricKind.GpuUtilization),
                row => Assert.True(row.Gpu is null || row.Gpu.Scope == GpuMetricScope.OverallBusiestEngine));
            Assert.All(rows.Items.Where(row => row.Metric is MetricKind.ProcessWorkingSet or
                MetricKind.ProcessPrivateWorkingSet or MetricKind.ProcessPrivateCommit),
                row => Assert.True(row.Value.Integer > 0, row.Value.Quality.Reason));
            Assert.All(rows.Items.Where(row => row.Metric == MetricKind.CpuUtilization),
                row => Assert.Contains("NtQuerySystemInformation/SystemProcessInformation", row.Source));
            Assert.All(rows.Items.Where(row => row.Metric == MetricKind.ProcessWorkingSet),
                row => Assert.Equal("NtQuerySystemInformation/SystemProcessInformation", row.Source));
            Assert.Contains(rows.Items, row => row.Metric == MetricKind.CpuUtilization && row.Value.Kind == ValueKind.Missing);
            Console.WriteLine($"LIVE_METRICS_PASS rows={rows.Items.Count} database={connection.DatabasePath}");
        }
        finally { await CloseOwnedAsync(owned); }
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Actual_TAP_snapshot_returns_correlated_trees_and_preserves_hello_ETW_and_saved_files(bool attach)
    {
        Process? owned = attach ? TargetLauncher.LaunchUnpackaged(Executable, "", AppDirectory) : null;
        TargetConnection? connection = null;
        try
        {
            connection = await TargetConnection.ConnectAsync(new(attach ? TargetLaunchKind.Attach : TargetLaunchKind.Unpackaged,
                attach ? owned!.Id.ToString() : Executable, "", AppDirectory, Sessions,
                Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, default);
            owned ??= Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
            await VerifyAsync(connection, owned);
            var first = await connection.TakeSnapshotAsync().WaitAsync(TimeSpan.FromSeconds(50));
            var firstBytes = await File.ReadAllBytesAsync(first.Path);
            var firstTree = TreeSnapshotCodec.Decode(firstBytes, connection.Scope.Process, first.CaptureId);
            Assert.NotEmpty(firstTree.XamlNodes);
            Assert.NotEmpty(firstTree.CompositionNodes);
            Assert.NotEmpty(firstTree.Correlations);
            var second = await connection.TakeSnapshotAsync().WaitAsync(TimeSpan.FromSeconds(50));
            var secondTree = TreeSnapshotCodec.Decode(await File.ReadAllBytesAsync(second.Path), connection.Scope.Process, second.CaptureId);
            Assert.NotEqual(first.CaptureId, second.CaptureId);
            Assert.NotEqual(first.Path, second.Path);
            Assert.NotEmpty(firstTree.XamlNodes.Select(n => n.ObjectAddress).Intersect(secondTree.XamlNodes.Select(n => n.ObjectAddress)));
            var hello = connection.LastHelloUtc;
            await Task.Delay(TimeSpan.FromSeconds(6));
            Assert.True(connection.LastHelloUtc > hello);
            Assert.False(connection.Completion.IsCompleted, connection.Failure?.ToString());
            var received = connection.ReceivedRuntimeEvents;
            Assert.True(SetWindowPos(owned.MainWindowHandle, IntPtr.Zero, 0, 0, 1200, 760, 0x16));
            var wait = Stopwatch.StartNew();
            while (connection.ReceivedRuntimeEvents <= received && wait.Elapsed < TimeSpan.FromSeconds(10)) await Task.Delay(100);
            Assert.True(connection.ReceivedRuntimeEvents > received, "XAML ETW must continue after TAP snapshots.");
            await connection.StopAsync().WaitAsync(TimeSpan.FromSeconds(25));
            Assert.False(owned.HasExited);
            Assert.Equal(SHA256.HashData(firstBytes), SHA256.HashData(await File.ReadAllBytesAsync(first.Path)));
            Assert.Empty(Directory.EnumerateFiles(Sessions, $"{connection.Scope.SessionId:N}.*.tmp"));
            Console.WriteLine($"ACTUAL_TAP_SNAPSHOT_PASS attach={attach} roots={first.Roots} xaml={first.XamlNodes} " +
                $"visuals={first.CompositionNodes} correlations={first.Correlations} path={first.Path}");
        }
        finally
        {
            try { if (connection is not null) await connection.DisposeAsync(); }
            finally { await CloseOwnedAsync(owned); }
        }
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Actual_unpacked_launch_or_attach_receives_target_hello_and_persists_ETW(bool attach)
    {
        Process? owned = attach ? TargetLauncher.LaunchUnpackaged(Executable, "", AppDirectory) : null;
        TargetConnection? connection = null;
        try
        {
            connection = await TargetConnection.ConnectAsync(new(attach ? TargetLaunchKind.Attach : TargetLaunchKind.Unpackaged,
                attach ? owned!.Id.ToString() : Executable, "", AppDirectory, Sessions,
                Path.Combine(AppDirectory, "ConnectionPayloads"), true,
                ExpectedCreationTimeFileTime: attach ? owned!.StartTime.ToFileTimeUtc() : null), null, default);
            owned ??= Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
            await VerifyAsync(connection, owned);
            await connection.StopAsync().WaitAsync(TimeSpan.FromSeconds(20));
            Assert.False(owned.HasExited, "Disconnect must leave the target process running.");
            Assert.True(connection.CommittedEvents > 0);
            using var reopened = SessionStore.OpenReadOnly(connection.DatabasePath);
            Assert.Equal(connection.CommittedEvents, reopened.Watermark.IngestOrdinal);
            Assert.Equal(0, reopened.ReadIngestionDiagnostics().Where(d =>
                d.Issue is IngestionIssue.DecodeFailure or IngestionIssue.UnknownSchema or
                    IngestionIssue.EtwEventsLost or IngestionIssue.EtwBuffersLost).Sum(d => d.Count));
            Console.WriteLine($"ACTUAL_CONNECTION_PASS attach={attach} pid={owned.Id} hello=hello " +
                $"received={connection.ReceivedEvents} committed={connection.CommittedEvents} database={connection.DatabasePath}");
            await connection.DisposeAsync();
            await connection.DisposeAsync();
        }
        finally
        {
            try { if (connection is not null) await connection.DisposeAsync(); }
            finally { await CloseOwnedAsync(owned); }
        }
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    [Trait("Category", "PackagedConnection")]
    public async Task Actual_packaged_activation_receives_target_hello_and_persists_ETW(bool attach)
    {
        var aumid = Environment.GetEnvironmentVariable("WINUI_PROFILER_TEST_AUMID");
        Assert.False(string.IsNullOrWhiteSpace(aumid),
            "Run scripts\\Tests\\Test-PackagedConnection.ps1 to register the owned packaged fixture.");
        TargetConnection? connection = null;
        Process? owned = null;
        try
        {
            if (attach)
            {
                owned = TargetLauncher.LaunchPackaged(aumid!, "");
                var ready = Stopwatch.StartNew();
                while (owned.MainWindowHandle == IntPtr.Zero && ready.Elapsed < TimeSpan.FromSeconds(15))
                {
                    await Task.Delay(100);
                    owned.Refresh();
                }
                Assert.NotEqual(IntPtr.Zero, owned.MainWindowHandle);
            }
            connection = await TargetConnection.ConnectAsync(new(attach ? TargetLaunchKind.Attach : TargetLaunchKind.Packaged,
                attach ? owned!.Id.ToString() : aumid!, "",
                null, Sessions, Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, default);
            owned ??= Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
            await VerifyAsync(connection, owned);
            await connection.StopAsync().WaitAsync(TimeSpan.FromSeconds(20));
            Assert.False(owned.HasExited);
            using var reopened = SessionStore.OpenReadOnly(connection.DatabasePath);
            Assert.True(reopened.Watermark.IngestOrdinal > 0);
            Console.WriteLine($"PACKAGED_CONNECTION_PASS attach={attach} aumid={aumid} pid={owned.Id} hello=hello " +
                $"committed={reopened.Watermark.IngestOrdinal} database={connection.DatabasePath}");
        }
        finally
        {
            try { if (connection is not null) await connection.DisposeAsync(); }
            finally { await CloseOwnedAsync(owned); }
        }
    }

    [Fact]
    public async Task Stale_selected_process_identity_is_rejected_before_injection()
    {
        var executable = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
            @"WindowsPowerShell\v1.0\powershell.exe");
        using var owned = TargetLauncher.LaunchUnpackaged(executable,
            "-NoProfile -NonInteractive -Command \"Start-Sleep -Seconds 60\"", null);
        try
        {
            var error = await Assert.ThrowsAsync<InvalidOperationException>(() => TargetConnection.ConnectAsync(
                new(TargetLaunchKind.Attach, owned.Id.ToString(), "", null, Sessions,
                    Path.Combine(AppDirectory, "ConnectionPayloads"), true,
                    ExpectedCreationTimeFileTime: owned.StartTime.ToFileTimeUtc() - 1), null, default));
            Assert.Contains("selected process exited or its PID was reused", error.Message);
            Assert.False(owned.HasExited);
            Assert.DoesNotContain(owned.Modules.Cast<ProcessModule>(), module => module.ModuleName == "WinUIProfiler.Tap.dll");
        }
        finally { if (!owned.HasExited) owned.Kill(); }
    }

    [Fact]
    public async Task Cancellation_while_waiting_for_non_XAML_target_preserves_target_and_stops_capture()
    {
        var executable = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
            @"WindowsPowerShell\v1.0\powershell.exe");
        using var owned = TargetLauncher.LaunchUnpackaged(executable,
            "-NoProfile -NonInteractive -Command \"Start-Sleep -Seconds 60\"", null);
        using var cancellation = new CancellationTokenSource(TimeSpan.FromSeconds(2));
        try
        {
            await Assert.ThrowsAnyAsync<OperationCanceledException>(() => TargetConnection.ConnectAsync(
                new(TargetLaunchKind.Attach, owned.Id.ToString(), "", null, Sessions,
                    Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, cancellation.Token));
            Assert.False(owned.HasExited);
        }
        finally { if (!owned.HasExited) owned.Kill(); }
    }

    [Fact]
    public async Task Heartbeat_keeps_authenticated_pipe_alive_and_target_exit_drains_recording()
    {
        TargetConnection? connection = null;
        Process? owned = null;
        try
        {
            connection = await TargetConnection.ConnectAsync(new(TargetLaunchKind.Unpackaged, Executable, "",
                AppDirectory, Sessions, Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, default);
            owned = Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
            await VerifyAsync(connection, owned);
            var first = connection.LastHelloUtc;
            await Task.Delay(TimeSpan.FromSeconds(32));
            Assert.False(connection.Completion.IsCompleted, connection.Failure?.ToString());
            Assert.Equal("Connected", connection.State);
            Assert.True(connection.LastHelloUtc > first);
            Assert.True(owned.CloseMainWindow());
            await owned.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
            await connection.Completion.WaitAsync(TimeSpan.FromSeconds(20));
            Assert.Null(connection.Failure);
            Assert.Equal("Disconnected", connection.State);
            using var reopened = SessionStore.OpenReadOnly(connection.DatabasePath);
            Assert.True(reopened.Watermark.IngestOrdinal > 0);
            Assert.Equal(connection.CommittedEvents, reopened.Watermark.IngestOrdinal);
        }
        finally
        {
            try { if (connection is not null) await connection.DisposeAsync(); }
            finally { await CloseOwnedAsync(owned); }
        }
    }

    private static async Task VerifyAsync(TargetConnection connection, Process owned)
    {
        Assert.Equal("Connected", connection.State);
        Assert.NotNull(connection.LastHelloUtc);
        Assert.NotEqual(0, connection.ListenerThreadId);
        Assert.NotEqual(connection.ListenerThreadId, connection.DecoderThreadId);
        Assert.NotEqual(connection.ListenerThreadId, connection.ProcessingThreadId);
        Assert.Equal((uint)owned.Id, connection.Scope.Process.ProcessId);
        Assert.Equal(owned.StartTime.ToFileTimeUtc(), connection.Scope.Process.CreationTimeFileTime);
        var deadline = Stopwatch.StartNew();
        while (owned.MainWindowHandle == IntPtr.Zero && deadline.Elapsed < TimeSpan.FromSeconds(10))
        {
            await Task.Delay(100);
            owned.Refresh();
        }

        Assert.NotEqual(IntPtr.Zero, owned.MainWindowHandle);
        Assert.True(SetWindowPos(owned.MainWindowHandle, IntPtr.Zero, 0, 0, 1180, 780, 0x16));
        while (connection.ReceivedRuntimeEvents == 0 && deadline.Elapsed < TimeSpan.FromSeconds(15))
            await Task.Delay(100);
        Assert.True(connection.ReceivedRuntimeEvents > 0, connection.Failure?.ToString() ?? "No target XAML ETW event was received.");
        owned.Refresh();
        var runtime = owned.Modules.Cast<ProcessModule>().Single(m => m.ModuleName == "Microsoft.UI.Xaml.dll");
        var tap = owned.Modules.Cast<ProcessModule>().Single(m => m.ModuleName == "WinUIProfiler.Tap.dll");
        using var manifest = JsonDocument.Parse(File.ReadAllText(Path.Combine(AppDirectory, "ProfilerAppDeployment.json")));
        var expected = manifest.RootElement.GetProperty("RuntimePayloads").EnumerateArray().Single(p =>
            string.Equals(p.GetProperty("Path").GetString(), runtime.FileName, StringComparison.OrdinalIgnoreCase));
        Assert.Equal(expected.GetProperty("SHA256").GetString(),
            Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(runtime.FileName))));
        var payload = ConnectionPayloadCatalog.Resolve(Path.Combine(AppDirectory, "ConnectionPayloads"), Architecture.X64);
        Assert.Equal(payload.Tap, tap.FileName, ignoreCase: true);
    }

    [Fact]
    public async Task Processing_limit_surfaces_without_another_target_event_or_user_stop()
    {
        TargetConnection? connection = null;
        Process? owned = null;
        try
        {
            connection = await TargetConnection.ConnectAsync(new(TargetLaunchKind.Unpackaged, Executable, "",
                AppDirectory, Sessions, Path.Combine(AppDirectory, "ConnectionPayloads"), true), null, default);
            owned = Process.GetProcessById(checked((int)connection.Scope.Process.ProcessId));
            await VerifyAsync(connection, owned);
            await Task.Delay(TimeSpan.FromSeconds(2));
            var worker = Assert.IsType<ProcessingWorker>(typeof(TargetConnection).GetField("worker",
                System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)!.GetValue(connection));
            // A real owner metadata limit seals ingress without requiring an ETW arrival.
            for (var i = 0; i <= SessionHealth.MaximumDiagnosticSamples; i++)
                worker.Report(new(IngestionIssue.UnknownSchema, "Synthetic diagnostic-limit fault injection",
                    EventCatalog.RuntimeProviderId, "ConnectionLimit-" + i, (uint)owned.Id));
            Assert.True(worker.StopRequested.IsCancellationRequested);
            Assert.False(worker.Completion.IsCompleted, "The acquisition lease still prevents worker finalization.");
            await connection.Completion.WaitAsync(TimeSpan.FromSeconds(4));
            Assert.Equal("Failed", connection.State);
            Assert.NotNull(connection.Failure);
            Assert.False(owned.HasExited);
            using var reopened = SessionStore.OpenReadOnly(connection.DatabasePath);
            Assert.True(reopened.Watermark.IngestOrdinal > 0);
            Assert.Equal(connection.CommittedEvents, reopened.Watermark.IngestOrdinal);
            Assert.Contains(reopened.ReadIngestionDiagnostics(), d => d.Issue == IngestionIssue.DiagnosticLimit);
        }
        finally
        {
            // This test deliberately fails the connection, so disposal must report it.
            try { if (connection is not null) await Assert.ThrowsAsync<IOException>(() => connection.DisposeAsync().AsTask()); }
            finally { await CloseOwnedAsync(owned); }
        }
    }

    private static async Task CloseOwnedAsync(Process? owned)
    {
        if (owned is null) return;
        try
        {
            if (!owned.HasExited)
            {
                owned.CloseMainWindow();
                try { await owned.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10)); }
                catch (TimeoutException) { owned.Kill(); await owned.WaitForExitAsync(); throw; }
            }
        }
        finally { owned.Dispose(); }
    }

    [DllImport("user32.dll", SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetWindowPos(IntPtr window, IntPtr after, int x, int y, int width, int height, uint flags);
}
