using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Query;

namespace WinUIProfiler.Engine.Storage;

public sealed class SessionStore : IDisposable
{
    private const int ApplicationId = 1465208912;
    private const int FailureBytes = 4096;
    private readonly FileStream? lease;
    internal readonly SqliteConnection? WriterConnection;
    internal readonly object Gate = new();
    private bool disposed;
    private bool writerCreated;
    private FailureRecord? failureRecord;
    internal bool Failed;
    internal string Path { get; }
    internal SessionScope Scope { get; }
    internal byte[] TokenKey { get; }
    public ProfilerQueryService Queries { get; }

    private SessionStore(string path, SqliteConnection db, FileStream? lease)
    {
        Path = path; this.lease = lease;
        using var command = Command(db, "SELECT ScopeJson,TokenKey FROM Sessions");
        using var reader = command.ExecuteReader();
        if (!reader.Read()) throw new InvalidDataException("Missing session identity.");
        Scope = Json<SessionScope>(reader.GetString(0)); Scope.Validate();
        TokenKey = (byte[])reader[1];
        if (TokenKey.Length != 32 || reader.Read()) throw new InvalidDataException("Invalid session identity.");
        WriterConnection = lease is null ? null : db;
        Queries = new(this);
        failureRecord = ReadFailure();
        Failed = failureRecord is not null;
    }

