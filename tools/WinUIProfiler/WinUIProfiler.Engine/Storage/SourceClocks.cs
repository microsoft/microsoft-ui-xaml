using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Engine.Storage;

public sealed partial class SessionStore
{
    private static void UpgradeClockSchema(SqliteConnection db, SqliteTransaction transaction)
    {
        using var stream = typeof(SessionStore).Assembly.GetManifestResourceStream("WinUIProfiler.Engine.Storage.Migrations.003_SourceClocks.sql")
            ?? throw new InvalidOperationException("Source-clock migration resource is missing.");
        using var reader = new StreamReader(stream);
        Execute(db, reader.ReadToEnd(), transaction);
    }
    public IReadOnlyList<SourceClockMetadata> ReadSourceClocks()
    {
        using var db = OpenReader();
        return SchemaVersion(db) < 3 ? [] : ReadClocks(db, ReadCheckpoint(db, null).Info.QpcFrequency);
    }
    internal static IReadOnlyList<SourceClockMetadata> ReadClocks(SqliteConnection db, long sessionFrequency, SqliteTransaction? transaction = null)
    {
        using var command = Command(db, "SELECT SourceId,ClockType,SourceFrequency,SessionFrequency,OriginQpc,PayloadVersion FROM SourceClocks ORDER BY SourceId LIMIT 17", transaction);
        using var rows = command.ExecuteReader();
        var result = new List<SourceClockMetadata>();
        while (rows.Read())
        {
            if (rows.GetInt32(5) != 1) throw new InvalidDataException("Unsupported source-clock payload version.");
            var clock = ReadClock(rows);
            if (clock.Mapping.SessionFrequency != sessionFrequency) throw new InvalidDataException("Source clock refers to another normalized clock.");
            result.Add(clock);
        }
        if (result.Count > SourceClockMetadata.MaximumSources) throw new InvalidDataException("Source-clock metadata bound exceeded.");
        return result.AsReadOnly();
    }
    private static SourceClockMetadata ReadClock(SqliteDataReader row, int offset = 0)
    {
        if (!Guid.TryParseExact(row.GetString(offset), "N", out var id)) throw new InvalidDataException("Invalid source-clock ID.");
        var clock = new SourceClockMetadata(id, row.GetInt32(offset + 1),
            new(row.GetInt64(offset + 2), row.GetInt64(offset + 3), row.GetInt64(offset + 4)));
        clock.Validate(); return clock;
    }
    public SourceProvenance? ReadSourceProvenance(long sourceOrdinal)
    {
        Require.Positive(sourceOrdinal);
        using var db = OpenReader();
        var version = SchemaVersion(db);
        if (version < 2) return null;
        using var command = Command(db, version < 3
            ? "SELECT OriginalProcessId,OriginalCreationFileTime,OriginalInstanceId,OriginalTimestampQpc FROM SourceProvenance WHERE SourceOrdinal=$ordinal"
            : """
              SELECT p.OriginalProcessId,p.OriginalCreationFileTime,p.OriginalInstanceId,p.OriginalTimestampQpc,
                     c.SourceId,c.ClockType,c.SourceFrequency,c.SessionFrequency,c.OriginQpc
              FROM SourceProvenance p LEFT JOIN SourceClocks c ON p.SourceClockId=c.SourceId WHERE p.SourceOrdinal=$ordinal
              """, null, ("$ordinal", sourceOrdinal));
        using var row = command.ExecuteReader();
        if (!row.Read()) return null;
        var original = new ProcessInstanceKey(checked((uint)row.GetInt64(0)), row.IsDBNull(1) ? null : row.GetInt64(1), Guid.ParseExact(row.GetString(2), "N"));
        original.Validate();
        var qpc = Require.Nonnegative(row.GetInt64(3));
        return new(original, qpc, version >= 3 && !row.IsDBNull(4) ? ReadClock(row, 4) : null);
    }
}

public sealed partial class SessionWriter
{
    private static void PersistClocks(SqliteConnection db, SqliteTransaction transaction, IReadOnlyList<SourceClockMetadata> clocks,
        long sessionFrequency, long committedOrdinal)
    {
        if (clocks.Count > SourceClockMetadata.MaximumSources) throw new ArgumentException("Source-clock metadata bound exceeded.");
        foreach (var clock in clocks)
        {
            clock.Validate();
            if (clock.Mapping.SessionFrequency != sessionFrequency) throw new ArgumentException("Source mapping uses another session frequency.");
            using var existing = SessionStore.Command(db, """
                SELECT ClockType,SourceFrequency,SessionFrequency,OriginQpc FROM SourceClocks WHERE SourceId=$id
                """, transaction, ("$id", clock.SourceId.ToString("N")));
            using (var row = existing.ExecuteReader())
            {
                if (row.Read())
                {
                    if (row.GetInt32(0) != clock.ClockType || row.GetInt64(1) != clock.Mapping.SourceFrequency ||
                        row.GetInt64(2) != clock.Mapping.SessionFrequency || row.GetInt64(3) != clock.Mapping.OriginQpc)
                        throw new ArgumentException("An original source-clock mapping is immutable.");
                    continue;
                }
            }
            using var count = SessionStore.Command(db, "SELECT count(*) FROM SourceClocks", transaction);
            if ((long)count.ExecuteScalar()! >= SourceClockMetadata.MaximumSources) throw new ArgumentException("Source-clock metadata bound exceeded.");
            SessionStore.Execute(db, "INSERT INTO SourceClocks VALUES($id,1,$type,$source,$session,$origin,$commit)", transaction,
                ("$id", clock.SourceId.ToString("N")), ("$type", clock.ClockType), ("$source", clock.Mapping.SourceFrequency),
                ("$session", clock.Mapping.SessionFrequency), ("$origin", clock.Mapping.OriginQpc), ("$commit", committedOrdinal));
        }
    }
}
