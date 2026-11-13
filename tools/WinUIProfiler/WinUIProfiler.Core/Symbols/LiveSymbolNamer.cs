// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Symbols;

// Names the functions in the sampled call stacks of a session that is still recording, from the symbol path. The names are
// kept in memory, since the recording writes the file meanwhile; SessionSymbolizer stores them when it ends.
// - Pass Request to CpuProfileReader.ReadStacks to show the names found so far and ask for the rest; pass NameOf to only
//   show them.
// - Each Name round names the addresses asked for, module by module, the module with the most of them first.
// - Rounds run one at a time, and not while SessionSymbolizer runs.
public sealed class LiveSymbolNamer : IDisposable
{
    // A round that has named addresses returns after about this long, so they can be shown while the next round runs.
    private static readonly TimeSpan RoundTime = TimeSpan.FromSeconds(2);

    private readonly object gate = new();
    // Canceled by Dispose, to stop a running round.
    private readonly CancellationTokenSource lifetime = new();
    private readonly Dictionary<long, string> names = [];
    // Addresses a round looked up, named or not; they aren't asked for again.
    private readonly HashSet<long> done = [];
    private readonly HashSet<long> wanted = [];
    // What was found for each module, except a module whose PDB identity isn't known yet.
    private readonly Dictionary<long, ModuleSymbols> modules = [];
    private SymbolResolver? resolver;
    private int rounds;
    private bool disposed;

    // Throws ArgumentException for a malformed symbol path.
    public LiveSymbolNamer(string sessionPath, string symbolPath)
    {
        ArgumentException.ThrowIfNullOrEmpty(sessionPath);
        _ = SymbolPaths.Normalize(symbolPath);
        SessionPath = sessionPath;
        SymbolPath = symbolPath;
    }

    public string SessionPath { get; }

    public string SymbolPath { get; }

    // The function name found for a code address (CodeAddresses.AddressId), or null.
    public string? NameOf(long addressId)
    {
        lock (gate)
        {
            return names.GetValueOrDefault(addressId);
        }
    }

    // Whether a read asked for addresses that no round has looked up yet.
    public bool Requested
    {
        get
        {
            lock (gate)
            {
                return wanted.Count > 0;
            }
        }
    }

    // As NameOf, and asks the next round for an address that no round looked up.
    public string? Request(long addressId)
    {
        lock (gate)
        {
            if (names.TryGetValue(addressId, out var name))
            {
                return name;
            }

            if (!done.Contains(addressId))
            {
                wanted.Add(addressId);
            }

            return null;
        }
    }