    public static SessionStore Create(string path, SessionScope scope, SessionInfo info, DataQuality quality, SessionStoreOptions? options = null)
    {
        scope.Validate(); ArgumentNullException.ThrowIfNull(quality);
        info = QueryResultValidation.Freeze(info, new(scope, new(0, 0), quality));
        _ = EncodeBounded(info); _ = EncodeBounded(quality);
        var pages = ValidateOptions(options);
        path = System.IO.Path.GetFullPath(path);
        var lease = AcquireLease(path);
        SqliteConnection? db = null;
        try
        {
            using (new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { }
            ReserveFailureFile(path);
            db = Connect(path, false);
            ConfigureWriter(db, pages);
            using var transaction = db.BeginTransaction();
            using var stream = typeof(SessionStore).Assembly.GetManifestResourceStream("WinUIProfiler.Engine.Storage.Migrations.001_Initial.sql")
                ?? throw new InvalidOperationException("Initial schema resource is missing.");
            using var reader = new StreamReader(stream);
            Execute(db, reader.ReadToEnd(), transaction);
            Execute(db, "INSERT INTO Sessions VALUES($id,$scope,$key,$hash)", transaction,
                ("$id", scope.SessionId.ToString("N")), ("$scope", Encode(scope)), ("$key", RandomNumberGenerator.GetBytes(32)), ("$hash", SchemaHash(db, transaction)));
            Execute(db, "INSERT INTO ProcessInstances VALUES($instance,$session,$pid,$creation)", transaction,
                ("$instance", scope.Process.InstanceId.ToString("N")), ("$session", scope.SessionId.ToString("N")),
                ("$pid", (long)scope.Process.ProcessId), ("$creation", scope.Process.CreationTimeFileTime));
            InsertCheckpoint(db, transaction, new(0, 0), info, quality);
            transaction.Commit();
            return new(path, db, lease);
        }
        catch { db?.Dispose(); lease.Dispose(); throw; }
    }

    public static SessionStore OpenWritable(string path, SessionStoreOptions? options = null)
    {
        var pages = ValidateOptions(options);
        path = Existing(path);
        var lease = AcquireLease(path);
        SqliteConnection? db = null;
        try
        {
            db = Connect(path, false); ValidateDatabase(db); ConfigureWriter(db, pages);
            return new(path, db, lease);
        }
        catch { db?.Dispose(); lease.Dispose(); throw; }
    }

    public static SessionStore OpenReadOnly(string path)
    {
        path = Existing(path);
        using var db = Connect(path, true);
        ValidateDatabase(db);
        return new(path, db, null);
    }

    public SessionWriter CreateWriter()
    {
        lock (Gate)
        {
            CheckDisposed();
            if (WriterConnection is null || writerCreated) throw new InvalidOperationException("A writable session permits exactly one writer.");
            writerCreated = true;
            return new(this);
        }
    }

    public CommittedWatermark Watermark
    {
        get { using var db = OpenReader(); return ReadCheckpoint(db, null).Watermark; }
    }
    public bool UnfinishedRecording
    {
        get { using var db = OpenReader(); return ReadCheckpoint(db, null).Info.Status == SessionStatus.Recording; }
    }
    internal SqliteConnection OpenReader()
    {
        CheckDisposed();
        return Connect(Path, true);
    }
    internal void CheckDisposed() => ObjectDisposedException.ThrowIf(disposed, this);
    private static string Existing(string path)
    {
        path = System.IO.Path.GetFullPath(path);
        if (!File.Exists(path)) throw new FileNotFoundException("Session database does not exist.", path);
        return path;
    }
    private static FileStream AcquireLease(string path) => new(path + ".writer", FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
    private static int ValidateOptions(SessionStoreOptions? options)
    {
        var pages = (options ?? new()).MaximumDatabasePages;
        if (pages < 1) throw new ArgumentOutOfRangeException(nameof(options));
        return pages;
    }
    private static void ConfigureWriter(SqliteConnection db, int maximumPages)
    {
        using var command = Command(db, "PRAGMA journal_mode=WAL");
        if (!string.Equals((string?)command.ExecuteScalar(), "wal", StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("SQLite WAL mode is required.");
        Execute(db, $"PRAGMA synchronous=FULL; PRAGMA max_page_count={maximumPages};");
    }
    internal static SqliteConnection Connect(string path, bool readOnly)
    {
        var db = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path, Mode = readOnly ? SqliteOpenMode.ReadOnly : SqliteOpenMode.ReadWrite,
            Pooling = false, DefaultTimeout = 5
        }.ToString());
        try
        {
            db.Open(); Execute(db, "PRAGMA foreign_keys=ON; PRAGMA trusted_schema=OFF;");
            if (readOnly) Execute(db, "PRAGMA query_only=ON;");
            return db;
        }
        catch { db.Dispose(); throw; }
    }
    private static void ValidateDatabase(SqliteConnection db)
    {
        try
        {
            using var version = Command(db, "PRAGMA user_version");
            var schemaVersion = Convert.ToInt32(version.ExecuteScalar());
            if (schemaVersion > 1) throw new NotSupportedException($"Future session schema {schemaVersion}.");
            if (schemaVersion != 1) throw new InvalidDataException($"Unsupported session schema {schemaVersion}.");
            using var app = Command(db, "PRAGMA application_id");
            if (Convert.ToInt32(app.ExecuteScalar()) != ApplicationId) throw new InvalidDataException("Not a WinUI profiler session.");
            using var integrity = Command(db, "PRAGMA quick_check");
            if (!Equals(integrity.ExecuteScalar(), "ok")) throw new InvalidDataException("SQLite integrity check failed.");
            using var hash = Command(db, "SELECT SchemaHash FROM Sessions");
            if (!Equals(hash.ExecuteScalar(), SchemaHash(db, null))) throw new InvalidDataException("Session schema was modified or is incomplete.");
            using var foreignKeys = Command(db, "PRAGMA foreign_key_check");
            using var violations = foreignKeys.ExecuteReader();
            if (violations.Read()) throw new InvalidDataException("Session foreign-key state is corrupt.");
            var checkpoint = ReadCheckpoint(db, null);
            using var identity = Command(db, """
                SELECT s.SessionId,s.ScopeJson,p.InstanceId,p.ProcessId,p.CreationFileTime
                FROM Sessions s JOIN ProcessInstances p ON p.SessionId=s.SessionId
                """);
            using (var row = identity.ExecuteReader())
            {
                if (!row.Read()) throw new InvalidDataException("Missing session/process identity.");
                var scope = Json<SessionScope>(row.GetString(1)); scope.Validate();
                if (row.GetString(0) != scope.SessionId.ToString("N") || row.GetString(2) != scope.Process.InstanceId.ToString("N") ||
                    row.GetInt64(3) != scope.Process.ProcessId || (row.IsDBNull(4) ? (long?)null : row.GetInt64(4)) != scope.Process.CreationTimeFileTime ||
                    row.Read())
                    throw new InvalidDataException("Session/process metadata is inconsistent.");
                _ = QueryResultValidation.Freeze(checkpoint.Info, new(scope, checkpoint.Watermark, checkpoint.Quality));
            }
            using var boundary = Command(db, """
                SELECT count(*),coalesce(max(Ordinal),0),coalesce(max(TimestampQpc),0) FROM SourceEvents
                """);
            using (var row = boundary.ExecuteReader())
            {
                row.Read();
                if (row.GetInt64(0) != checkpoint.Watermark.IngestOrdinal || row.GetInt64(1) != checkpoint.Watermark.IngestOrdinal ||
                    row.GetInt64(2) != checkpoint.Watermark.TimestampQpc)
                    throw new InvalidDataException("Source history and durable boundary disagree.");
            }
        }
        catch (Exception error) when (error is SqliteException or JsonException or ArgumentException)
        { throw new InvalidDataException("Invalid session database.", error); }
    }
    private static string SchemaHash(SqliteConnection db, SqliteTransaction? transaction)
    {
        using var command = Command(db, "SELECT type,name,sql FROM sqlite_master WHERE type IN ('table','index') AND name NOT LIKE 'sqlite_%' ORDER BY type,name", transaction);
        using var rows = command.ExecuteReader();
        var text = new StringBuilder();
        while (rows.Read()) text.Append(rows.GetString(0)).Append('|').Append(rows.GetString(1)).Append('|').Append(rows.IsDBNull(2) ? "" : rows.GetString(2)).Append('\n');
        return Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(text.ToString())));
    }
    internal static Checkpoint ReadCheckpoint(SqliteConnection db, CommittedWatermark? at, SqliteTransaction? transaction = null)
    {
        using var command = Command(db, at is null
            ? "SELECT Ordinal,TimestampQpc,InfoJson,QualityJson,Status,PayloadVersion,length(CAST(InfoJson AS BLOB)),length(CAST(QualityJson AS BLOB)) FROM Checkpoints ORDER BY Ordinal DESC LIMIT 1"
            : "SELECT Ordinal,TimestampQpc,InfoJson,QualityJson,Status,PayloadVersion,length(CAST(InfoJson AS BLOB)),length(CAST(QualityJson AS BLOB)) FROM Checkpoints WHERE Ordinal=$ordinal AND TimestampQpc=$qpc",
            transaction, ("$ordinal", at?.IngestOrdinal), ("$qpc", at?.TimestampQpc));
        using var row = command.ExecuteReader();
        if (!row.Read()) throw new ArgumentException("The requested boundary is not a durable committed checkpoint.");
        if (row.GetInt64(6) > ContractVersions.MaximumPayloadBytes || row.GetInt64(7) > ContractVersions.MaximumPayloadBytes)
            throw new InvalidDataException("Checkpoint metadata exceeds its payload bound.");
        var info = Json<SessionInfo>(row.GetString(2));
        if (row.GetInt32(4) != (int)info.Status || row.GetInt32(5) != 1) throw new InvalidDataException("Unsupported or inconsistent checkpoint payload.");
        return new(new(row.GetInt64(0), row.GetInt64(1)), info, Json<DataQuality>(row.GetString(3)));
    }
    internal static void InsertCheckpoint(SqliteConnection db, SqliteTransaction transaction, CommittedWatermark boundary, SessionInfo info, DataQuality quality) =>
        Execute(db, "INSERT INTO Checkpoints VALUES($ordinal,$qpc,1,$status,$info,$quality)", transaction,
            ("$ordinal", boundary.IngestOrdinal), ("$qpc", boundary.TimestampQpc), ("$status", (int)info.Status), ("$info", EncodeBounded(info)), ("$quality", EncodeBounded(quality)));

