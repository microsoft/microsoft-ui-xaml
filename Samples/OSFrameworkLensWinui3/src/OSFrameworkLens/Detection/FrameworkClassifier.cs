using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace OSFrameworkLens.Detection;

/// <summary>Confidence band derived from <see cref="ClassificationResult.Score"/> via <see cref="FrameworkClassifier.BandFor"/>.</summary>
internal enum ConfidenceBand
{
    None,    // score 0
    Low,     // 1–3, module-only
    Medium,  // 4, class regex only
    High,    // 5+, includes a strong per-element/per-window signal
}

/// <summary>Immutable result of classifying a single element.</summary>
internal sealed record ClassificationResult(
    FrameworkClassifier.Framework Best,
    ConfidenceBand Confidence,
    int Score,
    IReadOnlyList<string> MatchedModules,
    IReadOnlyList<string> Evidence);

/// <summary>
/// Score-based, explainable classifier:
///   +<see cref="UiaMatchScore"/> UIA FrameworkId match (per-element)
///   +<see cref="ClassRegexMatchScore"/> HWND class-name regex (per-window)
///   +1 per matching module (per-process, weak — tie-breaker only)
/// Per-process module evidence cannot outvote per-window evidence; that matters
/// for multi-framework processes like explorer.exe.
/// </summary>
internal sealed class FrameworkClassifier
{
    public const int UiaMatchScore = 5;
    public const int ClassRegexMatchScore = 4;
    // Per-module weight is implicit in MatchModules: each hit adds 1.

    private const int HighBandMinScore   = UiaMatchScore;
    private const int MediumBandScore    = ClassRegexMatchScore;
    private const int LowBandMinScore    = 1;

    public const string UnknownFrameworkId = "unknown";
    public const string Win32FamilyId      = "win32";
    public const string DefaultUnknownColor = "#404040";

    // UIA match + class regex + module list = at most 3 evidence lines per framework.
    private const int MaxEvidenceLinesPerFramework = 3;

    private static readonly Lazy<FrameworkClassifier> s_embedded =
        new(LoadEmbeddedCore, isThreadSafe: true);

    public sealed class Framework
    {
        [JsonPropertyName("id")] public string Id { get; set; } = "";
        [JsonPropertyName("displayName")] public string DisplayName { get; set; } = "";
        [JsonPropertyName("color")] public string Color { get; set; } = "#808080";
        [JsonPropertyName("uiaFrameworkIds")] public List<string> UiaFrameworkIds { get; set; } = new();
        [JsonPropertyName("classRegex")] public List<string> ClassRegex { get; set; } = new();
        [JsonPropertyName("modules")] public List<string> Modules { get; set; } = new();

        // Frameworks sharing a family aren't an island when one hosts the other
        // (e.g. comctl32 SysTreeView32 inside a CabinetWClass — both family="win32").
        [JsonPropertyName("family")] public string? Family { get; set; }
        [JsonIgnore] public string EffectiveFamily => string.IsNullOrEmpty(Family) ? Id : Family;

        [JsonIgnore] public List<Regex> CompiledClassRegex { get; } = new();

        // Pre-lowered for O(1) hashset comparison against a process's module set.
        [JsonIgnore] public IReadOnlyList<string> ModulesLowered { get; private set; } = Array.Empty<string>();

        internal void Freeze()
        {
            // Dedupe case-insensitively — frameworks.json may list the same module
            // under different casings; without this, MatchModules would credit it twice.
            var seen = new HashSet<string>(Modules.Count, StringComparer.Ordinal);
            var loweredUnique = new List<string>(Modules.Count);
            var originalsUnique = new List<string>(Modules.Count);
            foreach (var m in Modules)
            {
                var lower = m.ToLowerInvariant();
                if (!seen.Add(lower)) continue;
                loweredUnique.Add(lower);
                originalsUnique.Add(m);
            }
            Modules = originalsUnique;
            ModulesLowered = loweredUnique;
        }
    }

    private sealed class Root
    {
        [JsonPropertyName("frameworks")] public List<Framework> Frameworks { get; set; } = new();
    }

