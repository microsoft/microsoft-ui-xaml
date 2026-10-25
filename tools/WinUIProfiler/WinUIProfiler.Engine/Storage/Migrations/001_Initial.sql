PRAGMA application_id=1465208912;
PRAGMA user_version=1;
CREATE TABLE Sessions(
    SessionId TEXT PRIMARY KEY, ScopeJson TEXT NOT NULL, TokenKey BLOB NOT NULL,
    SchemaHash TEXT NOT NULL, CHECK(length(TokenKey)=32)
);
CREATE TABLE ProcessInstances(
    InstanceId TEXT PRIMARY KEY, SessionId TEXT NOT NULL REFERENCES Sessions,
    ProcessId INTEGER NOT NULL CHECK(ProcessId>0), CreationFileTime INTEGER
);
CREATE TABLE Checkpoints(
    Ordinal INTEGER PRIMARY KEY CHECK(Ordinal>=0), TimestampQpc INTEGER NOT NULL CHECK(TimestampQpc>=0),
    PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1),
    Status INTEGER NOT NULL, InfoJson TEXT NOT NULL, QualityJson TEXT NOT NULL
);
CREATE TABLE SourceEvents(
    Ordinal INTEGER PRIMARY KEY CHECK(Ordinal>0),
    CommitOrdinal INTEGER NOT NULL REFERENCES Checkpoints,
    InstanceId TEXT NOT NULL REFERENCES ProcessInstances,
    TimestampQpc INTEGER NOT NULL, ProviderId TEXT NOT NULL, ProducerProcessId INTEGER NOT NULL,
    ProducerEpoch TEXT NOT NULL, EventId INTEGER NOT NULL, EventVersion INTEGER NOT NULL,
    SourceSequence TEXT, PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1),
    Envelope BLOB NOT NULL
);
CREATE INDEX SourceEvents_process_time ON SourceEvents(InstanceId,TimestampQpc,Ordinal);
CREATE INDEX SourceEvents_stream ON SourceEvents(ProviderId,ProducerProcessId,ProducerEpoch,EventId,EventVersion,SourceSequence);

-- Append-only projection revisions. Child tables expose dataset-specific typed facts;
-- the versioned JSON preserves the complete immutable P01 contract, not query filters.
CREATE TABLE ProjectionRows(
    Id INTEGER PRIMARY KEY, CommitOrdinal INTEGER NOT NULL REFERENCES Checkpoints,
    SourceOrdinal INTEGER NOT NULL REFERENCES SourceEvents,
    Dataset TEXT NOT NULL, RowKey TEXT NOT NULL,
    TimestampQpc INTEGER NOT NULL, EndQpc INTEGER NOT NULL,
    ObjectKey TEXT, ParentKey TEXT, RuntimeId TEXT, Domain INTEGER,
    RootId TEXT, DispatcherId TEXT, Metric INTEGER, SnapshotId TEXT, SnapshotKind INTEGER,
    OwnerKey TEXT, Deleted INTEGER NOT NULL CHECK(Deleted IN(0,1)),
    PayloadVersion INTEGER NOT NULL CHECK(PayloadVersion=1),
    PayloadJson TEXT NOT NULL,
    UNIQUE(Dataset,RowKey,SourceOrdinal)
);
CREATE INDEX ProjectionRows_temporal ON ProjectionRows(Dataset,RowKey,SourceOrdinal DESC,CommitOrdinal,TimestampQpc);
CREATE INDEX ProjectionRows_time ON ProjectionRows(Dataset,TimestampQpc,SourceOrdinal,RowKey,CommitOrdinal);
CREATE INDEX ProjectionRows_object_time ON ProjectionRows(Dataset,ObjectKey,TimestampQpc,CommitOrdinal);
CREATE INDEX ProjectionRows_parent_child ON ProjectionRows(ParentKey,ObjectKey,TimestampQpc,CommitOrdinal);
CREATE INDEX ProjectionRows_metric ON ProjectionRows(Dataset,Metric,TimestampQpc,CommitOrdinal);
CREATE INDEX ProjectionRows_snapshot ON ProjectionRows(SnapshotId,CommitOrdinal);
CREATE INDEX ProjectionRows_owner ON ProjectionRows(OwnerKey,TimestampQpc,CommitOrdinal);