    private static void ReserveFailureFile(string path)
    {
        using var file = new FileStream(path + ".failure.json", FileMode.CreateNew, FileAccess.ReadWrite, FileShare.Read);
        var bytes = Encoding.UTF8.GetBytes("null".PadRight(FailureBytes));
        file.Write(bytes); file.Flush(true);
    }
    internal FailureRecord? ReadFailure()
    {
        if (Volatile.Read(ref failureRecord) is { } known) return known;
        var path = Path + ".failure.json";
        if (!File.Exists(path)) return null;
        try
        {
            using var file = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            if (file.Length > FailureBytes) throw new InvalidDataException("Oversized session failure record.");
            var bytes = new byte[checked((int)file.Length)]; file.ReadExactly(bytes);
            var record = JsonSerializer.Deserialize<FailureRecord>(bytes, WireJson.Options);
            if (record is not null && record.SessionId != Scope.SessionId) throw new InvalidDataException("Failure record belongs to another session.");
            return record;
        }
        catch (JsonException error) { throw new InvalidDataException("Invalid failure record; recording cannot be considered complete.", error); }
    }
    internal Exception? RecordFailure(CommittedWatermark watermark, Exception error)
    {
        var record = new FailureRecord(Scope.SessionId, watermark, (error as SqliteException)?.SqliteErrorCode,
            "Persistence failed; recording is incomplete. " + error.GetType().Name);
        Volatile.Write(ref failureRecord, record);
        Failed = true;
        try
        {
            var bytes = Encoding.UTF8.GetBytes(Encode(record).PadRight(FailureBytes));
            using var file = new FileStream(Path + ".failure.json", FileMode.OpenOrCreate, FileAccess.Write, FileShare.Read);
            file.Position = 0; file.Write(bytes); file.SetLength(FailureBytes); file.Flush(true);
            return null;
        }
        catch (Exception failure) when (failure is IOException or UnauthorizedAccessException) { return failure; }
    }
    internal static string Encode<T>(T value) => JsonSerializer.Serialize(value, WireJson.Options);
    internal static string EncodeBounded<T>(T value)
    {
        var json = Encode(value);
        if (Encoding.UTF8.GetByteCount(json) > ContractVersions.MaximumPayloadBytes) throw new ArgumentException("Checkpoint metadata exceeds its 1 MiB payload bound.");
        return json;
    }
    internal static T Json<T>(string json) => JsonSerializer.Deserialize<T>(json, WireJson.Options) ?? throw new InvalidDataException("Null persisted contract.");
    internal static SqliteCommand Command(SqliteConnection db, string sql, SqliteTransaction? transaction = null, params (string Name, object? Value)[] parameters)
    {
        var command = db.CreateCommand(); command.CommandText = sql; command.Transaction = transaction;
        foreach (var (name, value) in parameters) command.Parameters.AddWithValue(name, value ?? DBNull.Value);
        return command;
    }
    internal static void Execute(SqliteConnection db, string sql, SqliteTransaction? transaction = null, params (string Name, object? Value)[] parameters)
    {
        using var command = Command(db, sql, transaction, parameters); command.ExecuteNonQuery();
    }
    public void Dispose()
    {
        lock (Gate)
        {
            if (disposed) return;
            disposed = true; WriterConnection?.Dispose(); lease?.Dispose();
        }
    }
    internal sealed record Checkpoint(CommittedWatermark Watermark, SessionInfo Info, DataQuality Quality);
    internal sealed record FailureRecord(Guid SessionId, CommittedWatermark Watermark, int? SqliteErrorCode, string Reason);
}

public sealed record ProjectionChange(long SourceOrdinal, IQueryValue Value, bool Deleted = false);
public sealed record SessionBatch(IReadOnlyList<NormalizedEvent> Sources, IReadOnlyList<ProjectionChange> Changes,
    SessionInfo Info, DataQuality Quality, IReadOnlyList<SnapshotAttributeChange>? SnapshotAttributes = null,
    IReadOnlyList<RetentionEdgeChange>? RetentionEdges = null);
public sealed record SnapshotAttributeChange(long SourceOrdinal, Guid SnapshotId, SnapshotComparisonKey Key, TypedValue Value);
public sealed record RetentionEdgeChange(long SourceOrdinal, Guid SnapshotId, OwnershipEdge Edge);
public sealed record SessionStoreOptions(int MaximumDatabasePages = 1_073_741_823);
public sealed class SessionWriteException : IOException
{
    public int? SqliteErrorCode { get; }
    public Exception? FailureRecordError { get; }
    internal SessionWriteException(Exception error, Exception? failureRecordError)
        : base("Session persistence failed. The previous durable boundary is preserved; recording cannot continue.", error)
    {
        SqliteErrorCode = (error as SqliteException)?.SqliteErrorCode;
        FailureRecordError = failureRecordError;
    }
}