    private static readonly Framework s_unknown = new()
    {
        Id = UnknownFrameworkId,
        DisplayName = "Unknown",
        Color = DefaultUnknownColor,
    };

    private readonly List<Framework> _frameworks;

    public FrameworkClassifier(List<Framework> frameworks) => _frameworks = frameworks;

    internal IReadOnlyList<Framework> AllFrameworks => _frameworks;

    /// <summary>Shared, lazily-initialized classifier from embedded frameworks.json. Thread-safe.</summary>
    internal static FrameworkClassifier LoadEmbedded() => s_embedded.Value;

    private static FrameworkClassifier LoadEmbeddedCore()
    {
        var asm = Assembly.GetExecutingAssembly();
        var resourceName = asm.GetManifestResourceNames()
            .FirstOrDefault(n => n.EndsWith("frameworks.json", StringComparison.OrdinalIgnoreCase))
            ?? throw new InvalidOperationException("Embedded frameworks.json not found.");
        using var s = asm.GetManifestResourceStream(resourceName)!;
        using var r = new StreamReader(s);
        var json = r.ReadToEnd();
        var root = JsonSerializer.Deserialize<Root>(json) ?? throw new InvalidOperationException("frameworks.json invalid");
        foreach (var f in root.Frameworks)
        {
            foreach (var rx in f.ClassRegex)
                f.CompiledClassRegex.Add(new Regex(rx, RegexOptions.IgnoreCase | RegexOptions.Compiled));
            f.Freeze();
        }
        return new FrameworkClassifier(root.Frameworks);
    }

    internal static ConfidenceBand BandFor(int score) => score switch
    {
        >= HighBandMinScore => ConfidenceBand.High,
        MediumBandScore     => ConfidenceBand.Medium,
        >= LowBandMinScore  => ConfidenceBand.Low,
        _                   => ConfidenceBand.None,
    };

    // Shared no-match evidence — Classify runs at 30 Hz and most frames score 0.
    private static readonly IReadOnlyList<string> s_noMatchEvidence =
        new[] { "No fingerprints matched." };

    /// <summary>
    /// Classifies an element. Callers on the snapshot path should pass the lowered
    /// hashset cached by <see cref="ModuleInspector.GetModuleNamesWithLowered"/>.
    /// </summary>
    public ClassificationResult Classify(
        string uiaFrameworkId,
        string hwndClassName,
        string uiaClassName,
        HashSet<string> modulesLowered)
    {
        var best = s_unknown;
        var bestScore = 0;
        IReadOnlyList<string> bestModules = Array.Empty<string>();
        IReadOnlyList<string> bestEvidence = s_noMatchEvidence;

        foreach (var f in _frameworks)
        {
            var score = 0;
            // Lazy: most frameworks score 0, so don't allocate per framework per tick.
            List<string>? localEvidence = null;

            if (!string.IsNullOrEmpty(uiaFrameworkId)
                && f.UiaFrameworkIds.Any(x => string.Equals(x, uiaFrameworkId, StringComparison.OrdinalIgnoreCase)))
            {
                score += UiaMatchScore;
                (localEvidence ??= new List<string>(MaxEvidenceLinesPerFramework))
                    .Add($"UIA FrameworkId = '{uiaFrameworkId}' (+{UiaMatchScore})");
            }

            // Try both HWND class and UIA ClassName — they can disagree, and either
            // string can be the framework-identifying one (XAML Islands surface the
            // bridge class via UIA; DUI items surface "DirectUIHWND" via the HWND).
            // Whichever matches first wins; class score credited only once.
            Regex? hit = null;
            string matchedAgainst = string.Empty;
            if (!string.IsNullOrEmpty(hwndClassName))
            {
                hit = f.CompiledClassRegex.FirstOrDefault(rx => rx.IsMatch(hwndClassName));
                if (hit is not null) matchedAgainst = $"HWND class '{hwndClassName}'";
            }
            if (hit is null && !string.IsNullOrEmpty(uiaClassName) && !string.Equals(uiaClassName, hwndClassName, StringComparison.Ordinal))
            {
                hit = f.CompiledClassRegex.FirstOrDefault(rx => rx.IsMatch(uiaClassName));
                if (hit is not null) matchedAgainst = $"UIA ClassName '{uiaClassName}'";
            }
            if (hit is not null)
            {
                score += ClassRegexMatchScore;
                (localEvidence ??= new List<string>(MaxEvidenceLinesPerFramework))
                    .Add($"{matchedAgainst} matches /{hit}/ (+{ClassRegexMatchScore})");
            }

            var matched = MatchModules(f, modulesLowered);
            if (matched.Count > 0)
            {
                score += matched.Count;
                (localEvidence ??= new List<string>(MaxEvidenceLinesPerFramework))
                    .Add($"Modules: {string.Join(", ", matched)} (+{matched.Count})");
            }

            if (score > bestScore)
            {
                bestScore = score;
                best = f;
                bestModules = matched;
                var nextEvidence = new List<string>(MaxEvidenceLinesPerFramework + 1)
                {
                    $"== {f.DisplayName} (score {score}) =="
                };
                if (localEvidence is not null) nextEvidence.AddRange(localEvidence);
                bestEvidence = nextEvidence;
            }
        }

        return new ClassificationResult(best, BandFor(bestScore), bestScore, bestModules, bestEvidence);
    }