    // Names the addresses asked for, and returns how many it named. It returns early once it has named some and has run
    // for a while, so call it until it returns 0. Throws as SessionFile.Open does, and OperationCanceledException when the
    // token is canceled or the namer is disposed; the addresses the round didn't finish are asked for again.
    public int Name(CancellationToken cancellationToken = default)
    {
        CancellationTokenSource round;
        lock (gate)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            rounds++;
            // Linked under the lock, so Dispose cannot dispose lifetime first.
            round = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, lifetime.Token);
        }

        try
        {
            SessionSymbolizer.Gate.Wait(round.Token);
            try
            {
                return NameRound(round.Token);
            }
            finally
            {
                SessionSymbolizer.Gate.Release();
            }
        }
        finally
        {
            round.Dispose();
            SymbolResolver? closing = null;
            lock (gate)
            {
                if (--rounds == 0 && disposed)
                {
                    (closing, resolver) = (resolver, null);
                }
            }

            closing?.Dispose();
        }
    }

    // Cancels a running round, whose PDBs are closed when it ends.
    public void Dispose()
    {
        SymbolResolver? closing = null;
        lock (gate)
        {
            if (disposed)
            {
                return;
            }

            disposed = true;
            if (rounds == 0)
            {
                (closing, resolver) = (resolver, null);
            }
        }

        lifetime.Cancel();
        lifetime.Dispose();
        closing?.Dispose();
    }

    private int NameRound(CancellationToken cancellationToken)
    {
        HashSet<long> asked;
        lock (gate)
        {
            if (wanted.Count == 0)
            {
                return 0;
            }

            asked = [.. wanted];
        }

        Dictionary<long, List<SessionSymbolizer.StoredAddress>> addresses;
        List<SessionSymbolizer.StoredModule> stored;
        using (var connection = SessionFile.Open(SessionPath))
        {
            addresses = SessionSymbolizer.ReadAddresses(connection);
            stored = SessionSymbolizer.ReadModules(connection);
        }

        var work = stored
            .Select(module => (Module: module, Pending: addresses.TryGetValue(module.ModuleId, out var list)
                ? list.Where(address => asked.Contains(address.AddressId)).ToList()
                : []))
            .Where(item => item.Pending.Count > 0)
            .OrderByDescending(item => item.Pending.Count)
            .ToList();

        // Addresses outside a known module, or already named by the file, have nothing to look up.
        var inModules = work.SelectMany(item => item.Pending).Where(address => !address.Named)
            .Select(address => address.AddressId).ToHashSet();
        lock (gate)
        {
            foreach (var id in asked.Where(id => !inModules.Contains(id)))
            {
                done.Add(id);
                wanted.Remove(id);
            }
        }

        var stopwatch = Stopwatch.StartNew();
        var named = 0;
        foreach (var (module, all) in work)
        {
            if (named > 0 && stopwatch.Elapsed >= RoundTime)
            {
                break;
            }

            cancellationToken.ThrowIfCancellationRequested();
            var pending = all.Where(address => !address.Named).ToList();
            if (pending.Count == 0)
            {
                continue;
            }

            ModuleSymbols symbols;
            var found = new List<(long AddressId, string Name)>();
            try
            {
                symbols = Find(module, cancellationToken);
                if (symbols.FindName is { } findName)
                {
                    for (var index = 0; index < pending.Count; index++)
                    {
                        if (index % 1024 == 0)
                        {
                            cancellationToken.ThrowIfCancellationRequested();
                        }

                        var address = pending[index];
                        if (address.Rva is >= 0 and <= uint.MaxValue && findName((uint)address.Rva) is { } name)
                        {
                            found.Add((address.AddressId, name));
                        }
                    }
                }
            }
            catch (Exception error) when (error is not (OutOfMemoryException or OperationCanceledException))
            {
                // A module whose PDB can't be read stays unnamed; the others are still named.
                symbols = new ModuleSymbols(SymbolStatus.Error, null, null, error.Message);
                modules[module.ModuleId] = symbols;
                found.Clear();
            }

            lock (gate)
            {
                foreach (var (id, name) in found)
                {
                    names[id] = name;
                }

                foreach (var address in pending)
                {
                    wanted.Remove(address.AddressId);
                    // A module whose identity isn't known yet is tried again when a read asks again.
                    if (symbols.Status != SymbolStatus.NoIdentity)
                    {
                        done.Add(address.AddressId);
                    }
                }
            }

            named += found.Count;
        }

        return named;
    }

    private ModuleSymbols Find(SessionSymbolizer.StoredModule module, CancellationToken cancellationToken)
    {
        if (modules.TryGetValue(module.ModuleId, out var symbols))
        {
            return symbols;
        }

        var identity = SessionSymbolizer.StoredIdentity(module)
            ?? PeIdentity.Read(module.FilePath, module.TimeDateStamp, module.ImageSize);
        // Made once and kept across rounds, so what it found is reused; a round's token only cuts short its own lookups.
        resolver ??= new SymbolResolver(SymbolPath, cancellationToken: cancellationToken);
        symbols = resolver.Open(identity, cancellationToken);
        if (symbols.Status != SymbolStatus.NoIdentity)
        {
            modules[module.ModuleId] = symbols;
        }

        return symbols;
    }
}
