using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text.Json;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

[SupportedOSPlatform("windows")]
internal static class StartupIntegration
{
    private static readonly MarkerProvider Marker = new("WinUIProfiler-StartupFixture",
        new("b468d34b-f13d-422b-9a7f-9fb0b1547fb7"));

    internal static async Task RunAsync(string native)
    {
        var fixture = Path.Combine(native, "tests", "Fixture.exe");
        var root = Path.Combine(Path.GetTempPath(), "WinUIProfiler-Startup", Guid.NewGuid().ToString("N"));
        var architecture = RuntimeInformation.OSArchitecture == Architecture.Arm64 ? "ARM64" : "x64";
        string DeployLauncher(string directory, string launcher)
        {
            var deployment = Path.Combine(root, directory);
            var payload = Directory.CreateDirectory(Path.Combine(deployment, architecture)).FullName;
            File.Copy(launcher, Path.Combine(payload, "ProfilerLauncher.exe"));
            File.Copy(Path.Combine(native, "ProfilerRedirect.dll"), Path.Combine(payload, "ProfilerRedirect.dll"));
            string Hash(string name) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.Combine(payload, name))));
            File.WriteAllText(Path.Combine(deployment, "manifest.json"), JsonSerializer.Serialize(
                new NativeLauncherManifest(1, [new(architecture, Hash("ProfilerLauncher.exe"), Hash("ProfilerRedirect.dll"))])));
            return deployment;
        }
        var deployment = DeployLauncher("NativeLauncher", Path.Combine(native, "ProfilerLauncher.exe"));
        var selectionTestDeployment = DeployLauncher("SelectionTestLauncher", Path.Combine(native, "tests", "ProfilerLauncher.exe"));
        var runtime = new PrivateRuntimeLaunch(Path.Combine(native, "tests", "private"),
            Path.Combine(native, "tests", "original"), true, "", deployment);
        Console.WriteLine($"Startup integration evidence: {root}");
        foreach (var (mode, count) in new (string, int)[]
        {
            ("selection-race", 2),
            ("runtime", 1), ("handoff", 2), ("handoff-chain", 3), ("handoff-environment", 2),
            ("handoff-suspended", 2), ("handoff-ansi", 2), ("handoff-as-user", 2), ("handoff-as-user-ansi", 2),
            ("other-child", 1), ("create-failure", 1), ("late-child", 1)
        })
        {
            var directory = Directory.CreateDirectory(Path.Combine(root, mode)).FullName;
            var options = runtime with
            {
                LaunchDirectory = Path.Combine(directory, "launch"),
                LauncherDirectory = mode == "selection-race" ? selectionTestDeployment : deployment
            };
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(40));
            NativeLaunchSession? launch = null;
            try
            {
                var resultPath = Path.Combine(directory, "fixture.json");
                launch = await NativeLauncher.LaunchAsync(TargetLaunchKind.Unpackaged, fixture,
                    $"{NativeLaunchProtocol.Quote(resultPath)} {mode}", options, timeout.Token);
                Check(launch.Result.Processes.Count == count, $"{mode}: wrong startup process inventory");
                Check(count == 1
                    ? launch.Result.ActivationProcessId == launch.Result.ProcessId
                    : launch.Result.ActivationProcessId != launch.Result.ProcessId, $"{mode}: wrong selected UI process");
                using var result = await ReadJsonAsync(resultPath, timeout.Token);
                Check(result.RootElement.GetProperty("mux").GetInt32() == 2 &&
                    result.RootElement.GetProperty("muxc").GetInt32() == 2 &&
                    result.RootElement.GetProperty("pid").GetInt32() == launch.Result.ProcessId,
                    $"{mode}: the UI child did not load both private DLLs");
                NativeLauncher.VerifyModules(launch.Process, options);
                if (mode == "selection-race")
                {
                    using var parent = await ReadJsonAsync(resultPath + ".parent", timeout.Token);
                    Check(parent.RootElement.GetProperty("pid").GetInt32() == launch.Result.ActivationProcessId &&
                        parent.RootElement.GetProperty("mux").GetInt32() == 2 &&
                        parent.RootElement.GetProperty("muxc").GetInt32() == 2,
                        "A parent that already loaded WinUI must not remain selected after the barrier creates its child.");
                }
                if (mode == "other-child")
                {
                    using var other = await ReadJsonAsync(resultPath + ".0", timeout.Token);
                    Check(other.RootElement.GetProperty("mux").GetInt32() == 1 &&
                        other.RootElement.GetProperty("muxc").GetInt32() == 1,
                        "A different executable must not receive the redirector.");
                }
                if (mode == "late-child")
                {
                    File.WriteAllText(resultPath + ".continue", "");
                    using var child = await ReadJsonAsync(resultPath + ".late", timeout.Token);
                    Check(child.RootElement.GetProperty("mux").GetInt32() == 1 &&
                        child.RootElement.GetProperty("muxc").GetInt32() == 1,
                        "A child created after startup selection must not receive the redirector.");
                    await launch.Process.WaitForExitAsync(timeout.Token);
                    Check(launch.Process.ExitCode == 0, "The late child must finish normally.");
                }
                Console.WriteLine($"Passed native startup: {mode}");
            }
            finally
            {
                CloseOwned(options.LaunchDirectory, fixture);
                launch?.Dispose();
            }
        }

        foreach (var (mode, stage, error, count) in new (string, string, uint, int)[]
        {
            ("ambiguous", "runtime_ambiguous", 234, 3),
            ("overflow", "runtime_ready", 111, NativeLaunchProtocol.MaxStartupProcesses)
        })
        {
            var options = runtime with { LaunchDirectory = Path.Combine(root, mode, "launch") };
            Directory.CreateDirectory(Path.Combine(root, mode));
            try
            {
                using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(40));
                try
                {
                    using var unexpected = await NativeLauncher.LaunchAsync(TargetLaunchKind.Unpackaged, fixture,
                        $"{NativeLaunchProtocol.Quote(Path.Combine(root, mode, "fixture.json"))} {mode}", options, timeout.Token);
                    throw new InvalidDataException($"{mode}: an invalid startup group must fail.");
                }
                catch (InvalidOperationException failure)
                {
                    Check(!failure.Message.Contains("child_injected", StringComparison.Ordinal),
                        "A successful child injection must not be reported as a helper failure.");
                    var result = NativeLaunchProtocol.ParseResult(File.ReadAllText(
                        Path.Combine(options.LaunchDirectory, "1", NativeLaunchProtocol.ResultFile)));
                    Check(result.Stage == stage && result.Error == error && result.Processes.Count == count,
                        $"{mode}: startup must fail explicitly while preserving cleanup identities.");
                }
                Console.WriteLine($"Passed native startup: {mode}");
            }
            finally { CloseOwned(options.LaunchDirectory, fixture); }
        }

        await VerifyJunctionAsync(fixture, runtime, root);
        await VerifyEtwAsync(fixture, runtime, root);
    }

    // Windows reports a process's image path with junctions resolved, so the launcher and the helper in each startup
    // process must still recognize a launch through a junction as the requested executable.
    private static async Task VerifyJunctionAsync(string fixture, PrivateRuntimeLaunch runtime, string root)
    {
        var directory = Directory.CreateDirectory(Path.Combine(root, "junction")).FullName;
        var link = Path.Combine(directory, "fixture");
        var mklink = new ProcessStartInfo("cmd.exe") { UseShellExecute = false, RedirectStandardOutput = true };
        foreach (var argument in new[] { "/d", "/c", "mklink", "/J", link, Path.GetDirectoryName(fixture)! })
        {
            mklink.ArgumentList.Add(argument);
        }
        using (var process = Process.Start(mklink)!)
        {
            await process.StandardOutput.ReadToEndAsync();
            await process.WaitForExitAsync();
            Check(process.ExitCode == 0, "junction: could not create the fixture junction");
        }
        var target = Path.Combine(link, "Fixture.exe");
        // The fixture loads its original DLLs from beside its own path, which is the junction path.
        var options = runtime with
        {
            SourceDirectory = Path.Combine(link, "original"),
            LaunchDirectory = Path.Combine(directory, "launch")
        };
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(40));
        NativeLaunchSession? launch = null;
        try
        {
            var resultPath = Path.Combine(directory, "fixture.json");
            launch = await NativeLauncher.LaunchAsync(TargetLaunchKind.Unpackaged, target,
                $"{NativeLaunchProtocol.Quote(resultPath)} handoff", options, timeout.Token);
            Check(launch.Result.Processes.Count == 2 && launch.Result.ActivationProcessId != launch.Result.ProcessId,
                "junction: the child launched through the junction was not selected");
            using var result = await ReadJsonAsync(resultPath, timeout.Token);
            Check(result.RootElement.GetProperty("mux").GetInt32() == 2 &&
                result.RootElement.GetProperty("muxc").GetInt32() == 2 &&
                result.RootElement.GetProperty("pid").GetInt32() == launch.Result.ProcessId,
                "junction: the UI child did not load both private DLLs");
            NativeLauncher.VerifyModules(launch.Process, options);
            Console.WriteLine("Passed native startup: junction");
        }
        finally
        {
            CloseOwned(options.LaunchDirectory, target);
            launch?.Dispose();
            Directory.Delete(link);
        }
    }

    private static async Task VerifyEtwAsync(string fixture, PrivateRuntimeLaunch runtime, string root)
    {
        var directory = Directory.CreateDirectory(Path.Combine(root, "etw")).FullName;
        var options = runtime with { LaunchDirectory = Path.Combine(directory, "launch") };
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(45));
        var startQpc = Stopwatch.GetTimestamp();
        using var source = LiveEtwSource.StartBeforeLaunch(["Fixture.exe"], [new(Marker)]);
        var unrelatedStart = new ProcessStartInfo(fixture) { UseShellExecute = false, WorkingDirectory = Path.GetDirectoryName(fixture)! };
        unrelatedStart.ArgumentList.Add(Path.Combine(directory, "unrelated.json"));
        unrelatedStart.ArgumentList.Add("etw-unrelated");
        using var unrelated = Process.Start(unrelatedStart)!;
        NativeLaunchSession? launch = null;
        ProcessRecordingGroup? recordings = null;
        try
        {
            launch = await NativeLauncher.LaunchAsync(TargetLaunchKind.Unpackaged, fixture,
                $"{NativeLaunchProtocol.Quote(Path.Combine(directory, "fixture.json"))} handoff-etw", options, timeout.Token);
            Check(launch.Result.Processes.Count == 2 && launch.Result.ProcessId != launch.Result.ActivationProcessId,
                "The ETW fixture must select the child.");
            recordings = new(launch.Result.Processes.Select(process => new SessionStart(process.ProcessId, "Fixture",
                DateTime.FromFileTimeUtc(process.CreationTimeFileTime), Stopwatch.Frequency, startQpc,
                SessionCoverage.ProcessStart)).ToArray(), launch.Result.ProcessId, null, Path.Combine(directory, "sessions"));
            source.StartReading(launch.Result.Processes.ToDictionary(process => process.ProcessId,
                process => (long?)process.CreationTimeFileTime), null, recordings.Post);
            await launch.Process.WaitForExitAsync(timeout.Token);
            await unrelated.WaitForExitAsync(timeout.Token);
            Check(launch.Process.ExitCode == 0 && unrelated.ExitCode == 0, "ETW fixtures must exit normally.");
            source.Dispose();
            recordings.Close("Fixture exited", source.EventsLost);
            Check(source.Failure is null && source.EventsLost == 0 && recordings.Failure is null &&
                recordings.RecordingError is null, "The fixture capture must not lose events or fail.");
            foreach (var recording in recordings.Processes)
            {
                var events = new List<ProfilerEvent>();
                SessionFile.Read(recording.Recording!.Path, events.Add);
                var markers = events.Where(item => item.Provider == XamlProvider.Marker).ToArray();
                var expectedRole = recording.IsPrimary ? "child" : "parent";
                Check(markers.Any(item => item.Name == "Startup" &&
                    item.Fields.Any(field => field.Name == "Role" && Equals(field.Value, expectedRole))),
                    $"The early {expectedRole} event was not retained.");
                Check(markers.All(item => item.ProcessId == recording.Source.ProcessId &&
                    item.Fields.All(field => field.Name != "Role" || Equals(field.Value, expectedRole))),
                    "An unrelated process or another startup process contaminated the recording.");
                if (recording.IsPrimary)
                {
                    Check(markers.Any(item => item.Name == "AfterSelection"),
                        "The child provider stopped being enabled after the PID filter changed.");
                }
            }
            Console.WriteLine("Passed live ETW: early parent/child events, exited-parent identity, post-selection events, and unrelated-process exclusion.");
        }
        finally
        {
            source.Dispose();
            recordings?.Dispose();
            CloseOwned(options.LaunchDirectory, fixture);
            launch?.Dispose();
            if (!unrelated.HasExited)
            {
                unrelated.Kill();
                await unrelated.WaitForExitAsync();
            }
        }
    }

    private static async Task<JsonDocument> ReadJsonAsync(string path, CancellationToken token)
    {
        while (!File.Exists(path)) await Task.Delay(20, token);
        return JsonDocument.Parse(await File.ReadAllTextAsync(path, token));
    }

    private static void CloseOwned(string directory, string executable)
    {
        var resultPath = Path.Combine(directory, "1", NativeLaunchProtocol.ResultFile);
        if (!File.Exists(resultPath)) return;
        var result = NativeLaunchProtocol.ParseResult(File.ReadAllText(resultPath));
        foreach (var entry in result.Processes.Reverse())
        {
            try
            {
                using var process = Process.GetProcessById(entry.ProcessId);
                _ = process.SafeHandle;
                if (!process.HasExited && process.StartTime.ToFileTimeUtc() == entry.CreationTimeFileTime &&
                    string.Equals(process.MainModule?.FileName, executable, StringComparison.OrdinalIgnoreCase))
                {
                    process.Kill();
                    process.WaitForExit(5000);
                }
            }
            catch (ArgumentException) { }
            catch (InvalidOperationException) { }
        }
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new InvalidDataException(message);
    }
}
