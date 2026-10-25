using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text.Json;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

[SupportedOSPlatform("windows")]
[Collection("Native collector ETW")]
public sealed class NativeInjectionTests
{
    [Theory]
    [InlineData(true, false, 0x80070005u)]
    [InlineData(false, true, 0x80070057u)]
    public async Task Helper_rejects_reused_identity_or_missing_trust_before_loading_native_images(
        bool wrongCreation, bool missingTrust, uint expected)
    {
        var root = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\.."));
        var architecture = RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64";
        var helper = Path.Combine(root, @"tools\WinUIProfiler\WinUIProfiler.Injector\bin",
            architecture, @"Release\WinUIProfiler.Injector.exe");
        using var current = Process.GetCurrentProcess();
        var client = new ProcessInstanceKey((uint)current.Id, current.StartTime.ToFileTimeUtc(), Guid.NewGuid());
        var scope = new SessionScope(Guid.NewGuid(), new(client.ProcessId,
            client.CreationTimeFileTime + (wrongCreation ? 1 : 0), Guid.NewGuid()));
        var bootstrap = InjectorResolver.CreateBootstrap(TapClient.CreatePipeName(), scope, client,
            RandomNumberGenerator.GetBytes(32));
        var start = new ProcessStartInfo(helper)
        {
            UseShellExecute = false, RedirectStandardInput = true,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        foreach (var argument in new[] { @"C:\not-loaded\Microsoft.UI.Xaml.dll",
            @"C:\not-loaded\Microsoft.Internal.FrameworkUdk.dll", @"C:\not-loaded\WinUIProfiler.Tap.dll",
            "1", missingTrust ? "--untrusted" : "--trust-local-binaries" })
            start.ArgumentList.Add(argument);
        using var process = Process.Start(start) ?? throw new InvalidOperationException("Injector helper did not start.");
        try
        {
            var output = process.StandardOutput.ReadToEndAsync();
            var errors = process.StandardError.ReadToEndAsync();
            await process.StandardInput.WriteLineAsync(bootstrap);
            process.StandardInput.Close();
            await process.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(unchecked((int)expected), process.ExitCode);
            using var result = JsonDocument.Parse(await output);
            Assert.Equal(expected, result.RootElement.GetProperty("hresult").GetUInt32());
            Assert.False(result.RootElement.GetProperty("injectionAccepted").GetBoolean());
            Assert.Contains(missingTrust ? "arguments/trust" : "target identity", await errors);
        }
        finally
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync();
        }
    }

