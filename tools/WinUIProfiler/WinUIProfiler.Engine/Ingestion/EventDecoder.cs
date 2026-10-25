using System.Buffers.Binary;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

public sealed class EventDecoder
{
    private static readonly Guid SystemTraceProviderId = new("9e814aad-3204-11d2-9a82-006008a86939");
    private readonly SessionScope scope;
    private readonly QpcMapping clock;
    private ProcessInstanceKey process;
    private ProducerStreamKey stream;
    private Guid epoch;
    public SourceClockMetadata SourceClock { get; }
    public EventDecoder(SessionScope scope, QpcMapping clock)
    {
        scope.Validate(); this.scope = scope; this.clock = clock;
        _ = clock.Convert(clock.OriginQpc);
        SourceClock = new(Guid.NewGuid(), 1, clock);
    }

    public DecodedEvent? Decode(NativeRecord record)
    {
        // ETL partition metadata is transport control, not a selected-process observation.
        if (record.ProviderId == SystemTraceProviderId && record.DescriptorId == ushort.MaxValue &&
            record.DescriptorVersion == 2 && record.Opcode == 80 && record.Level == 0 &&
            record.Keywords == 0 && record.Name == "EventTrace/PartitionInfoExtensionV2") return null;
        if (IsProvablyForeignRuntime(record)) return null;
        if (record.ProviderId == EventCatalog.StandardXamlProviderId) return StandardXaml(record);
        if (record.ProviderId == EventCatalog.KernelProcessProviderId) return KernelProcessStart(record);
        if (record.ProviderId == EventCatalog.CollectorProviderId) return Collector(record);
        if (record.ProviderId != EventCatalog.RuntimeProviderId)
            throw new NotSupportedException("No supported native schema for this provider.");
        var f = new NativeFields(record.Fields);
        var definition = EventCatalog.Get(record.ProviderId, f.U16("SemanticEventId"));
        if (record.Name != definition.Identity.Name || f.U8("EventSchemaVersion") != definition.Identity.Version ||
            record.Level != definition.Level || record.Keywords != definition.Identity.Keywords)
            throw new NotSupportedException($"Unsupported native schema: {record.Name}, semantic ID {definition.Identity.EventId}, schema {f.U8("EventSchemaVersion")}, level {record.Level}, keywords {record.Keywords:X16}.");
        NativeSchema.Validate(record, definition.Identity.EventId);
        var original = record.ReadRuntimeTarget();
        if (original.ProcessId != record.ProducerProcessId) throw new InvalidDataException("Runtime source and semantic target PID disagree.");
        epoch = Require.Id(f.Guid("ProducerEpoch"));
        stream = new(record.ProviderId, record.ProducerProcessId, epoch);
        var sequence = Require.Positive(f.U64("SourceSequence"));
        // A producer GUID is not the collector's canonical GUID. Creation evidence binds them.
        if (original.ProcessId != scope.Process.ProcessId ||
            (original.CreationTimeFileTime is { } created && scope.Process.CreationTimeFileTime is { } selected
                ? created != selected : original != scope.Process))
            return null;
        process = scope.Process;
        IEventPayload payload = definition.Identity.EventId switch
        {
            1 => Capabilities(f),
            2 => new ObjectLifetimePayload(Object(f.Struct("Object")), f.Enum<LifetimeOperation>("Operation"),
                f.Optional("TypeId", (v, n) => v.U64(n)), f.Optional("Address", (v, n) => v.U64(n)), Quality(f.Struct("Quality")),
                OptionalClassification(f, "Classification"), Cause(f.Struct("Cause"))),
            3 => new TypeResolvedPayload(epoch, f.U64("TypeId"), f.Text("Name"), OptionalClassification(f, "Classification")),
            4 => new AssociationPayload(Object(f.Struct("Core")), Object(f.Struct("Peer")), f.Bool("Linked"), Cause(f.Struct("Cause"))),
            5 => Tree(f),
            6 => new VisualDeltaPayload(Object(f.Struct("Visual")), f.Enum<VisualProperty>("Property"), Value(f.Struct("Value")), Cause(f.Struct("Cause"))),
            7 => new CorrelationPayload(Object(f.Struct("From")), Object(f.Struct("To")), f.Enum<CorrelationKind>("Kind"), f.Bool("Linked"), Cause(f.Struct("Cause"))),
            8 => new BoundaryPayload(f.Enum<BoundaryKind>("Kind"), f.Enum<BoundaryPhase>("Phase"), f.U64("OperationId"),
                f.Optional("HResult", (v, n) => v.I32(n)), Cause(f.Struct("Cause"))),
            9 => new FramePayload(OptionalAttempt(f.Struct("Attempt")), f.Optional("SubmittedFrameId", (v, n) => v.U64(n)),
                f.Enum<FrameOutcome>("Outcome"), f.Optional("HResult", (v, n) => v.I32(n))),
            10 => new DevicePayload(f.Guid("DeviceEpoch"), f.Enum<DeviceOperation>("Operation"), f.Text("Api"), Quality(f.Struct("Quality"))),
            11 => Resource(f),
            12 => Allocation(f),
            13 => CaptureBoundary(f),
            14 => CaptureChunk(f),
            15 => new LossPayload(epoch, Interval(f.Struct("Interval")), f.Optional("LostEvents", (v, n) => v.U64(n)), f.Text("Reason"),
                LossDomains(f)),
            16 => new XamlPropertyChangedPayload(Object(f.Struct("Object")),
                Property(f.Struct("Property")), Value(f.Struct("Previous")), Value(f.Struct("Current")), Cause(f.Struct("Cause"))),
            _ => throw new NotSupportedException("Unsupported runtime definition.")
        };
        var envelope = new EventEnvelope(scope.SessionId, process, record.ProducerProcessId, record.ThreadId,
            clock.Convert(record.TimestampQpc), 1, sequence, record.ActivityId, definition.Identity, EventCodec.EncodePayload(payload, process));
        return new(EventCodec.Normalize(envelope, stream), new(original, record.TimestampQpc, SourceClock));
    }

