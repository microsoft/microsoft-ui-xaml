// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;

namespace WinUIProfiler.Engine.Storage;

// Readers attach the CPU database as cpu. Only main owns the recording clock and event retention boundary.
internal static class CpuSchema
{
    internal const int Version = 1;

    internal static string PathFor(string sessionPath) => sessionPath + ".cpu";

    internal static void Attach(SqliteConnection connection, string path) =>
        Sql.Execute(connection, "ATTACH DATABASE $path AS cpu", null, ("$path", path));

    internal const string Create = """
        CREATE TABLE CpuSessionInfo (
            SessionId TEXT NOT NULL,
            Trimmed INTEGER NOT NULL DEFAULT 0);
        CREATE TABLE Modules (
            ModuleId INTEGER PRIMARY KEY,
            ProcessId INTEGER NOT NULL,
            FilePath TEXT NOT NULL,
            Name TEXT NOT NULL,
            ImageBase INTEGER NOT NULL,
            ImageSize INTEGER NOT NULL,
            TimeDateStamp INTEGER,
            PdbName TEXT,
            PdbGuid TEXT,
            PdbAge INTEGER,
            SymbolStatus TEXT,
            SymbolFile TEXT);
        CREATE TABLE CodeAddresses (
            AddressId INTEGER PRIMARY KEY,
            ModuleId INTEGER REFERENCES Modules (ModuleId),
            Address INTEGER NOT NULL,
            Rva INTEGER,
            Function TEXT);
        CREATE TABLE CallStacks (
            StackId INTEGER PRIMARY KEY,
            ParentId INTEGER,
            AddressId INTEGER NOT NULL REFERENCES CodeAddresses (AddressId));
        CREATE TABLE CpuSamples (
            Qpc INTEGER NOT NULL,
            ProcessId INTEGER NOT NULL,
            ThreadId INTEGER NOT NULL,
            StackId INTEGER NOT NULL REFERENCES CallStacks (StackId));
        CREATE INDEX CpuSamplesByThread ON CpuSamples (ThreadId, Qpc);
        CREATE TABLE CpuSampling (
            IntervalMs REAL,
            Samples INTEGER NOT NULL,
            Dropped INTEGER NOT NULL,
            CompressedStacks INTEGER NOT NULL,
            SymbolPath TEXT,
            SymbolizedUtc TEXT,
            Note TEXT);
        CREATE VIEW CpuFrames AS
        -- Every node of the sampled call stacks with its frame. ParentId leads from a sample's leaf to its callers.
        SELECT c.StackId, c.ParentId, a.AddressId, a.ModuleId, COALESCE(m.Name, '?') AS Module, a.Function, a.Rva, a.Address,
            COALESCE(m.Name, '?') || '!' || COALESCE(a.Function, printf('0x%X', COALESCE(a.Rva, a.Address))) AS Frame
        FROM CallStacks c JOIN CodeAddresses a ON a.AddressId = c.AddressId LEFT JOIN Modules m ON m.ModuleId = a.ModuleId;
        CREATE VIEW HotFunctions AS
        -- Exclusive CPU samples by thread and frame, with the time they stand for at the sampling interval.
        SELECT x.ThreadId, f.Module, f.Frame, COUNT(*) AS Samples, COUNT(*) * (SELECT IntervalMs FROM CpuSampling) AS Ms
        FROM CpuSamples x JOIN CpuFrames f ON f.StackId = x.StackId
        GROUP BY x.ThreadId, f.Frame
        ORDER BY Samples DESC;
        """;

    // Connection-local views join the canonical clock and retention boundary without copying mutable metadata.
    // Filtering also keeps a failed/interrupted CPU cleanup from exposing samples outside the retained event range.
    internal const string ReadViews = """
        CREATE TEMP VIEW CpuSamples AS
        SELECT x.rowid AS rowid, x.*
        FROM main.SessionInfo s CROSS JOIN cpu.CpuSamples x
        WHERE s.CompleteFromQpc IS NULL OR x.Qpc >= s.CompleteFromQpc;
        CREATE TEMP VIEW CpuSampleDetails AS
        SELECT x.rowid AS SampleId, (x.Qpc - s.StartQpc) * 1000.0 / s.QpcFrequency AS Ms, x.ProcessId, x.ThreadId, x.StackId,
            f.Module, f.Function, f.Frame
        FROM temp.CpuSamples x CROSS JOIN main.SessionInfo s JOIN cpu.CpuFrames f ON f.StackId = x.StackId;
        CREATE TEMP VIEW HotFunctions AS
        SELECT d.ThreadId, d.Module, d.Frame, COUNT(*) AS Samples, COUNT(*) * (SELECT IntervalMs FROM cpu.CpuSampling) AS Ms
        FROM temp.CpuSampleDetails d
        GROUP BY d.ThreadId, d.Frame
        ORDER BY Samples DESC;
        """;
}
