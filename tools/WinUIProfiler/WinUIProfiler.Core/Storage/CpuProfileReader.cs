// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Symbols;

namespace WinUIProfiler.Engine.Storage;

// What a session's CPU sampling recorded (its CpuSampling row). IntervalMs is the time a sample stands for; Dropped are the
// samples a live session could not store in time; SymbolPath and SymbolizedUtc are of the last symbolizing; Note says why
// samples are missing or partial, when known. While a session records it has no CpuSampling row yet: IntervalMs is null and
// Samples counts the samples stored so far.
public sealed record CpuSamplingInfo(double? IntervalMs, long Samples, long Dropped, bool CompressedStacks, string? SymbolPath,
    DateTime? SymbolizedUtc, string? Note);

// A sampled thread. UiThread is true for a thread XAML ticked frames on (it has Frames rows).
public sealed record CpuThreadSamples(int ThreadId, long Samples, double? Ms, bool UiThread);

// A frame (Module!Function, or Module!0xRva until symbolized) and the samples it was in, as the frame that ran (exclusive)
// or anywhere on the stack (inclusive), with the time they stand for.
public sealed record CpuFrameSamples(string Module, string Frame, long Samples, double? Ms);

// A sampled module and how symbolizing named its functions: Status is a SymbolStatus name, null until symbolized; SymbolFile
// is the PDB used, or why none was found. Pdb is the PDB the module was built with (file name, GUID and age, as a symbol
// server keys it), null when unknown. Addresses is how many of its code addresses the stacks have.
public sealed record CpuModuleSymbols(string Name, string FilePath, string? Status, string? SymbolFile, string? Pdb, long Addresses);

// A session's CPU samples, of all its threads or of ThreadId: the threads by samples, the frames with the most samples,
// exclusive and inclusive, and the sampled modules.
public sealed record CpuProfile(CpuSamplingInfo Sampling, int? ThreadId, long Samples, IReadOnlyList<CpuThreadSamples> Threads,
    IReadOnlyList<CpuFrameSamples> Exclusive, IReadOnlyList<CpuFrameSamples> Inclusive, IReadOnlyList<CpuModuleSymbols> Modules);

// Reads the CPU samples of a session file, for the CLI and the app. The same rows are in its views (HotFunctions,
// CpuSampleDetails, CpuFrames) for any SQLite reader.
public static class CpuProfileReader
{
    // The CpuSampling row, or null when the session did not sample CPU (or was written before CPU sampling was added). While
    // the session records, the samples stored so far, or null when there are none yet.
    public static CpuSamplingInfo? ReadSampling(string path)
    {
        using var connection = SessionFile.Open(path);
        return ReadSampling(connection);
    }

    // Null when the session did not sample CPU. threadId keeps one thread's samples; top is how many frames each list has.
    public static CpuProfile? Read(string path, int? threadId = null, int top = 20, CancellationToken cancellationToken = default)
    {
        ArgumentOutOfRangeException.ThrowIfNegativeOrZero(top);
        using var connection = SessionFile.Open(path);
        // One snapshot of a session that may still be recording.
        using var transaction = connection.BeginTransaction(deferred: true);
        if (ReadSampling(connection, transaction) is not { } sampling)
        {
            return null;
        }

        var threads = ReadThreads(connection, sampling);
        var (frames, nodes, leaves) = LoadStacks(connection, threadId, null, cancellationToken);
        var exclusive = new long[frames.Count];
        var inclusive = new long[frames.Count];
        var onStack = new HashSet<int>();
        long total = 0;
        foreach (var (stackId, samples) in leaves)
        {
            total += samples;
            if (!nodes.TryGetValue(stackId, out var node))
            {
                continue;
            }

            exclusive[node.Frame] += samples;
            // A recursive function counts once per sample.
            onStack.Clear();
            for (var current = stackId; current != 0 && nodes.TryGetValue(current, out node); current = node.Parent)
            {
                if (onStack.Add(node.Frame))
                {
                    inclusive[node.Frame] += samples;
                }
            }
        }

        IReadOnlyList<CpuFrameSamples> Top(long[] counts) => [.. Enumerable.Range(0, frames.Count)
            .Where(id => counts[id] > 0)
            .OrderByDescending(id => counts[id]).ThenBy(id => frames[id].Frame, StringComparer.Ordinal)
            .Take(top)
            .Select(id => new CpuFrameSamples(frames[id].Module, frames[id].Frame, counts[id], counts[id] * sampling.IntervalMs))];

        var modules = new List<CpuModuleSymbols>();
        using (var command = connection.CreateCommand())
        {
            command.CommandText = """
                SELECT m.Name, m.FilePath, m.SymbolStatus, m.SymbolFile, m.PdbName, m.PdbGuid, m.PdbAge, COUNT(a.AddressId) AS Addresses
                FROM Modules m LEFT JOIN CodeAddresses a ON a.ModuleId = m.ModuleId
                GROUP BY m.ModuleId
                ORDER BY Addresses DESC, m.Name
                """;
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                modules.Add(new(reader.GetString(0), reader.GetString(1), reader.IsDBNull(2) ? null : reader.GetString(2),
                    reader.IsDBNull(3) ? null : reader.GetString(3), PdbKey(reader, 4), reader.GetInt64(7)));
            }
        }