    private DecodedEvent? StandardXaml(NativeRecord record)
    {
        if (record.ProducerProcessId != scope.Process.ProcessId) return null;
        var definition = EventCatalog.Get(EventCatalog.ObservationProviderId, 1);
        var payload = new ObservedXamlPayload(record.ProviderId, record.Name, record.DescriptorId,
            record.DescriptorVersion, record.Level, record.Keywords, record.Opcode, record.Task, record.RawPayload,
            new(MeasurementKind.Measured, CoverageState.Partial,
                "Standard XAML ETW source bytes; private object lifetime, baseline and submitted-frame semantics are unavailable."));
        var envelope = new EventEnvelope(scope.SessionId, scope.Process, record.ProducerProcessId, record.ThreadId,
            clock.Convert(record.TimestampQpc), 1, null, record.ActivityId, definition.Identity,
            EventCodec.EncodePayload(payload, scope.Process));
        // This namespace identifies the local observation stream, not a fabricated runtime epoch or sequence.
        return new(EventCodec.Normalize(envelope, new(definition.Identity.ProviderId, record.ProducerProcessId, scope.SessionId)),
            new(new(record.ProducerProcessId, null, scope.SessionId), record.TimestampQpc, SourceClock));
    }

    internal bool IsProvablyForeignRuntime(Guid provider, uint producerPid, ReadOnlySpan<byte> prefix)
    {
        if (provider == EventCatalog.StandardXamlProviderId)
            return producerPid != 0 && producerPid != scope.Process.ProcessId;
        if (provider != EventCatalog.RuntimeProviderId) return false;
        if (producerPid != scope.Process.ProcessId) return producerPid != 0;
        // Schema-1 common prefix is independent of the semantic body/name.
        if (prefix.Length < 56 || prefix[2] != 1 || prefix[31] != 1 ||
            BinaryPrimitives.ReadUInt16LittleEndian(prefix) == 0 ||
            new Guid(prefix.Slice(3, 16)) == Guid.Empty ||
            BinaryPrimitives.ReadUInt64LittleEndian(prefix[19..]) == 0 ||
            BinaryPrimitives.ReadUInt32LittleEndian(prefix[27..]) != producerPid ||
            new Guid(prefix.Slice(40, 16)) == Guid.Empty) return false;
        var creation = BinaryPrimitives.ReadInt64LittleEndian(prefix[32..]);
        return creation > 0 && scope.Process.CreationTimeFileTime is { } selected && creation != selected;
    }
    private bool IsProvablyForeignRuntime(NativeRecord record)
    {
        if (IsProvablyForeignRuntime(record.ProviderId, record.ProducerProcessId, [])) return true;
        if (record.ProviderId != EventCatalog.RuntimeProviderId) return false;
        try
        {
            var prefix = new NativeFields(record.Fields);
            if (prefix.U8("EventSchemaVersion") != 1 || prefix.U16("SemanticEventId") == 0 ||
                prefix.Guid("ProducerEpoch") == Guid.Empty || prefix.U64("SourceSequence") == 0) return false;
            var original = record.ReadRuntimeTarget(); original.Validate();
            return original.ProcessId == record.ProducerProcessId && original.CreationTimeFileTime is { } created &&
                scope.Process.CreationTimeFileTime is { } selected && created != selected;
        }
        catch (Exception error) when (error is ArgumentException or InvalidDataException or OverflowException) { return false; }
    }

