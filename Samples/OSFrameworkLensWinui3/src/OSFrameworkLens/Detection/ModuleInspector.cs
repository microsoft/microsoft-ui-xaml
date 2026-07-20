using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace OSFrameworkLens.Detection;

internal static class ModuleInspector
{
    // Win32 MAX_PATH is 260; long-path module paths can exceed it. 1024 covers
    // all real cases — and we only keep the file name, not the directory.
    private const int ModuleNameBufferSize = 1024;

    // Cache holds the display list AND a pre-lowered hashset so the 30 Hz tick
    // doesn't re-lower ~100 module names per frame.
    private sealed record CacheEntry(
        DateTime FetchedUtc,
        IReadOnlyList<string> Modules,
        HashSet<string> ModulesLowered,
        bool Available);

    private static readonly ConcurrentDictionary<uint, CacheEntry> s_cache = new();
    private static readonly TimeSpan s_ttl = TimeSpan.FromSeconds(2);
    // Shared sentinel — callers only call .Contains on it.
    private static readonly HashSet<string> s_emptyLowered = new(StringComparer.Ordinal);

    /// <summary>
    /// Returns the loaded module file names plus the cached lowered hashset for
    /// classifier lookups. Available=false means we couldn't enumerate (protected,
    /// elevated, or 32-bit target) — caller must not interpret it as "no modules".
    /// </summary>
    public static (IReadOnlyList<string> Modules, HashSet<string> ModulesLowered, bool Available)
        GetModuleNamesWithLowered(uint pid)
    {
        var entry = GetOrFetch(pid);
        return (entry.Modules, entry.ModulesLowered, entry.Available);
    }

    private static CacheEntry GetOrFetch(uint pid)
    {
        // Two-phase: cheap TTL check first (the 30 Hz common case), then GetOrAdd
        // on miss. The factory may run twice in a race; harmless because Fetch is idempotent.
        if (s_cache.TryGetValue(pid, out var hit) && DateTime.UtcNow - hit.FetchedUtc < s_ttl)
            return hit;

        return s_cache.AddOrUpdate(
            pid,
            addValueFactory: static p => BuildEntry(p),
            updateValueFactory: (p, existing) =>
                DateTime.UtcNow - existing.FetchedUtc < s_ttl ? existing : BuildEntry(p));
    }

    private static CacheEntry BuildEntry(uint pid)
    {
        var (mods, ok) = Fetch(pid);
        return new CacheEntry(DateTime.UtcNow, mods, BuildLowered(mods), ok);
    }

    private static HashSet<string> BuildLowered(IReadOnlyList<string> modules)
    {
        if (modules.Count == 0) return s_emptyLowered;
        var set = new HashSet<string>(modules.Count, StringComparer.Ordinal);
        for (int i = 0; i < modules.Count; i++)
            set.Add(modules[i].ToLowerInvariant());
        return set;
    }

    private static (IReadOnlyList<string> Modules, bool Available) Fetch(uint pid)
    {
        // 64-bit only: a 64-bit caller can't EnumProcessModulesEx a WOW64 target —
        // OpenProcess succeeds but enumeration returns ERROR_PARTIAL_COPY. We
        // surface Available=false and let the classifier degrade to UIA + class signals.
        var hProc = Native.OpenProcess(
            Native.PROCESS_QUERY_LIMITED_INFORMATION | Native.PROCESS_VM_READ, false, pid);
        if (hProc == IntPtr.Zero) return (Array.Empty<string>(), false);
        try
        {
            var inProc = FetchInProcess(hProc);
            // Empty list is itself a failure signal — real processes load dozens of modules.
            return inProc.Count > 0 ? (inProc, true) : (Array.Empty<string>(), false);
        }
        finally { Native.CloseHandle(hProc); }
    }

    private static IReadOnlyList<string> FetchInProcess(IntPtr hProc)
    {
        if (!Native.EnumProcessModulesEx(hProc, Array.Empty<IntPtr>(), 0, out var needed, Native.LIST_MODULES_ALL))
            return Array.Empty<string>();
        var count = (int)(needed / (uint)IntPtr.Size);
        if (count == 0) return Array.Empty<string>();
        var handles = new IntPtr[count];
        if (!Native.EnumProcessModulesEx(hProc, handles, needed, out _, Native.LIST_MODULES_ALL))
            return Array.Empty<string>();

        var names = new List<string>(count);
        var sb = new StringBuilder(ModuleNameBufferSize);
        foreach (var h in handles)
        {
            sb.Clear();
            if (Native.GetModuleFileNameExW(hProc, h, sb, (uint)sb.Capacity) > 0)
                names.Add(Path.GetFileName(sb.ToString()));
        }
        return names;
    }
}
