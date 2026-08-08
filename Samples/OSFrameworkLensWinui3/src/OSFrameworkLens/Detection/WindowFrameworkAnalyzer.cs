using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;

namespace OSFrameworkLens.Detection;

/// <summary>
/// Walks a top-level window's HWND tree and reports its per-framework composition.
/// Classification is by class name only (cheap, no SendMessage); we aggregate HWND count
/// and bounding-box area per framework, plus modules loaded in the process but absent
/// from the visible HWND set.
/// Caveats: XAML islands appear as one HWND containing many controls (area still honest);
/// virtualized lists only count painted children; non-visible HWNDs are skipped.
/// </summary>
internal sealed class WindowFrameworkAnalyzer
{
    private const int MaxHwnds = 4000;
    private const int MaxSampleClassesPerFramework = 8;
    private const double PercentScale = 100.0;

    private readonly FrameworkClassifier _classifier;

    public WindowFrameworkAnalyzer(FrameworkClassifier classifier) => _classifier = classifier;

    public sealed record FrameworkSlice(
        string DisplayName,
        string ColorHex,
        int HwndCount,
        long AreaPixels,
        double AreaPercent,
        IReadOnlyList<string> SampleClasses,
        string WhereDescription);

    public sealed record WindowAnalysis(
        IntPtr RootHwnd,
        string RootClassName,
        string ProcessName,
        uint Pid,
        int TotalHwndsScanned,
        int VisibleHwndsConsidered,
        IReadOnlyList<FrameworkSlice> Frameworks,
        IReadOnlyList<string> LoadedButNotSeenFrameworks,
        bool ModuleListAvailable,
        bool TruncatedAtLimit);

    public WindowAnalysis Analyze(IntPtr leafHwnd)
    {
        var sw = Stopwatch.StartNew();
        var root = leafHwnd == IntPtr.Zero ? IntPtr.Zero : Native.GetAncestor(leafHwnd, Native.GA_ROOT);
        if (root == IntPtr.Zero) root = leafHwnd;

        var rootClass = HwndInspector.GetClassName(root);
        var (pid, _) = HwndInspector.GetThreadProcess(root);
        var processName = HwndInspector.GetProcessNameSafe(pid);
        var rootBounds = HwndInspector.GetBounds(root);
        var rootArea = Math.Max(1L, AreaOf(rootBounds));

        var hwnds = new List<IntPtr> { root };
        var truncated = false;
        Native.EnumChildWindows(root, (hwnd, _) =>
        {
            if (hwnds.Count >= MaxHwnds) { truncated = true; return false; }
            hwnds.Add(hwnd);
            return true;
        }, IntPtr.Zero);

        var totalScanned = hwnds.Count;

        // Aggregate by framework id.
        var perFw = new Dictionary<string, FrameworkAccumulator>(StringComparer.Ordinal);
        var visibleConsidered = 0;

        foreach (var h in hwnds)
        {
            if (!Native.IsWindowVisible(h)) continue;
            var cls = HwndInspector.GetClassName(h);
            if (string.IsNullOrEmpty(cls)) continue;
            var bounds = HwndInspector.GetBounds(h);
            var area = AreaOf(bounds);
            if (area <= 0) continue;
            visibleConsidered++;

            var fw = _classifier.ClassifyByClassOnly(cls);
            if (!perFw.TryGetValue(fw.Id, out var acc))
            {
                acc = new FrameworkAccumulator(fw.DisplayName, fw.Color);
                perFw[fw.Id] = acc;
            }
            acc.HwndCount++;
            acc.AreaPixels += area;
            if (acc.UniqueClasses.Count < MaxSampleClassesPerFramework) acc.UniqueClasses.Add(cls);
        }

        // Percentages normalize against the root window's area (the user's mental model);
        // sum-of-visible-areas overflows badly when chrome+content stacks overlap. Cap at 100%.
        var slices = perFw.Values
            .Select(a =>
            {
                var sortedClasses = a.UniqueClasses.OrderBy(s => s, StringComparer.Ordinal).ToList();
                // Skip classes with no description so we don't invent meaning.
                var descriptions = sortedClasses
                    .Select(ClassNameDescriptions.TryDescribe)
                    .Where(d => d is not null)
                    .Distinct(StringComparer.Ordinal)
                    .ToList();
                var where = descriptions.Count == 0
                    ? string.Empty
                    : "Found in: " + string.Join("; ", descriptions);
                return new FrameworkSlice(
                    a.DisplayName, a.ColorHex,
                    a.HwndCount, a.AreaPixels,
                    Math.Min(PercentScale, PercentScale * a.AreaPixels / rootArea),
                    sortedClasses,
                    where);
            })
            .OrderByDescending(s => s.AreaPercent)
            .ThenByDescending(s => s.HwndCount)
            .ToList();

        // Frameworks loaded in the process but absent from the visible HWND set.
        // Lower-case the module list once here — pushing it into the inner loop was a real hotspot.
        var seenIds = new HashSet<string>(perFw.Keys, StringComparer.Ordinal);
        var (_, modsLower, moduleListAvailable) = ModuleInspector.GetModuleNamesWithLowered(pid);
        var loadedButNotSeen = new List<string>();
        if (moduleListAvailable)
        {
            foreach (var fw in _classifier.AllFrameworks)
            {
                if (fw.Id == FrameworkClassifier.UnknownFrameworkId) continue;
                if (seenIds.Contains(fw.Id)) continue;
                var matched = FrameworkClassifier.MatchModules(fw, modsLower);
                if (matched.Count > 0)
                    loadedButNotSeen.Add($"{fw.DisplayName}  ({string.Join(", ", matched)})");
            }
        }

        sw.Stop();
        Debug.WriteLine($"WindowFrameworkAnalyzer: {totalScanned} HWNDs scanned in {sw.ElapsedMilliseconds} ms");

        return new WindowAnalysis(
            RootHwnd: root,
            RootClassName: rootClass,
            ProcessName: processName,
            Pid: pid,
            TotalHwndsScanned: totalScanned,
            VisibleHwndsConsidered: visibleConsidered,
            Frameworks: slices,
            LoadedButNotSeenFrameworks: loadedButNotSeen,
            ModuleListAvailable: moduleListAvailable,
            TruncatedAtLimit: truncated);
    }

    private static long AreaOf(Native.RECT r)
    {
        var w = (long)(r.Right - r.Left);
        var h = (long)(r.Bottom - r.Top);
        if (w <= 0 || h <= 0) return 0;
        return w * h;
    }

    private sealed class FrameworkAccumulator
    {
        public string DisplayName;
        public string ColorHex;
        public int HwndCount;
        public long AreaPixels;
        public HashSet<string> UniqueClasses = new(StringComparer.Ordinal);

        public FrameworkAccumulator(string name, string color)
        {
            DisplayName = name; ColorHex = color;
        }
    }
}
