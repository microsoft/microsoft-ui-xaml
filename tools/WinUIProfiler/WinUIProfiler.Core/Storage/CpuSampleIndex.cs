// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Runtime.InteropServices;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Engine.Storage;

// Stores a session's CPU samples with their call stacks (SessionSchema: Modules, CodeAddresses, CallStacks, CpuSamples).
// Each module and code address is stored once, and the stacks as a tree from their root frames, so the callers a stack
// shares with others are stored once. Trimming deletes the oldest samples, then the stacks and code addresses no sample
// has any more. It finds those from counts, kept in memory, of what has each node and address, so it costs what it
// deletes rather than a search of the file. Used on the session writer's thread, in its transactions.
internal sealed class CpuSampleIndex : IDisposable
{
    // Stored in CpuSampling.Note once samples were deleted to keep the file within its size limit.
    internal const string TrimmedNote = "The oldest CPU samples were deleted to keep the file within its size limit.";

    // Space a sample takes, its row and its index entry, and a stack node and a code address when they're new.
    private const int SampleBytes = 48;
    private const int StackBytes = 32;
    private const int AddressBytes = 48;

    // The process of an address stored without a module that samples of several processes have.
    private const int MixedProcesses = int.MinValue;

    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertModule;
    private readonly SqliteCommand insertAddress;
    private readonly SqliteCommand insertStack;
    private readonly SqliteCommand insertSample;
    private readonly SqliteCommand deleteStack;
    private readonly SqliteCommand deleteAddress;
    private readonly Dictionary<SampledModule, long> modules = [];
    private readonly Dictionary<(long ModuleId, ulong Address), long> addresses = [];
    private readonly Dictionary<(long ParentId, long AddressId), long> stacks = [];
    // The stored stack nodes and code addresses by ID, with how many samples and nodes have each.
    private readonly Dictionary<long, StackNode> nodes = [];
    private readonly Dictionary<long, AddressUse> addressUses = [];
    // The stack nodes trimming left without a sample or a child node, for Prune to delete.
    private readonly List<long> released = [];
    // The addresses stored without a module, by AddressId: a module known only from the trace's end may have had them.
    private readonly Dictionary<long, UnplacedAddress> unplaced = [];
    // The last IDs given. Trimming deletes rows, so the dictionaries' sizes don't give the next.
    private long lastAddressId;
    private long lastStackId;
    private bool trimmed;

    internal CpuSampleIndex(SqliteConnection connection)
    {
        this.connection = connection;
        insertModule = Sql.Prepare(connection, """
            INSERT INTO Modules (ModuleId, ProcessId, FilePath, Name, ImageBase, ImageSize, TimeDateStamp, PdbName, PdbGuid, PdbAge)
            VALUES ($id, $pid, $path, $name, $base, $size, $stamp, $pdb, $guid, $age)
            """, ("$id", SqliteType.Integer), ("$pid", SqliteType.Integer), ("$path", SqliteType.Text), ("$name", SqliteType.Text),
            ("$base", SqliteType.Integer), ("$size", SqliteType.Integer), ("$stamp", SqliteType.Integer), ("$pdb", SqliteType.Text),
            ("$guid", SqliteType.Text), ("$age", SqliteType.Integer));
        insertAddress = Sql.Prepare(connection,
            "INSERT INTO CodeAddresses (AddressId, ModuleId, Address, Rva) VALUES ($id, $module, $address, $rva)",
            ("$id", SqliteType.Integer), ("$module", SqliteType.Integer), ("$address", SqliteType.Integer), ("$rva", SqliteType.Integer));
        insertStack = Sql.Prepare(connection,
            "INSERT INTO CallStacks (StackId, ParentId, AddressId) VALUES ($id, $parent, $address)",
            ("$id", SqliteType.Integer), ("$parent", SqliteType.Integer), ("$address", SqliteType.Integer));
        insertSample = Sql.Prepare(connection,
            "INSERT INTO CpuSamples (Qpc, ProcessId, ThreadId, StackId) VALUES ($qpc, $pid, $thread, $stack)",
            ("$qpc", SqliteType.Integer), ("$pid", SqliteType.Integer), ("$thread", SqliteType.Integer), ("$stack", SqliteType.Integer));
        deleteStack = Sql.Prepare(connection, "DELETE FROM CallStacks WHERE StackId = $id", ("$id", SqliteType.Integer));
        deleteAddress = Sql.Prepare(connection, "DELETE FROM CodeAddresses WHERE AddressId = $id", ("$id", SqliteType.Integer));
    }

