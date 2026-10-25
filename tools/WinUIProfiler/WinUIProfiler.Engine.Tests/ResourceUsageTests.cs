using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class ResourceUsageTests
{
    [Fact]
    public async Task Shared_backing_is_preserved_as_one_allocation_not_a_process_gpu_total()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(2)));
        var rows = await f.Usage(20);
        Assert.Equal(2, rows.Count);
        Assert.Single(rows.Select(r => (r.Resource.Process, r.Resource.ProducerEpoch, r.DeviceEpoch, r.BackingAllocationId)).Distinct());
        Assert.Equal(4096, rows.GroupBy(r => r.BackingAllocationId).Sum(g => g.First().Size.Integer));
        Assert.All(rows, r => Assert.Contains("not process GPU", r.Quality.Reason));
        Assert.DoesNotContain(f.LastChanges, c => c.Value is MetricSample);
    }

    [Fact]
    public async Task Destroying_one_alias_does_not_destroy_another_resources_shared_backing()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(2)), (30, f.Resource(1, ResourceOperation.Destroy)));
        var survivor = Assert.Single(await f.Usage(30));
        Assert.Equal(f.Key(2), survivor.Resource);
        Assert.Equal(f.Backing, survivor.BackingAllocationId);
        Assert.Equal(4096, survivor.Size.Integer);
    }

    [Fact]
    public async Task Unknown_owner_size_kind_and_birth_are_not_fabricated()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1, ResourceOperation.Share) with
        {
            Owner = null, BackingAllocationId = null, AllocationBytes = null,
            SizeProvenance = SizeProvenance.Unknown,
            Classification = new(ResourceKind.Unknown, ResourceKindEvidence.Unavailable, "not observed")
        }));
        var row = Assert.Single(await f.Usage(10));
        Assert.Null(row.Owner);
        Assert.Null(row.BackingAllocationId);
        Assert.Equal(ValueKind.Missing, row.Size.Kind);
        Assert.Equal(ResourceKind.Unknown, row.Classification.Kind);
        Assert.Contains("Unknown birth", row.Quality.Reason);
        var history = Assert.Single(await f.History(f.Key(1)));
        Assert.IsType<ResourceObservation>(history.Observation);
        Assert.Equal(ResourceOperation.Share, ((ResourceObservation)history.Observation).Operation);
    }

    [Fact]
    public async Task Ownership_release_and_Close_do_not_claim_resource_or_backing_destruction()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(1, ResourceOperation.OwnershipRelease)));
        Assert.Empty(await f.Usage(20, f.Owner));
        var released = Assert.Single(await f.Usage(20));
        Assert.Null(released.Owner);
        Assert.Equal(f.Backing, released.BackingAllocationId);
        Assert.Equal(4096, released.Size.Integer);
        f.Commit((30, f.Resource(1, ResourceOperation.Close)));
        Assert.Single(await f.Usage(30));
        f.Commit((40, f.Resource(1, ResourceOperation.Destroy)));
        Assert.Empty(await f.Usage(40));
        Assert.Equal(new[] { ResourceOperation.Create, ResourceOperation.OwnershipRelease, ResourceOperation.Close, ResourceOperation.Destroy },
            (await f.History(f.Key(1))).Select(r => ((ResourceObservation)r.Observation).Operation));
    }

    [Fact]
    public async Task Sparse_release_preserves_unknown_current_size_and_prior_backing_history_not_a_fake_zero()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(1, ResourceOperation.OwnershipRelease) with
        {
            Owner = null, BackingAllocationId = null, AllocationBytes = null, SizeProvenance = SizeProvenance.Unknown
        }));
        Assert.Equal(ValueKind.Missing, Assert.Single(await f.Usage(20)).Size.Kind);
        var history = await f.History(f.Key(1));
        Assert.Equal(f.Backing, ((ResourceObservation)history[0].Observation).BackingAllocationId);
        Assert.Equal(ResourceOperation.OwnershipRelease, ((ResourceObservation)history[1].Observation).Operation);
        Assert.DoesNotContain(history, h => h.Observation is ResourceObservation { Operation: ResourceOperation.Destroy });
    }

    [Fact]
    public async Task Epoch_qualified_runtime_reuse_never_merges_lifetimes()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(1, ResourceOperation.Destroy)));
        var epoch = Guid.NewGuid();
        var next = f.Resource(1) with
        {
            Resource = new(f.Scope.Process, IdentityDomain.Resource, epoch, 1),
            Owner = new(f.Scope.Process, IdentityDomain.XamlCore, epoch, 9)
        };
        f.Commit((30, next));
        Assert.Equal(next.Resource, Assert.Single(await f.Usage(30)).Resource);
        Assert.Equal(2, (await f.History(f.Key(1))).Count);
    }

    [Fact]
    public async Task Rebind_tombstones_previous_owner_and_backing_without_erasing_history()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        var next = f.Resource(1, ResourceOperation.Rebind) with { Owner = f.Key(3, IdentityDomain.XamlCore), BackingAllocationId = Guid.NewGuid() };
        f.Commit((20, next));
        Assert.Empty(await f.Usage(20, f.Owner));
        var row = Assert.Single(await f.Usage(20));
        Assert.Equal(next.Owner, row.Owner);
        Assert.Equal(next.BackingAllocationId, row.BackingAllocationId);
        Assert.Equal(f.Backing, Assert.Single(await f.Usage(10)).BackingAllocationId);
        Assert.Equal(2, (await f.History(f.Key(1))).Count);
    }

    [Fact]
    public async Task Resize_keeps_estimates_distinct_and_does_not_derive_bytes_from_dimensions()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        f.Commit((20, f.Resource(1, ResourceOperation.Resize) with
        {
            AllocationBytes = 8192, SizeProvenance = SizeProvenance.ApiDescriptionEstimate,
            Quality = new(MeasurementKind.Estimated, CoverageState.Partial, "API description only"), WidthPixels = 999, HeightPixels = 999
        }));
        var row = Assert.Single(await f.Usage(20));
        Assert.Equal(8192, row.Size.Integer);
        Assert.Equal(MeasurementKind.Estimated, row.Size.Quality.Kind);
        Assert.Equal(SizeProvenance.ApiDescriptionEstimate, row.Provenance);
    }

    [Theory]
    [InlineData(DeviceOperation.Lost)]
    [InlineData(DeviceOperation.Destroy)]
    public async Task Device_reset_invalidates_size_without_inventing_backing_destruction(DeviceOperation operation)
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, new DevicePayload(f.Device, operation, "D3D11", DataQuality.Complete)));
        var row = Assert.Single(await f.Usage(20));
        Assert.Equal(ValueKind.Missing, row.Size.Kind);
        Assert.Equal(SizeProvenance.Unknown, row.Provenance);
        Assert.Equal(f.Backing, row.BackingAllocationId);
        Assert.Single(await f.History(f.Key(1)));
        var next = f.Resource(1, ResourceOperation.Rebind) with { DeviceEpoch = Guid.NewGuid(), BackingAllocationId = Guid.NewGuid() };
        f.Commit((30, next));
        Assert.Equal(next.DeviceEpoch, Assert.Single(await f.Usage(30)).DeviceEpoch);
    }

    [Fact]
    public async Task Device_ownership_release_does_not_invalidate_observed_backing()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, new DevicePayload(f.Device, DeviceOperation.OwnershipRelease, "D3D11", ResourceFixture.Partial)));
        Assert.Equal(4096, Assert.Single(await f.Usage(20)).Size.Integer);
    }

    [Fact]
    public async Task Out_of_order_same_identity_uses_event_time_and_preserves_explicit_cause()
    {
        using var f = new ResourceFixture();
        var cause = new ChangeCause(CauseKind.ExplicitAttempt, new(f.Epoch, 1, Guid.NewGuid(), 1), null);
        f.Commit((30, f.Resource(1, ResourceOperation.Resize) with { AllocationBytes = 8192 }));
        f.Commit((10, f.Resource(1) with { Cause = cause }), (20, f.Resource(1, ResourceOperation.Resize)));
        Assert.Equal(8192, Assert.Single(await f.Usage(30)).Size.Integer);
        Assert.Equal(4096, Assert.Single(await f.Usage(10)).Size.Integer);
        var history = await f.History(f.Key(1));
        Assert.Equal(new long[] { 10, 20, 30 }, history.Select(r => r.TimestampQpc));
        Assert.Equal(cause, history[0].Cause);
        Assert.Equal(new long[] { 2, 3, 1 }, history.Select(r => r.IngestOrdinal));
    }

    [Fact]
    public async Task Destroy_received_before_birth_does_not_resurrect_resource()
    {
        using var f = new ResourceFixture();
        f.Commit((30, f.Resource(1, ResourceOperation.Destroy)));
        f.Commit((10, f.Resource(1)));
        Assert.Empty(await f.Usage(30));
        Assert.Single(await f.Usage(10));
    }

    [Fact]
    public void Late_new_row_identity_requiring_backdated_repair_is_rejected_explicitly()
    {
        using var f = new ResourceFixture();
        f.Commit((30, f.Resource(1, ResourceOperation.Rebind)));
        Assert.Throws<InvalidDataException>(() => f.Commit((10, f.Resource(1) with { BackingAllocationId = Guid.NewGuid() })));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public async Task Duplicate_facts_are_idempotent_and_conflicting_same_time_facts_fail()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        f.Commit((10, f.Resource(1)));
        Assert.Empty(f.LastChanges);
        Assert.Single(await f.History(f.Key(1)));
        Assert.Throws<InvalidDataException>(() => f.Commit((10, f.Resource(1) with { AllocationBytes = 1 })));
    }

    [Fact]
    public void Birth_identity_cannot_be_reused_after_destroy()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, f.Resource(1, ResourceOperation.Destroy)));
        Assert.Throws<InvalidDataException>(() => f.Commit((30, f.Resource(1))));
    }

    [Fact]
    public void Shared_backing_conflicting_simultaneous_actual_sizes_are_rejected()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        Assert.Throws<InvalidDataException>(() => f.Commit((10, f.Resource(2) with { AllocationBytes = 2 })));
    }

    [Fact]
    public async Task Loss_invalidates_carried_size_but_not_immutable_prior_observation()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (20, new LossPayload(f.Epoch, new(15, 20), 1, "lost resource transitions")));
        Assert.Equal(ValueKind.Missing, Assert.Single(await f.Usage(20)).Size.Kind);
        Assert.Equal(4096, Assert.Single(await f.Usage(10)).Size.Integer);
        Assert.Equal(4096, ((ResourceObservation)Assert.Single(await f.History(f.Key(1))).Observation).Size.Integer);
        f.Commit((30, f.Resource(1, ResourceOperation.Resize)));
        Assert.Equal(4096, Assert.Single(await f.Usage(30)).Size.Integer);
        Assert.Contains("continuity", Assert.Single(await f.Usage(30)).Quality.Reason);
    }

    [Fact]
    public async Task Health_invalidation_is_published_on_next_source_not_silently_evicted()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "gap"));
        f.Processor.Invalidate(health.Snapshot());
        f.Commit((20, new TypeResolvedPayload(f.Epoch, 1, "irrelevant")));
        Assert.Equal(ValueKind.Missing, Assert.Single(await f.Usage(20)).Size.Kind);
    }

    [Fact]
    public async Task Resource_correlations_preserve_both_endpoints_without_fabricating_size_or_birth()
    {
        using var f = new ResourceFixture();
        var correlation = new CorrelationPayload(f.Key(9, IdentityDomain.CompositionVisual), f.Key(1), CorrelationKind.VisualResource, true);
        f.Commit((10, correlation));
        var row = Assert.Single((await f.Store.Queries.GetCorrelations(new(f.Scope, f.Key(1), 10, new(128)), default)).Items);
        Assert.Equal(correlation.From, row.From);
        Assert.Equal(correlation.To, row.To);
        Assert.Equal(CorrelationKind.VisualResource, row.Kind);
        Assert.Empty(await f.Usage(10));
        Assert.IsType<CorrelationObservation>(Assert.Single(await f.History(f.Key(1))).Observation);
        f.Commit((20, correlation with { Linked = false }));
        Assert.Empty((await f.Store.Queries.GetCorrelations(new(f.Scope, f.Key(1), 20, new(128)), default)).Items);
    }

    [Fact]
    public async Task Raw_capture_never_publishes_and_verified_handoff_uses_only_current_End_source()
    {
        using var f = new ResourceFixture();
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id);
        f.Commit((10, chunk), (20, f.End(id)));
        Assert.Empty(await f.Usage(20));
        f.Publish(30, f.End(id), [chunk]);
        var row = Assert.Single(await f.Usage(30));
        Assert.Contains("Unknown birth", row.Quality.Reason);
        Assert.Equal(ResourceOperation.BaselinePresent, ((ResourceObservation)Assert.Single(await f.History(f.Key(1))).Observation).Operation);
        Assert.Equal(3, f.Store.Watermark.IngestOrdinal);
        Assert.All(f.LastChanges, c => Assert.Equal(3, c.SourceOrdinal));
    }

    [Fact]
    public void Verified_handoff_rejects_duplicate_resources_before_first_output()
    {
        using var f = new ResourceFixture();
        var id = Guid.NewGuid();
        var chunk = f.Chunk(id) with { Resources = [f.Resource(1), f.Resource(1)] };
        var source = f.Source(10, 1, f.End(id));
        using var rows = f.Processor.PublishBaseline(source, [chunk], f.Context(source)).GetEnumerator();
        Assert.Throws<ArgumentException>(() => rows.MoveNext());
    }

    [Fact]
    public void Retained_entries_and_bytes_stop_explicitly_without_evicting_prior_state()
    {
        using var f = new ResourceFixture(maximumEntries: 1);
        f.Commit((10, f.Resource(1)));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, f.Resource(2))));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
        var tiny = new ResourceUsageProcessor(65536, 512);
        var source = f.Source(20, 2, f.Resource(2) with { Classification = new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, new string('x', 2048)) });
        Assert.Throws<ProcessingLimitException>(() => tiny.Process([source], f.Context(source)).ToArray());
    }

    [Fact]
    public void Output_budget_is_incremental_and_enumeration_can_stop_after_first_candidate()
    {
        using var f = new ResourceFixture();
        var source = f.Source(10, 1, f.Resource(1));
        var context = f.Context(source);
        using var rows = f.Processor.Process([source], context).GetEnumerator();
        Assert.True(rows.MoveNext());
        context.OutputBudget.Consume(context.OutputBudget.RemainingBytes);
        Assert.Throws<ProcessingLimitException>(() => rows.MoveNext());
        using var second = new ResourceFixture();
        var other = second.Source(10, 1, second.Resource(1));
        Assert.Throws<ProcessingLimitException>(() => second.Processor.Process([other], second.Context(other) with { OutputBudget = new(1) }).ToArray());
    }

    [Fact]
    public async Task Persisted_reopen_and_pinned_watermark_do_not_depend_on_processor_or_mutable_inputs()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        var watermark = f.Store.Watermark;
        var first = Assert.Single(await f.Usage(10));
        f.Commit((20, f.Resource(1, ResourceOperation.Rebind) with { BackingAllocationId = Guid.NewGuid() }));
        using var reopened = f.Reopen();
        var pinned = await reopened.Queries.GetResourceUsage(new(f.Scope, new(10, 10), new(128, 0, watermark)), default);
        Assert.Equal(first, Assert.Single(pinned.Items));
        Assert.Equal(f.Backing, Assert.Single(pinned.Items).BackingAllocationId);
        var current = await reopened.Queries.GetResourceUsage(new(f.Scope, new(20, 20), new(128)), default);
        Assert.NotEqual(f.Backing, Assert.Single(current.Items).BackingAllocationId);
    }

    [Fact]
    public async Task Pending_health_and_device_loss_at_same_source_emit_one_revision_per_key()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "gap"));
        f.Processor.Invalidate(health.Snapshot());
        f.Commit((20, new DevicePayload(f.Device, DeviceOperation.Lost, "D3D11", ResourceFixture.Partial)));
        Assert.Equal(ValueKind.Missing, Assert.Single(await f.Usage(20)).Size.Kind);
    }

    [Fact]
    public void Late_identity_discovered_after_loss_cannot_escape_the_invalidation_revision()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (30, new LossPayload(f.Epoch, new(25, 30), 1, "gap")));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, f.Resource(1, ResourceOperation.Rebind) with { BackingAllocationId = Guid.NewGuid() })));
    }

    [Fact]
    public async Task Sizes_beyond_exact_integer_DTO_range_remain_missing_not_rounded_or_zero()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1) with { AllocationBytes = ulong.MaxValue }));
        var row = Assert.Single(await f.Usage(10));
        Assert.Equal(ValueKind.Missing, row.Size.Kind);
        Assert.Equal(SizeProvenance.Unknown, row.Provenance);
        Assert.Contains("exact signed", row.Size.Quality.Reason);
    }

    [Fact]
    public void One_owner_cannot_be_reused_across_process_instances_or_sessions()
    {
        using var first = new ResourceFixture();
        using var second = new ResourceFixture();
        first.Commit((10, first.Resource(1)));
        var source = second.Source(20, 1, second.Resource(1));
        Assert.Throws<ArgumentException>(() => first.Processor.Process([source], second.Context(source)).ToArray());
    }

    [Fact]
    public void An_old_verified_End_cannot_be_replayed_in_an_unrelated_writer_transaction()
    {
        using var f = new ResourceFixture();
        var id = Guid.NewGuid();
        var oldEnd = f.Source(10, 1, f.End(id));
        f.Commit((10, f.End(id)));
        var next = f.Source(20, 2, new TypeResolvedPayload(f.Epoch, 1, "next source"));
        var changes = f.Processor.PublishBaseline(oldEnd, [f.Chunk(id)], f.Context(next)).ToArray();
        Assert.Throws<ArgumentException>(() => f.Writer.Commit(new([next], changes, f.Info, ResourceFixture.Partial)));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public void Late_device_reset_never_invents_a_backdated_revision()
    {
        using var f = new ResourceFixture();
        f.Commit((30, f.Resource(1)));
        Assert.Throws<InvalidDataException>(() => f.Commit((20, new DevicePayload(f.Device, DeviceOperation.Lost, "D3D11", ResourceFixture.Partial))));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public void Long_running_single_resource_history_is_also_entry_bounded()
    {
        using var f = new ResourceFixture(maximumEntries: 2);
        f.Commit((10, f.Resource(1)), (20, f.Resource(1, ResourceOperation.Resize)));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((30, f.Resource(1, ResourceOperation.Resize))));
        Assert.Equal(2, f.Store.Watermark.IngestOrdinal);
    }

    [Fact]
    public async Task Verified_handoff_after_health_loss_invalidates_unstaged_resources_without_duplicate_revisions()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)), (11, f.Resource(2)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "gap"));
        f.Processor.Invalidate(health.Snapshot());
        var id = Guid.NewGuid();
        f.Publish(30, f.End(id), [f.Chunk(id)]);
        var rows = await f.Usage(30);
        Assert.Equal(4096, rows.Single(r => r.Resource == f.Key(1)).Size.Integer);
        Assert.Equal(ValueKind.Missing, rows.Single(r => r.Resource == f.Key(2)).Size.Kind);
    }

    [Fact]
    public async Task Duplicate_old_fact_after_health_loss_is_not_a_new_measurement()
    {
        using var f = new ResourceFixture();
        f.Commit((10, f.Resource(1)));
        var health = new SessionHealth();
        health.Report(new(IngestionIssue.SequenceGap, "gap"));
        f.Processor.Invalidate(health.Snapshot());
        f.Commit((10, f.Resource(1)));
        Assert.Equal(ValueKind.Missing, Assert.Single(await f.Usage(10)).Size.Kind);
        Assert.Single(await f.History(f.Key(1)));
    }

}

