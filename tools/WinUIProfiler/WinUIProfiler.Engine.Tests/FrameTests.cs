using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Storage;
using Xunit;
using Xunit.Abstractions;

namespace WinUIProfiler.Engine.Tests;

public sealed class FrameTests
{
    private readonly ITestOutputHelper output;

    public FrameTests(ITestOutputHelper output) => this.output = output;

    [Theory]
    [InlineData(32)]
    [InlineData(128)]
    [InlineData(512)]
    public void Outside_attempt_change_allocation_does_not_rebuild_completed_frame_history(int retainedAttempts)
    {
        using var f = new FramesFixture();
        var sources = new List<NormalizedEvent>();
        for (var i = 0; i < retainedAttempts; i++)
        {
            var attempt = f.Attempt((ulong)i + 1);
            var cause = new ChangeCause(CauseKind.ExplicitAttempt, attempt, null);
            var qpc = 10L * i + 1;
            foreach (var (timestamp, payload) in new (long, IEventPayload)[]
            {
                (qpc, f.Begin(attempt)), (qpc + 1, f.Property(cause)), (qpc + 2, f.End(attempt))
            })
                sources.Add(f.Source(timestamp, sources.Count + 1, payload));
        }
        var context = new ProcessingContext(f.Scope, new(sources.Count, 10L * retainedAttempts),
            DataQuality.Complete, new(SessionWriter.MaximumBatchBytes));
        foreach (var change in f.Processor.Process(sources, context))
            context.OutputBudget.Consume(BatchAccounting.JsonBytes(change.Value, ProjectionBudget.MaximumRecordBytes) + 16);

        // Warm generic/JIT paths before measuring only synchronous processor work.
        var outside = f.Property(new(CauseKind.OutsideAttempt, null, null));
        var ordinal = sources.Count;
        void AddOutside()
        {
            var source = f.Source(10L * retainedAttempts + ++ordinal, ordinal, outside);
            var batch = new ProcessingContext(f.Scope, new(ordinal, source.Envelope.TimestampQpc),
                DataQuality.Complete, new(SessionWriter.MaximumBatchBytes));
            using var changes = f.Processor.Process([source], batch).GetEnumerator();
            Assert.False(changes.MoveNext());
        }
        AddOutside();
        const int samples = 8;
        var beforeBytes = GC.GetAllocatedBytesForCurrentThread();
        var start = System.Diagnostics.Stopwatch.GetTimestamp();
        for (var i = 0; i < samples; i++) AddOutside();
        var elapsed = System.Diagnostics.Stopwatch.GetElapsedTime(start);
        var allocated = GC.GetAllocatedBytesForCurrentThread() - beforeBytes;
        output.WriteLine($"retainedAttempts={retainedAttempts}; retainedCausalChanges={retainedAttempts}; " +
            $"samples={samples}; allocatedBytes={allocated}; bytesPerOutside={allocated / samples}; " +
            $"elapsedTicks={elapsed.Ticks}; elapsedMilliseconds={elapsed.TotalMilliseconds:F3}");
        Assert.True(allocated / samples < 64 * 1024,
            $"OutsideAttempt allocated {allocated / samples} bytes/event with {retainedAttempts} completed attempts; " +
            "ordinary unassigned evidence must not reconstruct/serialize historical frame rows.");
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Post_startup_epoch_evidence_does_not_downgrade_completed_window(bool submittedAttempt)
    {
        using var f = new FramesFixture();
        CompleteStartup(f);
        var frozen = f.Store.Watermark;
        var expected = StartupCoverage(await f.Startup());
        Assert.Equal(11, expected.Length);
        Assert.All(expected, row => Assert.Equal(CoverageState.Complete, row.Quality.Coverage));
        Assert.Equal(new TimeRange(1, 101), Assert.Single(expected, row => row.Phase == "StartupWindow").Range);
        Assert.Equal(Enumerable.Range(1, 10).Select(i => (ulong?)i),
            expected.Where(row => row.SubmittedFrameId is not null).Select(row => row.SubmittedFrameId));
        CommitOtherEpoch(f, 110, submittedAttempt);

        Assert.Equal(expected, StartupCoverage(await f.Startup()));
        Assert.Equal(expected, StartupCoverage((await f.Store.Queries.GetStartupTimeline(
            new(f.Scope, new(128, 0, frozen)), default)).Items));
        using var reopened = f.Reopen();
        Assert.Equal(expected, StartupCoverage((await reopened.Queries.GetStartupTimeline(
            new(f.Scope, new(128)), default)).Items));
        Assert.Equal(expected, StartupCoverage((await reopened.Queries.GetStartupTimeline(
            new(f.Scope, new(128, 0, frozen)), default)).Items));
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Within_startup_epoch_evidence_remains_partial_without_rewriting_frozen_coverage(bool submittedAttempt)
    {
        using var f = new FramesFixture();
        CompleteStartup(f);
        var frozen = f.Store.Watermark;
        var expected = StartupCoverage(await f.Startup());
        Assert.All(expected, row => Assert.Equal(CoverageState.Complete, row.Quality.Coverage));
        CommitOtherEpoch(f, 55, submittedAttempt);

        var latest = StartupCoverage(await f.Startup());
        Assert.Equal(11, latest.Length);
        Assert.All(latest, row =>
        {
            Assert.Equal(CoverageState.Partial, row.Quality.Coverage);
            Assert.Contains("discontinuity", row.Quality.Reason);
        });
        Assert.Equal(expected, StartupCoverage((await f.Store.Queries.GetStartupTimeline(
            new(f.Scope, new(128, 0, frozen)), default)).Items));
        using var reopened = f.Reopen();
        Assert.Equal(latest, StartupCoverage((await reopened.Queries.GetStartupTimeline(
            new(f.Scope, new(128)), default)).Items));
        Assert.Equal(expected, StartupCoverage((await reopened.Queries.GetStartupTimeline(
            new(f.Scope, new(128, 0, frozen)), default)).Items));
    }

    private static StartupEntry[] StartupCoverage(IReadOnlyList<StartupEntry> rows) =>
        rows.Where(row => row.Phase == "StartupWindow" || row.Phase.StartsWith("StartupFrame:", StringComparison.Ordinal))
            .OrderBy(row => row.Phase, StringComparer.Ordinal).ToArray();

    private static void CompleteStartup(FramesFixture f)
    {
        f.Commit((1, f.Start(1)));
        for (ulong i = 1; i <= 10; i++)
        {
            var attempt = f.Attempt(i);
            f.Commit((10 * (long)i, f.Begin(attempt)), (10 * (long)i + 1, f.Submit(attempt, i)),
                (10 * (long)i + 2, f.End(attempt)));
        }
    }

    private static void CommitOtherEpoch(FramesFixture f, long qpc, bool submittedAttempt)
    {
        var epoch = Guid.NewGuid();
        var ordinal = f.Store.Watermark.IngestOrdinal + 1;
        var attempt = f.Attempt(1) with { ProducerEpoch = epoch };
        f.CommitSources(submittedAttempt
            ? [f.Source(qpc, ordinal, f.Begin(attempt), epoch),
                f.Source(qpc + 1, ordinal + 1, f.Submit(attempt, 1), epoch),
                f.Source(qpc + 2, ordinal + 2, f.End(attempt), epoch)]
            : [f.Source(qpc, ordinal, new FramePayload(null, null, FrameOutcome.StandaloneCommit, 0), epoch)]);
    }

    [Fact]
    public async Task Ten_physical_submissions_total_across_roots_and_shared_commit_participants_reopen()
    {
        using var f = new FramesFixture();
        f.Commit((1, f.Start(1)));
        for (ulong i = 1; i <= 11; i++)
        {
            var a = f.Attempt(i, i % 2 == 0 ? f.OtherRoot : f.Root);
            f.Commit((10 * (long)i, f.Begin(a)), (10 * (long)i + 1, f.Submit(a, i)),
                (10 * (long)i + 2, f.End(a)));
            if (i == 1)
            {
                var participant = f.Attempt(99, f.OtherRoot);
                f.Commit((10, f.Begin(participant)), (11, f.Submit(participant, 1)), (12, f.End(participant)));
            }
        }
        using var reopened = f.Reopen();
        var startup = (await reopened.Queries.GetStartupTimeline(new(f.Scope, new(128)), default)).Items;
        var frames = startup.Where(r => r.Phase.StartsWith("StartupFrame:", StringComparison.Ordinal)).ToArray();
        Assert.Equal(10, frames.Length);
        Assert.Equal(Enumerable.Range(1, 10).Select(i => (ulong?)i), frames.Select(r => r.SubmittedFrameId));
        var window = Assert.Single(startup, r => r.Phase == "StartupWindow");
        Assert.Equal(new TimeRange(1, 101), window.Range);
        Assert.Equal(CoverageState.Complete, window.Quality.Coverage);
        Assert.Equal(12, (await f.Frames()).Count);
    }

    [Fact]
    public async Task Fewer_than_ten_on_target_exit_remains_incomplete_without_manufactured_frames()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        f.Commit((1, f.Start(1)), (10, f.Begin(a)), (11, f.Submit(a, 1)), (12, f.End(a)),
            (30, new ProcessPayload(ProcessOperation.Exit, null, DataQuality.Complete)));
        var startup = await f.Startup();
        Assert.Single(startup, r => r.Phase.StartsWith("StartupFrame:", StringComparison.Ordinal));
        var window = Assert.Single(startup, r => r.Phase == "StartupWindow");
        Assert.Equal(CoverageState.Partial, window.Quality.Coverage);
        Assert.Contains("1/10", window.Quality.Reason);
        Assert.Contains("exit", window.Quality.Reason, StringComparison.OrdinalIgnoreCase);
        Assert.Single(await f.Frames());
    }

    [Theory]
    [InlineData(false)]
    [InlineData(true)]
    public async Task Attach_or_missing_exact_creation_qpc_is_unknown_not_startup(bool kernelEvidence)
    {
        using var f = new FramesFixture();
        var process = kernelEvidence ? f.KernelStart(5) : f.Source(5, 1,
            new ProcessPayload(ProcessOperation.Attach, 1, DataQuality.Complete));
        f.CommitSources([process]);
        var a = f.Attempt(1);
        f.Commit((10, f.Begin(a)), (11, f.Submit(a, 1)), (12, f.End(a)));
        Assert.DoesNotContain(await f.Startup(), r => r.Phase.StartsWith("StartupFrame:", StringComparison.Ordinal));
        Assert.Contains(await f.Startup(), r => r.Phase == "StartupUnknown" && r.Quality.Coverage == CoverageState.Partial);
    }

    [Fact]
    public async Task Ticks_no_device_standalone_and_failures_are_not_submitted_frames()
    {
        using var f = new FramesFixture();
        f.Commit((1, f.Start(1)));
        var idle = f.Attempt(1);
        var failed = f.Attempt(2);
        f.Commit((10, f.Begin(idle)), (11, f.End(idle)), (20, f.Begin(failed)),
            (21, new FramePayload(failed, null, FrameOutcome.Failed, -1)),
            (30, new FramePayload(null, null, FrameOutcome.StandaloneCommit, 0)),
            (40, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 1, null)),
            (42, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.End, 1, 0)));
        var frames = await f.Frames();
        Assert.Equal(2, frames.Count);
        Assert.All(frames, r => Assert.Null(r.Submission));
        Assert.Contains(frames, r => r.AttemptStatus == FrameAttemptStatus.Failed && r.HResult == -1);
        Assert.DoesNotContain(await f.Startup(), r => r.SubmittedFrameId is not null);
        Assert.Contains(await f.Startup(), r => r.Phase.StartsWith("Phase:LayoutMeasure:", StringComparison.Ordinal));
    }

    [Fact]
    public async Task Successful_submission_survives_later_attempt_failure_with_exact_identity_and_qpc()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(37);
        f.Commit((10, f.Begin(a)), (15, f.Submit(a, 999)),
            (20, new FramePayload(a, null, FrameOutcome.Failed, -7)));
        var frame = Assert.Single(await f.Frames());
        Assert.Equal(a, frame.Attempt);
        Assert.Equal(new TimeRange(10, 20), frame.Range);
        Assert.Equal(new FrameSubmission(999, 15, f.Epoch), frame.Submission);
        Assert.Equal(FrameAttemptStatus.Failed, frame.AttemptStatus);
    }

    [Fact]
    public async Task Exact_changes_and_overlapping_phases_remain_distinct_from_unknown_intervals()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        var b = f.Attempt(2, f.OtherRoot);
        var cause = new ChangeCause(CauseKind.ExplicitAttempt, a, null);
        f.Commit((10, f.Begin(a)), (11, f.Begin(b)),
            (12, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.Begin, 1, null, cause)),
            (13, new BoundaryPayload(BoundaryKind.LayoutArrange, BoundaryPhase.Begin, 2, null, cause)),
            (14, f.Property(cause)), (15, f.Property()),
            (16, new BoundaryPayload(BoundaryKind.LayoutMeasure, BoundaryPhase.End, 1, 0, cause)),
            (17, new BoundaryPayload(BoundaryKind.LayoutArrange, BoundaryPhase.End, 2, 0, cause)),
            (18, f.Submit(a, 1)), (19, f.End(a)), (20, f.End(b)));
        var frames = await f.Frames();
        var first = Assert.Single(frames, r => r.Attempt == a);
        var exact = Assert.Single(first.Changes, c => c.TimestampQpc == 14);
        Assert.Equal(5, exact.IngestOrdinal);
        Assert.Equal(FrameChangeRelation.Causal, exact.Relation);
        Assert.Equal(cause, exact.Cause);
        Assert.Equal(FrameChangeRelation.TemporalOverlap, Assert.Single(first.Changes, c => c.TimestampQpc == 15).Relation);
        Assert.DoesNotContain(Assert.Single(frames, r => r.Attempt == b).Changes, c => c.TimestampQpc == 14);
        var phases = (await f.Startup()).Where(r => r.Phase.StartsWith("Phase:", StringComparison.Ordinal)).ToArray();
        Assert.Contains(phases, p => p.Range == new TimeRange(12, 16));
        Assert.Contains(phases, p => p.Range == new TimeRange(13, 17));
    }

    [Fact]
    public async Task Changes_after_final_submission_and_before_late_begin_are_retained()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        var cause = new ChangeCause(CauseKind.ExplicitAttempt, a, null);
        f.Commit((15, f.Submit(a, 1)));
        f.Commit((18, f.Property(cause)), (19, f.Property()), (20, f.End(a)));
        f.Commit((10, f.Begin(a)), (12, f.Property()));
        var frame = Assert.Single(await f.Frames());
        Assert.Equal(new long[] { 12, 18, 19 }, frame.Changes.Select(c => c.TimestampQpc));
        Assert.Equal(new TimeRange(10, 20), frame.Range);
        Assert.Equal(15, frame.Submission!.SubmittedQpc);
        using var reopened = f.Reopen();
        Assert.Equal(3, Assert.Single((await reopened.Queries.GetFrames(new(f.Scope, new(0, 1000), new(128)), default)).Items).Changes.Count);
    }

    [Fact]
    public async Task Late_unknown_change_revises_all_overlapping_closed_attempts_and_loss_removes_each_link()
    {
        using var f = new FramesFixture();
        var first = f.Attempt(1);
        var second = f.Attempt(2, f.OtherRoot);
        f.Commit((10, f.Begin(first)), (20, f.Begin(second)), (30, f.End(first)), (40, f.End(second)));
        var frozen = f.Store.Watermark;
        f.Commit((25, f.Property()));
        var frames = await f.Frames();
        Assert.Equal(2, frames.Count);
        Assert.All(frames, frame =>
        {
            var change = Assert.Single(frame.Changes);
            Assert.Equal(25, change.TimestampQpc);
            Assert.Equal(5, change.IngestOrdinal);
            Assert.Equal(FrameChangeRelation.TemporalOverlap, change.Relation);
        });
        Assert.All((await f.Store.Queries.GetFrames(new(f.Scope, new(0, 1000), new(128, 0, frozen)), default)).Items,
            frame => Assert.Empty(frame.Changes));
        f.Commit((50, new LossPayload(f.Epoch, new(24, 26), 1, "late source gap")));
        Assert.All(await f.Frames(), frame => Assert.Empty(frame.Changes));
        f.Commit((26, f.Property(new(CauseKind.ExplicitAttempt, first, null))));
        using var reopened = f.Reopen();
        var persisted = (await reopened.Queries.GetFrames(new(f.Scope, new(0, 1000), new(128)), default)).Items;
        var exact = Assert.Single(Assert.Single(persisted, frame => frame.Attempt == first).Changes);
        Assert.Equal(FrameChangeRelation.Causal, exact.Relation);
        Assert.Equal(CoverageState.Partial, exact.Quality.Coverage);
        Assert.Empty(Assert.Single(persisted, frame => frame.Attempt == second).Changes);
    }

    [Fact]
    public async Task Event_time_startup_ranking_can_revise_tenth_without_leaving_an_eleventh_row()
    {
        using var f = new FramesFixture();
        f.Commit((1, f.Start(1)));
        for (ulong i = 1; i <= 10; i++)
        {
            var a = f.Attempt(i);
            f.Commit((100 + (long)i, f.Submit(a, i)));
        }
        f.Commit((50, f.Submit(f.Attempt(11), 11)));
        var startup = (await f.Startup()).Where(r => r.Phase.StartsWith("StartupFrame:", StringComparison.Ordinal)).ToArray();
        Assert.Equal(10, startup.Length);
        Assert.Equal(new ulong?[] { 11, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, startup.Select(r => r.SubmittedFrameId));
    }

    [Fact]
    public async Task Loss_invalidates_interval_links_but_not_exact_attempt_evidence_or_submission()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        f.Commit((1, f.Start(1)), (10, f.Begin(a)), (12, f.Property()),
            (13, f.Property(new(CauseKind.ExplicitAttempt, a, null))), (14, f.Submit(a, 1)), (20, f.End(a)));
        f.Commit((22, new LossPayload(f.Epoch, new(11, 15), 1, "gap")));
        var frame = Assert.Single(await f.Frames());
        Assert.DoesNotContain(frame.Changes, c => c.Relation == FrameChangeRelation.TemporalOverlap);
        Assert.Equal(CoverageState.Partial, Assert.Single(frame.Changes).Quality.Coverage);
        Assert.NotNull(frame.Submission);
        Assert.Equal(CoverageState.Partial, Assert.Single(await f.Startup(), r => r.Phase == "StartupWindow").Quality.Coverage);
    }

    [Fact]
    public async Task Epochs_and_explicit_outside_attempt_are_not_time_inferred_as_causal()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        var other = a with { ProducerEpoch = Guid.NewGuid() };
        f.Commit((10, f.Begin(a)), (12, f.Property(new(CauseKind.OutsideAttempt, null, null))), (20, f.End(a)));
        f.CommitSources([f.Source(10, 4, f.Begin(other), other.ProducerEpoch),
            f.Source(15, 5, f.Submit(other, 1), other.ProducerEpoch),
            f.Source(20, 6, f.End(other), other.ProducerEpoch)]);
        var frames = await f.Frames();
        Assert.Equal(2, frames.Count);
        Assert.All(frames, r => Assert.Empty(r.Changes));
    }

    [Fact]
    public async Task Health_invalidation_is_explicit_and_prevents_reusing_temporal_coverage()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        f.Commit((10, f.Begin(a)), (12, f.Property()), (20, f.End(a)));
        var diagnostic = new IngestionDiagnostic(IngestionIssue.SequenceGap, "missing source");
        f.Processor.Invalidate(new([new(IngestionIssue.SequenceGap, 1, diagnostic)]));
        f.Commit((25, f.Property(new(CauseKind.ExplicitAttempt, a, null))));
        var frame = Assert.Single(await f.Frames());
        Assert.Single(frame.Changes);
        Assert.Equal(CoverageState.Partial, frame.Changes[0].Quality.Coverage);
    }

    [Fact]
    public async Task Exact_post_terminal_changes_do_not_extend_the_interval_for_unknown_changes()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        f.Commit((10, f.Begin(a)), (15, f.Submit(a, 1)), (20, f.End(a)));
        f.Commit((22, f.Property()), (25, f.Property(new(CauseKind.ExplicitAttempt, a, null))));
        var frame = Assert.Single(await f.Frames());
        Assert.Equal(25, Assert.Single(frame.Changes).TimestampQpc);
        Assert.Equal(new TimeRange(10, 25), frame.Range);
    }

    [Fact]
    public void Conflicting_physical_commit_timestamps_are_rejected_even_without_startup_evidence()
    {
        using var f = new FramesFixture();
        f.Commit((10, f.Submit(f.Attempt(1), 7)));
        Assert.Throws<InvalidDataException>(() => f.Commit((11, f.Submit(f.Attempt(2, f.OtherRoot), 7))));
    }

    [Fact]
    public async Task Unchanged_frames_are_not_reemitted_and_metric_sources_are_delegated()
    {
        using var f = new FramesFixture();
        f.Commit((10, f.Begin(f.Attempt(1))), (20, f.End(f.Attempt(1))));
        var metric = new MetricPayload(MetricKind.ProcessPrivateCommit,
            new(ValueKind.Integer, ValueUnit.Bytes, 123, null, null, null, DataQuality.Complete),
            new(21, 22), "test", "1", null);
        var changes = f.Commit((22, metric));
        Assert.IsType<MetricSample>(Assert.Single(changes).Value);
        var samples = (await f.Store.Queries.GetMetrics(new(f.Scope, new(0, 100), new(128)), default)).Items;
        Assert.Equal(123, Assert.Single(samples).Value.Integer);
    }

    [Fact]
    public void Metric_only_capture_after_health_invalidation_needs_no_frame_or_startup_state()
    {
        using var f = new FramesFixture();
        f.Processor.Invalidate(new([new(IngestionIssue.SequenceGap, 1,
            new(IngestionIssue.SequenceGap, "before first frame"))]));
        var metric = new MetricPayload(MetricKind.ProcessPrivateCommit,
            new(ValueKind.Integer, ValueUnit.Bytes, 42, null, null, null, DataQuality.Complete),
            new(10, 10), "test", "1", null);
        Assert.IsType<MetricSample>(Assert.Single(f.Commit((10, metric))).Value);
    }

    [Fact]
    public void Unchanged_revisions_do_not_require_their_old_record_size_in_the_output_budget()
    {
        using var f = new FramesFixture();
        var a = f.Attempt(1);
        f.Commit((10, f.Begin(a)), (20, f.End(a)));
        var source = f.Source(30, 3, new FramePayload(null, null, FrameOutcome.StandaloneCommit, 0));
        Assert.Empty(f.Processor.Process([source], new(f.Scope, new(3, 30), DataQuality.Complete, new(1))));
    }

    [Fact]
    public void Entry_byte_and_output_limits_fail_explicitly_without_history_eviction()
    {
        using var f = new FramesFixture(1);
        f.Commit((10, f.Begin(f.Attempt(1))));
        Assert.Throws<ProcessingLimitException>(() => f.Commit((20, f.Begin(f.Attempt(2)))));
        Assert.Equal(1, f.Store.Watermark.IngestOrdinal);
        using var g = new FramesFixture();
        var source = g.Source(10, 1, g.Begin(g.Attempt(1)));
        using var output = g.Processor.Process([source],
            new(g.Scope, new(1, 10), DataQuality.Complete, new(1))).GetEnumerator();
        Assert.Throws<ProcessingLimitException>(() => output.MoveNext());
        using var h = new FramesFixture();
        var big = h.Property() with { Property = new(1, "UIElement", new string('p', 60000)) };
        h.Commit((1, big));
        Assert.Throws<ProcessingLimitException>(() =>
        {
            for (var i = 2; i < 400; i++) h.Commit((i, big));
        });
    }
}