    /// <summary>HWND-class-only classification — used for island host detection.</summary>
    internal Framework ClassifyByClassOnly(string className)
    {
        if (string.IsNullOrEmpty(className)) return s_unknown;
        foreach (var f in _frameworks)
            if (f.CompiledClassRegex.Any(rx => rx.IsMatch(className)))
                return f;
        return s_unknown;
    }

    /// <summary>
    /// Best-effort guess at a window's host framework when its root HWND class isn't
    /// in our fingerprint table. Returns (null, empty) when no other framework's
    /// modules are present — the honest answer is "plain Win32 with no other runtimes".
    /// </summary>
    internal (Framework? Guess, IReadOnlyList<string> MatchedModules) GuessHostByModules(
        IReadOnlyList<string> processModules, string excludeFamily)
    {
        if (processModules == null || processModules.Count == 0)
            return (null, Array.Empty<string>());

        var modulesLowered = ToLoweredSet(processModules);
        Framework? best = null;
        IReadOnlyList<string> bestMatched = Array.Empty<string>();

        foreach (var f in _frameworks)
        {
            if (string.Equals(f.EffectiveFamily, excludeFamily, StringComparison.OrdinalIgnoreCase))
                continue;
            if (f.Id == UnknownFrameworkId) continue;

            var matched = MatchModules(f, modulesLowered);

            if (matched.Count > bestMatched.Count)
            {
                best = f;
                bestMatched = matched;
            }
        }

        return (best, bestMatched);
    }

    internal static HashSet<string> ToLoweredSet(IReadOnlyList<string> processModules)
    {
        var set = new HashSet<string>(processModules.Count, StringComparer.Ordinal);
        foreach (var m in processModules) set.Add(m.ToLowerInvariant());
        return set;
    }

    // Typical hit count per framework is 1–3 (e.g. WPF: PresentationCore + PresentationFramework + WindowsBase).
    private const int InitialMatchedModulesCapacity = 4;

    /// <summary>
    /// Returns the original-case framework module names present in <paramref name="loweredProcessModules"/>.
    /// Lazy-allocates the result (most checks find zero matches) and returns
    /// <see cref="Array.Empty{T}"/> on no match — callers cannot mutate shared state.
    /// </summary>
    internal static IReadOnlyList<string> MatchModules(Framework framework, HashSet<string> loweredProcessModules)
    {
        List<string>? matched = null;
        var lowered = framework.ModulesLowered;
        var original = framework.Modules;
        for (int i = 0; i < lowered.Count; i++)
        {
            if (loweredProcessModules.Contains(lowered[i]))
                (matched ??= new List<string>(InitialMatchedModulesCapacity)).Add(original[i]);
        }
        return matched ?? (IReadOnlyList<string>)Array.Empty<string>();
    }
}
