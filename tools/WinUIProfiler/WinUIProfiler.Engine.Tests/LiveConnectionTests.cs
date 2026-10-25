using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

[SupportedOSPlatform("windows")]
public sealed class LiveConnectionTests
{
    [Theory]
    [InlineData(false, 0x11UL)]
    [InlineData(true, 0x3fUL)]
    public void Capture_profile_is_explicit_and_excludes_allocator(bool detailed, ulong keywords)
    {
        var capture = new BufferedRuntimeEvents(detailed);
        Assert.Equal(keywords, capture.SelectedKeywords);
        Assert.Equal(0UL, capture.SelectedKeywords & (ulong)EventKeyword.Allocator);
        Assert.Equal(detailed ? Microsoft.Diagnostics.Tracing.TraceEventLevel.Verbose :
            Microsoft.Diagnostics.Tracing.TraceEventLevel.Informational, capture.StandardXamlLevel);
        Assert.Equal(detailed ? ulong.MaxValue : 0x1044UL, capture.StandardXamlKeywords);
    }
    [Fact]
    public void Live_connection_exposes_a_typed_cancellable_connection_operation()
    {
        var assembly = typeof(InjectorResolver).Assembly;
        var connection = assembly.GetType("WinUIProfiler.Engine.Target.TargetConnection");
        Assert.NotNull(connection);
        Assert.True(typeof(IAsyncDisposable).IsAssignableFrom(connection));
        var connect = connection.GetMethod("ConnectAsync", BindingFlags.Public | BindingFlags.Static);
        Assert.NotNull(connect);
        Assert.Contains(connect.GetParameters(), parameter => parameter.ParameterType == typeof(CancellationToken));
    }

    [Fact]
    public void Owned_native_records_expose_retained_byte_accounting_for_listener_queue()
    {
        var record = typeof(WinUIProfiler.Engine.Ingestion.NativeRecord);
        var property = record.GetProperty("RetainedBytes");
        Assert.NotNull(property);
        Assert.Equal(typeof(long), property.PropertyType);
    }

    [Fact]
    public async Task Injection_trust_is_required_before_launch_or_ETW_acquisition()
    {
        var options = new TargetConnectionOptions(TargetLaunchKind.Unpackaged, @"C:\missing.exe", "",
            null, @"C:\missing-session", @"C:\missing-payload", false);
        await Assert.ThrowsAsync<UnauthorizedAccessException>(() => TargetConnection.ConnectAsync(options, null, default));
    }

    [Theory]
    [InlineData("")]
    [InlineData("0")]
    [InlineData("-1")]
    [InlineData("not-a-pid")]
    public async Task Invalid_attach_selection_is_rejected_before_ETW(string pid)
    {
        var options = new TargetConnectionOptions(TargetLaunchKind.Attach, pid, "", null, "", "", true);
        await Assert.ThrowsAnyAsync<ArgumentException>(() => TargetConnection.ConnectAsync(options, null, default));
    }

    [Fact]
    public async Task Profiler_cannot_select_itself_as_the_target()
    {
        var options = new TargetConnectionOptions(TargetLaunchKind.Attach, Environment.ProcessId.ToString(), "",
            null, "", "", true);
        await Assert.ThrowsAsync<ArgumentException>(() => TargetConnection.ConnectAsync(options, null, default));
    }

    [Fact]
    public void Retained_native_byte_accounting_includes_owned_fields_and_raw_copy()
    {
        NativeRecord Record(string text) => new(EventCatalog.RuntimeProviderId, "test", 4, 1, 1, 1, 1, null,
            new Dictionary<string, object> { ["text"] = text }, new byte[256]);
        var small = Record("x");
        var large = Record(new string('x', 512));
        Assert.Equal(1022, large.RetainedBytes - small.RetainedBytes);
        Assert.True(small.RetainedBytes > 512);
    }

    [Fact]
    public void Native_payload_manifest_checks_exact_deployed_file_hashes()
    {
        var directory = Directory.CreateTempSubdirectory("WinUIProfiler-Payload-");
        var native = Directory.CreateDirectory(Path.Combine(directory.FullName, "x64"));
        var helper = Path.Combine(native.FullName, "WinUIProfiler.Injector.exe");
        var tap = Path.Combine(native.FullName, "WinUIProfiler.Tap.dll");
        var manifest = Path.Combine(directory.FullName, "manifest.json");
        try
        {
            File.WriteAllText(helper, "helper fixture");
            File.WriteAllText(tap, "tap fixture");
            string Hash(string file) => Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(file)));
            File.WriteAllText(manifest, JsonSerializer.Serialize(new ConnectionPayloadManifest(1,
                [new("x64", Hash(helper), Hash(tap))])));
            Assert.Equal((helper, tap), ConnectionPayloadCatalog.Resolve(directory.FullName, Architecture.X64));
            File.AppendAllText(tap, "modified");
            Assert.Throws<InvalidDataException>(() => ConnectionPayloadCatalog.Resolve(directory.FullName, Architecture.X64));
            Assert.Throws<FileNotFoundException>(() => ConnectionPayloadCatalog.Resolve(directory.FullName, Architecture.Arm64));
        }
        finally
        {
            foreach (var file in new[] { manifest, helper, tap }) File.Delete(file);
            native.Delete();
            directory.Delete();
        }
    }
}
