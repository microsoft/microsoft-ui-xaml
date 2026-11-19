using System.Globalization;
using System.Runtime.InteropServices;

namespace RenderBench;

public enum AdapterVerdict
{
    Qualifies,
    SoftwareAdapter,
    BasicRenderDriver,
    MicrosoftVirtualAdapter,
}

// One DXGI adapter. Luid uses the DeviceInfo.AdapterLuid packing: (HighPart << 32) | LowPart.
public sealed record AdapterInfo(string Description, uint VendorId, uint DeviceId, uint Flags, ulong Luid, ulong DedicatedVideoMemory);

// C# mirror of the native D2D-HW adapter policy (dxaml\xcp\components\base\inc\D2DDeviceFactory.h). Keep in sync: the
// harness script uses it to decide whether a D2D-HW run can be executed on this machine.
public static class AdapterPolicy
{
    public const string LuidEnvironmentVariable = "WINUI_RENDERING_ADAPTER_LUID";
    public const string NoHardwareAdapterReason = "no hardware adapter";
    public const uint MicrosoftVendorId = 0x1414;
    public const uint BasicRenderDriverDeviceId = 0x8c;
    public const uint SoftwareFlag = 0x2; // DXGI_ADAPTER_FLAG_SOFTWARE

    public static AdapterVerdict Classify(AdapterInfo adapter)
    {
        if ((adapter.Flags & SoftwareFlag) != 0)
        {
            return AdapterVerdict.SoftwareAdapter;
        }
        if (adapter.VendorId == MicrosoftVendorId)
        {
            return adapter.DeviceId == BasicRenderDriverDeviceId ? AdapterVerdict.BasicRenderDriver : AdapterVerdict.MicrosoftVirtualAdapter;
        }
        return AdapterVerdict.Qualifies;
    }

    public static bool Qualifies(AdapterInfo adapter) => Classify(adapter) == AdapterVerdict.Qualifies;

