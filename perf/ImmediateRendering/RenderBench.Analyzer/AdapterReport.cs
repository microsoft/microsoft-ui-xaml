namespace RenderBench.Analyzer;

// `RenderBench.Analyzer adapters`: tells Run-RenderBench.ps1 whether D2D-HW can run on this machine, using the same
// policy as the native factory (AdapterPolicy). Exit codes: 0 a hardware adapter qualifies, 1 none (D2D-HW runs are
// recorded as skipped), 2 WINUI_RENDERING_ADAPTER_LUID is malformed (the native factory would fail with E_INVALIDARG).
public static class AdapterReport
{
    public const int Qualifies = 0;
    public const int NoHardwareAdapter = 1;
    public const int BadOverride = 2;

    public static int Print(string? overrideText) => Evaluate(DxgiAdapters.Enumerate(), overrideText, Console.Out);

    public static int Evaluate(IReadOnlyList<AdapterInfo> adapters, string? overrideText, TextWriter output)
    {
        ulong overrideLuid = 0;
        if (!string.IsNullOrWhiteSpace(overrideText) && !AdapterPolicy.TryParseLuidOverride(overrideText, out overrideLuid))
        {
            output.WriteLine($"{AdapterPolicy.LuidEnvironmentVariable}='{overrideText}' is not a hex LUID");
            return BadOverride;
        }
        foreach (var adapter in adapters)
        {
            output.WriteLine($"  0x{adapter.Luid:x} {adapter.VendorId:x4}:{adapter.DeviceId:x4} flags=0x{adapter.Flags:x} " +
                $"{AdapterPolicy.Classify(adapter)} '{adapter.Description}' ({adapter.DedicatedVideoMemory / (1024 * 1024)} MB)");
        }
        var chosen = AdapterPolicy.Choose(adapters, overrideLuid);
        string pinned = overrideLuid != 0 ? $" ({AdapterPolicy.LuidEnvironmentVariable}=0x{overrideLuid:x})" : "";
        if (chosen is null)
        {
            output.WriteLine($"d2d-hw: {AdapterPolicy.NoHardwareAdapterReason}{pinned}");
            return NoHardwareAdapter;
        }
        output.WriteLine($"d2d-hw: 0x{chosen.Luid:x} '{chosen.Description}'{pinned}");
        return Qualifies;
    }
}
