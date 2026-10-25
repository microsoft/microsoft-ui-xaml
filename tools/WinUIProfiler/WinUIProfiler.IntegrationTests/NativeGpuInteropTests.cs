using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeGpuInteropTests
{
    private static string Architecture => RuntimeInformation.ProcessArchitecture switch
    {
        System.Runtime.InteropServices.Architecture.X64 => "x64",
        System.Runtime.InteropServices.Architecture.Arm64 => "ARM64",
        _ => throw new PlatformNotSupportedException("Native metrics support x64 and ARM64.")
    };
    private static string Dll => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin", Architecture, @"Release\WinUIProfiler.Native.dll");

    [Fact]
    public void Actual_gpu_capture_keeps_pdh_filetime_separate_from_acquisition_boot_clock()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        using var sampler = new NativeGpuSampler(Dll);
        var first = sampler.Capture(process.SafeHandle, target);
        Assert.Equal(target, first.Sample.Process);
        Assert.Equal(3ul, first.Sample.ClockId);
        Assert.Equal(2ul, first.AcquisitionClockId);
        Assert.True(first.AcquisitionEnd100ns >= first.AcquisitionBegin100ns);
        Assert.Equal("English PDH GPU Engine", first.Sample.Source);
        if (first.RowCount == 0)
        {
            Assert.Equal(ValueKind.Missing, first.Sample.Utilization.Kind);
            Assert.Null(first.Sample.Utilization.Number);
        }
        else
        {
            Assert.Equal(1u, first.Sample.Status);
        }
        Assert.All(new[] { first.Sample.DedicatedCommitted, first.Sample.SharedCommitted,
            first.Sample.DedicatedResident, first.Sample.SharedResident },
            value => Assert.Equal(ValueKind.Missing, value.Kind));
    }

    [Fact]
    public void Gpu_identity_mismatch_and_invalid_handle_are_explicit_missing_samples()
    {
        using var process = Process.GetCurrentProcess();
        var target = Identity(process);
        using var sampler = new NativeGpuSampler(Dll);
        var wrong = new ProcessInstanceKey(target.ProcessId, target.CreationTimeFileTime + 1, Guid.NewGuid());
        var mismatch = sampler.Capture(process.SafeHandle, wrong);
        Assert.Equal(6u, mismatch.Sample.Status);
        Assert.NotEqual(0u, mismatch.Win32Error);
        Assert.Equal(ValueKind.Missing, mismatch.Sample.Utilization.Kind);
        using var invalid = new SafeProcessHandle(nint.Zero, false);
        var absent = sampler.Capture(invalid, target);
        Assert.Equal(2u, absent.Sample.Status);
        Assert.NotEqual(0u, absent.Win32Error);
    }

    [Fact]
    public void Gpu_sampler_reuses_restricted_loader_and_disposal_guards()
    {
        Assert.Throws<ArgumentException>(() => new NativeGpuSampler("WinUIProfiler.Native.dll"));
        Assert.Throws<EntryPointNotFoundException>(() => new NativeGpuSampler(Path.Combine(
            Environment.SystemDirectory, "version.dll")));
        using var process = Process.GetCurrentProcess();
        var sampler = new NativeGpuSampler(Dll);
        sampler.Dispose();
        sampler.Dispose();
        Assert.Throws<ObjectDisposedException>(() => sampler.Capture(process.SafeHandle, Identity(process)));
    }

    private static ProcessInstanceKey Identity(Process process) =>
        new(checked((uint)process.Id), process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());
}