    // Hex digits with an optional 0x prefix and surrounding spaces; at most 16 digits; zero is rejected.
    public static bool TryParseLuidOverride(string? text, out ulong luid)
    {
        luid = 0;
        string trimmed = (text ?? "").Trim();
        if (trimmed.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
        {
            trimmed = trimmed[2..];
        }
        if (trimmed.Length is 0 or > 16 || !trimmed.All(Uri.IsHexDigit))
        {
            return false;
        }
        luid = ulong.Parse(trimmed, NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
        return luid != 0;
    }

    // Adapters are in preference order. overrideLuid == 0 means no override; an override is strict (never substitutes).
    public static AdapterInfo? Choose(IEnumerable<AdapterInfo> adapters, ulong overrideLuid = 0)
        => overrideLuid != 0
            ? adapters.FirstOrDefault(a => a.Luid == overrideLuid && Qualifies(a))
            : adapters.FirstOrDefault(Qualifies);
}

// A DXGI adapter kept open for sampling. Pointer is an IDXGIAdapter1; Adapter3 is null before Windows 10.
public sealed unsafe class DxgiAdapter : IDisposable
{
    nint _adapter1;
    nint _adapter3;

    internal DxgiAdapter(nint adapter1, AdapterInfo info)
    {
        _adapter1 = adapter1;
        Info = info;
        Guid adapter3Id = new("645967A4-1392-4310-A798-8053CE3E93FD");
        if (Marshal.QueryInterface(adapter1, in adapter3Id, out nint adapter3) >= 0)
        {
            _adapter3 = adapter3;
        }
    }

    public AdapterInfo Info { get; }

    // IDXGIAdapter3::QueryVideoMemoryInfo for the calling process: segment group 0 = local, 1 = non-local.
    public bool TryQueryVideoMemory(out long localBytes, out long nonLocalBytes)
    {
        localBytes = nonLocalBytes = 0;
        if (_adapter3 == 0)
        {
            return false;
        }
        var query = (delegate* unmanaged[Stdcall]<nint, uint, int, VideoMemoryInfo*, int>)(*(nint**)_adapter3)[14];
        VideoMemoryInfo info;
        bool any = false;
        if (query(_adapter3, 0, 0, &info) >= 0)
        {
            localBytes = (long)info.CurrentUsage;
            any = true;
        }
        if (query(_adapter3, 0, 1, &info) >= 0)
        {
            nonLocalBytes = (long)info.CurrentUsage;
            any = true;
        }
        return any;
    }

    public void Dispose()
    {
        if (_adapter3 != 0)
        {
            Marshal.Release(_adapter3);
            _adapter3 = 0;
        }
        if (_adapter1 != 0)
        {
            Marshal.Release(_adapter1);
            _adapter1 = 0;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct VideoMemoryInfo
    {
        public ulong Budget;
        public ulong CurrentUsage;
        public ulong AvailableForReservation;
        public ulong CurrentReservation;
    }
}

// DXGI enumeration in the same order the native factory uses: IDXGIFactory6::EnumAdapterByGpuPreference
// (HIGH_PERFORMANCE), falling back to IDXGIFactory1::EnumAdapters1. Raw vtable calls keep the kit free of interop packages.
public static unsafe class DxgiAdapters
{
    const int EnumAdapters1Slot = 12;
    const int EnumAdapterByGpuPreferenceSlot = 29;
    const int GetDesc1Slot = 10;
    const uint HighPerformance = 1; // DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE

    public static List<AdapterInfo> Enumerate()
    {
        var adapters = Open(out _);
        var infos = adapters.Select(a => a.Info).ToList();
        adapters.ForEach(a => a.Dispose());
        return infos;
    }

    public static List<DxgiAdapter> Open(out bool usedGpuPreference)
    {
        usedGpuPreference = false;
        var result = new List<DxgiAdapter>();
        Guid factory1Id = new("770aae78-f26f-4dba-a829-253c83d1b387");
        if (CreateDXGIFactory1(&factory1Id, out nint factory) < 0)
        {
            return result;
        }
        try
        {
            Guid adapter1Id = new("29038f61-3839-4626-91fd-086879011a05");
            Guid factory6Id = new("c1b6694f-ff09-44a9-b03c-77900a0a1d17");
            if (Marshal.QueryInterface(factory, in factory6Id, out nint factory6) >= 0)
            {
                var byPreference = (delegate* unmanaged[Stdcall]<nint, uint, uint, Guid*, nint*, int>)(*(nint**)factory6)[EnumAdapterByGpuPreferenceSlot];
                for (uint i = 0; ; i++)
                {
                    nint adapter;
                    if (byPreference(factory6, i, HighPerformance, &adapter1Id, &adapter) < 0)
                    {
                        break;
                    }
                    Add(result, adapter);
                }
                Marshal.Release(factory6);
                usedGpuPreference = result.Count > 0;
            }
            if (result.Count == 0)
            {
                var enumAdapters1 = (delegate* unmanaged[Stdcall]<nint, uint, nint*, int>)(*(nint**)factory)[EnumAdapters1Slot];
                for (uint i = 0; ; i++)
                {
                    nint adapter;
                    if (enumAdapters1(factory, i, &adapter) < 0)
                    {
                        break;
                    }
                    Add(result, adapter);
                }
            }
        }
        finally
        {
            Marshal.Release(factory);
        }
        return result;
    }

    // Takes ownership of adapter (an IDXGIAdapter1 reference).
    static void Add(List<DxgiAdapter> result, nint adapter)
    {
        var getDesc1 = (delegate* unmanaged[Stdcall]<nint, AdapterDesc1*, int>)(*(nint**)adapter)[GetDesc1Slot];
        AdapterDesc1 desc;
        if (getDesc1(adapter, &desc) < 0)
        {
            Marshal.Release(adapter);
            return;
        }
        var chars = new ReadOnlySpan<char>(desc.Description, 128);
        int end = chars.IndexOf('\0');
        string description = new(end < 0 ? chars : chars[..end]);
        ulong luid = ((ulong)(uint)desc.LuidHighPart << 32) | desc.LuidLowPart;
        result.Add(new DxgiAdapter(adapter, new AdapterInfo(description, desc.VendorId, desc.DeviceId, desc.Flags, luid,
            (ulong)desc.DedicatedVideoMemory)));
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct AdapterDesc1
    {
        public fixed char Description[128];
        public uint VendorId;
        public uint DeviceId;
        public uint SubSysId;
        public uint Revision;
        public nuint DedicatedVideoMemory;
        public nuint DedicatedSystemMemory;
        public nuint SharedSystemMemory;
        public uint LuidLowPart;
        public int LuidHighPart;
        public uint Flags;
    }

    [DllImport("dxgi.dll")]
    static extern int CreateDXGIFactory1(Guid* riid, out nint factory);
}
