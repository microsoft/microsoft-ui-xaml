using System.Buffers.Binary;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class GpuAdapterInventoryTests
{
    [Fact]
    public void Graphics_visible_inventory_preserves_metadata_and_selects_distinct_physical_scopes()
    {
        var inventory = GpuAdapterInventoryCodec.Decode(Buffer());
        Assert.Equal(1u, inventory.Scope);
        Assert.Equal(100ul, inventory.Begin100ns);
        Assert.Equal(200ul, inventory.End100ns);
        Assert.Equal(2, inventory.Adapters.Count);
        Assert.Equal(2u, inventory.Adapters[0].PhysicalCount);
        Assert.True(inventory.Adapters[0].IsHardware);
        Assert.True(inventory.Adapters[0].SupportsMemory);
        Assert.Equal(new[] { new PhysicalAdapterKey(123, -1, 0), new(123, -1, 1), new(456, 0, 0) },
            inventory.SelectMemoryAdapters(hardwareOnly: true));
    }

    [Fact]
    public void Partial_inventory_preserves_failed_lookup_unknowns_and_usable_nonhardware_scope()
    {
        var bytes = Buffer();
        U32(bytes, 12, 3); U32(bytes, 52, 9);
        U32(bytes, 80 + 24, 0x1414); U32(bytes, 80 + 28, 0x008C);
        U32(bytes, 80 + 12, 63); U32(bytes, 80 + 40, 2); U32(bytes, 80 + 48, 0);
        const int second = 160;
        U32(bytes, second + 8, 3); U32(bytes, second + 12, 3);
        U32(bytes, second + 44, uint.MaxValue); U32(bytes, second + 48, uint.MaxValue);
        U32(bytes, second + 52, uint.MaxValue); U32(bytes, second + 60, 0x80070057);
        U32(bytes, second + 64, 0x80004001); U32(bytes, second + 68, 0x80004001);
        var inventory = GpuAdapterInventoryCodec.Decode(bytes);
        Assert.Equal(3u, inventory.Status);
        Assert.True(inventory.Adapters[0].IsBasicRenderDriver);
        Assert.False(inventory.Adapters[0].IsHardware);
        Assert.Null(inventory.Adapters[1].PhysicalCount);
        Assert.Null(inventory.Adapters[1].IsHardware);
        Assert.Null(inventory.Adapters[1].SupportsMemory);
        Assert.Equal(0x80070057u, inventory.Adapters[1].LookupHResult);
        Assert.Empty(inventory.SelectMemoryAdapters(hardwareOnly: true));
        Assert.Equal(2, inventory.SelectMemoryAdapters(hardwareOnly: false).Count);
    }

    [Fact]
    public void Stale_or_truncated_inventory_cannot_silently_configure_a_prefix()
    {
        foreach (var flags in new uint[] { 5, 2 })
        {
            var bytes = Buffer();
            U32(bytes, 12, 3); U32(bytes, 52, flags);
            if (flags == 5) U32(bytes, 68, 0);
            var inventory = GpuAdapterInventoryCodec.Decode(bytes);
            Assert.Throws<InvalidOperationException>(() => inventory.SelectMemoryAdapters(false));
        }
    }

    [Fact]
    public void Selection_bound_is_explicit_instead_of_silently_polling_a_prefix()
    {
        var bytes = Buffer();
        U32(bytes, 44, 5);
        for (var index = 0; index < 5; index++)
        {
            Array.Copy(bytes, 80, bytes, 80 + index * 80, 80);
            U32(bytes, 80 + index * 80 + 20, (uint)index);
            U32(bytes, 80 + index * 80 + 44, 64);
        }
        var inventory = GpuAdapterInventoryCodec.Decode(bytes);
        Assert.Throws<InvalidOperationException>(() => inventory.SelectMemoryAdapters(false));
    }

    [Theory]
    [InlineData(0)] [InlineData(1)] [InlineData(2)] [InlineData(3)]
    [InlineData(4)] [InlineData(5)] [InlineData(6)] [InlineData(7)]
    [InlineData(8)] [InlineData(9)] [InlineData(10)] [InlineData(11)]
    [InlineData(12)] [InlineData(13)] [InlineData(14)] [InlineData(15)]
    [InlineData(16)] [InlineData(17)]
    public void Corrupt_layout_scope_flags_clocks_rows_and_inactive_bytes_are_rejected(int fault)
    {
        var bytes = Buffer();
        switch (fault)
        {
            case 0: Array.Resize(ref bytes, 5199); break;
            case 1: U32(bytes, 0, 5201); break;
            case 2: U32(bytes, 4, 2); break;
            case 3: U32(bytes, 8, 7); break;
            case 4: U32(bytes, 40, 2); break;
            case 5: U32(bytes, 44, 65); break;
            case 6: U32(bytes, 48, 63); break;
            case 7: U32(bytes, 52, 32); break;
            case 8: U32(bytes, 64, 2); break;
            case 9: U32(bytes, 72, 1); break;
            case 10: U64(bytes, 16, 3); break;
            case 11: U64(bytes, 24, 201); break;
            case 12: U32(bytes, 80, 79); break;
            case 13: U32(bytes, 80 + 12, 128); break;
            case 14: U32(bytes, 80 + 48, 2); break;
            case 15: bytes[^1] = 1; break;
            case 16: U32(bytes, 80 + 44, 0); break;
            case 17: U32(bytes, 80 + 12, 23); U32(bytes, 80 + 48, uint.MaxValue); break;
        }
        Assert.Throws<InvalidDataException>(() => GpuAdapterInventoryCodec.Decode(bytes));
    }

    internal static byte[] Buffer()
    {
        var bytes = new byte[5200];
        U32(bytes, 0, 5200); U32(bytes, 4, 1); U32(bytes, 8, 8);
        U64(bytes, 16, 2); U64(bytes, 24, 100); U64(bytes, 32, 200);
        U32(bytes, 40, 1); U32(bytes, 44, 2); U32(bytes, 48, 64);
        U32(bytes, 52, 1); U32(bytes, 56, 0x887A0002); U32(bytes, 64, 1); U32(bytes, 68, 1);
        for (var index = 0; index < 2; index++)
        {
            var row = 80 + index * 80;
            U32(bytes, row, 80); U32(bytes, row + 4, 1); U32(bytes, row + 12, 31);
            U32(bytes, row + 16, index == 0 ? uint.MaxValue : 0);
            U32(bytes, row + 20, index == 0 ? 123u : 456u);
            U32(bytes, row + 24, 0x10DE); U32(bytes, row + 28, 1234);
            U32(bytes, row + 44, index == 0 ? 2u : 1u);
            U32(bytes, row + 48, 1); U32(bytes, row + 52, 1);
        }
        return bytes;
    }

    private static void U32(byte[] bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    private static void U64(byte[] bytes, int offset, ulong value) =>
        BinaryPrimitives.WriteUInt64LittleEndian(bytes.AsSpan(offset), value);
}