internal sealed class FramesFixture : IDisposable
{
    internal SessionScope Scope { get; } = new(Guid.NewGuid(), new(42, 123456, Guid.NewGuid()));
    internal Guid Epoch { get; } = Guid.NewGuid();
    internal Guid Root { get; } = Guid.NewGuid();
    internal Guid OtherRoot { get; } = Guid.NewGuid();
    internal FrameProcessor Processor { get; }
    internal SessionStore Store { get; }
    private SessionWriter? writer;
    private readonly string path;
    private readonly SessionInfo info = new(SessionMode.Offline, SessionStatus.Recording, 1_000_000, null, [], null);

    internal FramesFixture(int maximumEntries = 65536)
    {
        path = Path.GetFullPath(Path.Combine("build", "p11-frame-validation", "sessions", Guid.NewGuid() + ".sqlite"));
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        Processor = new(maximumEntries);
        Store = SessionStore.Create(path, Scope, info, DataQuality.Complete);
    }

    internal AttemptReference Attempt(ulong id, Guid? root = null) => new(Epoch, id, root ?? Root, 17);
    internal FramePayload Begin(AttemptReference a) => new(a, null, FrameOutcome.Attempt, null);
    internal FramePayload Submit(AttemptReference a, ulong id) => new(a, id, FrameOutcome.Submitted, 0);
    internal FramePayload End(AttemptReference a) => new(a, null, FrameOutcome.AttemptCompleted, 0);
    internal ProcessPayload Start(long qpc) => new(ProcessOperation.Start, qpc, DataQuality.Complete);
    internal XamlPropertyChangedPayload Property(ChangeCause? cause = null) => new(
        new(Scope.Process, IdentityDomain.XamlCore, Epoch, 1), new(1, "UIElement", "Opacity"),
        new(ValueKind.Number, ValueUnit.Scalar, null, 1, null, null, DataQuality.Complete),
        new(ValueKind.Number, ValueUnit.Scalar, null, .5, null, null, DataQuality.Complete), cause);
    internal NormalizedEvent Source(long qpc, long ordinal, IEventPayload payload, Guid? epoch = null)
    {
        var identity = EventCatalog.All.Single(d => d.PayloadType == payload.GetType() &&
            (payload is LossPayload ? d.Identity.ProviderId == EventCatalog.RuntimeProviderId : true)).Identity;
        return EventCodec.Normalize(new(Scope.SessionId, Scope.Process, 42, 7, qpc, ordinal, null, null,
            identity, EventCodec.EncodePayload(payload, Scope.Process)), new(identity.ProviderId, 42, epoch ?? Epoch));
    }