CREATE TABLE Capabilities(
    SourceOrdinal INTEGER NOT NULL REFERENCES SourceEvents, Capability INTEGER NOT NULL, Version INTEGER NOT NULL,
    Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT,
    PRIMARY KEY(SourceOrdinal,Capability)
);
CREATE TABLE MetricSamples(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, Metric INTEGER NOT NULL,
    ValueKind INTEGER NOT NULL, Unit INTEGER NOT NULL, IntegerValue INTEGER, NumberValue REAL,
    BooleanValue INTEGER, TextValue TEXT, Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT
);
CREATE INDEX MetricSamples_metric ON MetricSamples(Metric,ProjectionId);
CREATE TABLE ObjectLifetimes(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, ObjectKey TEXT NOT NULL,
    Operation INTEGER NOT NULL, Address TEXT, TypeName TEXT
);
CREATE INDEX ObjectLifetimes_object ON ObjectLifetimes(ObjectKey,ProjectionId);
CREATE TABLE ObjectProperties(
    ProjectionId INTEGER NOT NULL REFERENCES ProjectionRows, ObjectKey TEXT NOT NULL, PropertyId TEXT NOT NULL,
    ValueKind INTEGER NOT NULL, Unit INTEGER NOT NULL, IntegerValue INTEGER, NumberValue REAL,
    BooleanValue INTEGER, TextValue TEXT, Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT,
    PRIMARY KEY(ProjectionId,ObjectKey,PropertyId)
);
CREATE INDEX ObjectProperties_object ON ObjectProperties(ObjectKey,PropertyId,ProjectionId);
CREATE TABLE Relationships(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, FromKey TEXT NOT NULL, ToKey TEXT,
    Relationship INTEGER NOT NULL, RuntimeId TEXT NOT NULL, StartQpc INTEGER NOT NULL, EndQpc INTEGER NOT NULL
);
CREATE INDEX Relationships_validity ON Relationships(FromKey,ToKey,StartQpc,EndQpc,ProjectionId);
CREATE TABLE Roots(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, RootId TEXT NOT NULL,
    DispatcherId TEXT NOT NULL, ObjectKey TEXT NOT NULL, Connected INTEGER NOT NULL
);
CREATE INDEX Roots_identity ON Roots(RootId,DispatcherId,ProjectionId);
CREATE TABLE FrameAttempts(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows,
    ProducerEpoch TEXT NOT NULL, AttemptId TEXT NOT NULL, RootId TEXT NOT NULL, DispatcherId TEXT NOT NULL,
    Status INTEGER NOT NULL, HResult INTEGER
);
CREATE INDEX FrameAttempts_id ON FrameAttempts(ProducerEpoch,AttemptId,RootId,DispatcherId,ProjectionId);
CREATE TABLE FrameSubmissions(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, ProducerEpoch TEXT NOT NULL,
    FrameId TEXT NOT NULL, SubmittedQpc INTEGER NOT NULL
);
CREATE INDEX FrameSubmissions_id ON FrameSubmissions(ProducerEpoch,FrameId,ProjectionId);
CREATE TABLE FrameChanges(
    ProjectionId INTEGER NOT NULL REFERENCES ProjectionRows, IngestOrdinal INTEGER NOT NULL REFERENCES SourceEvents,
    TimestampQpc INTEGER NOT NULL, ObjectKey TEXT, Relation INTEGER NOT NULL, PayloadJson TEXT NOT NULL,
    PRIMARY KEY(ProjectionId,IngestOrdinal)
);
CREATE INDEX FrameChanges_object_time ON FrameChanges(ObjectKey,TimestampQpc,ProjectionId);
CREATE TABLE Resources(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, ResourceKey TEXT NOT NULL,
    DeviceEpoch TEXT NOT NULL, AllocationId TEXT, ResourceKind INTEGER NOT NULL,
    Provenance INTEGER NOT NULL, ValueKind INTEGER NOT NULL, Unit INTEGER NOT NULL,
    IntegerValue INTEGER, NumberValue REAL, BooleanValue INTEGER, TextValue TEXT,
    Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT
);
CREATE INDEX Resources_id ON Resources(ResourceKey,DeviceEpoch,AllocationId,ProjectionId);
CREATE TABLE ResourceAllocations(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, AllocatorEpoch TEXT NOT NULL,
    AllocationId TEXT NOT NULL, PreviousAllocationId TEXT, Operation INTEGER NOT NULL,
    RequestedBytes TEXT NOT NULL, ActualBytes TEXT,
    RequestedBytesHigh INTEGER NOT NULL, RequestedBytesLow INTEGER NOT NULL,
    ActualBytesHigh INTEGER, ActualBytesLow INTEGER
);
CREATE INDEX ResourceAllocations_id ON ResourceAllocations(AllocatorEpoch,AllocationId,ProjectionId);
CREATE TABLE ResourceOwners(
    ProjectionId INTEGER NOT NULL REFERENCES ProjectionRows, ResourceKey TEXT NOT NULL, OwnerKey TEXT NOT NULL,
    PRIMARY KEY(ProjectionId,ResourceKey,OwnerKey)
);
CREATE INDEX ResourceOwners_owner ON ResourceOwners(OwnerKey,ResourceKey,ProjectionId);
CREATE TABLE Snapshots(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, SnapshotId TEXT NOT NULL,
    Kind INTEGER NOT NULL, State INTEGER NOT NULL, StartQpc INTEGER NOT NULL, EndQpc INTEGER
);
CREATE INDEX Snapshots_id ON Snapshots(SnapshotId,ProjectionId);
CREATE TABLE SnapshotAttributes(
    ProjectionId INTEGER NOT NULL REFERENCES ProjectionRows, SnapshotId TEXT NOT NULL,
    Attribute INTEGER NOT NULL, GroupKey TEXT NOT NULL, Grouping INTEGER NOT NULL,
    ObjectKey TEXT, TypeName TEXT, RootId TEXT, ResourceKind INTEGER,
    ValueKind INTEGER NOT NULL, Unit INTEGER NOT NULL,
    IntegerValue INTEGER, NumberValue REAL, BooleanValue INTEGER, TextValue TEXT,
    Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT,
    PRIMARY KEY(ProjectionId,Attribute,GroupKey)
);
CREATE INDEX SnapshotAttributes_id ON SnapshotAttributes(SnapshotId,Attribute,GroupKey,ProjectionId);
CREATE TABLE SnapshotArtifacts(
    ProjectionId INTEGER PRIMARY KEY REFERENCES ProjectionRows, SnapshotId TEXT NOT NULL,
    RelativePath TEXT NOT NULL, Sha256 TEXT NOT NULL
);
CREATE INDEX SnapshotArtifacts_id ON SnapshotArtifacts(SnapshotId,ProjectionId);
CREATE TABLE RetentionEdges(
    ProjectionId INTEGER NOT NULL REFERENCES ProjectionRows, EdgeIndex INTEGER NOT NULL,
    SnapshotId TEXT NOT NULL, FromKey TEXT NOT NULL, ToKey TEXT NOT NULL, Relationship TEXT NOT NULL, Evidence INTEGER NOT NULL,
    Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT,
    PRIMARY KEY(ProjectionId,EdgeIndex)
);
CREATE INDEX RetentionEdges_endpoints ON RetentionEdges(FromKey,ToKey,ProjectionId);
CREATE INDEX RetentionEdges_snapshot ON RetentionEdges(SnapshotId,ProjectionId);
CREATE TABLE CaptureEpochs(
    SourceOrdinal INTEGER PRIMARY KEY REFERENCES SourceEvents, CaptureId TEXT NOT NULL,
    Kind INTEGER NOT NULL, Phase INTEGER NOT NULL, RootId TEXT, DispatcherId TEXT, PayloadJson TEXT NOT NULL
);
CREATE INDEX CaptureEpochs_id ON CaptureEpochs(CaptureId,Phase,SourceOrdinal);
CREATE TABLE QualityIntervals(
    CommitOrdinal INTEGER PRIMARY KEY REFERENCES Checkpoints, StartQpc INTEGER NOT NULL, EndQpc INTEGER NOT NULL,
    Measurement INTEGER NOT NULL, Coverage INTEGER NOT NULL, Reason TEXT
);
CREATE INDEX QualityIntervals_time ON QualityIntervals(StartQpc,EndQpc,CommitOrdinal);