    private DecodedEvent? KernelProcessStart(NativeRecord record)
    {
        var definition = EventCatalog.Get(EventCatalog.KernelProcessProviderId, 1);
        if (record.DescriptorId != 1 || record.DescriptorVersion != 4 || record.Name != definition.Identity.Name ||
            record.Level != definition.Level || record.Keywords != definition.Identity.Keywords)
            throw new NotSupportedException("Only the explicit Kernel-Process ProcessStart v4 schema is supported.");
        var f = new NativeFields(record.Fields);
        var pid = Require.Positive(f.U32("ProcessID"));
        if (f.Get("CreateTime") is not DateTime time || time.Kind == DateTimeKind.Unspecified)
            throw new InvalidDataException("Kernel process creation FILETIME is unavailable.");
        var creation = Require.Positive(time.ToFileTimeUtc());
        // Header PID is the creator/emitter. The payload is the process being created.
        if (pid != scope.Process.ProcessId || creation != scope.Process.CreationTimeFileTime) return null;
        var timestamp = clock.Convert(record.TimestampQpc);
        var payload = new ProcessPayload(ProcessOperation.Start, null,
            new(MeasurementKind.Measured, CoverageState.Partial, "Creation FILETIME is from the kernel payload; exact process creation QPC is not supplied."));
        var envelope = new EventEnvelope(scope.SessionId, scope.Process, record.ProducerProcessId, record.ThreadId, timestamp,
            1, null, record.ActivityId, definition.Identity, EventCodec.EncodePayload(payload, scope.Process));
        // A trace-local namespace, not an invented producer epoch/sequence or native settlement claim.
        return new(EventCodec.Normalize(envelope, new(record.ProviderId, record.ProducerProcessId, scope.SessionId)),
            new(scope.Process, record.TimestampQpc, SourceClock));
    }