    // The samples the file has: trimming deletes the oldest.
    internal long Samples { get; private set; }

    // About how much storing the samples adds to the file: their rows, and the stack nodes and code addresses not stored
    // yet, each once.
    internal long IncomingBytes(List<CompletedSample> samples)
    {
        // Those the batch adds, with IDs below zero, which no stored row has.
        var newAddresses = new Dictionary<(SampledModule? Module, ulong Address), long>();
        var newStacks = new Dictionary<(long ParentId, long AddressId), long>();
        var bytes = (long)samples.Count * SampleBytes;
        foreach (var sample in samples)
        {
            long stackId = 0;
            for (var i = sample.Frames.Length - 1; i >= 0; i--)
            {
                var frame = sample.Frames[i];
                if (!StoredAddress(frame, out var addressId) && !newAddresses.TryGetValue((frame.Module, frame.Address), out addressId))
                {
                    addressId = -(newAddresses.Count + 1);
                    newAddresses.Add((frame.Module, frame.Address), addressId);
                    bytes += AddressBytes;
                }

                if (!stacks.TryGetValue((stackId, addressId), out var node) && !newStacks.TryGetValue((stackId, addressId), out node))
                {
                    node = -(newStacks.Count + 1);
                    newStacks.Add((stackId, addressId), node);
                    bytes += StackBytes;
                }

                stackId = node;
            }
        }

        return bytes;
    }

    internal void Add(CompletedSample sample, SqliteTransaction transaction)
    {
        insertModule.Transaction = transaction;
        insertAddress.Transaction = transaction;
        insertStack.Transaction = transaction;
        insertSample.Transaction = transaction;

        // The frames are leaf first; the tree grows from the root.
        long stackId = 0;
        for (var i = sample.Frames.Length - 1; i >= 0; i--)
        {
            var frame = sample.Frames[i];
            var addressId = AddressId(frame);
            if (frame.Module is null)
            {
                NoteUnplaced(addressId, frame.Address, sample);
            }

            if (!stacks.TryGetValue((stackId, addressId), out var node))
            {
                node = ++lastStackId;
                insertStack.Parameters["$id"].Value = node;
                insertStack.Parameters["$parent"].Value = stackId == 0 ? DBNull.Value : stackId;
                insertStack.Parameters["$address"].Value = addressId;
                insertStack.ExecuteNonQuery();
                stacks.Add((stackId, addressId), node);
                nodes.Add(node, new(stackId, addressId, 0));
                Address(addressId).Nodes++;
                if (stackId != 0)
                {
                    Node(stackId).Users++;
                }
            }

            stackId = node;
        }

        if (stackId == 0)
        {
            return;
        }

        insertSample.Parameters["$qpc"].Value = sample.Qpc;
        insertSample.Parameters["$pid"].Value = sample.ProcessId;
        insertSample.Parameters["$thread"].Value = sample.ThreadId;
        insertSample.Parameters["$stack"].Value = stackId;
        insertSample.ExecuteNonQuery();
        Node(stackId).Users++;
        Samples++;
    }

    // Deletes the samples from before qpc, as trimming the events deletes those from before it, and returns how many.
    // Prune then deletes the stacks they leave unused.
    internal long Trim(long qpc, SqliteTransaction transaction) =>
        Deleted(Sql.Longs(connection, "DELETE FROM CpuSamples WHERE Qpc < $qpc RETURNING StackId", transaction, ("$qpc", qpc)));

    // Deletes the first count samples stored, the oldest, but none from beforeQpc on when it's given, and returns how many
    // it deleted. Prune then deletes the stacks they leave unused.
    internal long TrimOldest(long count, long? beforeQpc, SqliteTransaction transaction)
    {
        count = Math.Min(count, Samples);
        return count <= 0 ? 0 : Deleted(Sql.Longs(connection, """
            DELETE FROM CpuSamples
            WHERE rowid <= (SELECT rowid FROM CpuSamples ORDER BY rowid LIMIT 1 OFFSET $skip) AND ($before IS NULL OR Qpc < $before)
            RETURNING StackId
            """, transaction, ("$skip", count - 1), ("$before", beforeQpc)));
    }

    // The time of the first sample stored, which is the oldest but for the few a live session stores late; null when
    // there are none. Call outside a transaction.
    internal long? FirstQpc() =>
        Samples == 0 ? null : Sql.Scalar(connection, "SELECT Qpc FROM CpuSamples ORDER BY rowid LIMIT 1");

