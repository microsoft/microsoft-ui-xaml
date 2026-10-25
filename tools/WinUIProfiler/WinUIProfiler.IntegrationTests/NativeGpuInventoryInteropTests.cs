using System.Diagnostics;
using System.Runtime.InteropServices;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.IntegrationTests;

public sealed class NativeGpuInventoryInteropTests
{
    private static string Dll => Path.Combine(
        Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\..")),
        @"tools\WinUIProfiler\WinUIProfiler.Native\bin",
        RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "ARM64" : "x64",
        @"Release\WinUIProfiler.Native.dll");

    [Fact]
    public void Actual_optional_inventory_preserves_graphics_scope_unknowns_and_exact_memory_selection()
    {
        using var inventorySource = new NativeGpuAdapterInventory(Dll);
        Assert.True(inventorySource.IsSupported);
        var inventory = inventorySource.Capture();
        Assert.Equal(1u, inventory.Scope);
        Assert.InRange(inventory.Adapters.Count, 0, 64);
        Assert.True(inventory.End100ns >= inventory.Begin100ns);
        Assert.All(inventory.Adapters, adapter =>
        {
            if ((adapter.KnownFlags & 4) == 0) Assert.Null(adapter.PhysicalCount);
            if ((adapter.KnownFlags & 8) == 0) Assert.Null(adapter.IsHardware);
            if ((adapter.KnownFlags & 16) == 0) Assert.Null(adapter.SupportsMemory);
        });
        if ((inventory.Flags & 7) != 1 || !inventory.FactoryCurrentBegin ||
            !inventory.FactoryCurrentEnd || inventory.Win32Error != 0)
        {
            Assert.NotEqual(0u, inventory.Status);
            Assert.Throws<InvalidOperationException>(() => inventory.SelectMemoryAdapters(false));
        }
        else
        {
            var selected = inventory.SelectMemoryAdapters(false);
            Assert.InRange(selected.Count, 0, 256);
            using var process = Process.GetCurrentProcess();
            var target = LiveEtwSource.Identify(process);
            using var sampler = new NativeGpuMemorySampler(Dll);
            foreach (var adapter in selected)
            {
                var sample = sampler.Capture(process.SafeHandle, target, adapter);
                Assert.Equal(target, sample.Process);
                Assert.Equal(adapter, sample.Adapter);
                Assert.Contains(sample.Status, new uint[] { 0, 2, 3 });
                var payloads = NativeMetricPayloads.Create(sample,
                    new(Stopwatch.GetTimestamp(), Stopwatch.GetTimestamp()),
                    new(Stopwatch.Frequency, Stopwatch.Frequency));
                Assert.Equal(4, payloads.Count);
                Assert.All(payloads, payload => Assert.Equal(adapter, payload.Gpu!.Adapter));
            }
        }
    }

    [Fact]
    public void Inventory_optional_export_absence_and_disposal_are_explicit()
    {
        using var missing = new NativeGpuAdapterInventory(Path.Combine(Environment.SystemDirectory, "kernel32.dll"));
        Assert.False(missing.IsSupported);
        Assert.Throws<NotSupportedException>(() => missing.Capture());
        using var source = new NativeGpuAdapterInventory(Dll);
        source.Dispose();
        Assert.Throws<ObjectDisposedException>(() => source.Capture());
    }
}