    [Theory]
    [InlineData("Debug", false)]
    [InlineData("Release", false)]
    [InlineData("Debug", true)]
    [InlineData("Release", true)]
    public async Task Architecture_matched_helper_injects_verified_local_WinUI_and_authenticates_TAP(
        string configuration, bool enableRuntimeEvents)
    {
        using var runtimeEvents = enableRuntimeEvents ?
            new TraceEventSession($"WinUIProfiler-Injection-{Guid.NewGuid():N}",
                TraceEventSessionOptions.Create | TraceEventSessionOptions.NoRestartOnCreate) : null;
        runtimeEvents?.EnableProvider(EventCatalog.RuntimeProviderId,
            TraceEventLevel.Informational, 0xff);
        var root = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\.."));
        var architecture = RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64";
        var rid = architecture == "ARM64" ? "win-arm64" : "win-x64";
        var directory = Path.Combine(root, @"tools\WinUIProfiler\WinUIProfiler.App\bin",
            architecture, configuration, @"net10.0-windows10.0.26100.0", rid);
        using var manifest = JsonDocument.Parse(await File.ReadAllTextAsync(Path.Combine(directory, "ProfilerAppDeployment.json")));
        var executable = manifest.RootElement.GetProperty("Executable").GetString()!;
        var helper = Path.Combine(root, @"tools\WinUIProfiler\WinUIProfiler.Injector\bin",
            architecture, @"Release\WinUIProfiler.Injector.exe");
        var tap = Path.Combine(root, @"tools\WinUIProfiler\WinUIProfiler.Tap\bin",
            architecture, @"Release\WinUIProfiler.Tap.dll");
        Assert.True(File.Exists(helper), $"Build the architecture-matched injector: {helper}");
        using var target = Process.Start(new ProcessStartInfo(executable)
        {
            UseShellExecute = false, WorkingDirectory = directory,
            RedirectStandardOutput = true, RedirectStandardError = true
        })
            ?? throw new InvalidOperationException("Owned WinUI target did not start.");
        var targetOutput = target.StandardOutput.BaseStream.CopyToAsync(Stream.Null);
        var targetErrors = target.StandardError.BaseStream.CopyToAsync(Stream.Null);
        try
        {
            var deadline = Stopwatch.StartNew();
            string? runtime = null, udk = null;
            while (deadline.Elapsed < TimeSpan.FromSeconds(30))
            {
                Assert.False(target.HasExited, "Owned WinUI target exited before diagnostics readiness.");
                target.Refresh();
                foreach (ProcessModule module in target.Modules)
                {
                    if (module.ModuleName.Equals("Microsoft.UI.Xaml.dll", StringComparison.OrdinalIgnoreCase)) runtime = module.FileName;
                    if (module.ModuleName.Equals("Microsoft.Internal.FrameworkUdk.dll", StringComparison.OrdinalIgnoreCase)) udk = module.FileName;
                }
                if (runtime is not null && udk is not null && target.MainWindowHandle != IntPtr.Zero) break;
                await Task.Delay(50);
            }
            Assert.NotNull(runtime);
            Assert.NotNull(udk);
            foreach (var path in new[] { runtime, udk })
            {
                var expected = manifest.RootElement.GetProperty("RuntimePayloads").EnumerateArray().Single(file =>
                    string.Equals(file.GetProperty("Path").GetString(), path, StringComparison.OrdinalIgnoreCase));
                await using var bytes = File.OpenRead(path);
                Assert.Equal(expected.GetProperty("SHA256").GetString(),
                    Convert.ToHexString(await SHA256.HashDataAsync(bytes)));
                if (path == runtime)
                {
                    await using var product = File.OpenRead(expected.GetProperty("Source").GetString()!);
                    Assert.Equal(Convert.ToHexString(await SHA256.HashDataAsync(product)),
                        expected.GetProperty("SHA256").GetString());
                }
            }
            var scope = new SessionScope(Guid.NewGuid(), new((uint)target.Id,
                target.StartTime.ToFileTimeUtc(), Guid.NewGuid()));
            var resolved = InjectorResolver.Resolve(scope.Process, new(helper, tap, runtime, udk),
                trustLocalBinaries: true);
            Assert.Equal(RuntimeInformation.ProcessArchitecture, resolved.Architecture);
            var nonce = RandomNumberGenerator.GetBytes(32);
            var pipe = TapClient.CreatePipeName();
            using var current = Process.GetCurrentProcess();
            var bootstrap = InjectorResolver.CreateBootstrap(pipe, scope,
                new((uint)current.Id, current.StartTime.ToFileTimeUtc(), Guid.NewGuid()), nonce);
            var start = new ProcessStartInfo(resolved.Payloads.Helper)
            {
                UseShellExecute = false,
                RedirectStandardInput = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true
            };
            foreach (var argument in new[] { resolved.Payloads.Runtime, resolved.Payloads.FrameworkUdk,
                resolved.Payloads.Tap, "1", "--trust-local-binaries" })
                start.ArgumentList.Add(argument);
            using var injector = Process.Start(start) ?? throw new InvalidOperationException("Injector helper did not start.");
            try
            {
                var output = injector.StandardOutput.ReadToEndAsync();
                var errors = injector.StandardError.ReadToEndAsync();
                await injector.StandardInput.WriteLineAsync(bootstrap);
                injector.StandardInput.Close();
                await injector.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(15));
                Assert.True(injector.ExitCode == 0, $"Injector exit {injector.ExitCode}: {await output} {await errors}");
            }
            finally
            {
                if (!injector.HasExited) injector.Kill(entireProcessTree: true);
                await injector.WaitForExitAsync();
            }
            await using var client = await TapClient.ConnectAsync(pipe, scope, nonce, TimeSpan.FromSeconds(10), default);
            Assert.Equal("hello", await client.HelloAsync(TimeSpan.FromSeconds(10), default));
            var response = await client.SendAsync(TapMessageType.Capabilities, new CapabilitiesRequestBody(1),
                TimeSpan.FromSeconds(10), default);
            Assert.IsType<CapabilitiesBody>(response.Body);
            await client.SendAsync(TapMessageType.Disconnect, new DisconnectBody("Owned live injection qualified."),
                TimeSpan.FromSeconds(10), default);
            Assert.False(target.HasExited, "Disconnect must not terminate the target.");
            Assert.True(target.CloseMainWindow(), "Owned target window could not be closed normally.");
            await target.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(10));
            Assert.Equal(0, target.ExitCode);
        }
        finally
        {
            if (!target.HasExited) target.Kill(entireProcessTree: true);
            await target.WaitForExitAsync();
            await Task.WhenAll(targetOutput, targetErrors);
        }
    }
}
