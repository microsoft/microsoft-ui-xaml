using Microsoft.UI.Xaml;

namespace RenderBench.Coverage;

// XAML coverage corpus (test plan, "XAML coverage"). A page is a static method marked [CoveragePage] that takes a
// CoverageContext and returns a CoveragePage; pages are discovered by reflection. Each feature family lives in its own
// file, Coverage.<Family>.cs, so branches working on different families never edit the same file.
[AttributeUsage(AttributeTargets.Method)]
public sealed class CoveragePageAttribute(string family, string name) : Attribute
{
    // Family id from the test plan, e.g. "F1-shapes".
    public string Family { get; } = family;
    public string Name { get; } = name;
    public string Id => $"{Family}/{Name}";
    // Unsupported-content details the page may report (for example "Mica:approximated"). Any other detail fails.
    public string[] ExpectedUnsupported { get; init; } = [];
    // The window is expected to fall back to Composition (interop by design: SwapChainPanel, media, app visuals).
    public bool ExpectFallback { get; init; }
    // Maximum mean absolute per-channel difference (0..1) against the Composition rendering of the same page.
    public double MaxMeanAbsDiff { get; init; } = 0.01;
}

public sealed class CoverageContext
{
    public required Window Window { get; init; }
    public int Seed { get; init; } = 1;
}

public sealed class CoveragePage
{
    public required FrameworkElement Root { get; init; }
    // Drives the page into the state to capture (open a flyout, scroll, run a transition to a known point). Awaited
    // after the root is loaded and before the capture.
    public Func<Task>? Exercise { get; init; }
    public Action? Detach { get; init; }
}