    internal NormalizedEvent KernelStart(long qpc)
    {
        var identity = EventCatalog.Get(EventCatalog.KernelProcessProviderId, 1).Identity;
        return EventCodec.Normalize(new(Scope.SessionId, Scope.Process, 42, 7, qpc, 1, null, null,
            identity, EventCodec.EncodePayload(new ProcessPayload(ProcessOperation.Start, null,
                new(MeasurementKind.Measured, CoverageState.Partial, "No exact creation QPC.")), Scope.Process)),
            new(identity.ProviderId, 42, Scope.SessionId));
    }

    internal IReadOnlyList<ProjectionChange> Commit(params (long Qpc, IEventPayload Payload)[] events) =>
        CommitSources(events.Select((e, i) => Source(e.Qpc, Store.Watermark.IngestOrdinal + i + 1, e.Payload)).ToArray());
    internal IReadOnlyList<ProjectionChange> CommitSources(NormalizedEvent[] events)
    {
        var context = new ProcessingContext(Scope, new(events[^1].Envelope.IngestOrdinal,
            Math.Max(Store.Watermark.TimestampQpc, events.Max(e => e.Envelope.TimestampQpc))),
            DataQuality.Complete, new(SessionWriter.MaximumBatchBytes));
        var changes = new List<ProjectionChange>();
        foreach (var change in Processor.Process(events, context))
        {
            context.OutputBudget.Consume(BatchAccounting.JsonBytes(change.Value, ProjectionBudget.MaximumRecordBytes) + 16);
            changes.Add(change);
        }
        (writer ??= Store.CreateWriter()).Commit(new(events, changes, info, DataQuality.Complete));
        return changes;
    }

    internal async Task<IReadOnlyList<FrameInfo>> Frames() =>
        (await Store.Queries.GetFrames(new(Scope, new(0, 10000), new(128)), default)).Items;
    internal async Task<IReadOnlyList<StartupEntry>> Startup() =>
        (await Store.Queries.GetStartupTimeline(new(Scope, new(128)), default)).Items;
    internal SessionStore Reopen() => SessionStore.OpenReadOnly(path);
    public void Dispose()
    {
        Store.Dispose();
        foreach (var suffix in new[] { "", "-wal", "-shm", ".writer", ".failure.json" })
            if (File.Exists(path + suffix)) File.Delete(path + suffix);
    }
}
