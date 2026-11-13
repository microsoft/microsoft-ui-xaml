// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Symbols;

// What symbolizing a session file did. The module counts are by how each sampled module's symbols were found; Skipped
// modules were already resolved. Named is how many code addresses of the sampled modules have a function name, of
// Addresses.
public sealed record SymbolizeResult(string SymbolPath, int Modules, int Resolved, int NotFound, int NoIdentity, int Portable,
    int Failed, int Skipped, long Named, long Addresses);

// Names the functions in a session file's sampled call stacks (CodeAddresses.Function), from the PDBs of their modules,
// searching only the given symbol path. A module's PDB identity is the one the trace recorded, or else the one read from
// its binary when that is still the image that was loaded. Run it on a finished session file; run it again with another
// symbol path to name the modules whose PDBs were not found (force redoes the resolved ones too). Each module's results are
// stored as it is done, so a canceled run keeps them.
public static class SessionSymbolizer
{
    // Held while PDBs are found and read (here and by LiveSymbolNamer), so two lookups don't download the same PDB into
    // the same cache at once.
    internal static readonly SemaphoreSlim Gate = new(1, 1);

    internal sealed record StoredModule(long ModuleId, string FilePath, string Name, int TimeDateStamp, int ImageSize,
        string? PdbName, string? PdbGuid, int? PdbAge, string? Status);

    internal readonly record struct StoredAddress(long AddressId, long Rva, bool Named);

    // Throws ArgumentException for a malformed symbol path, FileNotFoundException and InvalidDataException for a file that
    // is not a session file with CPU samples. Progress gets a line per module; warn gets problems with the symbol path.
    public static SymbolizeResult Symbolize(string sessionPath, string symbolPath, bool force = false,
        Action<string>? progress = null, Action<string>? warn = null, TextWriter? log = null,
        CancellationToken cancellationToken = default)
    {
        ArgumentException.ThrowIfNullOrEmpty(sessionPath);
        var normalized = SymbolPaths.Normalize(symbolPath);
        Gate.Wait(cancellationToken);
        try
        {
            return SymbolizeFile(sessionPath, symbolPath, normalized, force, progress, warn, log, cancellationToken);
        }
        finally
        {
            Gate.Release();
        }
    }

    private static SymbolizeResult SymbolizeFile(string sessionPath, string symbolPath, string normalized, bool force,
        Action<string>? progress, Action<string>? warn, TextWriter? log, CancellationToken cancellationToken)
    {
        using var connection = Open(sessionPath);
        var addresses = ReadAddresses(connection);
        var modules = ReadModules(connection).Where(module => addresses.ContainsKey(module.ModuleId))
            .OrderByDescending(module => addresses[module.ModuleId].Count).ToList();

        var counts = new Dictionary<SymbolStatus, int>();
        int skipped = 0;
        long named = 0, total = 0;
        string searched;
        SymbolResolver? resolver = null;
        try
        {
            foreach (var module in modules)
            {
                cancellationToken.ThrowIfCancellationRequested();
                var stored = addresses[module.ModuleId];
                total += stored.Count;
                if (!force && module.Status == nameof(SymbolStatus.Resolved))
                {
                    skipped++;
                    named += stored.Count(address => address.Named);
                    continue;
                }

                resolver ??= new SymbolResolver(symbolPath, warn, log, cancellationToken);
                var (status, moduleNamed, message) = Symbolize(connection, resolver, module, stored, cancellationToken);
                counts[status] = counts.GetValueOrDefault(status) + 1;
                named += moduleNamed;
                progress?.Invoke($"{module.Name}: {message}");
            }

            // Stored and returned without what its URLs may carry as credentials, such as a SAS token.
            searched = SymbolPaths.Redact(resolver?.SearchPath ?? normalized);
            Sql.Execute(connection, "UPDATE CpuSampling SET SymbolPath = $path, SymbolizedUtc = $utc", null,
                ("$path", searched), ("$utc", DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture)));
        }
        finally
        {
            resolver?.Dispose();
        }

