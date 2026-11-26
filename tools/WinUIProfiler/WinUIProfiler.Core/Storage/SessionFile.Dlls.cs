// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

public sealed record SessionDllLoad(long Sequence, double? LoadTimeMilliseconds, long? FaultCount, string DllPath);

public static partial class SessionFile
{
    // Changes whenever DllLoads can: a DLL event is stored or trimmed, or hard fault capture starts. Null when the
    // recording predates the DLL tables. Reading it costs a few index lookups, so a live view can poll it.
    public static string? ReadDllChangeToken(string path)
    {
        using var connection = Open(path);
        if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'HardFaults'") == 0)
        {
            return null;
        }

        using var command = Command(connection, null, """
            SELECT (SELECT MIN(Seq) FROM DllImageEvents), (SELECT MAX(Seq) FROM DllImageEvents),
                (SELECT MIN(Seq) FROM HardFaults), (SELECT MAX(Seq) FROM HardFaults), HardFaultCaptureEnabled,
                StartQpc, QpcFrequency
            FROM SessionInfo
            """);
        using var reader = command.ExecuteReader();
        if (!reader.Read())
        {
            return "";
        }

        return string.Join(',', Enumerable.Range(0, reader.FieldCount)
            .Select(i => Convert.ToString(reader.GetValue(i), System.Globalization.CultureInfo.InvariantCulture)));
    }

    // Null means the recording predates the DLLs view; an empty list means no loads were observed.
    public static IReadOnlyList<SessionDllLoad>? ReadDllLoads(string path)
    {
        using var connection = Open(path);
        if (Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'view' AND name = 'DllLoads'") == 0)
        {
            return null;
        }

        using var command = Command(connection, null, """
            SELECT Seq, LoadTimeMs, FaultCount, DllPath FROM DllLoads ORDER BY FaultCount DESC NULLS LAST, Qpc, Seq
            """);
        using var reader = command.ExecuteReader();
        var loads = new List<SessionDllLoad>();
        while (reader.Read())
        {
            loads.Add(new(reader.GetInt64(0), NullableDouble(reader, 1), reader.IsDBNull(2) ? null : reader.GetInt64(2),
                reader.GetString(3)));
        }

        SessionQueries.Report(command, loads.Count);
        return loads;
    }
}
