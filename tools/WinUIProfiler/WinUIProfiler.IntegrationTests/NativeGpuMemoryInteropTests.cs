using System.Diagnostics;
using System.Runtime.InteropServices;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeGpuMemoryInteropTests
{
    private static string Dll => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin",
        RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64",
        @"Release\WinUIProfiler.Native.dll");
    private static readonly PhysicalAdapterKey MissingAdapter = new(uint.MaxValue, int.MaxValue, 0);

    [Fact]
    public void Actual_export_preserves_requested_scope_and_unavailable_adapter_without_zero_substitution()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        using var sampler = new NativeGpuMemorySampler(Dll);
        Assert.True(sampler.IsSupported);
        var sample = sampler.Capture(process.SafeHandle, target, MissingAdapter);
        Assert.Equal(target, sample.Process);
        Assert.Equal(MissingAdapter, sample.Adapter);
        Assert.Equal(2u, sample.Status);
        Assert.True(sample.End100ns >= sample.Begin100ns);
        Assert.All(Values(sample), value =>
        {
            Assert.Equal(ValueKind.Missing, value.Kind);
            Assert.Null(value.Integer);
        });
    }

    [Fact]
    public void Actual_identity_mismatch_is_terminal_even_when_requested_adapter_is_unavailable()
    {
        using var process = Process.GetCurrentProcess();
        var actual = Identity(process);
        var wrong = new ProcessInstanceKey(actual.ProcessId, actual.CreationTimeFileTime + 1, Guid.NewGuid());
        using var sampler = new NativeGpuMemorySampler(Dll);
        var rejected = sampler.Capture(process.SafeHandle, wrong, MissingAdapter);
        Assert.Equal(wrong, rejected.Process);
        Assert.Equal(6u, rejected.Status);
        Assert.All(Values(rejected), value => Assert.Equal(ValueKind.Missing, value.Kind));
        Assert.Equal(2u, sampler.Capture(process.SafeHandle, actual, MissingAdapter).Status);
    }

    [Fact]
    public void Missing_optional_export_is_explicit_not_a_library_load_failure_or_measured_zero()
    {
        using var process = Process.GetCurrentProcess();
        using var sampler = new NativeGpuMemorySampler(Path.Combine(Environment.SystemDirectory, "kernel32.dll"));
        Assert.False(sampler.IsSupported);
        var error = Assert.Throws<NotSupportedException>(() =>
        {
            sampler.Capture(process.SafeHandle, Identity(process), MissingAdapter);
        });
        Assert.Contains("WupCaptureGpuMemory", error.Message);
    }

    [Fact]
    public void Disposed_sampler_cannot_call_unloaded_native_code()
    {
        using var process = Process.GetCurrentProcess();
        using var sampler = new NativeGpuMemorySampler(Dll);
        sampler.Dispose();
        Assert.Throws<ObjectDisposedException>(() =>
        {
            sampler.Capture(process.SafeHandle, Identity(process), MissingAdapter);
        });
    }

    private static ProcessInstanceKey Identity(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());

    private static IEnumerable<TypedValue> Values(ProcessGpuMemorySample sample) =>
        [sample.DedicatedCommitted, sample.SharedCommitted, sample.DedicatedResident, sample.SharedResident];
}
