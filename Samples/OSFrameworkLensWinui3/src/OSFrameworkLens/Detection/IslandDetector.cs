using System;
using System.Collections.Generic;

namespace OSFrameworkLens.Detection;

/// <summary>
/// Decides whether a leaf element lives in a host window from a different framework
/// family (an "island"). Matched: root class is known and in a different family.
/// Inferred: root class unknown and leaf is non-Win32 — host is guessed from modules
/// loaded in the root process.
/// </summary>
internal sealed class IslandDetector
{
    private readonly FrameworkClassifier _classifier;
    public IslandDetector(FrameworkClassifier classifier) => _classifier = classifier;

    internal readonly record struct Result(bool IsIsland, string HostDisplayName, bool HostFingerprintMatched, string HostGuessText);
    private static readonly Result None = new(false, "", false, "");

    public Result Detect(FrameworkClassifier.Framework leafFw, IntPtr leafHwnd, IntPtr rootHwnd,
                         string rootClassName, IReadOnlyList<string> processModules)
    {
        if (rootHwnd == leafHwnd || string.IsNullOrEmpty(rootClassName)) return None;

        var rootFw = _classifier.ClassifyByClassOnly(rootClassName);
        if (rootFw.Id != FrameworkClassifier.UnknownFrameworkId)
            return SameFamily(rootFw.EffectiveFamily, leafFw.EffectiveFamily)
                ? None : new Result(true, rootFw.DisplayName, true, "");

        // Root unknown → only an island if the leaf is a known, non-Win32 framework.
        if (leafFw.Id == FrameworkClassifier.UnknownFrameworkId
            || SameFamily(leafFw.EffectiveFamily, FrameworkClassifier.Win32FamilyId))
            return None;

        var (guess, mods) = _classifier.GuessHostByModules(processModules, leafFw.EffectiveFamily);
        var hostGuess = guess is not null && mods.Count > 0
            ? $"Best guess: {guess.DisplayName} host ({string.Join(", ", mods)} loaded in root process)"
            : "Best guess: plain Win32 host (no other framework runtimes detected in root process)";
        return new Result(true, "", false, hostGuess);
    }

    private static bool SameFamily(string a, string b) =>
        string.Equals(a, b, StringComparison.OrdinalIgnoreCase);
}

