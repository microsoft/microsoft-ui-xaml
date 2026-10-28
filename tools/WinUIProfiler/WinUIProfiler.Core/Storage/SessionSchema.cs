// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

// The session file: one SQLite database per connection or recorded trace.
internal static class SessionSchema
{
    // Stored in PRAGMA user_version; readers reject other versions. Not bumped while the format still changes in
    // development.
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
            Provider TEXT NOT NULL,    -- a XamlProvider name, such as Xaml, XamlDiagnostics or DxgKrnl
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
        CREATE TABLE Objects (         -- UI elements the events mention (ObjectIndex)
            ObjectId INTEGER PRIMARY KEY,
            Address INTEGER NOT NULL,
            Type TEXT,                 -- from DependencyObject_Enter, once seen
            Name TEXT);                -- x:Name, when set
        CREATE TABLE ObjectEvents (    -- one row per mention: an event field that holds an element's address
            Seq INTEGER NOT NULL REFERENCES Events (Seq),
            ObjectId INTEGER NOT NULL REFERENCES Objects (ObjectId),
            Field INTEGER NOT NULL,    -- index of the field in the event
            PRIMARY KEY (Seq, ObjectId, Field)) WITHOUT ROWID;
        CREATE INDEX ObjectTimeline ON ObjectEvents (ObjectId, Seq);
        CREATE TABLE Frames (          -- drawn frames per UI thread (FrameIndex)
            FrameId INTEGER PRIMARY KEY,
            ThreadId INTEGER NOT NULL,
            FrameNumber INTEGER NOT NULL, -- the UI thread's frame counter, which advances when a tick draws
            Ticks INTEGER NOT NULL,    -- CoreServices_Frame ticks that carried this number; the last one drew
            StartQpc INTEGER,          -- start of its first tick
            LastTickStartQpc INTEGER,  -- start of its last tick: the one that drew, once the frame is drawn
            EndQpc INTEGER,            -- end of its last tick
            TickTimeQpc INTEGER NOT NULL); -- time spent in its ticks, in QPC units (excludes the gaps between ticks)
        CREATE TABLE FrameEvents (     -- the frame each UI-thread event belongs to
            Seq INTEGER PRIMARY KEY REFERENCES Events (Seq),
            FrameId INTEGER NOT NULL REFERENCES Frames (FrameId),
            Bucket TEXT NOT NULL);     -- Xaml, Composition or Other
        CREATE INDEX FrameContents ON FrameEvents (FrameId);
        """;
}