    private DecodedEvent? Collector(NativeRecord record)
    {
        var f = new NativeFields(record.Fields);
        var id = f.U16("SemanticEventId");
        if (id is < 1 or > 3 || f.U8("EventSchemaVersion") != 1)
            throw new NotSupportedException("Only collector Process/Metric/Loss schema 1 is supported.");
        var definition = EventCatalog.Get(EventCatalog.CollectorProviderId, id);
        if (record.Name != definition.Identity.Name || record.Level != definition.Level ||
            record.Keywords != definition.Identity.Keywords)
            throw new NotSupportedException("Unsupported collector event metadata.");
        NativeSchema.ValidateCollector(record, id);
        var original = new ProcessInstanceKey(f.U32("TargetProcessId"), f.I64("TargetCreationTimeFileTime"),
            f.Guid("TargetInstanceId"));
        if (f.Guid("SessionId") != scope.SessionId || original != scope.Process) return null;
        var producerEpoch = Require.Id(f.Guid("ProducerEpoch"));
        var sequence = Require.Positive(f.U64("SourceSequence"));
        if (f.I64("QpcFrequency") != clock.SourceFrequency)
            throw new InvalidDataException("Collector frequency disagrees with the recorded QPC clock.");
        var producer = new ProducerStreamKey(record.ProviderId, record.ProducerProcessId, producerEpoch);
        IEventPayload payload = id switch
        {
            1 => CollectorProcess(f, record.TimestampQpc),
            2 => CollectorMetric(f, record.TimestampQpc),
            3 => CollectorLoss(f, record.TimestampQpc, producer),
            _ => throw new NotSupportedException("Unsupported collector definition.")
        };
        var envelope = new EventEnvelope(scope.SessionId, scope.Process, record.ProducerProcessId, record.ThreadId,
            clock.Convert(record.TimestampQpc), 1, sequence, record.ActivityId, definition.Identity,
            EventCodec.EncodePayload(payload, scope.Process));
        return new(EventCodec.Normalize(envelope, producer),
            new(original, record.TimestampQpc, SourceClock));
    }

    private ProcessPayload CollectorProcess(NativeFields f, long timestamp)
    {
        var hasCreation = f.Bool("HasCreationQpc");
        var creation = f.I64("CreationQpc");
        if (!hasCreation && creation != 0 || creation > timestamp)
            throw new InvalidDataException("Collector creation QPC is inactive or follows its observation.");
        return new(f.Enum<ProcessOperation>("Operation"), hasCreation ? clock.Convert(creation) : null,
            CollectorQuality(f));
    }

    private LossPayload CollectorLoss(NativeFields f, long timestamp, ProducerStreamKey producer)
    {
        var begin = f.I64("IntervalStartQpc");
        var end = f.I64("IntervalEndQpc");
        var hasLostEvents = f.Bool("HasLostEvents");
        var lostEvents = f.U64("LostEvents");
        var hasDomains = f.Bool("HasDomains");
        var domains = f.Array("Domains", 256);
        if (begin > end || end > timestamp || !hasLostEvents && lostEvents != 0 ||
            f.U32("DomainCount") != domains.Length || !hasDomains && domains.Length != 0)
            throw new InvalidDataException("Collector loss has inconsistent intervals, counts or optional arms.");
        return new(producer.ProducerEpoch, new(clock.Convert(begin), clock.Convert(end)),
            hasLostEvents ? lostEvents : null, f.Text("Reason"), hasDomains
                ? domains.Select(domain => new SequenceDomainKey(producer, scope.Process,
                    domain.U16("EventId"), domain.U8("EventVersion"))).ToArray() : null);
    }

    private static DataQuality CollectorQuality(NativeFields f)
    {
        var reason = f.Text("Reason");
        return new(f.Enum<MeasurementKind>("MeasurementKind"),
            f.Enum<CoverageState>("Coverage"), reason.Length == 0 ? null : reason);
    }