    // Deletes the stack nodes the trimmed samples left unused, then the callers that leaves unused, and the code addresses
    // no node has any more, and forgets them, so a later sample stores them again rather than referring to the deleted
    // rows. Call in the trimming transaction.
    internal void Prune(SqliteTransaction transaction)
    {
        deleteStack.Transaction = transaction;
        deleteAddress.Transaction = transaction;
        while (released.Count > 0)
        {
            var id = released[^1];
            released.RemoveAt(released.Count - 1);

            // Skips a node deleted already, or used again since it was released.
            if (!nodes.TryGetValue(id, out var node) || node.Users > 0)
            {
                continue;
            }

            nodes.Remove(id);
            stacks.Remove((node.ParentId, node.AddressId));
            deleteStack.Parameters["$id"].Value = id;
            deleteStack.ExecuteNonQuery();
            if (--Address(node.AddressId).Nodes == 0)
            {
                addressUses.Remove(node.AddressId, out var address);
                addresses.Remove((address.ModuleId, address.Address));
                unplaced.Remove(node.AddressId);
                deleteAddress.Parameters["$id"].Value = node.AddressId;
                deleteAddress.ExecuteNonQuery();
            }

            if (node.ParentId != 0 && --Node(node.ParentId).Users == 0)
            {
                released.Add(node.ParentId);
            }
        }
    }

    // Stores what the sampling recorded: the modules of the addresses stored without one that a module known only from
    // the trace's end had, the modules' PDB identities, which a trace can give after their first sample, and the
    // CpuSampling row, noting when samples were trimmed.
    internal void Finish(CpuSamplingSummary? summary, long dropped, string? note, SqliteTransaction transaction)
    {
        if (summary is not null)
        {
            Place(summary.Modules, transaction);
        }

        foreach (var (module, id) in modules)
        {
            Sql.Execute(connection,
                "UPDATE Modules SET TimeDateStamp = $stamp, PdbName = $pdb, PdbGuid = $guid, PdbAge = $age WHERE ModuleId = $id",
                transaction, ("$id", id), ("$stamp", module.TimeDateStamp == 0 ? null : module.TimeDateStamp),
                ("$pdb", module.PdbName), ("$guid", module.PdbName is null ? null : module.PdbGuid.ToString("D", CultureInfo.InvariantCulture)),
                ("$age", module.PdbName is null ? null : module.PdbAge));
        }

        if (trimmed)
        {
            note = note is null ? TrimmedNote : $"{note} {TrimmedNote}";
        }

        Sql.Execute(connection, """
            INSERT INTO CpuSampling (IntervalMs, Samples, Dropped, CompressedStacks, Note)
            VALUES ($interval, (SELECT COUNT(*) FROM CpuSamples), $dropped, $compressed, $note)
            """, transaction, ("$interval", summary?.IntervalMilliseconds), ("$dropped", dropped),
            ("$compressed", summary?.CompressedStacks == true), ("$note", note));
    }

    private long AddressId(CodeFrame frame)
    {
        var moduleId = frame.Module is { } module ? ModuleId(module) : 0;
        if (!addresses.TryGetValue((moduleId, frame.Address), out var id))
        {
            id = ++lastAddressId;
            insertAddress.Parameters["$id"].Value = id;
            insertAddress.Parameters["$module"].Value = moduleId == 0 ? DBNull.Value : moduleId;
            insertAddress.Parameters["$address"].Value = unchecked((long)frame.Address);
            insertAddress.Parameters["$rva"].Value = frame.Module is { } owner ? unchecked((long)(frame.Address - owner.ImageBase)) : DBNull.Value;
            insertAddress.ExecuteNonQuery();
            addresses.Add((moduleId, frame.Address), id);
            addressUses.Add(id, new(moduleId, frame.Address, 0));
        }

        return id;
    }

    private ref StackNode Node(long stackId) => ref CollectionsMarshal.GetValueRefOrNullRef(nodes, stackId);

    private ref AddressUse Address(long addressId) => ref CollectionsMarshal.GetValueRefOrNullRef(addressUses, addressId);

