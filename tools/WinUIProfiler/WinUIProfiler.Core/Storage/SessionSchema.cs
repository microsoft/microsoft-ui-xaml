// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

// The session file: one SQLite database per connection or recorded trace.
internal static class SessionSchema
{
    // Stored in PRAGMA user_version; readers reject other versions.
    internal const int Version = 1;

    internal const string Create = """
        CREATE TABLE SessionInfo (
            SessionId TEXT NOT NULL,
            ProcessId INTEGER NOT NULL,
            ProcessName TEXT,
            ProcessCreatedUtc TEXT,
            QpcFrequency INTEGER,      -- for a recorded trace, derived from the events
            StartQpc INTEGER,          -- time zero of the recording; for a recorded trace, its first event
            StartedUtc TEXT NOT NULL,
            Source TEXT NOT NULL,      -- Live or Trace
            EtlPath TEXT,
            Coverage TEXT NOT NULL,    -- ProcessStart, Connection or Trace
            LimitBytes INTEGER NOT NULL,
            CompleteFromSeq INTEGER,   -- set when older events were dropped to stay within LimitBytes
            CompleteFromQpc INTEGER,
            EndedUtc TEXT,
            EndReason TEXT,
            LostByEtw INTEGER,
            DroppedByProfiler INTEGER);
        CREATE TABLE EventTypes (
            TypeId INTEGER PRIMARY KEY,
            Provider TEXT NOT NULL,    -- Xaml, XamlProfiler or DxgKrnl (XamlProvider)
            Name TEXT NOT NULL,
            Opcode INTEGER NOT NULL,
            FieldNames TEXT NOT NULL,  -- comma-separated names; the Nth names the Nth value in Events.Payload
            FieldTypes TEXT NOT NULL); -- comma-separated .NET type names (FieldType), one per value
        CREATE TABLE Events (
            Seq INTEGER PRIMARY KEY,   -- arrival order: 1, 2, 3...
            Qpc INTEGER NOT NULL,
            ThreadId INTEGER NOT NULL,
            TypeId INTEGER NOT NULL REFERENCES EventTypes (TypeId),
            Payload TEXT NOT NULL);    -- JSON array of the field values
        """;
}