        return new(sampling, threadId, total, threads, Top(exclusive), Top(inclusive), modules);
    }

    // The call stacks of the session's CPU samples, of all its threads or of threadId, for call trees; null when the session
    // did not sample CPU. functionName names the code addresses the file does not name yet (by CodeAddresses.AddressId; see
    // LiveSymbolNamer), or returns null.
    public static CpuStacks? ReadStacks(string path, int? threadId = null, CancellationToken cancellationToken = default,
        Func<long, string?>? functionName = null)
    {
        using var connection = SessionFile.Open(path);
        // One snapshot of a session that may still be recording.
        using var transaction = connection.BeginTransaction(deferred: true);
        if (ReadSampling(connection, transaction) is not { } sampling)
        {
            return null;
        }

        var threads = ReadThreads(connection, sampling);
        var (frames, nodes, leaves) = LoadStacks(connection, threadId, functionName, cancellationToken);
        var stacks = new int[leaves.Count][];
        var counts = new long[leaves.Count];
        var walk = new List<int>();
        for (var leaf = 0; leaf < leaves.Count; leaf++)
        {
            // From the leaf node up its parents to the root frame. A missing node ends the stack, and the node count bounds
            // it, as a stored stack can't have a cycle.
            walk.Clear();
            var current = leaves[leaf].StackId;
            while (current != 0 && walk.Count < nodes.Count && nodes.TryGetValue(current, out var node))
            {
                walk.Add(node.Frame);
                current = node.Parent;
            }

            walk.Reverse();
            stacks[leaf] = [.. walk];
            counts[leaf] = leaves[leaf].Samples;
        }

        return new(sampling, threadId, threads, frames, stacks, counts);
    }

    // The sampled threads, most samples first, marking the UI threads.
    private static List<CpuThreadSamples> ReadThreads(SqliteConnection connection, CpuSamplingInfo sampling)
    {
        var threads = new List<CpuThreadSamples>();
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT t.ThreadId, t.Samples, u.ThreadId IS NOT NULL
            FROM (SELECT ThreadId, COUNT(*) AS Samples FROM CpuSamples GROUP BY ThreadId) t
            LEFT JOIN (SELECT DISTINCT ThreadId FROM Frames) u ON u.ThreadId = t.ThreadId
            ORDER BY t.Samples DESC, t.ThreadId
            """;
        using var reader = command.ExecuteReader();
        while (reader.Read())
        {
            var samples = reader.GetInt64(1);
            threads.Add(new(reader.GetInt32(0), samples, samples * sampling.IntervalMs, reader.GetBoolean(2)));
        }

        return threads;
    }

    // The samples by their stack's leaf node, of all threads or of threadId, and every stack node with its frame, by index
    // into Frames as many nodes share a frame. A frame is Named once symbolizing (or functionName) found its function.
    private static (List<(string Module, string Frame, bool Named)> Frames, Dictionary<long, (long Parent, int Frame)> Nodes,
        List<(long StackId, long Samples)> Leaves) LoadStacks(SqliteConnection connection, int? threadId,
        Func<long, string?>? functionName, CancellationToken cancellationToken)
    {
        var leaves = new List<(long StackId, long Samples)>();
        using (var command = connection.CreateCommand())
        {
            command.CommandText = threadId is null
                ? "SELECT StackId, COUNT(*) FROM CpuSamples GROUP BY StackId"
                : "SELECT StackId, COUNT(*) FROM CpuSamples WHERE ThreadId = $thread GROUP BY StackId";
            command.Parameters.AddWithValue("$thread", threadId ?? 0);
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                leaves.Add((reader.GetInt64(0), reader.GetInt64(1)));
            }
        }

        cancellationToken.ThrowIfCancellationRequested();
        var frames = new List<(string Module, string Frame, bool Named)>();
        var frameIds = new Dictionary<string, int>(StringComparer.Ordinal);
        var nodes = new Dictionary<long, (long Parent, int Frame)>();
        using (var command = connection.CreateCommand())
        {
            command.CommandText = "SELECT StackId, ParentId, Module, Frame, Function IS NOT NULL, AddressId FROM CpuFrames";
            using var reader = command.ExecuteReader();
            while (reader.Read())
            {
                var module = reader.GetString(2);
                var frame = reader.GetString(3);
                var named = reader.GetBoolean(4);
                if (!named && functionName?.Invoke(reader.GetInt64(5)) is { } function)
                {
                    frame = module + "!" + function;
                    named = true;
                }

                if (!frameIds.TryGetValue(frame, out var id))
                {
                    frameIds[frame] = id = frames.Count;
                    frames.Add((module, frame, named));
                }

                nodes[reader.GetInt64(0)] = (reader.IsDBNull(1) ? 0 : reader.GetInt64(1), id);
            }
        }

        cancellationToken.ThrowIfCancellationRequested();
        return (frames, nodes, leaves);
    }

    // The PdbName, PdbGuid and PdbAge columns from column on, as a symbol server keys the PDB; null when not recorded.
    private static string? PdbKey(SqliteDataReader reader, int column) =>
        !reader.IsDBNull(column) && reader.GetString(column) is { Length: > 0 } name
            && Guid.TryParse(reader.IsDBNull(column + 1) ? null : reader.GetString(column + 1), out var guid) && guid != Guid.Empty
            ? new PdbIdentity(name, guid, reader.IsDBNull(column + 2) ? 0 : reader.GetInt32(column + 2)).Key
            : null;

    private static CpuSamplingInfo? ReadSampling(SqliteConnection connection, SqliteTransaction? transaction = null)
    {
        if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'CpuSampling'",
            transaction) == 0)
        {
            return null;
        }

        using (var command = connection.CreateCommand())
        {
            command.CommandText = """
                SELECT IntervalMs, Samples, Dropped, CompressedStacks, SymbolPath, SymbolizedUtc, Note FROM CpuSampling LIMIT 1
                """;
            using var reader = command.ExecuteReader();
            if (reader.Read())
            {
                DateTime? symbolized = reader.IsDBNull(5) ? null
                    : DateTime.Parse(reader.GetString(5), CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
                return new(reader.IsDBNull(0) ? null : reader.GetDouble(0), reader.GetInt64(1), reader.GetInt64(2),
                    reader.GetBoolean(3), reader.IsDBNull(4) ? null : reader.GetString(4), symbolized,
                    reader.IsDBNull(6) ? null : reader.GetString(6));
            }
        }

        // The row is written when the recording ends; until then the samples stored so far are read.
        var samples = Sql.Scalar(connection, "SELECT COUNT(*) FROM CpuSamples", transaction);
        return samples > 0 ? new(null, samples, 0, false, null, null, null) : null;
    }
}
