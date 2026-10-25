CREATE TABLE SourceClocks(
    SourceId TEXT PRIMARY KEY, PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1),
    ClockType INTEGER NOT NULL CHECK(ClockType=1),
    SourceFrequency INTEGER NOT NULL CHECK(SourceFrequency>0),
    SessionFrequency INTEGER NOT NULL CHECK(SessionFrequency>0),
    OriginQpc INTEGER NOT NULL CHECK(OriginQpc>=0),
    FirstCommittedOrdinal INTEGER NOT NULL REFERENCES Checkpoints(Ordinal)
);
ALTER TABLE SourceProvenance ADD COLUMN SourceClockId TEXT REFERENCES SourceClocks(SourceId);
PRAGMA user_version=3;
