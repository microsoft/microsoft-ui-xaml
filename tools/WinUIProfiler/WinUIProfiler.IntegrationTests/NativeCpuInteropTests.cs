using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeCpuInteropTests
{
    private static string Architecture => RuntimeInformation.ProcessArchitecture switch
    {
        System.Runtime.InteropServices.Architecture.X64 => "x64",
        System.Runtime.InteropServices.Architecture.Arm64 => "ARM64",
        _ => throw new PlatformNotSupportedException("Native metrics support x64 and ARM64.")
    };
    private static string Dll(string architecture) => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin", architecture, @"Release\WinUIProfiler.Native.dll");

    [Fact]
    public void Actual_cpu_capture_warms_up_then_normalizes_original_boot_clock_intervals()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        using var sampler = new NativeCpuSampler(Dll(Architecture));
        var first = sampler.Capture(process.SafeHandle, target);
        Assert.Equal(1u, first.Sample.Status);
        Assert.Equal(ValueKind.Missing, first.Sample.ProcessUtilization.Kind);
        Assert.Equal(0u, first.ErrorCode);
        Assert.Equal(0u, first.ErrorDomain);
        Thread.Sleep(100);
        var second = sampler.Capture(process.SafeHandle, target);
        Assert.Equal(target, second.Sample.Process);
        Assert.Equal(2ul, second.Sample.ClockId);
        Assert.Equal(first.Sample.End100ns, second.Sample.Begin100ns);
        Assert.True(second.Sample.End100ns > second.Sample.Begin100ns);
        Assert.True(second.AcquisitionBegin100ns <= second.Sample.End100ns);
        Assert.True(second.Sample.End100ns <= second.AcquisitionEnd100ns);
        Assert.Equal(ValueKind.Number, second.Sample.SystemUtilization.Kind);
        Assert.Equal(ValueKind.Number, second.Sample.InterruptUtilization.Kind);
        Assert.Contains("compatibility", second.Sample.Source);
        Assert.Equal(ValueKind.Missing, second.Sample.ProcessUtility.Kind);
        Assert.True(second.Sample.Status is 0 or 3);
    }

    [Fact]
    public void Cpu_identity_failure_and_invalid_handle_are_missing_and_clear_previous_baseline()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        using var sampler = new NativeCpuSampler(Dll(Architecture));
        sampler.Capture(process.SafeHandle, target);
        var changed = new ProcessInstanceKey(target.ProcessId, target.CreationTimeFileTime + 1, Guid.NewGuid());
        var mismatch = sampler.Capture(process.SafeHandle, changed);
        Assert.Equal(6u, mismatch.Sample.Status);
        Assert.Equal(ValueKind.Missing, mismatch.Sample.ProcessUtilization.Kind);
        Assert.NotEqual(0u, mismatch.ErrorCode);
        Assert.Equal(1u, mismatch.ErrorDomain);
        using var invalid = new SafeProcessHandle(nint.Zero, false);
        var absent = sampler.Capture(invalid, target);
        Assert.Equal(2u, absent.Sample.Status);
        Assert.NotEqual(0u, absent.ErrorCode);
        Assert.Equal(ValueKind.Missing, absent.Sample.SystemUtilization.Kind);
        Assert.Equal(1u, sampler.Capture(process.SafeHandle, target).Sample.Status);
    }

    [Fact]
    public void Cpu_loader_rejects_ambiguous_paths_wrong_architecture_and_missing_exports()
    {
        Assert.Throws<ArgumentException>(() => new NativeCpuSampler("WinUIProfiler.Native.dll"));
        Assert.Throws<DllNotFoundException>(() => new NativeCpuSampler(Path.Combine(
            Path.GetDirectoryName(Dll(Architecture))!, "Missing.dll")));
        Assert.Throws<BadImageFormatException>(() => new NativeCpuSampler(Dll(Architecture == "x64" ? "ARM64" : "x64")));
        Assert.Throws<EntryPointNotFoundException>(() => new NativeCpuSampler(
            Path.Combine(Environment.SystemDirectory, "version.dll")));
    }

    [Fact]
    public void Cpu_sampler_disposal_is_idempotent_and_prohibits_native_invocation()
    {
        using var process = Process.GetCurrentProcess();
        var sampler = new NativeCpuSampler(Dll(Architecture));
        sampler.Dispose();
        sampler.Dispose();
        Assert.Throws<ObjectDisposedException>(() => sampler.Capture(process.SafeHandle, Identity(process)));
    }

    private static ProcessInstanceKey Identity(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());
}