internal sealed class ResourceFixture : IDisposable
{
    internal static DataQuality Partial { get; } = new(MeasurementKind.Measured, CoverageState.Partial, "Presence is not a complete lifetime baseline.");
    internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, 123456, Guid.NewGuid()));
    internal Guid Epoch { get; } = Guid.NewGuid();
    internal Guid Device { get; } = Guid.NewGuid();
    internal Guid Backing { get; } = Guid.NewGuid();
    internal ObjectKey Owner => Key(9, IdentityDomain.XamlCore);
    internal ResourceUsageProcessor Processor { get; }
    internal SessionStore Store { get; }
    internal SessionInfo Info => info;
    internal SessionWriter Writer => writer ??= Store.CreateWriter();
    internal IReadOnlyList<ProjectionChange> LastChanges { get; private set; } = [];
    private readonly SessionInfo info = new(SessionMode.Offline, SessionStatus.Recording, 1_000_000, null, [], null);
    private readonly string path = Path.GetFullPath(Path.Combine("TestResults", "p10-" + Guid.NewGuid() + ".sqlite"));
    private SessionWriter? writer;
    internal ResourceFixture(int maximumEntries = 65536)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        Store = SessionStore.Create(path, Scope, info, Partial);
        Processor = new(maximumEntries);
    }
    internal ObjectKey Key(ulong id, IdentityDomain domain = IdentityDomain.Resource) => new(Scope.Process, domain, Epoch, id);
    internal ResourcePayload Resource(ulong id, ResourceOperation operation = ResourceOperation.Create) =>
        new(Key(id), operation, Device, Backing, 4096, SizeProvenance.ActualAllocation, Owner, 32, 32, Partial,
            new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "CreateTexture2D"));
    internal NormalizedEvent Source(long qpc, long ordinal, IEventPayload payload)
    {
        var identity = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() && d.Identity.ProviderId == EventCatalog.RuntimeProviderId).Identity;
        var epoch = payload is ResourcePayload p ? p.Resource.ProducerEpoch : Epoch;
        return EventCodec.Normalize(new(Scope.SessionId, Scope.Process, 42, 1, qpc, ordinal, (ulong)ordinal, null, identity,
            EventCodec.EncodePayload(payload, Scope.Process)), new(identity.ProviderId, 42, epoch));
    }
    internal ProcessingContext Context(NormalizedEvent source) =>
        new(Scope, new(source.Envelope.IngestOrdinal, Math.Max(Store.Watermark.TimestampQpc, source.Envelope.TimestampQpc)),
            Partial, new(SessionWriter.MaximumBatchBytes));
    internal void Commit(params (long Qpc, IEventPayload Payload)[] input)
    {
        var events = input.Select((item, i) => Source(item.Qpc, Store.Watermark.IngestOrdinal + i + 1, item.Payload)).ToArray();
        var context = Context(events.MaxBy(e => e.Envelope.TimestampQpc)!) with
        {
            BatchBoundary = new(events[^1].Envelope.IngestOrdinal, Math.Max(Store.Watermark.TimestampQpc, events.Max(e => e.Envelope.TimestampQpc)))
        };
        var changes = new List<ProjectionChange>();
        foreach (var row in Processor.Process(events, context))
        {
            changes.Add(row);
            context.OutputBudget.Consume(BatchAccounting.JsonBytes(row.Value, ProjectionBudget.MaximumRecordBytes) + BatchAccounting.ChangeHeaderBytes);
        }
        LastChanges = changes;
        (writer ??= Store.CreateWriter()).Commit(new(events, changes, info, Partial));
    }
    internal async Task<IReadOnlyList<ResourceUsageEntry>> Usage(long at, ObjectKey? owner = null) =>
        (await Store.Queries.GetResourceUsage(new(Scope, new(at, at), new(128), owner), default)).Items;
    internal async Task<IReadOnlyList<ObjectHistoryEntry>> History(ObjectKey key) =>
        (await Store.Queries.GetObjectHistory(new(Scope, key, new(0, 1000), new(128)), default)).Items;
    internal CaptureBoundaryPayload End(Guid id) => new(id, CaptureKind.Baseline, CapturePhase.End, null, null, [], 1, Partial);
    internal CaptureChunkPayload Chunk(Guid id) => new(id, 0, Guid.NewGuid(), 1, [], [], [], [], [Resource(1)], [], [], [], []);
    internal void Publish(long qpc, CaptureBoundaryPayload end, IReadOnlyList<CaptureChunkPayload> chunks)
    {
        var source = Source(qpc, Store.Watermark.IngestOrdinal + 1, end);
        var context = Context(source);
        var changes = new List<ProjectionChange>();
        foreach (var row in Processor.PublishBaseline(source, chunks, context))
        {
            changes.Add(row);
            context.OutputBudget.Consume(BatchAccounting.JsonBytes(row.Value, ProjectionBudget.MaximumRecordBytes) + BatchAccounting.ChangeHeaderBytes);
        }
        LastChanges = changes;
        (writer ??= Store.CreateWriter()).Commit(new([source], changes, info, Partial));
    }
    internal SessionStore Reopen() => SessionStore.OpenReadOnly(path);
    public void Dispose()
    {
        Store.Dispose();
        foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
            if (File.Exists(path + suffix)) File.Delete(path + suffix);
    }
}
