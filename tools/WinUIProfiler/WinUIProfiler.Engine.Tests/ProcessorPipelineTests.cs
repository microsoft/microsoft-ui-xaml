using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class ProcessorPipelineTests
{
    [Theory]
    [InlineData(true)]
    [InlineData(false)]
    public async Task Default_owner_persists_all_five_projections_and_composed_metrics_while_explicit_empty_stays_raw(bool useDefault)
    {
        using var f = new ProjectionFixture("ObjectStateProcessor");
        var core = f.Key(1);
        var visual = new ObjectKey(f.Scope.Process, IdentityDomain.CompositionVisual, f.Epoch, 1);
        var resource = new ObjectKey(f.Scope.Process, IdentityDomain.Resource, f.Epoch, 1);
        var attempt = new AttemptReference(f.Epoch, 1, f.Root, 1);
        var property = f.Property(core) with { Cause = new(CauseKind.ExplicitAttempt, attempt, null) };
        var inputs = new (long Qpc, IEventPayload Payload)[]
        {
            (1, new ProcessPayload(ProcessOperation.Start, 0, DataQuality.Complete)),
            (10, new ObjectLifetimePayload(core, LifetimeOperation.Birth, null, null, DataQuality.Complete,
                Classification: ProjectionFixture.Element)),
            (11, f.Edge(core)),
            (12, new TreeEdgePayload(null, visual, RelationshipKind.VisualRoot, 0, f.Root, 1, true,
                Membership: TreeMembershipKind.Composition)),
            (13, new VisualDeltaPayload(visual, VisualProperty.Opacity,
                new(ValueKind.Number, ValueUnit.Scalar, null, .5, null, null, DataQuality.Complete))),
            (14, new ResourcePayload(resource, ResourceOperation.Create, Guid.NewGuid(), Guid.NewGuid(), 4096,
                SizeProvenance.ActualAllocation, core, 32, 32, DataQuality.Complete,
                new(ResourceKind.Texture2D, ResourceKindEvidence.CreationApi, "Synthetic default-pipeline fixture"))),
            (15, new CorrelationPayload(visual, resource, CorrelationKind.VisualResource, true)),
            (16, new FramePayload(attempt, null, FrameOutcome.Attempt, null)),
            (17, property),
            (18, new FramePayload(attempt, 1, FrameOutcome.Submitted, 0)),
            (19, new FramePayload(attempt, null, FrameOutcome.AttemptCompleted, 0)),
            (21, new MetricPayload(MetricKind.ProcessPrivateWorkingSet,
                new(ValueKind.Integer, ValueUnit.Bytes, 4096, null, null, null, DataQuality.Complete),
                new(20, 21), "Synthetic default-pipeline fixture", "1", null))
        };
        await using var worker = new ProcessingWorker(f.Store, f.Info, processors: useDefault ? null : []);
        var collectorEpoch = Guid.NewGuid();
        foreach (var (qpc, payload) in inputs)
        {
            var provider = payload is ProcessPayload or MetricPayload ? EventCatalog.CollectorProviderId : EventCatalog.RuntimeProviderId;
            var identity = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() && d.Identity.ProviderId == provider).Identity;
            var stream = new ProducerStreamKey(provider, provider == EventCatalog.RuntimeProviderId ? 42u : 77u,
                provider == EventCatalog.RuntimeProviderId ? f.Epoch : collectorEpoch);
            var source = EventCodec.Normalize(new(f.Scope.SessionId, f.Scope.Process, stream.ProducerProcessId, 1,
                qpc, 1, null, null, identity, EventCodec.EncodePayload(payload, f.Scope.Process)), stream);
            Assert.True(worker.TryEnqueue(new(source, new(f.Scope.Process, qpc))));
        }
        worker.Complete(SourceCompletion.FiniteEnd);
        var result = await worker.Completion.WaitAsync(TimeSpan.FromSeconds(10));
        Assert.Equal(SessionStatus.Completed, result.Status);
        Assert.Equal(inputs.Length, result.DurableBoundary.IngestOrdinal);
        Assert.NotEqual(worker.OwnerThreadId, worker.WriterThreadId);
        var expected = useDefault ? 1 : 0;
        Assert.Equal(useDefault, (await f.History(core)).Any(h => h.Observation is LifetimeObservation));
        Assert.Equal(expected, (await f.Store.Queries.GetXamlTree(new(f.Scope, 21, new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.Store.Queries.GetCompositionTree(new(f.Scope, 21, new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.Store.Queries.GetResourceUsage(new(f.Scope, new(21, 21), new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.Store.Queries.GetFrames(new(f.Scope, new(0, 21), new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.Store.Queries.GetMetrics(new(f.Scope, new(0, 22), new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.Store.Queries.GetCorrelations(new(f.Scope, resource, 15, new(128)), default)).Items.Count);
        Assert.Equal(expected, (await f.History(resource)).Count(h => h.Observation is CorrelationObservation));
        Assert.All((await f.Store.Queries.GetXamlTree(new(f.Scope, 21, new(128)), default)).Items,
            row => Assert.NotEqual(CoverageState.Complete, row.Quality.Coverage));
    }
}
