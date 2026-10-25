using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeMemoryInteropTests
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
    public void Actual_native_dll_memory_capture_preserves_process_and_byte_definitions()
    {
        using var process = Process.GetCurrentProcess();
        var target = new ProcessInstanceKey(checked((uint)process.Id),
            process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());
        using var sampler = new NativeMemorySampler(Dll(Architecture));
        var sample = sampler.Capture(process.SafeHandle, target);
        Assert.Equal(target, sample.Process);
        Assert.Equal(2ul, sample.ClockId);
        Assert.True(sample.End100ns >= sample.Begin100ns);
        Assert.Equal(ValueKind.Integer, sample.TotalWorkingSet.Kind);
        Assert.Equal(ValueKind.Integer, sample.PrivateCommit.Kind);
        Assert.True(sample.TotalWorkingSet.Integer > 0);
        Assert.True(sample.PrivateCommit.Integer > 0);
        if (sample.Source.EndsWith("_EX2", StringComparison.Ordinal))
        {
            Assert.Equal(ValueKind.Integer, sample.PrivateWorkingSet.Kind);
            Assert.True(sample.PrivateWorkingSet.Integer <= sample.TotalWorkingSet.Integer);
        }
        else
        {
            Assert.EndsWith("_EX", sample.Source);
            Assert.Equal(ValueKind.Missing, sample.PrivateWorkingSet.Kind);
        }
    }

    [Fact]
    public void Native_identity_mismatch_and_invalid_handle_are_explicit_missing_measurements()
    {
        using var process = Process.GetCurrentProcess();
        var target = new ProcessInstanceKey(checked((uint)process.Id),
            process.StartTime.ToUniversalTime().ToFileTimeUtc() + 1, Guid.NewGuid());
        using var sampler = new NativeMemorySampler(Dll(Architecture));
        var mismatch = sampler.Capture(process.SafeHandle, target);
        Assert.Equal(ValueKind.Missing, mismatch.PrivateCommit.Kind);
        Assert.NotEqual(0u, mismatch.NativeError);
        using var invalid = new SafeProcessHandle(nint.Zero, false);
        var absent = sampler.Capture(invalid, target);
        Assert.Equal(ValueKind.Missing, absent.TotalWorkingSet.Kind);
        Assert.NotEqual(0u, absent.NativeError);
    }

    [Fact]
    public void Native_loader_never_falls_back_for_relative_missing_wrong_architecture_or_missing_exports()
    {
        Assert.Throws<ArgumentException>(() => new NativeMemorySampler("WinUIProfiler.Native.dll"));
        Assert.Throws<DllNotFoundException>(() => new NativeMemorySampler(Path.Combine(
            Path.GetDirectoryName(Dll(Architecture))!, "Missing.dll")));
        Assert.Throws<BadImageFormatException>(() => new NativeMemorySampler(Dll(Architecture == "x64" ? "ARM64" : "x64")));
        Assert.Throws<EntryPointNotFoundException>(() => new NativeMemorySampler(
            Path.Combine(Environment.SystemDirectory, "version.dll")));
    }

    [Fact]
    public void Disposed_native_sampler_cannot_invoke_unloaded_code()
    {
        using var process = Process.GetCurrentProcess();
        var target = new ProcessInstanceKey(checked((uint)process.Id),
            process.StartTime.ToUniversalTime().ToFileTimeUtc(), Guid.NewGuid());
        var sampler = new NativeMemorySampler(Dll(Architecture));
        sampler.Dispose();
        sampler.Dispose();
        Assert.Throws<ObjectDisposedException>(() => sampler.Capture(process.SafeHandle, target));
    }
}
