PRAGMA user_version=2;
CREATE TABLE IngestionDiagnostics(
    Issue INTEGER PRIMARY KEY, Count INTEGER NOT NULL CHECK(Count>0),
    CommittedOrdinal INTEGER NOT NULL REFERENCES Checkpoints(Ordinal),
    PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1), SampleJson TEXT NOT NULL
);
CREATE TABLE SessionTerminal(
    Singleton INTEGER PRIMARY KEY CHECK(Singleton=1),
    CommittedOrdinal INTEGER NOT NULL REFERENCES Checkpoints(Ordinal),
    TimestampQpc INTEGER NOT NULL, PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1),
    InfoJson TEXT NOT NULL, QualityJson TEXT NOT NULL
);
CREATE TABLE IngestionDiagnosticSamples(
    SampleKey TEXT PRIMARY KEY, Count INTEGER NOT NULL CHECK(Count>0),
    CommittedOrdinal INTEGER NOT NULL REFERENCES Checkpoints(Ordinal),
    PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1), SampleJson TEXT NOT NULL
);
CREATE TABLE SourceProvenance(
    SourceOrdinal INTEGER PRIMARY KEY REFERENCES SourceEvents(Ordinal),
    OriginalProcessId INTEGER NOT NULL, OriginalCreationFileTime INTEGER,
    OriginalInstanceId TEXT NOT NULL, OriginalTimestampQpc INTEGER NOT NULL
);
