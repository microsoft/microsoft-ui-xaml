using System;

namespace OSFrameworkLens.Detection;

internal sealed class SnapshotService : IDisposable
{
    private readonly FrameworkClassifier _classifier = FrameworkClassifier.LoadEmbedded();
    private readonly UiaInspector _uia = new();
    private readonly IslandDetector _islandDetector;

    public SnapshotService() => _islandDetector = new IslandDetector(_classifier);

    /// <summary>Runs a per-window framework breakdown for the Analyze button. Background-thread safe.</summary>
    public WindowFrameworkAnalyzer.WindowAnalysis AnalyzeWindow(IntPtr rootHwnd) =>
        new WindowFrameworkAnalyzer(_classifier).Analyze(rootHwnd);

    public ElementSnapshot? Snapshot(int screenX, int screenY)
    {
        var pt = new Native.POINT { X = screenX, Y = screenY };

        var hwnd = HwndInspector.DeepestHwndAt(pt);
        if (hwnd == IntPtr.Zero) return null;
        var className = HwndInspector.GetClassName(hwnd);
        var (title, titleAvailable) = HwndInspector.GetWindowTitleSafe(hwnd);
        var (pid, tid) = HwndInspector.GetThreadProcess(hwnd);
        var processName = HwndInspector.GetProcessNameSafe(pid);
        var bounds = HwndInspector.GetBounds(hwnd);
        var root = HwndInspector.GetRoot(hwnd);

        // UIA reaches windowless content (XAML islands, DUI items) that ChildWindowFromPointEx can't.
        var uia = _uia.FromPoint(screenX, screenY);
        var uiaFrameworkId = uia?.FrameworkId ?? string.Empty;
        var uiaClassName = uia?.ClassName ?? string.Empty;

        var (modules, modulesLowered, moduleListAvailable) = ModuleInspector.GetModuleNamesWithLowered(pid);

        var result = _classifier.Classify(uiaFrameworkId, className, uiaClassName, modulesLowered);
        var fw = result.Best;

        var rootClassName = HwndInspector.GetClassName(root);
        var island = _islandDetector.Detect(fw, hwnd, root, rootClassName, modules);

        return new ElementSnapshot(
            FrameworkId: fw.Id,
            FrameworkDisplayName: fw.DisplayName,
            FrameworkColorHex: fw.Color,
            Confidence: result.Confidence.ToString(),
            Score: result.Score,
            ClassName: className,
            Title: title,
            TitleAvailable: titleAvailable,
            ProcessName: processName,
            Pid: pid,
            Tid: tid,
            Hwnd: hwnd,
            RootHwnd: root,
            RootClassName: rootClassName,
            Bounds: (bounds.Left, bounds.Top, bounds.Right, bounds.Bottom),
            UiaFrameworkId: uiaFrameworkId,
            UiaControlType: uia?.ControlType ?? string.Empty,
            UiaName: uia?.Name ?? string.Empty,
            UiaAutomationId: uia?.AutomationId ?? string.Empty,
            UiaClassName: uiaClassName,
            MatchedModules: result.MatchedModules,
            AllModules: modules,
            ModuleListAvailable: moduleListAvailable,
            EvidenceLog: result.Evidence,
            IsIsland: island.IsIsland,
            IslandHostFrameworkDisplayName: island.HostDisplayName,
            IslandHostFingerprintMatched: island.HostFingerprintMatched,
            IslandHostGuessText: island.HostGuessText);
    }

    public void Dispose() => _uia.Dispose();
}
