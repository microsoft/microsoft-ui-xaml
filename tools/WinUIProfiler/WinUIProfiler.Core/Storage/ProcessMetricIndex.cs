using System.Text;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Metrics;

namespace WinUIProfiler.Engine.Storage;

// Only the session writer thread uses this index.
internal sealed class ProcessMetricIndex(SqliteConnection connection)
{
    private long acquisitionId;
    internal static long IncomingBytes(MetricReading reading) => reading.Values.Sum(value =>
        256L + Encoding.UTF8.GetByteCount(value.Source) +
        (value.UnavailableReason is { } reason ? Encoding.UTF8.GetByteCount(reason) : 0));

    internal void Add(MetricReading reading, SqliteTransaction transaction)
    {
        var id = ++acquisitionId;
        foreach (var value in reading.Values)
        {
            var name = value.Metric switch
            {
                ProcessMetric.Cpu => "cpu",
                ProcessMetric.Gpu => "gpu",
                ProcessMetric.WorkingSet => "workingSet",
                ProcessMetric.PrivateWorkingSet => "privateWorkingSet",
                ProcessMetric.PrivateCommit => "privateCommit",
                _ => throw new ArgumentOutOfRangeException(nameof(reading))
            };
            Sql.Execute(connection, """
                INSERT INTO ProcessMetrics
                    (AcquisitionId, Qpc, ProcessId, CreationTimeFileTime, InstanceId, Metric, Value, Unit, Source, UnavailableReason)
                VALUES ($id, $qpc, $pid, $created, $instance, $metric, $value, $unit, $source, $reason)
                """, transaction, ("$id", id), ("$qpc", reading.TimestampQpc), ("$pid", (long)reading.Process.ProcessId),
                ("$created", reading.Process.CreationTimeFileTime), ("$instance", reading.Process.InstanceId.ToString("D")),
                ("$metric", name), ("$value", value.Value),
                ("$unit", value.Metric <= ProcessMetric.Gpu ? "percent" : "MiB"),
                ("$source", value.Source), ("$reason", value.UnavailableReason));
        }
    }

    internal long? FirstQpc(SqliteTransaction? transaction = null)
    {
        using var command = connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText = "SELECT MIN(Qpc) FROM ProcessMetrics";
        return command.ExecuteScalar() is long qpc ? qpc : null;
    }

    internal bool TrimOldest(SqliteTransaction? transaction = null)
    {
        var current = transaction ?? connection.BeginTransaction();
        using var owned = transaction is null ? current : null;
        // AcquisitionId, rather than Qpc alone, bounds the deletion even when acquisitions share a timestamp.
        var cutoff = Sql.Scalar(connection, """
            SELECT COALESCE(MAX(AcquisitionId), 0) FROM
                (SELECT AcquisitionId FROM ProcessMetrics GROUP BY AcquisitionId ORDER BY AcquisitionId
                 LIMIT MAX(1, (SELECT COUNT(*) / 100 FROM ProcessMetrics)))
            """, current);
        if (cutoff == 0) return false;
        var nextQpc = Sql.Scalar(connection,
            "SELECT MAX(Qpc) FROM ProcessMetrics WHERE AcquisitionId <= $cutoff", current, ("$cutoff", cutoff));
        if (nextQpc < long.MaxValue) nextQpc++;
        Sql.Execute(connection, "DELETE FROM ProcessMetrics WHERE AcquisitionId <= $cutoff", current, ("$cutoff", cutoff));
        RecordRetention(current, nextQpc);
        owned?.Commit();
        return true;
    }

    internal void TrimBefore(long qpc, SqliteTransaction transaction)
    {
        using var command = connection.CreateCommand();
        command.Transaction = transaction;
        command.CommandText = "DELETE FROM ProcessMetrics WHERE Qpc < $qpc";
        command.Parameters.AddWithValue("$qpc", qpc);
        if (command.ExecuteNonQuery() > 0) RecordRetention(transaction, qpc);
    }

    private void RecordRetention(SqliteTransaction transaction, long fallback)
    {
        Sql.Execute(connection, """
            INSERT INTO ProcessMetricRetention (CompleteFromQpc)
            SELECT MAX(COALESCE((SELECT MAX(CompleteFromQpc) FROM ProcessMetricRetention), 0),
                       COALESCE((SELECT MIN(Qpc) FROM ProcessMetrics), $fallback));
            DELETE FROM ProcessMetricRetention WHERE rowid NOT IN (SELECT MAX(rowid) FROM ProcessMetricRetention);
            """, transaction, ("$fallback", fallback));
    }
}
