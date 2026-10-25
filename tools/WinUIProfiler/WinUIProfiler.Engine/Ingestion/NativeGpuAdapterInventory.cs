using System.Runtime.InteropServices;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class NativeGpuAdapterInventory : IDisposable
{
    private readonly object gate = new();
    private readonly NativeMetricLibrary module;
    private readonly EnumerateAdapters? enumerate;

    internal NativeGpuAdapterInventory(string path)
    {
        module = new(path);
        try
        {
            enumerate = module.TryExport<EnumerateAdapters>("WupEnumerateGpuAdapters");
        }
        catch
        {
            module.Dispose();
            throw;
        }
    }

    internal bool IsSupported => enumerate is not null;

    internal GpuAdapterInventorySnapshot Capture()
    {
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(module.IsClosed, this);
            if (enumerate is null)
                throw new NotSupportedException("The native metric library does not export WupEnumerateGpuAdapters.");
            var request = new InventoryRequest(1);
            var buffer = new byte[5200];
            var result = enumerate(in request, buffer, checked((uint)buffer.Length));
            GC.KeepAlive(module);
            if (result != 0)
                throw new COMException("The native graphics inventory rejected its ABI request.", unchecked((int)result));
            return GpuAdapterInventoryCodec.Decode(buffer);
        }
    }

    public void Dispose()
    {
        lock (gate) module.Dispose();
    }

    [StructLayout(LayoutKind.Sequential)]
    private readonly struct InventoryRequest(uint scope)
    {
        public readonly uint Size = 16;
        public readonly uint Version = 1;
        public readonly uint Scope = scope;
        public readonly uint Reserved = 0;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate uint EnumerateAdapters(in InventoryRequest request, [Out] byte[] output, uint capacity);
}
