// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Engine.Storage;

// Stores a session's CPU samples with their call stacks (SessionSchema: Modules, CodeAddresses, CallStacks, CpuSamples).
// Each module and code address is stored once, and the stacks as a tree from their root frames, so the callers a stack
// shares with others are stored once. Used on the session writer's thread, in its transactions.
internal sealed class CpuSampleIndex : IDisposable
{
    private readonly SqliteConnection connection;
    private readonly SqliteCommand insertModule;
    private readonly SqliteCommand insertAddress;
    private readonly SqliteCommand insertStack;
    private readonly SqliteCommand insertSample;
    private readonly Dictionary<SampledModule, long> modules = [];
    private readonly Dictionary<(long ModuleId, ulong Address), long> addresses = [];
    private readonly Dictionary<(long ParentId, long AddressId), long> stacks = [];

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
    }

    // Samples stored, including those trimming deleted later.
    internal long Stored { get; private set; }

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
            var addressId = AddressId(sample.Frames[i]);
            if (!stacks.TryGetValue((stackId, addressId), out var node))
            {
                node = stacks.Count + 1;
                insertStack.Parameters["$id"].Value = node;
                insertStack.Parameters["$parent"].Value = stackId == 0 ? DBNull.Value : stackId;
                insertStack.Parameters["$address"].Value = addressId;
                insertStack.ExecuteNonQuery();
                stacks.Add((stackId, addressId), node);
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
        Stored++;
    }

    // Deletes the samples from before qpc, as trimming deletes the events from before it. The stacks stay.
    internal void Trim(long qpc, SqliteTransaction transaction) =>
        Sql.Execute(connection, "DELETE FROM CpuSamples WHERE Qpc < $qpc", transaction, ("$qpc", qpc));

    // Stores what the sampling recorded: the modules' PDB identities, which a trace can give after their first sample,
    // and the CpuSampling row.
    internal void Finish(CpuSamplingSummary? summary, long dropped, string? note, SqliteTransaction transaction)
    {
        foreach (var (module, id) in modules)
        {
            Sql.Execute(connection,
                "UPDATE Modules SET TimeDateStamp = $stamp, PdbName = $pdb, PdbGuid = $guid, PdbAge = $age WHERE ModuleId = $id",
                transaction, ("$id", id), ("$stamp", module.TimeDateStamp == 0 ? null : module.TimeDateStamp),
                ("$pdb", module.PdbName), ("$guid", module.PdbName is null ? null : module.PdbGuid.ToString("D", CultureInfo.InvariantCulture)),
                ("$age", module.PdbName is null ? null : module.PdbAge));
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
            id = addresses.Count + 1;
            insertAddress.Parameters["$id"].Value = id;
            insertAddress.Parameters["$module"].Value = moduleId == 0 ? DBNull.Value : moduleId;
            insertAddress.Parameters["$address"].Value = unchecked((long)frame.Address);
            insertAddress.Parameters["$rva"].Value = frame.Module is { } owner ? unchecked((long)(frame.Address - owner.ImageBase)) : DBNull.Value;
            insertAddress.ExecuteNonQuery();
            addresses.Add((moduleId, frame.Address), id);
        }

        return id;
    }

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

    public void Dispose()
    {
        insertModule.Dispose();
        insertAddress.Dispose();
        insertStack.Dispose();
        insertSample.Dispose();
    }
}
