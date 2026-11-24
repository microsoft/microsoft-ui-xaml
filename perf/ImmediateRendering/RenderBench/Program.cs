using Microsoft.UI.Composition;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Settings;

namespace RenderBench;

// Replaces the XAML-generated Main (DISABLE_XAML_GENERATED_MAIN) so the composition engine and XamlOptionalChanges can be
// chosen before Application.Start, per "Opting Into WinUI 3 Composition Engine & XamlOptionalChanges Performance APIs".
public static class Program
{
    // Every XamlChangeId known to this repo (dxaml\xcp\tools\XCPTypesAutoGen\XamlOM\Model\Microsoft.UI.Xaml.Settings.cs);
    // the last two aren't in the 2.3.9 projection, and EnableChange returns false for ids the runtime doesn't recognize.
    static readonly (string Name, XamlChangeId Id)[] s_allOptionalChanges =
    [
        ("IconNoGridOptimization", XamlChangeId.IconNoGridOptimization),
        ("OptimizeApplyStyles", XamlChangeId.OptimizeApplyStyles),
        ("DefaultStyleOptimizations", XamlChangeId.DefaultStyleOptimizations),
        ("DeferContextFlyoutInit", XamlChangeId.DeferContextFlyoutInit),
        ("GamepadKeyRouting", (XamlChangeId)63117108),
        ("CollectionMoveNotifications", (XamlChangeId)1503),
    ];

    [STAThread]
    static int Main(string[] args)
    {
        WinRT.ComWrappersSupport.InitializeComWrappers();
        var (engine, changes) = BenchOptions.ParseStartup(args);

        // ---- coverage ----
        if (BenchOptions.TryParse(args, out var listOptions, out _) && listOptions.ListCoverage is { } catalogPath)
        {
            var (pages, errors) = Coverage.CoverageCatalog.Discover(typeof(Program).Assembly.GetTypes());
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(catalogPath))!);
            File.WriteAllText(catalogPath, Coverage.CoverageCatalog.Serialize(pages));
            File.WriteAllLines(Path.Combine(Path.GetDirectoryName(Path.GetFullPath(catalogPath))!, "catalog-errors.txt"), errors);
            return errors.Count == 0 ? 0 : 5;
        }

        // Native immediate-backend test hooks (S21 device lost, S22 capture) are read at startup.
        string dataDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "RenderBench");
        foreach (var (name, value) in BenchOptions.StartupEnvironment(args, Environment.GetEnvironmentVariable(BackendModes.EnvironmentVariable),
            Environment.ProcessId, dataDirectory, Environment.GetEnvironmentVariable))
        {
            Directory.CreateDirectory(dataDirectory);
            Environment.SetEnvironmentVariable(name, value);
            Console.Error.WriteLine($"startup: {name}={value}");
        }

        if (changes == OptionalChangesChoice.All)
        {
            foreach (var (name, id) in s_allOptionalChanges)
            {
                Console.Error.WriteLine($"startup: XamlOptionalChanges.EnableChange({name}) = {XamlOptionalChanges.EnableChange(id)}");
            }
        }

        if (engine == CompositionEngineChoice.System)
        {
            // Internal test path from the wiki: needs HKLM\Software\Microsoft\WinUI\RenderingEngine\Switcher\ProcessOverrides
            // RenderBench.exe=1 (Run-RenderBench.ps1 -CompositionEngine System sets it) instead of a LAF token.
            bool selected;
            try
            {
                selected = CompositionEngine.TrySetProcessEngine(CompositionEngineType.System);
            }
            catch (Exception e)
            {
                Console.Error.WriteLine($"startup: CompositionEngine.TrySetProcessEngine(System) threw {e}");
                selected = false;
            }
            Console.Error.WriteLine($"startup: CompositionEngine.TrySetProcessEngine(System) = {selected}");
            if (!selected)
            {
                // Don't silently measure the lifted engine under a System label.
                BenchEventSource.Log.RunCompleted(4, "system composition engine not selected");
                return 4;
            }
        }

        // Decision scenes: session dwm.exe memory before this process creates any window (DWM delta baseline).
        (long PrivateBytes, long WorkingSetBytes, int ProcessId, double CpuMs) dwm;
        using (var dwmSampler = new DwmSampler())
        {
            dwm = dwmSampler.Take();
        }
        BenchEventSource.Log.DwmMemorySample(0, dwm.PrivateBytes, dwm.WorkingSetBytes, dwm.ProcessId, dwm.CpuMs);

        Application.Start(_ =>
        {
            SynchronizationContext.SetSynchronizationContext(new DispatcherQueueSynchronizationContext(DispatcherQueue.GetForCurrentThread()));
            new App();
        });
        return 0;
    }
}