    private MetricPayload CollectorMetric(NativeFields f, long timestamp)
    {
        var begin = f.I64("AcquisitionBeginQpc");
        var end = f.I64("AcquisitionEndQpc");
        if (begin > end || end > timestamp)
            throw new InvalidDataException("Collector acquisition interval disagrees with the recorded QPC clock.");
        var kind = f.Enum<ValueKind>("ValueKind");
        var integer = f.I64("Integer");
        var number = f.F64("Number");
        if (kind != ValueKind.Integer && integer != 0 || kind != ValueKind.Number && number != 0)
            throw new InvalidDataException("Inactive collector value fields must be canonical zeros.");
        var quality = CollectorQuality(f);
        var value = new TypedValue(kind, f.Enum<ValueUnit>("ValueUnit"),
            kind == ValueKind.Integer ? integer : null, kind == ValueKind.Number ? number : null,
            null, null, quality);
        var hasGpu = f.Bool("HasGpu");
        var hasAdapter = f.Bool("HasAdapter");
        var hasEngine = f.Bool("HasEngine");
        if (!hasGpu && (f.U8("GpuScope") != 0 || hasAdapter || hasEngine) ||
            !hasAdapter && (f.U32("AdapterLuidLow") != 0 || f.I32("AdapterLuidHigh") != 0 ||
                f.U32("PhysicalAdapterIndex") != 0) ||
            !hasEngine && (f.U32("EngineId") != 0 || f.U8("EngineClass") != 0))
            throw new InvalidDataException("Inactive collector GPU identities must be canonical defaults.");
        GpuSampleIdentity? gpu = hasGpu ? new(hasAdapter
            ? new PhysicalAdapterKey(f.U32("AdapterLuidLow"), f.I32("AdapterLuidHigh"), f.U32("PhysicalAdapterIndex")) : null,
            f.Enum<GpuMetricScope>("GpuScope"), hasEngine ? new GpuEngineKey(f.U32("EngineId"),
                f.Enum<GpuEngineClass>("EngineClass")) : null) : null;
        var observation = new MetricSourceObservation(f.Enum<MetricClock>("NativeClock"),
            f.I64("NativeFrequency"), f.U64("NativeBegin"), f.U64("NativeEnd"),
            f.Enum<MetricSampleStatus>("NativeStatus"), f.U64("NativeAcquisitionBegin"),
            f.U64("NativeAcquisitionEnd"), f.U32("NativeError"),
            f.Enum<MetricErrorDomain>("NativeErrorDomain"), f.U32("PdhError"));
        return new(f.Enum<MetricKind>("Metric"), value,
            new(clock.Convert(begin), clock.Convert(end)), f.Text("Source"), f.Text("SourceVersion"), gpu)
        {
            SourceObservation = observation
        };
    }

