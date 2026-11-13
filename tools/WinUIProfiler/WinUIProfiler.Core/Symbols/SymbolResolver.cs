// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Diagnostics.Symbols;

namespace WinUIProfiler.Engine.Symbols;

// How far symbol lookup got for a module; Modules.SymbolStatus stores the name.
public enum SymbolStatus
{
    // The module's PDB was found and names its functions.
    Resolved,

    // No location in the symbol path has the module's PDB.
    NotFound,

    // The trace did not record which PDB the module was built with, and the binary could not be read.
    NoIdentity,

    // The module has a portable PDB (managed code), which has no native function names.
    Portable,

    // Finding or reading the PDB failed.
    Error,
}

// What SymbolResolver found for a module. FindName maps an RVA to the name of the function containing it, or null.
internal sealed record ModuleSymbols(SymbolStatus Status, string? SymbolFile, Func<uint, string?>? FindName, string? Error = null);

// Finds and opens modules' PDBs, searching only the given symbol path (see SymbolPaths).
// - Network shares in the path are checked first, in parallel. One that does not answer within 10 seconds is left out,
//   so a missing share costs one wait rather than one per PDB.
// - Results are kept per PDB. Not thread-safe.
internal sealed class SymbolResolver : IDisposable
{
    private static readonly TimeSpan ShareTimeout = TimeSpan.FromSeconds(10);
    private static readonly TimeSpan ServerAnswerTimeout = TimeSpan.FromSeconds(30);

    private readonly object gate = new();
    private readonly HashSet<string> warned = [];
    private readonly Action<string>? warn;
    private readonly Dictionary<PdbIdentity, ModuleSymbols> results = [];
    private readonly SymbolReader reader;

    // Throws ArgumentException when the symbol path is malformed. Warnings may arrive on any thread, one at a time. Stop
    // cuts short the wait for network shares and the symbol server requests, and a lookup it interrupts finds nothing.
    internal SymbolResolver(string symbolPath, Action<string>? warn = null, TextWriter? log = null, CancellationToken stop = default)
    {
        this.warn = warn;
        var sources = Reachable(SymbolPaths.Sources(symbolPath), stop);
        SearchPath = string.Join(';', sources);
        if (sources.Count == 0)
        {
            Warn("No location in the symbol path is reachable, so functions will not be named.");
        }

        // SymbolReader reads _NT_SYMBOL_PATH only when given null.
        reader = new SymbolReader(log ?? TextWriter.Null, SearchPath, new SymbolServerHandler(Warn, stop))
        {
            // PDBs from any location in the path are trusted; the user chose them.
            SecurityCheck = _ => true,
            ServerTimeout = ServerAnswerTimeout,
        };
    }

    // The symbol path SymbolReader searches.
    internal string SearchPath { get; }

    internal ModuleSymbols Open(PdbIdentity? identity)
    {
        if (identity is null || identity.Guid == Guid.Empty || string.IsNullOrWhiteSpace(Path.GetFileName(identity.PdbName)))
        {
            return new ModuleSymbols(SymbolStatus.NoIdentity, null, null);
        }

        if (identity.Portable)
        {
            return new ModuleSymbols(SymbolStatus.Portable, null, null);
        }

        if (!results.TryGetValue(identity, out var result))
        {
            result = Find(identity);
            results[identity] = result;
        }

        return result;
    }

    public void Dispose() => reader.Dispose();

    private ModuleSymbols Find(PdbIdentity identity)
    {
        try
        {
            // Only the file name: SymbolReader would also look for the PDB where it was built.
            var pdbPath = reader.FindSymbolFilePath(Path.GetFileName(identity.PdbName), identity.Guid, identity.Age);
            if (pdbPath is null)
            {
                return new ModuleSymbols(SymbolStatus.NotFound, null, null);
            }

            var module = reader.OpenNativeSymbolFile(pdbPath);
            return module is null
                ? new ModuleSymbols(SymbolStatus.Portable, pdbPath, null)
                : new ModuleSymbols(SymbolStatus.Resolved, pdbPath, Lookup(pdbPath, module));
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            // The message is stored and shown, so it must not carry the symbol path's credentials.
            return new ModuleSymbols(SymbolStatus.Error, null, null, SymbolPaths.Redact(error.Message));
        }
    }

    // SymbolReader keeps a few PDBs open and disposes the oldest, so a lookup reopens its PDB when that happened.
    private Func<uint, string?> Lookup(string pdbPath, NativeSymbolModule module) => rva =>
    {
        string name;
        try
        {
            name = module.FindNameForRva(rva);
        }
        catch (ObjectDisposedException)
        {
            module = reader.OpenNativeSymbolFile(pdbPath) ?? throw new InvalidOperationException($"{pdbPath} could not be reopened.");
            name = module.FindNameForRva(rva);
        }

        return string.IsNullOrEmpty(name) ? null : name;
    };

    // Leaves out locations on network shares that do not answer.
    private List<SymbolSource> Reachable(List<SymbolSource> sources, CancellationToken stop)
    {
        static string? ShareOf(string? location) =>
            location is not null && SymbolPaths.IsUnc(location) ? Path.GetPathRoot(location) : null;

        var probes = sources
            .SelectMany(source => new[] { ShareOf(source.Cache), ShareOf(source.Target) })
            .OfType<string>()
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .ToDictionary(share => share, share => Task.Run(() => Directory.Exists(share)), StringComparer.OrdinalIgnoreCase);
        if (probes.Count == 0)
        {
            return sources;
        }

        try
        {
            Task.WaitAll([.. probes.Values], (int)ShareTimeout.TotalMilliseconds, stop);
        }
        catch (AggregateException)
        {
            // A failed probe counts as unreachable below.
        }

        var unreachable = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach (var (share, probe) in probes)
        {
            if (!(probe.IsCompletedSuccessfully && probe.Result))
            {
                unreachable.Add(share);
                Warn($"Symbol path location {share} is not reachable, so it is skipped.");
            }
        }

        return sources
            .Where(source => ShareOf(source.Cache) is not { } cache || !unreachable.Contains(cache))
            .Where(source => ShareOf(source.Target) is not { } target || !unreachable.Contains(target))
            .ToList();
    }

    private void Warn(string message)
    {
        lock (gate)
        {
            if (warned.Add(message))
            {
                warn?.Invoke(message);
            }
        }
    }
}
