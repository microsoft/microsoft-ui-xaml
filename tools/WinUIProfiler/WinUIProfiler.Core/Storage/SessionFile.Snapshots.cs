using System.Text;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.Engine.Storage;

public sealed record SessionTreeSnapshot(Guid CaptureId, DateTime SavedUtc, string JsonPath, long StartQpc, long EndQpc,
    int Roots, int XamlNodes, int CompositionNodes, int Correlations);

public static partial class SessionFile
{
    public static IReadOnlyList<SessionTreeSnapshot> ReadTreeSnapshots(string path, CancellationToken token = default)
    {
        token.ThrowIfCancellationRequested();
        using var connection = Open(path);
        if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'TreeSnapshots'") == 0)
            return [];
        using var command = Command(connection, null, """
            SELECT CaptureId, SavedUtc, JsonPath, StartQpc, EndQpc, Roots, XamlNodes, CompositionNodes, Correlations
            FROM SnapshotSummary ORDER BY EndQpc, CaptureId
            """);
        using var reader = command.ExecuteReader();
        var snapshots = new List<SessionTreeSnapshot>();
        while (reader.Read())
        {
            token.ThrowIfCancellationRequested();
            snapshots.Add(new(Guid.Parse(reader.GetString(0)), ParseUtc(reader.GetString(1)), reader.GetString(2),
                reader.GetInt64(3), reader.GetInt64(4), reader.GetInt32(5), reader.GetInt32(6), reader.GetInt32(7), reader.GetInt32(8)));
        }
        SessionQueries.Report(command, snapshots.Count);
        return snapshots;
    }

    public static TreeSnapshotDocument ReadTreeSnapshot(string path, Guid captureId, CancellationToken token = default)
    {
        token.ThrowIfCancellationRequested();
        using var connection = Open(path);
        if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'TreeSnapshots'") == 0)
            throw new InvalidDataException("This session file did not record tree snapshots.");
        using var command = Command(connection, null, "SELECT Document FROM TreeSnapshots WHERE CaptureId = $id",
            ("$id", captureId.ToString("D")));
        var document = command.ExecuteScalar() as string ??
            throw new InvalidDataException($"Snapshot {captureId:D} is not retained in this session file.");
        var snapshot = TreeSnapshotCodec.DecodeSaved(Encoding.UTF8.GetBytes(document), captureId: captureId, token: token);
        SessionQueries.Report(command, 1);
        return snapshot;
    }
}