    private RuntimeCapabilitiesPayload Capabilities(NativeFields f)
    {
        var entries = f.Array("Capabilities", 128);
        if (f.U32("CapabilityCount") != entries.Length || f.Guid("Epoch") != epoch || f.I64("QpcFrequency") != clock.SourceFrequency)
            throw new InvalidDataException("Capabilities prefix, count or recorded clock disagrees.");
        return new(epoch, f.Text("Build"), EnumU32<HostArchitecture>(f.U32("Architecture")), process, clock.SessionFrequency,
            entries.Select(e => new CapabilityInfo(e.Enum<Capability>("Capability"), e.I32("Version"), Quality(e.Struct("Quality")))).ToArray());
    }
    private static T EnumU32<T>(uint value) where T : struct, Enum =>
        value <= int.MaxValue ? Require.Enum((T)Enum.ToObject(typeof(T), value)) : throw new InvalidDataException("Invalid enum code.");
    private ObjectKey Object(NativeFields f)
    {
        if (f.Guid("ProducerEpoch") != epoch) throw new InvalidDataException("Object epoch differs from the common epoch.");
        return new(process, f.Enum<IdentityDomain>("Domain"), epoch, f.U64("RuntimeObjectId"));
    }
    private ObjectKey? OptionalObject(NativeFields f)
    {
        var value = f.Struct("Value");
        return f.Bool("Present") ? Object(value) : null;
    }
    private static DataQuality Quality(NativeFields f) => new(f.Enum<MeasurementKind>("Kind"), f.Enum<CoverageState>("Coverage"), f.NullableText("Reason"));
    private static XamlTypeClassification Classification(NativeFields f) =>
        new(f.Enum<XamlObjectKind>("Kind"), f.Enum<ClassificationEvidence>("Evidence"), f.Text("Source"));
    private static XamlTypeClassification? OptionalClassification(NativeFields f, string name)
    {
        var optional = f.Struct(name); var value = optional.Struct("Value");
        return optional.Bool("Present") ? Classification(value) : null;
    }
    private AttemptReference? OptionalAttempt(NativeFields f)
    {
        var value = f.Struct("Value");
        if (!f.Bool("Present")) return null;
        var attempt = new AttemptReference(value.Guid("ProducerEpoch"), value.U64("AttemptId"), value.Guid("RootId"), value.U64("DispatcherId"));
        attempt.Validate();
        if (attempt.ProducerEpoch != epoch) throw new InvalidDataException("Attempt has a foreign producer epoch.");
        return attempt;
    }
    private ChangeCause Cause(NativeFields f) => new(f.Enum<CauseKind>("Kind"), OptionalAttempt(f.Struct("Attempt")), f.NullableText("Reason"));
    private static TypedValue Value(NativeFields f)
    {
        var kind = f.Enum<ValueKind>("Kind"); var unit = f.Enum<ValueUnit>("Unit");
        var integer = f.I64("Integer"); var number = f.F64("Number"); var boolean = f.Bool("Boolean"); var text = f.Text("Text");
        return new(kind, unit, kind == ValueKind.Integer ? integer : null, kind == ValueKind.Number ? number : null,
            kind == ValueKind.Boolean ? boolean : null, kind == ValueKind.Text ? text : null, Quality(f.Struct("Quality")));
    }
    private TreeEdgePayload Tree(NativeFields f) => new(OptionalObject(f.Struct("Parent")), Object(f.Struct("Child")),
        f.Enum<RelationshipKind>("Relationship"), f.I32("Order"), f.Guid("RootId"), f.U64("DispatcherId"), f.Bool("Connected"),
        Cause(f.Struct("Cause")), f.Enum<TreeMembershipKind>("Membership"));
    private ResourcePayload Resource(NativeFields f) => new(Object(f.Struct("Resource")), f.Enum<ResourceOperation>("Operation"),
        f.Guid("DeviceEpoch"), f.Optional("BackingAllocationId", (v, n) => v.Guid(n)), f.Optional("AllocationBytes", (v, n) => v.U64(n)),
        f.Enum<SizeProvenance>("SizeProvenance"), OptionalObject(f.Struct("Owner")), f.Optional("WidthPixels", (v, n) => v.U32(n)),
        f.Optional("HeightPixels", (v, n) => v.U32(n)), Quality(f.Struct("Quality")),
        new ResourceClassification(f.Struct("Classification").Enum<ResourceKind>("Kind"),
            f.Struct("Classification").Enum<ResourceKindEvidence>("Evidence"), f.Struct("Classification").Text("Source")), Cause(f.Struct("Cause")));
    private AllocationPayload Allocation(NativeFields f) => new(f.U64("AllocationId"), f.Enum<AllocationOperation>("Operation"),
        f.Optional("PreviousAllocationId", (v, n) => v.U64(n)), f.U64("RequestedBytes"), f.Optional("ActualBytes", (v, n) => v.U64(n)),
        f.Guid("AllocatorEpoch"), f.Optional("BaselineCaptureId", (v, n) => v.Guid(n)), Quality(f.Struct("Quality")), Cause(f.Struct("Cause")));
    private TimeRange Interval(NativeFields f) => new(clock.Convert(f.I64("StartQpc")), clock.Convert(f.I64("EndQpc")));
    private static XamlPropertyKey Property(NativeFields f) => new(f.U64("PropertyId"), f.Text("DeclaringType"), f.Text("Name"));
    private SequenceDomainKey Domain(NativeFields f)
    {
        var d = new SequenceDomainKey(stream, process, f.U16("EventId"), f.U8("EventVersion")); d.Validate(); return d;
    }
    private SequenceDomainKey[]? LossDomains(NativeFields f)
    {
        var domains = f.Array("Domains", 256).Select(Domain).ToArray();
        if (!f.Bool("HasDomains") && domains.Length != 0) throw new InvalidDataException("Absent loss domains must have empty storage.");
        return f.Bool("HasDomains") ? domains : null;
    }
    private CaptureBoundaryPayload CaptureBoundary(NativeFields f) => new(f.Guid("CaptureId"), f.Enum<CaptureKind>("Kind"),
        f.Enum<CapturePhase>("Phase"), f.Optional("RootId", (v, n) => v.Guid(n)), f.Optional("DispatcherId", (v, n) => v.U64(n)),
        f.Array("Domains", 256).Select(v =>
        {
            var domain = Domain(v.Struct("Domain")); var cut = v.Struct("SettledCut"); var cv = cut.Struct("Value");
            ProducerSequenceCut? settled = cut.Bool("Present")
                ? new(cv.Guid("CutId"), domain, cv.U64("ReservedThrough"), cv.U64("SettledThrough")) : null;
            return new SequenceDomainRange(domain, v.Optional("BeforeSequence", (x, n) => x.U64(n)),
                v.Optional("ThroughSequence", (x, n) => x.U64(n)), Quality(v.Struct("Quality")), settled);
        }).ToArray(), f.Optional("ChunkCount", (v, n) => v.U32(n)), Quality(f.Struct("Quality")));

