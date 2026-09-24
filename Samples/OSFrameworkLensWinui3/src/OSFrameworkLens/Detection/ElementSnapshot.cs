using System.Collections.Generic;

namespace OSFrameworkLens.Detection;

internal sealed record ElementSnapshot(
    string FrameworkId,
    string FrameworkDisplayName,
    string FrameworkColorHex,
    string Confidence,
    int Score,
    string ClassName,
    string Title,
    bool TitleAvailable,
    string ProcessName,
    uint Pid,
    uint Tid,
    nint Hwnd,
    nint RootHwnd,
    string RootClassName,
    (int Left, int Top, int Right, int Bottom) Bounds,
    string UiaFrameworkId,
    string UiaControlType,
    string UiaName,
    string UiaAutomationId,
    string UiaClassName,
    IReadOnlyList<string> MatchedModules,
    IReadOnlyList<string> AllModules,
    bool ModuleListAvailable,
    IReadOnlyList<string> EvidenceLog,
    bool IsIsland,
    string IslandHostFrameworkDisplayName,
    bool IslandHostFingerprintMatched,
    string IslandHostGuessText);
