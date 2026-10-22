using System.Text.Json;
using System.Text.Json.Serialization;
using WinUIProfiler.Contracts;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class CorrectionContractTests
{
    internal static readonly ResultContext Context = new(new(CoreTests.Session, CoreTests.Process), new(1000, 1000), DataQuality.Complete);
    internal static readonly AttemptReference Attempt = new(CoreTests.Epoch, 1, CoreTests.Session, 1);
    internal static readonly ChangeCause Explicit = new(CauseKind.ExplicitAttempt, Attempt, null);
    internal static readonly XamlTypeClassification UiElement = new(XamlObjectKind.UIElement, ClassificationEvidence.RuntimeType, "runtime IsUIElement");
    internal static readonly ResourceClassification Texture = new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "CreateTexture2D");
    private static readonly JsonSerializerOptions Json = new()
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
        RespectRequiredConstructorParameters = true,
        RespectNullableAnnotations = true,
        Converters = { new JsonStringEnumConverter(allowIntegerValues: false) }
    };
    internal static TypedValue Number(double value, ValueUnit unit = ValueUnit.Pixels) => new(ValueKind.Number, unit, null, value, null, null, DataQuality.Complete);
    internal static TypedValue Integer(long value, ValueUnit unit = ValueUnit.Bytes) => new(ValueKind.Integer, unit, value, null, null, null, DataQuality.Complete);
    internal static TypedValue Missing(ValueUnit unit) => new(ValueKind.Missing, unit, null, null, null, null, new(MeasurementKind.Unavailable, CoverageState.Unavailable, "attribute not captured"));
    internal static T RoundTrip<T>(T value) => JsonSerializer.Deserialize<T>(JsonSerializer.Serialize(value, Json), Json)!;
    internal static QueryPage<T> Page<T>(params T[] rows) where T : IQueryValue => new(Context, rows, new(100, 0, null, false));
    internal static EventEnvelope Normalize(IEventPayload body) => EventCodec.Normalize(BoundaryTests.Envelope(body),
        new(EventCatalog.RuntimeProviderId, 42, CoreTests.Epoch)).Envelope;
    internal static SnapshotMetadata Snapshot(SnapshotState state, SnapshotArtifact? artifact = null, CaptureExtent? extent = null) =>
        new(CoreTests.Epoch, CoreTests.Session, CoreTests.Process, SnapshotKind.Lightweight,
            extent ?? new(10, state is SnapshotState.Pending or SnapshotState.Capturing ? null : 40,
                CaptureMechanism.RuntimeBridge, CaptureConsistency.NonAtomic, []),
            100, CoreTests.Epoch, state,
            state == SnapshotState.Complete ? DataQuality.Complete : new(MeasurementKind.Unavailable, CoverageState.Partial, "attempt has no completed artifact"),
            artifact, state is SnapshotState.Failed or SnapshotState.Cancelled ? "target disconnected" : null,
            1, "synthetic/1", 1_000_000);

    [Theory]
    [InlineData(CauseKind.Unknown)]
    [InlineData(CauseKind.OutsideAttempt)]
    [InlineData(CauseKind.ExplicitAttempt)]
    public void R1_Property_and_membership_observations_keep_typed_causality(CauseKind kind)
    {
        var cause = new ChangeCause(kind, kind == CauseKind.ExplicitAttempt ? Attempt : null, kind == CauseKind.Unknown ? "no bridge context" : null);
        var property = new XamlPropertyChangedPayload(CoreTests.Key(), new(1, "Panel", "Width"), Missing(ValueUnit.Pixels), Number(20), cause);
        var captured = EventCodec.ReadPayload<XamlPropertyChangedPayload>(Normalize(property));
        Assert.Equal(cause, captured.Cause);
        var edge = new TreeEdgePayload(null, CoreTests.Key(), RelationshipKind.Root, 0, CoreTests.Session, 1, true, cause, TreeMembershipKind.Visual);
        Assert.Equal(cause, EventCodec.ReadPayload<TreeEdgePayload>(Normalize(edge)).Cause);
        var history = Page(new ObjectHistoryEntry(CoreTests.Key(), 100, 10,
            new PropertyObservation(property.Property, property.Previous, property.Current), cause, DataQuality.Complete),
            new ObjectHistoryEntry(CoreTests.Key(), 101, 11,
                new MembershipObservation(null, RelationshipKind.Root, 0, CoreTests.Session, 1, true, TreeMembershipKind.Visual), cause, DataQuality.Complete));
        var restored = RoundTrip(history);
        Assert.IsType<PropertyObservation>(restored.Items[0].Observation);
        Assert.IsType<MembershipObservation>(restored.Items[1].Observation);
    }

    [Fact]
    public void R1_Attempt_identity_separates_dispatchers_epochs_and_post_submit_failure()
    {
        var otherDispatcher = Attempt with { DispatcherId = 2 };
        var otherEpoch = Attempt with { ProducerEpoch = CoreTests.Session };
        Assert.NotEqual(Attempt, otherDispatcher);
        Assert.NotEqual(Attempt, otherEpoch);
        Assert.Throws<ArgumentException>(() => Normalize(new VisualDeltaPayload(
            new(CoreTests.Process, IdentityDomain.CompositionVisual, CoreTests.Epoch, 1), VisualProperty.Opacity,
            Number(0.5, ValueUnit.Scalar), new(CauseKind.ExplicitAttempt, otherEpoch, null))));
        var submitted = EventCodec.ReadPayload<FramePayload>(Normalize(new FramePayload(Attempt, 1, FrameOutcome.Submitted, 0)));
        var failed = EventCodec.ReadPayload<FramePayload>(Normalize(new FramePayload(Attempt, null, FrameOutcome.Failed, -1)));
        Assert.Equal(submitted.Attempt, failed.Attempt);
        var change = new FrameChange(100, 10, CoreTests.Key(), new PropertyObservation(new(1, "Panel", "Width"), Number(1), Number(2)),
            Explicit, FrameChangeRelation.Causal, DataQuality.Complete);
        var frame = RoundTrip(Page(new FrameInfo(Attempt, new(90, 110), FrameAttemptStatus.Failed,
            new(1, 105, CoreTests.Epoch), -1, [change])));
        Assert.NotNull(frame.Items[0].Submission);
        Assert.Equal(FrameAttemptStatus.Failed, frame.Items[0].AttemptStatus);
        Assert.Throws<ArgumentException>(() => Page(new FrameInfo(otherDispatcher, new(90, 110), FrameAttemptStatus.Failed,
            null, -1, [change])));
    }

    [Fact]
    public void R1_Invalid_cause_discriminators_and_unqualified_attempts_are_rejected()
    {
        Assert.ThrowsAny<ArgumentException>(() => new ChangeCause(CauseKind.ExplicitAttempt, null, null).Validate());
        Assert.ThrowsAny<ArgumentException>(() => new ChangeCause(CauseKind.OutsideAttempt, Attempt, null).Validate());
        Assert.ThrowsAny<ArgumentException>(() => new ChangeCause(CauseKind.Unknown, null, null).Validate());
        Assert.ThrowsAny<ArgumentException>(() => Normalize(new FramePayload(default(AttemptReference), null, FrameOutcome.Failed, -1)));
    }

    [Theory]
    [InlineData(SnapshotState.Pending)]
    [InlineData(SnapshotState.Capturing)]
    [InlineData(SnapshotState.Failed)]
    [InlineData(SnapshotState.Cancelled)]
    public void R2_Snapshot_attempts_without_artifacts_are_honest_query_rows(SnapshotState state)
    {
        var row = RoundTrip(Page(Snapshot(state))).Items[0];
        Assert.Equal(state, row.State);
        Assert.Null(row.Artifact);
        if (state is SnapshotState.Failed or SnapshotState.Cancelled) Assert.False(string.IsNullOrWhiteSpace(row.StatusReason));
    }

    [Theory]
    [InlineData("..\\escape.bin", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")]
    [InlineData("capture.bin", "not-a-hash")]
    public void R2_Existing_artifacts_are_validated(string path, string hash) =>
        Assert.ThrowsAny<ArgumentException>(() => Snapshot(SnapshotState.Complete, new(path, hash)));

    [Fact]
    public void R2_Complete_snapshot_cannot_be_an_artifact_free_fake_success() =>
        Assert.ThrowsAny<ArgumentException>(() => Snapshot(SnapshotState.Complete));

    [Fact]
    public void R3_Capture_extent_preserves_non_atomic_multi_scope_intervals_and_freezes_them()
    {
        var scopes = new[]
        {
            new CaptureScopeBoundary(new(CaptureScopeKind.XamlRoot, CoreTests.Session, 1, null), new(10, 20), DataQuality.Complete),
            new CaptureScopeBoundary(new(CaptureScopeKind.XamlRoot, CoreTests.Epoch, 2, null), new(20, 40), DataQuality.Complete)
        };
        var row = Snapshot(SnapshotState.Complete, new("capture.bin", new string('a', 64)),
            new(10, 40, CaptureMechanism.RuntimeBridge, CaptureConsistency.NonAtomic, scopes));
        scopes[0] = null!;
        var restored = RoundTrip(Page(row)).Items[0];
        Assert.Equal(CaptureConsistency.NonAtomic, restored.Extent.Consistency);
        Assert.Equal(new TimeRange(20, 40), restored.Extent.Boundaries[1].Interval);
        Assert.NotNull(restored.Extent.Boundaries[0]);
    }

    [Fact]
    public void R3_Comparison_rows_have_distinct_attribute_grouping_keys_and_signed_deltas()
    {
        var first = new SnapshotComparisonKey(SnapshotAttribute.ObjectCount, ComparisonGrouping.Session, null, null, null, null);
        var second = new SnapshotComparisonKey(SnapshotAttribute.ConnectedElementCount, ComparisonGrouping.Type, null, "Panel", null, null);
        var page = RoundTrip(Page(
            new SnapshotDelta(CoreTests.Epoch, first, Integer(5, ValueUnit.Count), Integer(2, ValueUnit.Count), Integer(-3, ValueUnit.Count), ComparisonCompatibility.Compatible, null),
            new SnapshotDelta(CoreTests.Epoch, second, Integer(5, ValueUnit.Count), Missing(ValueUnit.Count), Missing(ValueUnit.Count), ComparisonCompatibility.MissingCurrent, "attribute missing")));
        Assert.NotEqual(page.Items[0].Key, page.Items[1].Key);
        Assert.Equal(-3, page.Items[0].Delta.Integer);
        Assert.Equal(ValueKind.Missing, page.Items[1].Current.Kind);
        var incompatible = RoundTrip(Page(new SnapshotDelta(CoreTests.Epoch,
            new(SnapshotAttribute.AllocatedBytes, ComparisonGrouping.ResourceKind, null, null, null, ResourceKind.Texture2D),
            Integer(8), Integer(2, ValueUnit.Count), Missing(ValueUnit.Bytes), ComparisonCompatibility.Incompatible, "attribute units differ")));
        Assert.Equal(ComparisonCompatibility.Incompatible, incompatible.Items[0].Compatibility);
    }

    [Fact]
    public void R4_Tree_composition_and_resource_query_rows_preserve_observation_attributes()
    {
        var edge = EventCodec.ReadPayload<TreeEdgePayload>(Normalize(new TreeEdgePayload(null, CoreTests.Key(), RelationshipKind.Root,
            0, CoreTests.Session, 1, true, Explicit, TreeMembershipKind.Visual)));
        var row = new TreeEntry(edge.Child, edge.Parent, edge.Relationship, edge.Order, edge.Connected, edge.RootId, edge.DispatcherId,
            "Panel", DataQuality.Complete, edge.Membership, UiElement);
        Assert.Equal(edge.RootId, RoundTrip(Page(row)).Items[0].RootId);
        Assert.Equal(edge.DispatcherId, RoundTrip(Page(row)).Items[0].DispatcherId);
        var visual = new ObjectKey(CoreTests.Process, IdentityDomain.CompositionVisual, CoreTests.Epoch, 2);
        var visualEvent = EventCodec.ReadPayload<VisualDeltaPayload>(Normalize(new VisualDeltaPayload(visual, VisualProperty.Opacity, Number(0.5, ValueUnit.Scalar), Explicit)));
        var composition = RoundTrip(Page(new CompositionTreeEntry(row with
        {
            Object = visual, Relationship = RelationshipKind.VisualRoot, Membership = TreeMembershipKind.Composition,
            Classification = new(XamlObjectKind.NotApplicable, ClassificationEvidence.RuntimeBridge, "composition identity")
        }, [new(visualEvent.Property, visualEvent.Value, 100, Explicit)])));
        Assert.Equal(0.5, composition.Items[0].Properties[0].Value.Number);
        var resource = new ObjectKey(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 3);
        var observed = EventCodec.ReadPayload<ResourcePayload>(Normalize(new ResourcePayload(resource, ResourceOperation.Create,
            CoreTests.Epoch, CoreTests.Session, 4096, SizeProvenance.ActualAllocation, CoreTests.Key(), 32, 32, DataQuality.Complete, Texture, Explicit)));
        var usage = RoundTrip(Page(new ResourceUsageEntry(resource, observed.Owner, observed.BackingAllocationId, observed.DeviceEpoch,
            Integer(4096), observed.SizeProvenance, observed.Classification, observed.Quality)));
        Assert.Equal(Texture, usage.Items[0].Classification);
    }

    [Theory]
    [InlineData(GpuMetricScope.PerEngine)]
    [InlineData(GpuMetricScope.OverallBusiestEngine)]
    public void R4_Gpu_utilization_preserves_physical_adapter_engine_and_aggregation(GpuMetricScope scope)
    {
        var gpu = new GpuSampleIdentity(new(0x1234, -1, 1), scope, new(0, GpuEngineClass.ThreeD));
        var body = new MetricPayload(MetricKind.GpuUtilization, Number(12.5, ValueUnit.Percent), new(0, 100), "OS GPU engine counters", "1", gpu);
        var observed = EventCodec.ReadPayload<MetricPayload>(EventCodec.Capture(BoundaryTests.Envelope(body, EventCatalog.CollectorProviderId)));
        var row = RoundTrip(Page(new MetricSample(100, observed.Metric, observed.Value, observed.Interval, observed.Source, observed.SourceVersion, observed.Gpu))).Items[0];
        Assert.Equal(gpu, row.Gpu);
        Assert.Equal(scope, row.Gpu!.Scope);
        Assert.Equal(1u, row.Gpu.Adapter!.Value.PhysicalAdapterIndex);
    }

    [Theory]
    [InlineData(MetricKind.GpuDedicatedCommittedBytes)]
    [InlineData(MetricKind.GpuSharedCommittedBytes)]
    [InlineData(MetricKind.GpuDedicatedResidentBytes)]
    [InlineData(MetricKind.GpuSharedResidentBytes)]
    public void R4_Gpu_memory_components_remain_distinct(MetricKind metric)
    {
        var gpu = new GpuSampleIdentity(new(1234, 0, 0), GpuMetricScope.PerAdapterMemory, null);
        var observed = EventCodec.ReadPayload<MetricPayload>(EventCodec.Capture(BoundaryTests.Envelope(
            new MetricPayload(metric, Integer(4096), new(0, 100), "OS GPU memory", "1", gpu), EventCatalog.CollectorProviderId)));
        Assert.Equal(metric, RoundTrip(Page(new MetricSample(100, metric, observed.Value, observed.Interval, observed.Source, observed.SourceVersion, observed.Gpu))).Items[0].Metric);
    }

    [Theory]
    [InlineData(XamlObjectKind.DependencyObject, TreeMembershipKind.Logical)]
    [InlineData(XamlObjectKind.DependencyObject, TreeMembershipKind.Visual)]
    [InlineData(XamlObjectKind.UIElement, TreeMembershipKind.Visual)]
    [InlineData(XamlObjectKind.UIElement, TreeMembershipKind.Logical)]
    public void R4_Ui_element_and_membership_evidence_are_not_guessed_from_type_names(XamlObjectKind kind, TreeMembershipKind membership)
    {
        var classification = new XamlTypeClassification(kind, ClassificationEvidence.RuntimeType, "runtime type flags");
        var observed = EventCodec.ReadPayload<ObjectLifetimePayload>(Normalize(new ObjectLifetimePayload(CoreTests.Key(), LifetimeOperation.Birth, 1, null, DataQuality.Complete, classification)));
        var row = RoundTrip(Page(new TreeEntry(CoreTests.Key(), null, RelationshipKind.Root, 0, true, CoreTests.Session, 1,
            "SameOptionalDisplayName", DataQuality.Complete, membership, observed.Classification!))).Items[0];
        Assert.Equal(kind, row.Classification.Kind);
        Assert.Equal(membership, row.Membership);
    }

    [Fact]
    public void R4_Unknown_resource_kind_is_explicit_not_inferred_and_known_kind_needs_evidence()
    {
        var resource = new ObjectKey(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 2);
        var unknown = new ResourceClassification(ResourceKind.Unknown, ResourceKindEvidence.Unavailable, "kind was not observed");
        var payload = new ResourcePayload(resource, ResourceOperation.Create, CoreTests.Epoch, null, null,
            SizeProvenance.Unknown, null, null, null, DataQuality.Complete, unknown);
        var observed = EventCodec.ReadPayload<ResourcePayload>(Normalize(payload));
        var row = RoundTrip(Page(new ResourceUsageEntry(resource, null, null, CoreTests.Epoch, Missing(ValueUnit.Bytes),
            SizeProvenance.Unknown, observed.Classification, observed.Quality))).Items[0];
        Assert.Equal(ResourceKind.Unknown, row.Classification.Kind);
        Assert.Equal(ValueKind.Missing, row.Size.Kind);
        Assert.Throws<ArgumentException>(() => Normalize(payload with
        {
            Classification = unknown with { Kind = ResourceKind.Texture2D }
        }));
    }

    [Fact]
    public void R5_Published_frame_and_composition_nested_arrays_are_owned()
    {
        var changes = new[] { new FrameChange(100, 10, null, new BoundaryObservation(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 2, null),
            Explicit, FrameChangeRelation.Causal, DataQuality.Complete) };
        var frame = Page(new FrameInfo(Attempt, new(90, 110), FrameAttemptStatus.Completed, null, 0, changes));
        changes[0] = null!;
        Assert.NotNull(frame.Items[0].Changes[0]);
        var properties = new[] { new ObservedVisualProperty(VisualProperty.Opacity, Number(0.5, ValueUnit.Scalar), 100, Explicit) };
        var visual = new ObjectKey(CoreTests.Process, IdentityDomain.CompositionVisual, CoreTests.Epoch, 2);
        var tree = new TreeEntry(visual, null, RelationshipKind.VisualRoot, 0, true, CoreTests.Session, 1, null,
            DataQuality.Complete, TreeMembershipKind.Composition, new(XamlObjectKind.NotApplicable, ClassificationEvidence.RuntimeType, "visual identity"));
        var composition = Page(new CompositionTreeEntry(tree, properties));
        properties[0] = null!;
        Assert.NotNull(RoundTrip(composition).Items[0].Properties[0]);
    }

    [Theory]
    [InlineData("timestamp")]
    [InlineData("enum")]
    [InlineData("boolean")]
    [InlineData("source")]
    [InlineData("version")]
    public void R5_Metric_result_rows_are_validated_independently(string invalid)
    {
        var row = new MetricSample(100, MetricKind.CpuUtilization, Number(10, ValueUnit.Percent), new(0, 100), "OS", "1");
        row = invalid switch
        {
            "timestamp" => row with { TimestampQpc = -1 },
            "enum" => row with { Metric = (MetricKind)999 },
            "boolean" => row with { Value = new(ValueKind.Boolean, ValueUnit.Percent, null, null, true, null, DataQuality.Complete) },
            "source" => row with { Source = "" },
            _ => row with { SourceVersion = "" }
        };
        Assert.ThrowsAny<ArgumentException>(() => Page(row));
    }

    [Fact]
    public void R5_Result_boundary_checks_process_membership_and_freezes_session_capabilities()
    {
        var foreign = new ObjectKey(new(42, null, CoreTests.Epoch), IdentityDomain.XamlCore, CoreTests.Epoch, 1);
        Assert.Throws<ArgumentException>(() => Page(new RetentionPath(CoreTests.Key(),
            [new(CoreTests.Key(), foreign, "edge", OwnershipEvidence.Proven, DataQuality.Complete)], "root", OwnershipEvidence.Proven, DataQuality.Complete)));
        var capabilities = new[] { new CapabilityInfo(Capability.Cpu, 1, DataQuality.Complete) };
        var producers = new[] { new ProducerInfo(EventCatalog.RuntimeProviderId, CoreTests.Epoch, 1, "runtime/1", HostArchitecture.X64, capabilities) };
        var result = new QueryResult<SessionInfo>(Context, new(SessionMode.LiveAttach, SessionStatus.Recording, 1_000_000, null, producers, null));
        capabilities[0] = null!;
        producers[0] = null!;
        Assert.Equal(Capability.Cpu, RoundTrip(result).Value.Producers[0].Capabilities[0].Capability);
    }

    [Theory]
    [InlineData(CoverageState.Complete)]
    [InlineData(CoverageState.Partial)]
    public void R6_Estimated_size_quality_is_independent_of_coverage(CoverageState coverage)
    {
        var quality = new DataQuality(MeasurementKind.Estimated, coverage, "description-derived byte count");
        var resource = new ObjectKey(CoreTests.Process, IdentityDomain.Resource, CoreTests.Epoch, 2);
        var e = Normalize(new ResourcePayload(resource, ResourceOperation.Create, CoreTests.Epoch, null, 16,
            SizeProvenance.ApiDescriptionEstimate, null, 2, 2, quality, Texture));
        Assert.Equal(quality, EventCodec.ReadPayload<ResourcePayload>(e).Quality);
    }

    [Fact]
    public void R6_String_sizes_are_rejected_and_missing_bytes_are_preserved()
    {
        CaptureChunkPayload Chunk(TypedValue size) => new(CoreTests.Session, 0, CoreTests.Session, 1,
            [new(CoreTests.Key(), null, null, size)], [], [], [], [], [], [], [], []);
        Assert.Throws<ArgumentException>(() => Normalize(Chunk(new(ValueKind.Text, ValueUnit.String, null, null, null, "16", DataQuality.Complete))));
        Assert.Equal(ValueKind.Missing, EventCodec.ReadPayload<CaptureChunkPayload>(Normalize(Chunk(Missing(ValueUnit.Bytes)))).Objects[0].Size.Kind);
    }
}