    private CaptureChunkPayload CaptureChunk(NativeFields f)
    {
        var keys = f.Array("Keys").Select(k => new ObjectKey(process, k.Enum<IdentityDomain>("Domain"), epoch, k.U64("RuntimeObjectId"))).ToArray();
        var qualities = f.Array("Qualities").Select(q => new DataQuality(q.Enum<MeasurementKind>("Kind"), q.Enum<CoverageState>("Coverage"),
            FlatText(q, "Reason"))).ToArray();
        var values = f.Array("Values").Select(v =>
        {
            var kind = v.Enum<ValueKind>("Kind"); var unit = v.Enum<ValueUnit>("Unit");
            var integer = v.I64("Integer"); var number = v.F64("Number"); var boolean = v.Bool("Boolean"); var text = v.Text("Text");
            if (kind != ValueKind.Integer && integer != 0 || kind != ValueKind.Number && number != 0 ||
                kind != ValueKind.Boolean && boolean || kind != ValueKind.Text && text.Length != 0)
                throw new InvalidDataException("Inactive capture value arms are not canonical defaults.");
            var value = new TypedValue(kind, unit, kind == ValueKind.Integer ? integer : null, kind == ValueKind.Number ? number : null,
                kind == ValueKind.Boolean ? boolean : null, kind == ValueKind.Text ? text : null, Ref(qualities, v.U32("QualityIndex")));
            value.Validate(); return value;
        }).ToArray();
        var classifications = f.Array("Classifications");
        foreach (var c in classifications)
        {
            _ = c.Enum<ResourceKind>("Kind"); _ = c.Enum<ResourceKindEvidence>("Evidence"); _ = Require.Text(c.Text("Source"));
        }
        var causes = f.Array("Causes").Select(c =>
        {
            var attemptId = c.U64("AttemptId"); var rootId = c.Guid("RootId"); var dispatcher = c.U64("DispatcherId");
            var has = c.Bool("HasAttempt");
            if (!has && (attemptId != 0 || rootId != Guid.Empty || dispatcher != 0)) throw new InvalidDataException("Inactive cause attempt is not canonical.");
            var cause = new ChangeCause(c.Enum<CauseKind>("Kind"), has ? new(epoch, attemptId, rootId, dispatcher) : null, FlatText(c, "Reason"));
            cause.Validate(epoch); return cause;
        }).ToArray();
        ObjectKey Key(NativeFields v, string name) => Ref(keys, v.U32(name));
        ObjectKey? NullableKey(NativeFields v, string name) => v.U32(name) is var i && i == 0 ? null : Ref(keys, i);
        ChangeCause C(NativeFields v) => Ref(causes, v.U32("CauseIndex"));
        DataQuality Q(NativeFields v) => Ref(qualities, v.U32("QualityIndex"));
        XamlTypeClassification? X(NativeFields v)
        {
            var i = v.U32("ClassificationIndex"); return i == 0 ? null : Classification(Ref(classifications, i));
        }
        var root = f.Guid("RootId"); var dispatcher = f.U64("DispatcherId");
        return new(f.Guid("CaptureId"), f.U32("ChunkIndex"), root, dispatcher,
            f.Array("Objects").Select(v => new SnapshotObject(Key(v, "KeyIndex"), FlatText(v, "TypeName"),
                FlatOptional(v, "Address", (x, n) => x.U64(n)), Ref(values, v.U32("SizeIndex")), X(v))).ToArray(),
            f.Array("Ownership", 8192).Select(v => new OwnershipEdge(Key(v, "FromIndex"), Key(v, "ToIndex"),
                v.Text("Relationship"), v.Enum<OwnershipEvidence>("Evidence"), Q(v))).ToArray(),
            f.Array("Tree").Select(v => new TreeEdgePayload(NullableKey(v, "ParentIndex"), Key(v, "ChildIndex"),
                v.Enum<RelationshipKind>("Relationship"), v.I32("Order"), root, dispatcher, v.Bool("Connected"), C(v), v.Enum<TreeMembershipKind>("Membership"))).ToArray(),
            f.Array("Correlations").Select(v => new CorrelationPayload(Key(v, "FromIndex"), Key(v, "ToIndex"),
                v.Enum<CorrelationKind>("Kind"), v.Bool("Linked"), C(v))).ToArray(),
            f.Array("Resources").Select(v =>
            {
                var c = Ref(classifications, v.U32("ClassificationIndex"));
                return new ResourcePayload(Key(v, "ResourceIndex"), v.Enum<ResourceOperation>("Operation"), v.Guid("DeviceEpoch"),
                    FlatOptional(v, "BackingAllocationId", (x, n) => x.Guid(n)), FlatOptional(v, "AllocationBytes", (x, n) => x.U64(n)),
                    v.Enum<SizeProvenance>("SizeProvenance"), NullableKey(v, "OwnerIndex"), FlatOptional(v, "WidthPixels", (x, n) => x.U32(n)),
                    FlatOptional(v, "HeightPixels", (x, n) => x.U32(n)), Q(v),
                    new(c.Enum<ResourceKind>("Kind"), c.Enum<ResourceKindEvidence>("Evidence"), c.Text("Source")), C(v));
            }).ToArray(),
            f.Array("Visuals").Select(v => new VisualDeltaPayload(Key(v, "VisualIndex"), v.Enum<VisualProperty>("Property"), Ref(values, v.U32("ValueIndex")), C(v))).ToArray(),
            f.Array("Associations").Select(v => new AssociationPayload(Key(v, "CoreIndex"), Key(v, "PeerIndex"), v.Bool("Linked"), C(v))).ToArray(),
            f.Array("Types").Select(v => new TypeResolvedPayload(epoch, v.U64("TypeId"), v.Text("Name"), X(v))).ToArray(),
            f.Array("Allocations").Select(v => new AllocationPayload(v.U64("AllocationId"), v.Enum<AllocationOperation>("Operation"),
                FlatOptional(v, "PreviousAllocationId", (x, n) => x.U64(n)), v.U64("RequestedBytes"),
                FlatOptional(v, "ActualBytes", (x, n) => x.U64(n)), v.Guid("AllocatorEpoch"),
                FlatOptional(v, "BaselineCaptureId", (x, n) => x.Guid(n)), Q(v), C(v))).ToArray());
    }
    private static T Ref<T>(T[] rows, uint index) => index > 0 && index <= rows.Length
        ? rows[index - 1] : throw new InvalidDataException("Capture table reference is zero or out of bounds.");
    private static string? FlatText(NativeFields f, string name)
    {
        var has = f.Bool("Has" + name); var value = f.Text(name);
        if (!has && value.Length != 0) throw new InvalidDataException("Inactive text is not canonical.");
        return has ? value : null;
    }
    private static T? FlatOptional<T>(NativeFields f, string name, Func<NativeFields, string, T> read) where T : struct
    {
        var has = f.Bool("Has" + name); var value = read(f, name);
        if (!has && !EqualityComparer<T>.Default.Equals(value, default)) throw new InvalidDataException("Inactive optional is not canonical.");
        return has ? value : null;
    }
}