    private long ModuleId(SampledModule module)
    {
        if (!modules.TryGetValue(module, out var id))
        {
            id = modules.Count + 1;
            insertModule.Parameters["$id"].Value = id;
            insertModule.Parameters["$pid"].Value = module.ProcessId;
            insertModule.Parameters["$path"].Value = module.FilePath;
            insertModule.Parameters["$name"].Value = Path.GetFileName(module.FilePath) is { Length: > 0 } name ? name : module.FilePath;
            insertModule.Parameters["$base"].Value = unchecked((long)module.ImageBase);
            insertModule.Parameters["$size"].Value = module.ImageSize;
            insertModule.Parameters["$stamp"].Value = module.TimeDateStamp == 0 ? DBNull.Value : module.TimeDateStamp;
            insertModule.Parameters["$pdb"].Value = (object?)module.PdbName ?? DBNull.Value;
            insertModule.Parameters["$guid"].Value = module.PdbName is null ? DBNull.Value : module.PdbGuid.ToString("D", CultureInfo.InvariantCulture);
            insertModule.Parameters["$age"].Value = module.PdbName is null ? DBNull.Value : module.PdbAge;
            insertModule.ExecuteNonQuery();
            modules.Add(module, id);
        }

        return id;
    }

    // The stored code address of the frame, when it has one.
    private bool StoredAddress(CodeFrame frame, out long addressId)
    {
        long moduleId = 0;
        addressId = 0;
        return (frame.Module is not { } module || modules.TryGetValue(module, out moduleId)) &&
            addresses.TryGetValue((moduleId, frame.Address), out addressId);
    }

    // Notes the process and the time of a sample with an address stored without a module.
    private void NoteUnplaced(long addressId, ulong address, CompletedSample sample)
    {
        unplaced[addressId] = unplaced.TryGetValue(addressId, out var seen)
            ? new(address, seen.ProcessId == sample.ProcessId ? seen.ProcessId : MixedProcesses,
                Math.Min(seen.FirstQpc, sample.Qpc), Math.Max(seen.LastQpc, sample.Qpc))
            : new(address, sample.ProcessId, sample.Qpc, sample.Qpc);
    }

    // Places each address stored without a module in the module known only from the trace's end (SampledModule.EndOnly)
    // that had it while it was sampled, when no other module could have.
    private void Place(IReadOnlyList<SampledModule> known, SqliteTransaction transaction)
    {
        var endOnly = known.Where(module => module.EndOnly).ToArray();
        if (endOnly.Length == 0)
        {
            return;
        }

        insertModule.Transaction = transaction;
        foreach (var (addressId, address) in unplaced)
        {
            if (!endOnly.Any(module => Had(module, address)) ||
                known.Where(module => Had(module, address)).Take(2).ToArray() is not [{ EndOnly: true } owner])
            {
                continue;
            }

            Sql.Execute(connection, "UPDATE CodeAddresses SET ModuleId = $module, Rva = $rva WHERE AddressId = $id", transaction,
                ("$id", addressId), ("$module", ModuleId(owner)), ("$rva", unchecked((long)(address.Address - owner.ImageBase))));
        }
    }

    // Whether the module had the address all the time it was sampled: a driver's is any process's.
    private static bool Had(SampledModule module, UnplacedAddress address) =>
        module.Contains(address.Address) && (module.ProcessId == -1 || module.ProcessId == address.ProcessId) &&
        module.Covers(address.FirstQpc) && module.Covers(address.LastQpc);

    // Counts the deleted samples, given by their stacks, and releases each one's node: Prune deletes those no sample or
    // child node has any more.
    private long Deleted(List<long> stackIds)
    {
        foreach (var stackId in stackIds)
        {
            if (--Node(stackId).Users == 0)
            {
                released.Add(stackId);
            }
        }

        Samples -= stackIds.Count;
        trimmed |= stackIds.Count > 0;
        return stackIds.Count;
    }

    // An address stored without a module, with the process of the samples that have it (MixedProcesses for several), and
    // the first and last time it was sampled.
    private readonly record struct UnplacedAddress(ulong Address, int ProcessId, long FirstQpc, long LastQpc);

    // A stored stack node, its key in stacks, and how many samples (as their leaf) and child nodes have it.
    private record struct StackNode(long ParentId, long AddressId, long Users);

    // A stored code address, its key in addresses, and how many stack nodes have it.
    private record struct AddressUse(long ModuleId, ulong Address, long Nodes);

    public void Dispose()
    {
        insertModule.Dispose();
        insertAddress.Dispose();
        insertStack.Dispose();
        insertSample.Dispose();
        deleteStack.Dispose();
        deleteAddress.Dispose();
    }
}