        return new SymbolizeResult(searched, modules.Count, counts.GetValueOrDefault(SymbolStatus.Resolved),
            counts.GetValueOrDefault(SymbolStatus.NotFound), counts.GetValueOrDefault(SymbolStatus.NoIdentity),
            counts.GetValueOrDefault(SymbolStatus.Portable), counts.GetValueOrDefault(SymbolStatus.Error), skipped, named, total);
    }

    // Symbolizes one module in one transaction. A cancel rolls it back promptly, so the file is never left mid-write: a
    // hot journal would keep read-only opens from reading it. A module that was resolved stays so when a forced run does
    // not find its PDB again: its names are still right.
    private static (SymbolStatus Status, long Named, string Message) Symbolize(SqliteConnection connection, SymbolResolver resolver,
        StoredModule module, List<StoredAddress> addresses, CancellationToken cancellationToken)
    {
        var identity = StoredIdentity(module);
        var readFromImage = false;
        if (identity is null)
        {
            identity = PeIdentity.Read(module.FilePath, module.TimeDateStamp, module.ImageSize);
            readFromImage = identity is not null;
        }

        var symbols = resolver.Open(identity);

        // A canceled lookup fails, which must not be stored as the module's status.
        cancellationToken.ThrowIfCancellationRequested();
        using var transaction = connection.BeginTransaction();
        if (readFromImage)
        {
            Sql.Execute(connection, "UPDATE Modules SET PdbName = $pdb, PdbGuid = $guid, PdbAge = $age WHERE ModuleId = $id",
                transaction, ("$pdb", identity!.PdbName), ("$guid", identity.Guid.ToString("D", CultureInfo.InvariantCulture)),
                ("$age", identity.Age), ("$id", module.ModuleId));
        }

        long named = 0;
        if (symbols.FindName is { } findName)
        {
            using var update = Sql.Prepare(connection, "UPDATE CodeAddresses SET Function = $function WHERE AddressId = $id",
                ("$function", SqliteType.Text), ("$id", SqliteType.Integer));
            update.Transaction = transaction;
            var visited = 0;
            foreach (var address in addresses)
            {
                if ((++visited & 1023) == 0)
                {
                    // Disposing the transaction uncommitted rolls it back.
                    cancellationToken.ThrowIfCancellationRequested();
                }

                var name = address.Rva is >= 0 and <= uint.MaxValue ? findName((uint)address.Rva) : null;
                update.Parameters["$function"].Value = (object?)name ?? DBNull.Value;
                update.Parameters["$id"].Value = address.AddressId;
                update.ExecuteNonQuery();
                named += name is null ? 0 : 1;
            }
        }

        var keep = symbols.Status != SymbolStatus.Resolved && module.Status == nameof(SymbolStatus.Resolved);
        var key = identity?.Key;
        if (!keep)
        {
            Sql.Execute(connection, "UPDATE Modules SET SymbolStatus = $status, SymbolFile = $file WHERE ModuleId = $id", transaction,
                ("$status", symbols.Status.ToString()), ("$file", symbols.SymbolFile ?? symbols.Error ?? key), ("$id", module.ModuleId));
        }

        transaction.Commit();
        if (keep)
        {
            return (SymbolStatus.Resolved, addresses.Count(address => address.Named),
                $"kept the earlier names; {key} was not found this time ({symbols.Status})");
        }

        return (symbols.Status, named, symbols.Status switch
        {
            SymbolStatus.Resolved => $"named {named} of {addresses.Count} addresses from {symbols.SymbolFile}",
            SymbolStatus.NotFound => $"{key} is not on the symbol path",
            SymbolStatus.NoIdentity => $"the trace does not say which PDB it was built with, and {module.FilePath} could not be read (or has changed)",
            SymbolStatus.Portable => "managed code (portable PDB), which has no native function names",
            _ => $"error: {symbols.Error}",
        });
    }

    internal static PdbIdentity? StoredIdentity(StoredModule module) =>
        module.PdbName is { Length: > 0 } name && Guid.TryParse(module.PdbGuid, out var guid) && guid != Guid.Empty
            ? new PdbIdentity(name, guid, module.PdbAge ?? 0)
            : null;

    private static SqliteConnection Open(string path)
    {
        if (!File.Exists(path))
        {
            throw new FileNotFoundException("The session file was not found.", path);
        }

        var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadWrite,
            Pooling = false
        }.ToString());
        try
        {
            connection.Open();
            var version = Sql.Scalar(connection, "PRAGMA user_version");
            if (version != SessionSchema.Version)
            {
                throw new InvalidDataException(
                    $"'{path}' is not a WinUI Profiler session file of version {SessionSchema.Version} (found version {version}).");
            }

            if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'CodeAddresses'") == 0)
            {
                throw new InvalidDataException($"'{path}' was written by an earlier WinUI Profiler, without CPU samples.");
            }

            return connection;
        }
        catch
        {
            connection.Dispose();
            throw;
        }
    }

    internal static List<StoredModule> ReadModules(SqliteConnection connection)
    {
        var modules = new List<StoredModule>();
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT ModuleId, FilePath, Name, TimeDateStamp, ImageSize, PdbName, PdbGuid, PdbAge, SymbolStatus FROM Modules
            """;
        using var reader = command.ExecuteReader();
        while (reader.Read())
        {
            modules.Add(new StoredModule(reader.GetInt64(0), reader.GetString(1), reader.GetString(2),
                reader.IsDBNull(3) ? 0 : reader.GetInt32(3), reader.GetInt32(4), reader.IsDBNull(5) ? null : reader.GetString(5),
                reader.IsDBNull(6) ? null : reader.GetString(6), reader.IsDBNull(7) ? null : reader.GetInt32(7),
                reader.IsDBNull(8) ? null : reader.GetString(8)));
        }

        return modules;
    }

    // The code addresses in modules, by module: one pass, as CodeAddresses has no index by module.
    internal static Dictionary<long, List<StoredAddress>> ReadAddresses(SqliteConnection connection)
    {
        var byModule = new Dictionary<long, List<StoredAddress>>();
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT ModuleId, AddressId, Rva, Function IS NOT NULL FROM CodeAddresses WHERE ModuleId IS NOT NULL AND Rva IS NOT NULL
            """;
        using var reader = command.ExecuteReader();
        while (reader.Read())
        {
            var moduleId = reader.GetInt64(0);
            if (!byModule.TryGetValue(moduleId, out var list))
            {
                byModule[moduleId] = list = [];
            }

            list.Add(new StoredAddress(reader.GetInt64(1), reader.GetInt64(2), reader.GetBoolean(3)));
        }

        return byModule;
    }
}
