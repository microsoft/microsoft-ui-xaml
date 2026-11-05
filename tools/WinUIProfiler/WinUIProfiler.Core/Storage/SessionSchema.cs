// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

// The session file: one SQLite database per connection or recorded trace. Its views are the shared queries: the app, the
// CLI and AI agents read the same rows from them (see the README). Each view's comment, inside its definition, is kept
// in sqlite_master; QueryCatalog lists them. Times are in ms from the start of the recording.
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
            StartedQpc INTEGER,        -- QPC read with StartedUtc, to place UTC times such as ProcessCreatedUtc on the QPC clock (live)
            Source TEXT NOT NULL,      -- Live or Trace
            EtlPath TEXT,
            Coverage TEXT NOT NULL,    -- ProcessStart, Connection or Trace
            LimitBytes INTEGER NOT NULL,
            CompleteFromSeq INTEGER,   -- set when older events were dropped to stay within LimitBytes
            CompleteFromQpc INTEGER,
            EndedUtc TEXT,
            EndReason TEXT,
            LostByEtw INTEGER,
            DroppedByProfiler INTEGER,
            ProcessStartQpc INTEGER);  -- a recorded trace's process creation, from the kernel's process start event, if recorded
        CREATE TABLE EventTypes (
            TypeId INTEGER PRIMARY KEY,
            Provider TEXT NOT NULL,    -- a XamlProvider name, such as Xaml, XamlDiagnostics or DxgKrnl; Marker for a marker provider
            Name TEXT NOT NULL,
            Opcode INTEGER NOT NULL,
            FieldNames TEXT NOT NULL,  -- comma-separated names; the Nth names the Nth value in Events.Payload
            FieldTypes TEXT NOT NULL,  -- comma-separated .NET type names (FieldType), one per value
            TraceLogging INTEGER NOT NULL, -- 1 for a TraceLogging event, 0 for a manifest event
            ProviderName TEXT NOT NULL, -- the ETW provider's name, such as Microsoft.UI.Xaml, or a marker provider's
            ProviderId TEXT NOT NULL);  -- the ETW provider's GUID
        CREATE TABLE Events (
            Seq INTEGER PRIMARY KEY,   -- arrival order: 1, 2, 3...
            Qpc INTEGER NOT NULL,
            ThreadId INTEGER NOT NULL,
            TypeId INTEGER NOT NULL REFERENCES EventTypes (TypeId),
            Payload TEXT NOT NULL);    -- JSON array of the field values
        CREATE TABLE Objects (         -- UI elements and composition visuals the events mention (ObjectIndex)
            ObjectId INTEGER PRIMARY KEY,
            Kind TEXT NOT NULL,        -- Element or Visual (ObjectKind)
            Address INTEGER NOT NULL,  -- a visual's is its IVisual pointer
            Type TEXT,                 -- an element's from DependencyObject_Enter, a visual's from its insertion, once seen
            Name TEXT);                -- an element's x:Name, when set; a visual's owner element's label, when known
        CREATE TABLE ObjectEvents (    -- one row per mention: an event field that holds an object's address
            Seq INTEGER NOT NULL REFERENCES Events (Seq),
            ObjectId INTEGER NOT NULL REFERENCES Objects (ObjectId),
            Field INTEGER NOT NULL,    -- index of the field in the event; -1 - i when field i holds its parent (cleared children)
            PRIMARY KEY (Seq, ObjectId, Field)) WITHOUT ROWID;
        CREATE INDEX ObjectTimeline ON ObjectEvents (ObjectId, Seq);
        CREATE TABLE Frames (          -- UI-thread ticks (FrameIndex)
            FrameId INTEGER PRIMARY KEY,
            ThreadId INTEGER NOT NULL,
            FrameNumber INTEGER NOT NULL, -- the UI thread's frame counter, which advances after a tick that draws
            StartQpc INTEGER,          -- null when the tick started before the recording, or trimming deleted its start
            EndQpc INTEGER);           -- null while the tick runs
        CREATE TABLE FrameEvents (     -- the tick each UI-thread event belongs to
            Seq INTEGER PRIMARY KEY REFERENCES Events (Seq),
            FrameId INTEGER NOT NULL REFERENCES Frames (FrameId),
            Bucket TEXT NOT NULL);     -- Xaml, Composition or Other
        CREATE INDEX FrameContents ON FrameEvents (FrameId);
        -- The events the InterestingRows view reads, found by type instead of a scan of every event.
        CREATE INDEX EventsByType ON Events (TypeId);
        CREATE VIEW EventDetails AS
        -- Every event, readable: its type, its fields by name, the tick it belongs to (TickId, Bucket) and the UI elements and
        -- visuals it mentions. An event outside every tick is in bucket Other.
        SELECT e.Seq, e.Qpc, e.ThreadId, (e.Qpc - s.StartQpc) * 1000.0 / s.QpcFrequency AS Ms, e.TypeId, t.Provider, t.Name, t.Opcode,
            (SELECT json_group_object(n.value, CASE v.type WHEN 'true' THEN json('true') WHEN 'false' THEN json('false') ELSE v.value END)
                FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') n JOIN json_each(e.Payload) v ON v.key = n.key) AS Fields,
            r.FrameId AS TickId, COALESCE(r.Bucket, 'Other') AS Bucket,
            (SELECT json_group_array(json_object('ObjectId', m.ObjectId, 'Kind', o.Kind, 'Type', o.Type, 'Name', o.Name, 'Field',
                    CASE WHEN m.Field >= 0 THEN json_extract('["' || replace(t.FieldNames, ',', '","') || '"]', '$[' || m.Field || ']')
                        ELSE 'child of ' || json_extract('["' || replace(t.FieldNames, ',', '","') || '"]', '$[' || (-1 - m.Field) || ']') END))
                FROM ObjectEvents m JOIN Objects o ON o.ObjectId = m.ObjectId WHERE m.Seq = e.Seq) AS Mentions,
            e.Payload
        FROM Events e JOIN EventTypes t ON t.TypeId = e.TypeId CROSS JOIN SessionInfo s LEFT JOIN FrameEvents r ON r.Seq = e.Seq;
        CREATE VIEW TickEvents AS
        -- The events of each tick (EventDetails' columns), read through the tick's index: a tick's events are
        -- SELECT ... FROM TickEvents WHERE TickId = :tick ORDER BY Seq.
        SELECT r.FrameId AS TickId, d.Seq, d.Qpc, d.ThreadId, d.Ms, d.TypeId, d.Provider, d.Name, d.Opcode, d.Fields, d.Bucket, d.Mentions,
            d.Payload
        FROM FrameEvents r CROSS JOIN EventDetails d
        WHERE d.Seq = r.Seq;
        CREATE VIEW Ticks AS
        -- Every UI-thread tick with stored events. Drew: whether the thread's next tick has a higher frame number (null for the
        -- thread's latest tick). Slow: the tick took more than a 60 Hz frame, 1000 / 60 ms. StartMs is null when the tick
        -- started before the recording, EndMs while it runs. FirstSeq and LastSeq are its first and last stored events.
        WITH
        Counts AS (
            SELECT FrameId, COUNT(*) AS Events, SUM(Bucket = 'Xaml') AS Xaml, SUM(Bucket = 'Composition') AS Composition,
                SUM(Bucket = 'Other') AS Other, MIN(Seq) AS FirstSeq, MAX(Seq) AS LastSeq
            FROM FrameEvents GROUP BY FrameId),
        Times AS (
            SELECT f.FrameId, f.ThreadId, f.FrameNumber,
                (f.StartQpc - s.StartQpc) * 1000.0 / s.QpcFrequency AS StartMs,
                (f.EndQpc - s.StartQpc) * 1000.0 / s.QpcFrequency AS EndMs,
                LEAD(f.FrameNumber) OVER (PARTITION BY f.ThreadId ORDER BY f.FrameId) > f.FrameNumber AS Drew
            FROM Frames f CROSS JOIN SessionInfo s)
        SELECT t.FrameId AS TickId, t.ThreadId, t.FrameNumber, t.StartMs, t.EndMs, t.EndMs - t.StartMs AS TickMs, t.Drew,
            COALESCE(t.EndMs - t.StartMs > 1000.0 / 60, 0) AS Slow, c.Events, c.Xaml, c.Composition, c.Other, c.FirstSeq, c.LastSeq
        FROM Times t JOIN Counts c ON c.FrameId = t.FrameId;
        CREATE VIEW InterestingRows AS
        -- The regions of interest and the interesting rows, by these rules (see the README):
        -- - A region starts on a thread at a navigation, the start of a PerfXamlEvent operation or a window focus change, and
        --   ends at the end of that thread's next tick. A trigger during a tick moves the start back to the tick's start. A
        --   trigger while a region is open is ignored. A focus change before the thread's first row is held, and the next
        --   trigger's region starts at it. A region still open at the end of the recording is not listed.
        -- - The rows are ticks (Frame), UpdateLayout passes, operations, navigations, focus changes and graphics device
        --   creations. A tick or layout pass is listed when it starts in a region; the others always are.
        -- RegionId: the region the row starts in, by the region's StartSeq; a region's is its own. TickId: for a region, the
        -- tick it ends with; for a Frame row, its tick (Ticks.TickId). Times are in ms; StartSeq and StopSeq are in Events.
        WITH
        -- The event types the rules read, and where each field they use sits in Payload.
        Types AS MATERIALIZED (
            SELECT t.TypeId, t.Provider, t.Name,
                (SELECT f.key FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') f WHERE f.value = 'IsStart') AS IsStartAt,
                (SELECT f.key FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') f WHERE f.value = 'FrameNumber') AS FrameNumberAt,
                (SELECT f.key FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') f WHERE f.value = 'ObjectPointer') AS PointerAt,
                (SELECT f.key FROM json_each('["' || replace(t.FieldNames, ',', '","') || '"]') f WHERE f.value = 'EventName') AS EventNameAt
            FROM EventTypes t
            WHERE (t.Provider = 'Xaml' AND t.Name IN ('CoreServices_Frame', 'Layout/Start', 'Layout/Stop', 'CreateGraphicsDevice/Start',
                    'CreateGraphicsDevice/Stop', 'FrameNavigating', 'FrameNavigated', 'PerfXamlEvent'))
                OR t.Provider = 'Win32k'),
        -- Those events, with the fields the rules use. A Win32k focus event's second value is the thread that got focus.
        Ev AS MATERIALIZED (
            SELECT e.Seq, e.ThreadId, y.Provider, y.Name,
                (e.Qpc - s.StartQpc) * 1000.0 / s.QpcFrequency AS Ms,
                json_extract(e.Payload, '$[' || y.IsStartAt || ']') AS IsStart,
                json_extract(e.Payload, '$[' || y.FrameNumberAt || ']') AS FrameNumber,
                json_extract(e.Payload, '$[' || y.PointerAt || ']') AS Pointer,
                json_extract(e.Payload, '$[' || y.EventNameAt || ']') AS EventName,
                CASE WHEN y.Provider = 'Win32k' THEN json_extract(e.Payload, '$[1]') END AS Gained
            FROM Types y CROSS JOIN Events e CROSS JOIN SessionInfo s WHERE e.TypeId = y.TypeId),
        -- Starts and stops of ticks, UpdateLayout passes and graphics device creations.
        Marks AS MATERIALIZED (
            SELECT Seq, ThreadId, Ms, FrameNumber,
                CASE WHEN Name = 'CoreServices_Frame' THEN 'Frame' WHEN Name GLOB 'Layout/*' THEN 'UpdateLayout' ELSE 'CreateGraphicsDevice' END AS Type,
                CASE WHEN Name = 'CoreServices_Frame' THEN COALESCE(IsStart, 0) = 1 ELSE Name GLOB '*/Start' END AS Starts
            FROM Ev
            WHERE Provider = 'Xaml' AND Name IN ('CoreServices_Frame', 'Layout/Start', 'Layout/Stop', 'CreateGraphicsDevice/Start', 'CreateGraphicsDevice/Stop')),
        -- A stop ends the first start after the previous stop of its kind on its thread. Later starts before the stop are
        -- ignored (one is already open), and so is a stop without a start.
        MarkGroups AS (
            SELECT *, COALESCE(MAX(CASE WHEN NOT Starts THEN Seq END) OVER (PARTITION BY ThreadId, Type ORDER BY Seq
                ROWS BETWEEN UNBOUNDED PRECEDING AND 1 PRECEDING), 0) AS AfterStop
            FROM Marks),
        Pairs AS MATERIALIZED (
            SELECT Type, ThreadId, MIN(CASE WHEN Starts THEN Seq END) AS StartSeq, MIN(CASE WHEN NOT Starts THEN Seq END) AS StopSeq
            FROM MarkGroups GROUP BY ThreadId, Type, AfterStop),
        Spans AS MATERIALIZED (
            SELECT p.Type, p.ThreadId, p.StartSeq, p.StopSeq, a.Ms AS StartMs, b.Ms AS StopMs, a.FrameNumber
            FROM Pairs p JOIN Marks a ON a.Seq = p.StartSeq JOIN Marks b ON b.Seq = p.StopSeq),
        -- PerfXamlEvent operations. One object's operations nest: a stop ends the innermost open one of its thread and object,
        -- and a stop with none open is ignored. Depth is the running count, kept from going below zero.
        OpMarks AS MATERIALIZED (
            SELECT Seq, ThreadId, Ms, COALESCE(Pointer, 0) AS Pointer, COALESCE(IsStart, 0) = 1 AS Starts,
                CASE WHEN EventName IS NULL OR EventName = '' THEN 'PerfXamlEvent' ELSE EventName END AS OpName
            FROM Ev WHERE Provider = 'Xaml' AND Name = 'PerfXamlEvent'),
        OpTotals AS (
            SELECT *, SUM(CASE WHEN Starts THEN 1 ELSE -1 END) OVER (PARTITION BY ThreadId, Pointer ORDER BY Seq ROWS UNBOUNDED PRECEDING) AS Total
            FROM OpMarks),
        OpDepths AS (
            SELECT *, Total - MIN(0, MIN(Total) OVER (PARTITION BY ThreadId, Pointer ORDER BY Seq ROWS UNBOUNDED PRECEDING)) AS DepthAfter
            FROM OpTotals),
        OpLevels AS (
            SELECT *, CASE WHEN Starts THEN DepthAfter
                ELSE LAG(DepthAfter, 1, 0) OVER (PARTITION BY ThreadId, Pointer ORDER BY Seq) END AS Level
            FROM OpDepths),
        OpNext AS (
            SELECT *, LEAD(Seq) OVER v AS NextSeq, LEAD(Ms) OVER v AS NextMs, LEAD(Starts) OVER v AS NextStarts
            FROM OpLevels WHERE Level > 0
            WINDOW v AS (PARTITION BY ThreadId, Pointer, Level ORDER BY Seq)),
        Ops AS MATERIALIZED (
            SELECT ThreadId, Seq AS StartSeq, Ms AS StartMs, NextSeq AS StopSeq, NextMs AS StopMs, OpName
            FROM OpNext WHERE Starts AND NextSeq IS NOT NULL AND NOT NextStarts),
        Navs AS MATERIALIZED (
            SELECT Seq, ThreadId, Ms,
                CASE Name WHEN 'FrameNavigating' THEN 'Navigating' ELSE 'Navigated' END AS Type,
                CASE Name WHEN 'FrameNavigating' THEN 'Frame::Navigating' ELSE 'Frame::Navigated' END AS RowName
            FROM Ev WHERE Provider = 'Xaml' AND Name IN ('FrameNavigating', 'FrameNavigated')),
        -- When each thread got its first row, interesting or not: the first completed tick, layout pass, graphics device
        -- creation or operation, or the first navigation.
        FirstRows AS MATERIALIZED (
            SELECT ThreadId, MIN(Seq) AS FirstRowSeq FROM (
                SELECT ThreadId, StopSeq AS Seq FROM Spans
                UNION ALL SELECT ThreadId, Seq FROM Navs
                UNION ALL SELECT ThreadId, StopSeq FROM Ops)
            GROUP BY ThreadId),
        -- Focus changes, by the thread that got focus. One on a thread that has rows is a row and a trigger right away; one
        -- before is held, and the next trigger's region starts at it.
        Focus AS MATERIALIZED (
            SELECT f.Seq, CAST(f.Gained AS INTEGER) AS ThreadId, f.Ms, r.FirstRowSeq,
                r.FirstRowSeq IS NOT NULL AND f.Seq > r.FirstRowSeq AS Immediate
            FROM Ev f LEFT JOIN FirstRows r ON r.ThreadId = CAST(f.Gained AS INTEGER)
            WHERE f.Provider = 'Win32k' AND f.Gained > 0),
        Triggers AS MATERIALIZED (
            SELECT ThreadId, Seq, Ms, RowName AS Trigger FROM Navs
            UNION ALL SELECT ThreadId, Seq, Ms, OpName FROM OpMarks WHERE Starts
            UNION ALL SELECT ThreadId, Seq, Ms, 'Hwnd Focus' FROM Focus WHERE Immediate),
        -- A region ends at the end of the next tick on its thread. Every trigger before that end, after the previous tick's
        -- end, falls into the same region, which opened at the first of them; the others are ignored.
        Closes AS MATERIALIZED (
            SELECT ThreadId, StopSeq AS Seq, StopMs AS Ms, StartSeq AS TickStartSeq, StartMs AS TickStartMs FROM Spans WHERE Type = 'Frame'),
        Opened AS MATERIALIZED (
            SELECT ThreadId, CloseSeq, MIN(Seq) AS OpenSeq FROM (
                SELECT t.ThreadId, t.Seq, (SELECT MIN(c.Seq) FROM Closes c WHERE c.ThreadId = t.ThreadId AND c.Seq > t.Seq) AS CloseSeq
                FROM Triggers t)
            GROUP BY ThreadId, CloseSeq),
        -- A region starts at a held focus change, at the start of the tick in progress, or at its trigger.
        Regions AS MATERIALIZED (
            SELECT o.ThreadId, o.OpenSeq, o.CloseSeq, t.Ms AS OpenMs, t.Trigger AS OpenTrigger, c.TickStartSeq, c.TickStartMs, c.Ms AS CloseMs,
                (SELECT MAX(f.Seq) FROM Focus f WHERE f.ThreadId = o.ThreadId AND NOT f.Immediate AND f.Seq < o.OpenSeq
                    AND (f.FirstRowSeq IS NULL OR o.OpenSeq <= f.FirstRowSeq)) AS HeldSeq
            FROM Opened o
            JOIN Triggers t ON t.ThreadId = o.ThreadId AND t.Seq = o.OpenSeq
            LEFT JOIN Closes c ON c.ThreadId = o.ThreadId AND c.Seq = o.CloseSeq),
        -- The rows found, before they are linked to their regions and ticks.
        Found AS MATERIALIZED (
            SELECT 'Region' AS Type, 'Region of Interest' AS Name, r.ThreadId,
                CASE WHEN r.HeldSeq IS NOT NULL THEN r.HeldSeq WHEN r.TickStartSeq < r.OpenSeq THEN r.TickStartSeq ELSE r.OpenSeq END AS StartSeq,
                r.CloseSeq AS StopSeq,
                CASE WHEN r.HeldSeq IS NOT NULL THEN (SELECT f.Ms FROM Focus f WHERE f.Seq = r.HeldSeq)
                    WHEN r.TickStartSeq < r.OpenSeq THEN r.TickStartMs ELSE r.OpenMs END AS StartMs,
                r.CloseMs AS StopMs, NULL AS FrameNumber,
                CASE WHEN r.HeldSeq IS NOT NULL THEN 'Hwnd Focus' ELSE r.OpenTrigger END AS Trigger
            FROM Regions r WHERE r.CloseSeq IS NOT NULL
            UNION ALL
            -- A tick is interesting when a region was open at its start, or a region opened during it (unless at a held focus).
            SELECT 'Frame', 'Frame', s.ThreadId, s.StartSeq, s.StopSeq, s.StartMs, s.StopMs, s.FrameNumber, NULL
            FROM Spans s WHERE s.Type = 'Frame' AND EXISTS (
                SELECT 1 FROM Regions r WHERE r.ThreadId = s.ThreadId AND r.CloseSeq = s.StopSeq
                    AND (r.OpenSeq < s.StartSeq OR r.HeldSeq IS NULL))
            UNION ALL
            -- A layout pass is interesting when a region was open at its start.
            SELECT 'UpdateLayout', 'UpdateLayout', s.ThreadId, s.StartSeq, s.StopSeq, s.StartMs, s.StopMs, NULL, NULL
            FROM Spans s WHERE s.Type = 'UpdateLayout' AND EXISTS (
                SELECT 1 FROM Regions r WHERE r.ThreadId = s.ThreadId AND r.OpenSeq < s.StartSeq AND (r.CloseSeq IS NULL OR r.CloseSeq > s.StartSeq))
            UNION ALL
            SELECT 'CreateGraphicsDevice', 'Create graphics device', ThreadId, StartSeq, StopSeq, StartMs, StopMs, NULL, NULL
            FROM Spans WHERE Type = 'CreateGraphicsDevice'
            UNION ALL
            SELECT 'Operation', OpName, ThreadId, StartSeq, StopSeq, StartMs, StopMs, NULL, NULL FROM Ops
            UNION ALL
            SELECT Type, RowName, ThreadId, Seq, Seq, Ms, Ms, NULL, NULL FROM Navs
            UNION ALL
            -- Focus changes on a thread with rows, and each thread's latest held focus change, logged with its first row.
            SELECT 'FocusChange', 'Hwnd Focus', ThreadId, Seq, Seq, Ms, Ms, NULL, NULL FROM Focus WHERE Immediate
            UNION ALL
            SELECT 'FocusChange', 'Hwnd Focus', f.ThreadId, f.Seq, f.Seq, f.Ms, f.Ms, NULL, NULL
            FROM Focus f WHERE NOT f.Immediate AND f.FirstRowSeq IS NOT NULL
                AND f.Seq = (SELECT MAX(g.Seq) FROM Focus g WHERE g.ThreadId = f.ThreadId AND NOT g.Immediate))
        SELECT r.Type, r.Name, r.Trigger, r.ThreadId, r.StartSeq, r.StopSeq, r.StartMs, r.StopMs, r.StopMs - r.StartMs AS DurationMs,
            r.FrameNumber,
            (SELECT MIN(g.StartSeq) FROM Found g
                WHERE g.Type = 'Region' AND g.ThreadId = r.ThreadId AND r.StartSeq BETWEEN g.StartSeq AND g.StopSeq) AS RegionId,
            -- The tick whose last stored event is the row's stop: the Frames tab lists a region right above it.
            CASE WHEN r.Type IN ('Region', 'Frame') THEN (
                SELECT fe.FrameId FROM FrameEvents fe JOIN Frames f ON f.FrameId = fe.FrameId
                WHERE fe.Seq = r.StopSeq AND f.ThreadId = r.ThreadId
                    AND NOT EXISTS (SELECT 1 FROM FrameEvents x WHERE x.FrameId = fe.FrameId AND x.Seq > fe.Seq)) END AS TickId
        FROM Found r;
        CREATE VIEW InterestingRowEvents AS
        -- The events of each interesting row: its thread's events from StartSeq to StopSeq. For a region, RowType is 'Region'
        -- and RowStartSeq is its RegionId. A focus change that another thread wrote is not among them.
        SELECT r.Type AS RowType, r.StartSeq AS RowStartSeq, r.RegionId, d.*
        FROM InterestingRows r JOIN EventDetails d ON d.ThreadId = r.ThreadId AND d.Seq BETWEEN r.StartSeq AND r.StopSeq;
        CREATE VIEW FrameList AS
        -- The Frames tab's list, in order (ListIndex). ItemType Tick: every tick, with the region that ends with it (RegionId).
        -- Region: every region, right above the tick it ends with, or by its start when no tick has its stop as last event.
        -- Row: the interesting rows outside every region; a Frame row is not listed, because its tick is. The Show filter:
        -- All shows every item, Interesting the items with InInteresting, Slow the items with InSlow. A tick's Events, Xaml,
        -- Composition and Other are its event counts (Ticks).
        WITH
        Rows AS MATERIALIZED (SELECT * FROM InterestingRows),
        TickList AS MATERIALIZED (SELECT * FROM Ticks),
        Items AS (
            SELECT 'Tick' AS ItemType, k.TickId, g.StartSeq AS RegionId, NULL AS RowType, NULL AS Name, NULL AS Trigger, k.ThreadId,
                k.FrameNumber, k.FirstSeq AS StartSeq, k.LastSeq AS StopSeq, k.StartMs, k.EndMs AS StopMs, k.TickMs AS DurationMs, k.Drew,
                k.Slow, k.Events, k.Xaml, k.Composition, k.Other, COALESCE(k.StartMs, k.EndMs, 1e308) AS SortMs, k.TickId AS SortTick
            FROM TickList k LEFT JOIN Rows g ON g.Type = 'Region' AND g.TickId = k.TickId
            UNION ALL
            SELECT 'Region', g.TickId, g.StartSeq, 'Region', g.Name, g.Trigger, g.ThreadId, NULL, g.StartSeq, g.StopSeq,
                g.StartMs, g.StopMs, g.DurationMs, NULL, NULL, NULL, NULL, NULL, NULL, COALESCE(k.StartMs, k.EndMs, g.StartMs), g.TickId
            FROM Rows g LEFT JOIN TickList k ON k.TickId = g.TickId
            WHERE g.Type = 'Region'
            UNION ALL
            SELECT 'Row', NULL, NULL, r.Type, r.Name, r.Trigger, r.ThreadId, NULL, r.StartSeq, r.StopSeq, r.StartMs, r.StopMs,
                r.DurationMs, NULL, NULL, NULL, NULL, NULL, NULL, r.StartMs, NULL
            FROM Rows r WHERE r.Type NOT IN ('Region', 'Frame') AND r.RegionId IS NULL)
        -- By start; a region sorts with its tick, right above it. A tick whose start is not stored sorts by its end.
        SELECT ROW_NUMBER() OVER (ORDER BY SortMs, SortTick IS NULL, SortTick, ItemType = 'Tick', StartSeq) AS ListIndex,
            ItemType, TickId, RegionId, RowType, Name, Trigger, ThreadId, FrameNumber, StartSeq, StopSeq, StartMs, StopMs, DurationMs,
            Drew, Slow, Events, Xaml, Composition, Other,
            ItemType <> 'Tick' OR RegionId IS NOT NULL AS InInteresting, COALESCE(ItemType = 'Tick' AND Slow, 0) AS InSlow
        FROM Items
        ORDER BY ListIndex;
        CREATE VIEW LaunchSummary AS
        -- The first 10 ticks that drew on the first UI thread (the thread of the first tick), and when each ended: from process
        -- start when the recording saw it (Coverage ProcessStart, or a recorded trace with the kernel's process start event),
        -- otherwise from the start of the recording. Empty for a trimmed file (CompleteFromSeq set), which no longer has them.
        WITH
        TickList AS MATERIALIZED (SELECT * FROM Ticks),
        -- When the process started, in ms from the start of the recording. A recorded trace has it on the events' clock
        -- (ProcessStartQpc). For a launch, StartedQpc was read with StartedUtc, so the creation time (100 ns ticks, parsed
        -- from the ISO 8601 text) maps onto the QPC clock. A connection to a running process did not see it start.
        Info AS (
            SELECT s.CompleteFromSeq,
                CASE WHEN s.Coverage = 'Connection' THEN NULL
                WHEN s.ProcessStartQpc IS NOT NULL THEN (s.ProcessStartQpc - s.StartQpc) * 1000.0 / s.QpcFrequency
                WHEN s.StartedQpc IS NOT NULL AND s.ProcessCreatedUtc IS NOT NULL THEN
                    (s.StartedQpc - s.StartQpc) * 1000.0 / s.QpcFrequency - (
                        (CAST(strftime('%s', substr(s.StartedUtc, 1, 19)) AS INTEGER) * 10000000 + CASE WHEN instr(s.StartedUtc, '.') > 0
                            THEN CAST(substr(rtrim(substr(s.StartedUtc, instr(s.StartedUtc, '.') + 1), 'Z') || '0000000', 1, 7) AS INTEGER) ELSE 0 END)
                        - (CAST(strftime('%s', substr(s.ProcessCreatedUtc, 1, 19)) AS INTEGER) * 10000000 + CASE WHEN instr(s.ProcessCreatedUtc, '.') > 0
                            THEN CAST(substr(rtrim(substr(s.ProcessCreatedUtc, instr(s.ProcessCreatedUtc, '.') + 1), 'Z') || '0000000', 1, 7) AS INTEGER) ELSE 0 END)
                    ) / 10000.0 END AS ProcessStartMs
            FROM SessionInfo s)
        SELECT ROW_NUMBER() OVER (ORDER BY k.TickId) AS Position, k.TickId, k.ThreadId, k.FrameNumber, k.EndMs,
            k.EndMs - COALESCE(i.ProcessStartMs, 0) AS Ms,
            CASE WHEN i.ProcessStartMs IS NULL THEN 'RecordingStart' ELSE 'ProcessStart' END AS MeasuredFrom
        FROM TickList k CROSS JOIN Info i
        WHERE i.CompleteFromSeq IS NULL AND k.Drew = 1 AND k.EndMs IS NOT NULL
            AND k.ThreadId = (SELECT ThreadId FROM TickList ORDER BY TickId LIMIT 1)
        ORDER BY k.TickId
        LIMIT 10;
        CREATE VIEW QueryCatalog (Name, Shows, RowsAreIdentifiedBy, Example) AS
        -- What each view shows, how its rows are identified, and an example. Start here.
        VALUES
            ('EventDetails', 'Every event with its fields by name, its tick and bucket, and the objects it mentions', 'Seq',
                'SELECT Seq, Ms, Name, Bucket, Fields FROM EventDetails WHERE ThreadId = :thread AND Seq BETWEEN :first AND :last ORDER BY Seq'),
            ('TickEvents', 'The events of each tick, fast by TickId', 'TickId, then Seq',
                'SELECT Seq, Ms, Name, Bucket, Fields FROM TickEvents WHERE TickId = :tick ORDER BY Seq'),
            ('Ticks', 'Every UI-thread tick: times, whether it drew, whether it was slow, event counts by bucket', 'TickId',
                'SELECT * FROM Ticks WHERE Slow ORDER BY TickMs DESC'),
            ('InterestingRows', 'Regions of interest and the interesting rows, with the region each starts in and its tick',
                'Type and StartSeq; a region by RegionId (its StartSeq)',
                'SELECT * FROM InterestingRows WHERE RegionId = :region ORDER BY StartSeq'),
            ('InterestingRowEvents', 'The events of each interesting row (its thread, from its start to its stop)',
                'RowType and RowStartSeq, then Seq',
                'SELECT Seq, Ms, Name, Bucket, Fields FROM InterestingRowEvents WHERE RowType = ''Region'' AND RowStartSeq = :region ORDER BY Seq'),
            ('FrameList', 'The Frames tab list in order, with the Show filter of each item', 'ListIndex',
                'SELECT * FROM FrameList WHERE InInteresting ORDER BY ListIndex'),
            ('LaunchSummary', 'When the first 10 ticks that drew ended, from process start or recording start', 'Position',
                'SELECT * FROM LaunchSummary'),
            ('QueryCatalog', 'This list', 'Name', 'SELECT * FROM QueryCatalog');
        """;
}
