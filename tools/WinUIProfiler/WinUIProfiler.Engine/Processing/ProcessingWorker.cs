using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Processing;

public enum SourceCompletion { FiniteEnd, UserStop, Cancelled, Failed }
public sealed record ProcessingOptions(int IngressItems = 4096, long IngressBytes = 8 * 1024 * 1024,
    int PersistenceItems = 8, long PersistenceBytes = 16 * 1024 * 1024,
    int BatchItems = 128, int BatchBytes = 4 * 1024 * 1024, int BacklogTimeoutMilliseconds = 5000)
{
    internal void Validate()
    {
        if (IngressItems is < 1 or > 65536 || IngressBytes is < 1024 or > 256L * 1024 * 1024 ||
            PersistenceItems is < 1 or > 128 || PersistenceBytes is < 1024 or > 256L * 1024 * 1024 ||
            BatchItems is < 1 or > SessionWriter.MaximumBatchRecords || BatchBytes is < 1024 or > SessionWriter.MaximumBatchBytes ||
            BacklogTimeoutMilliseconds is < 1 or > 60000)
            throw new ArgumentOutOfRangeException(nameof(ProcessingOptions));
    }
}
public sealed record ProcessingContext(SessionScope Scope, CommittedWatermark BatchBoundary, DataQuality Quality, ProjectionBudget OutputBudget);
public interface IEventProcessor
{
    // Yield one candidate at a time; the owner can dispose enumeration without requesting the rest.
    IEnumerable<ProjectionChange> Process(IReadOnlyList<NormalizedEvent> events, ProcessingContext context);
    void Invalidate(HealthSnapshot health);
}
public sealed record ProcessingResult(SessionStatus Status, string? StopReason, CommittedWatermark DurableBoundary,
    HealthSnapshot Health, long Accepted, long Processed, Exception? FailureRecordError);

public sealed class ProcessingWorker : IAsyncDisposable
{
    public const int MaximumAcceptedStreams = 128;
    private readonly SessionStore store;
    private SessionInfo info;
    private readonly ProcessingOptions options;
    private readonly IEventProcessor[] processors;
    private readonly BoundedQueue<DecodedEvent> ingress;
    private readonly BoundedQueue<SessionBatch> persistence;
    private readonly SessionHealth health = new();
    private readonly Dictionary<Guid, SourceClockMetadata> sourceClocks = [];
    private readonly Dictionary<ProducerStreamKey, bool> acceptedStreams = [];
    private readonly CancellationTokenSource stop = new();
    private readonly TaskCompletionSource<ProcessingResult> completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly TaskCompletionSource processingDrained = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly ManualResetEventSlim ownerDone = new();
    private readonly ManualResetEventSlim writerReady = new();
    private readonly ManualResetEventSlim acquisitionDone = new(true);
    private readonly object lifecycle = new();
    private readonly Thread ownerThread;
    private readonly Thread writerThread;
    private bool sealedIngress;
    private bool acquisitionActive;
    private bool healthFinalizing;
    private SessionStatus status = SessionStatus.Recording;
    private string? stopReason;
    private long accepted;
    private long processed;
    private volatile bool writerFailed;
    public CancellationToken StopRequested => stop.Token;
    public Task<ProcessingResult> Completion => completion.Task;
    public Task ProcessingDrained => processingDrained.Task;
    public int OwnerThreadId { get; private set; }
    public int WriterThreadId { get; private set; }
    public (int Items, long Bytes) IngressUsage => ingress.Usage;
    public (int Items, long Bytes) PersistenceUsage => persistence.Usage;

    public ProcessingWorker(SessionStore store, SessionInfo info, ProcessingOptions? options = null, IReadOnlyList<IEventProcessor>? processors = null)
    {
        this.store = store; this.options = options ?? new(); this.options.Validate();
        if (store.Watermark.IngestOrdinal != 0 || store.ReadIngestionDiagnostics().Count != 0)
            throw new InvalidOperationException("A fresh processing owner requires a new recording. Recovery needs explicit processor/continuity reconstruction, not blind resume.");
        this.info = QueryResultValidation.Freeze(info, new(store.Scope, store.Watermark, Partial("No complete runtime baseline or projection coverage has been established.")));
        if (info.Status != SessionStatus.Recording || !store.UnfinishedRecording) throw new InvalidOperationException("Worker needs an unfinished writable recording.");
        this.processors = processors?.ToArray() ??
            [new ObjectStateProcessor(), new FrameProcessor(), new XamlTreeProcessor(),
                new CompositionTreeProcessor(), new ResourceUsageProcessor()];
        if (this.processors.Length > 5 || this.processors.Any(p => p is null)) throw new ArgumentException("At most five owner-thread projections are supported.");
        ingress = new(this.options.IngressItems, this.options.IngressBytes);
        persistence = new(this.options.PersistenceItems, this.options.PersistenceBytes);
        ownerThread = new(OwnState) { IsBackground = true, Name = $"WinUIProfiler processor {store.Scope.SessionId:N}" };
        writerThread = new(Write) { IsBackground = true, Name = $"WinUIProfiler SQLite {store.Scope.SessionId:N}" };
        writerThread.Start(); ownerThread.Start();
    }

    public bool TryEnqueue(DecodedEvent source)
    {
        ArgumentNullException.ThrowIfNull(source);
        var normalized = EventCodec.Normalize(source.Event.Envelope, source.Event.Stream);
        if (normalized.Envelope.SessionId != store.Scope.SessionId || normalized.Envelope.Process != store.Scope.Process)
            throw new ArgumentException("Ingress event belongs to another session/process instance.");
        source.Provenance.OriginalTarget.Validate(); Require.Nonnegative(source.Provenance.OriginalTimestampQpc);
        if (source.Provenance.OriginalTarget.ProcessId != store.Scope.Process.ProcessId)
            throw new ArgumentException("Source provenance belongs to another target PID.");
        if (source.Provenance.Clock is { } clock &&
            (clock.Mapping.SessionFrequency != info.QpcFrequency || clock.Mapping.Convert(source.Provenance.OriginalTimestampQpc) != normalized.Envelope.TimestampQpc))
            throw new ArgumentException("Source timestamp and its original clock mapping disagree.");
        var captured = new DecodedEvent(normalized, source.Provenance);
        var bytes = BatchAccounting.SourceBytes(normalized, source.Provenance);
        lock (lifecycle)
        {
            if (sealedIngress) return false;
            if (source.Provenance.Clock is { } sourceClock) RegisterSourceClock(sourceClock);
            if (!acceptedStreams.ContainsKey(normalized.Stream) && acceptedStreams.Count >= MaximumAcceptedStreams)
            {
                StopAtLimit(IngestionIssue.SourceFailure, "Accepted stream/epoch metadata bound reached.");
                return false;
            }
            if (ingress.TryAdd(captured, bytes))
            {
                acceptedStreams.TryAdd(normalized.Stream, info.Producers.Any(p => p.ProviderId == normalized.Stream.ProviderId && p.Epoch == normalized.Stream.ProducerEpoch));
                Interlocked.Increment(ref accepted); return true;
            }
            health.Report(new(IngestionIssue.QueueOverflow, "Ingress item/byte bound reached; acquisition stopped, no older history was evicted."));
            Seal(SessionStatus.StoppedAtLimit, "Ingress item/byte bound reached.");
            return false;
        }
    }
    public void RegisterSourceClock(SourceClockMetadata clock)
    {
        clock.Validate();
        if (clock.Mapping.SessionFrequency != info.QpcFrequency) throw new ArgumentException("Source clock uses another normalized session frequency.");
        lock (lifecycle)
        {
            if (healthFinalizing) throw new InvalidOperationException("Source clock registration is finalized.");
            if (sourceClocks.TryGetValue(clock.SourceId, out var existing))
            {
                if (existing != clock) throw new ArgumentException("An original source-clock mapping is immutable.");
                return;
            }
            if (sourceClocks.Count >= SourceClockMetadata.MaximumSources)
            {
                StopAtLimit(IngestionIssue.SourceFailure, "Source-clock metadata bound reached.");
                throw new InvalidDataException("Source-clock metadata bound reached.");
            }
            sourceClocks.Add(clock.SourceId, clock);
        }
    }
    private IReadOnlyList<SourceClockMetadata> ClockSnapshot() { lock (lifecycle) return sourceClocks.Values.ToArray(); }
    public void Report(IngestionDiagnostic diagnostic, long count = 1)
    {
        lock (lifecycle)
        {
            if (healthFinalizing) throw new InvalidOperationException("Acquisition/processing are finalized; no diagnostic may be accepted after terminal persistence.");
            if (!health.Report(diagnostic, count))
                Seal(SessionStatus.StoppedAtLimit, "Distinct diagnostic limit reached; no schema evidence was evicted.");
        }
    }
    internal IDisposable RegisterAcquisition()
    {
        lock (lifecycle)
        {
            if (sealedIngress || acquisitionActive) throw new InvalidOperationException("A worker permits one active acquisition lifetime.");
            acquisitionActive = true; acquisitionDone.Reset(); return new AcquisitionLease(this);
        }
    }
    private sealed class AcquisitionLease(ProcessingWorker owner) : IDisposable
    {
        private ProcessingWorker? worker = owner;
        public void Dispose()
        {
            if (Interlocked.Exchange(ref worker, null) is { } w)
                lock (w.lifecycle) { w.acquisitionActive = false; w.acquisitionDone.Set(); }
        }
    }
    public void ReportSequenceAssessment(SequenceGapAssessment assessment)
    {
        if (assessment.State == SequenceContinuityState.GapDetected)
            Report(new(IngestionIssue.SequenceGap, Require.Text(assessment.Reason) + $" Missing sequences: {assessment.MissingObservations}."));
    }
    public SequenceGapAssessment ReconcileSequences(SequenceObservationWindow window, ListenerDomainInterval listener,
        ProducerSequenceCut? cut = null, SequenceDeliveryClosure? closure = null, SequenceReceiptEvidence? receipt = null)
    {
        if (listener.Domain.Target != store.Scope.Process || receipt?.ProcessedThroughOrdinal > Interlocked.Read(ref processed))
            throw new ArgumentException("Sequence receipt cannot claim foreign or not-yet-processed sources.");
        var result = SequenceContinuity.Assess(window, listener, cut, closure, receipt);
        ReportSequenceAssessment(result); return result;
    }
    public void Complete(SourceCompletion reason)
    {
        lock (lifecycle)
        {
            if (healthFinalizing) return;
            Seal(reason switch { SourceCompletion.FiniteEnd => SessionStatus.Completed, SourceCompletion.Failed => SessionStatus.Failed, _ => SessionStatus.Stopped },
                reason switch { SourceCompletion.FiniteEnd => "Finite source enumerated; no baseline/delivery closure is implied.", SourceCompletion.Failed => "Acquisition failed.", SourceCompletion.UserStop => "User stopped acquisition; delivery closure is unproven.", _ => "Acquisition cancelled; delivery closure is unproven." });
        }
    }
    private void Seal(SessionStatus terminal, string reason)
    {
        if (!sealedIngress || terminal == SessionStatus.Failed || terminal == SessionStatus.StoppedAtLimit && status != SessionStatus.Failed)
        {
            status = terminal; stopReason = reason;
        }
        sealedIngress = true;
        ingress.Complete();
        _ = stop.CancelAsync();
    }
    private void StopAtLimit(IngestionIssue issue, string reason)
    {
        Report(new(issue, reason));
        lock (lifecycle) Seal(SessionStatus.StoppedAtLimit, reason);
    }
    private void OwnState()
    {
        OwnerThreadId = Environment.CurrentManagedThreadId;
        writerReady.Wait();
        var ordinal = store.Watermark.IngestOrdinal;
        var timestamp = store.Watermark.TimestampQpc;
        long healthVersion = -1;
        try
        {
            DecodedEvent? pending = null;
            while (pending is not null || ingress.Take(out pending, out _))
            {
                if (writerFailed) break;
                var sources = new List<NormalizedEvent>();
                var provenance = new List<SourceProvenanceChange>();
                var batchInfo = info;
                var batchTimestamp = timestamp;
                var sourceBytes = 0L;
                var containsLoss = false;
                long batchBytes = 0;
                var quality = BatchQuality(false);
                do
                {
                    var candidate = pending!;
                    pending = null;
                    var e = candidate.Event.Envelope;
                    var source = EventCodec.Normalize(new(e.SessionId, e.Process, e.ProducerProcessId, e.ThreadId, e.TimestampQpc,
                        checked(ordinal + sources.Count + 1), e.SourceSequence, e.ActivityId, e.Identity, e.OwnedPayload), candidate.Event.Stream);
                    var nextInfo = IncludeCapabilities(batchInfo, source);
                    var nextLoss = containsLoss || IsLoss(source);
                    var nextQuality = BatchQuality(nextLoss);
                    var candidateBytes = BatchAccounting.SourceBytes(source, candidate.Provenance);
                    long totalBytes;
                    try { totalBytes = sourceBytes + candidateBytes + BatchAccounting.MetadataBytes(nextInfo, nextQuality, options.BatchBytes); }
                    catch (ProcessingLimitException) { totalBytes = options.BatchBytes + 1L; }
                    if (totalBytes > options.BatchBytes)
                    {
                        if (sources.Count == 0) throw new ProcessingLimitException();
                        pending = candidate;
                        break;
                    }
                    sources.Add(source);
                    provenance.Add(new(source.Envelope.IngestOrdinal, candidate.Provenance));
                    sourceBytes += candidateBytes; batchBytes = totalBytes;
                    batchInfo = nextInfo; containsLoss = nextLoss; quality = nextQuality;
                    batchTimestamp = Math.Max(batchTimestamp, source.Envelope.TimestampQpc);
                    if (sources.Count >= options.BatchItems) break;
                } while (ingress.Take(out pending, out _, false));

                if (containsLoss)
                    Report(new(IngestionIssue.ProducerLoss, "Producer explicitly reported loss; affected projections require a new baseline."), sources.LongCount(IsLoss));
                var snapshot = health.Snapshot();
                if (snapshot.Version != healthVersion)
                {
                    foreach (var processor in processors) processor.Invalidate(snapshot);
                    healthVersion = snapshot.Version;
                }
                var budget = new ProjectionBudget(options.BatchBytes - batchBytes);
                var context = new ProcessingContext(store.Scope, new(ordinal + sources.Count, batchTimestamp), quality, budget);
                var changes = new List<ProjectionChange>();
                var readOnlySources = sources.AsReadOnly();
                foreach (var processor in processors)
                {
                    if (budget.RemainingBytes <= BatchAccounting.ChangeHeaderBytes || budget.RemainingRecords == 0)
                        throw new ProcessingLimitException();
                    using var produced = processor.Process(readOnlySources, context).GetEnumerator();
                    while (true)
                    {
                        if (budget.RemainingBytes <= BatchAccounting.ChangeHeaderBytes || budget.RemainingRecords == 0)
                            throw new ProcessingLimitException();
                        if (!produced.MoveNext()) break;
                        var change = produced.Current;
                        if (change.SourceOrdinal < ordinal + 1 || change.SourceOrdinal > context.BatchBoundary.IngestOrdinal)
                            throw new InvalidDataException("Projection change belongs to another source batch.");
                        var cap = (int)Math.Min(ProjectionBudget.MaximumRecordBytes, budget.RemainingBytes - BatchAccounting.ChangeHeaderBytes);
                        _ = BatchAccounting.JsonBytes(change.Value, cap);
                        var frozen = SessionWriter.Freeze(change.Value, new(store.Scope, context.BatchBoundary, quality));
                        var bytes = BatchAccounting.JsonBytes(frozen, cap) + BatchAccounting.ChangeHeaderBytes;
                        budget.Consume(bytes);
                        batchBytes += bytes;
                        changes.Add(new(change.SourceOrdinal, frozen, change.Deleted));
                    }
                }
                var batch = new SessionBatch(readOnlySources, changes.AsReadOnly(), batchInfo, quality, Provenance: provenance.AsReadOnly());
                if (!persistence.Add(batch, batchBytes, TimeSpan.FromMilliseconds(options.BacklogTimeoutMilliseconds)))
                {
                    StopAtLimit(IngestionIssue.PersistenceBacklog, "Bounded persistence backlog exceeded its size/deadline; uncommitted tail is incomplete.");
                    break;
                }
                ordinal += sources.Count; timestamp = batchTimestamp; info = batchInfo;
                lock (lifecycle)
                    foreach (var source in sources)
                        if (source.Envelope.Identity.ProviderId == EventCatalog.RuntimeProviderId && source.Envelope.Identity.EventId == 1)
                            acceptedStreams[source.Stream] = true;
                Interlocked.Add(ref processed, sources.Count);
            }
        }
        catch (ProcessingLimitException)
        {
            StopAtLimit(IngestionIssue.PersistenceBacklog, "Owner input/metadata or incremental projection budget exceeded; uncommitted source batch is incomplete.");
        }
        catch (Exception error)
        {
            Report(new(IngestionIssue.SourceFailure, "Processing failed: " + error.GetType().Name));
            lock (lifecycle) Seal(SessionStatus.Failed, "Processing failed: " + error.GetType().Name);
        }
        finally
        {
            processingDrained.TrySetResult();
            acquisitionDone.Wait();
            try
            {
                ProducerStreamKey[] missing;
                lock (lifecycle) missing = acceptedStreams.Where(pair => !pair.Value).Select(pair => pair.Key).ToArray();
                foreach (var stream in missing)
                    Report(new(IngestionIssue.MissingMetadata, $"No capabilities/build metadata for accepted producer epoch {stream.ProducerEpoch:N}; independently enabled observations remain accepted.",
                        stream.ProviderId, "ProducerEpoch/" + stream.ProducerEpoch.ToString("N"), stream.ProducerProcessId));
                if (acceptedStreams.Count == 0 && info.Producers.Count == 0)
                    Report(new(IngestionIssue.MissingMetadata, "No runtime capabilities/build metadata was received; independently enabled observations remain accepted."));
                var finalHealth = health.Snapshot();
                if (finalHealth.Version != healthVersion)
                    foreach (var processor in processors) processor.Invalidate(finalHealth);
            }
            catch (Exception error)
            {
                Report(new(IngestionIssue.SourceFailure, "Final invalidation failed: " + error.GetType().Name));
                lock (lifecycle) Seal(SessionStatus.Failed, "Final invalidation failed.");
            }
            finally { persistence.Complete(); ownerDone.Set(); }
        }
    }
    private DataQuality BatchQuality(bool containsLoss) => Partial(containsLoss || health.Snapshot().HasContinuityFailure
        ? "Ingestion discontinuity; projections invalid until a proven baseline." : "Runtime observational/baseline coverage is not established.");
    private static bool IsLoss(NormalizedEvent source) =>
        EventCatalog.Get(source.Envelope.Identity.ProviderId, source.Envelope.Identity.EventId).PayloadType == typeof(LossPayload);
    private static SessionInfo IncludeCapabilities(SessionInfo metadata, NormalizedEvent source)
    {
        if (source.Envelope.Identity.ProviderId != EventCatalog.RuntimeProviderId || source.Envelope.Identity.EventId != 1) return metadata;
        var capabilities = EventCodec.ReadPayload<RuntimeCapabilitiesPayload>(source.Envelope);
        var producers = metadata.Producers.Where(p => p.ProviderId != source.Stream.ProviderId || p.Epoch != source.Stream.ProducerEpoch).ToList();
        if (producers.Count >= 128) throw new ProcessingLimitException();
        producers.Add(new(source.Stream.ProviderId, source.Stream.ProducerEpoch, 1, capabilities.Build, capabilities.Architecture, capabilities.Capabilities));
        return metadata with { Producers = producers.AsReadOnly() };
    }
    private void Write()
    {
        WriterThreadId = Environment.CurrentManagedThreadId;
        var durable = store.Watermark;
        Exception? failureRecordError = null;
        try
        {
            var writer = store.CreateWriter();
            writerReady.Set();
            while (persistence.Take(out var batch, out _))
            {
                durable = writer.Commit(batch);
                writer.CommitHealth(health.Snapshot(), clocks: ClockSnapshot());
            }
            ownerDone.Wait();
            acquisitionDone.Wait();
            lock (lifecycle) healthFinalizing = true;
            var snapshot = health.Snapshot();
            lock (lifecycle)
            {
                if (status == SessionStatus.Completed && snapshot.HasContinuityFailure)
                { status = SessionStatus.Stopped; stopReason = "Finite source ended with ingestion loss/schema failures; recording is incomplete."; }
            }
            var terminal = info with { Status = status, StopReason = stopReason };
            writer.CommitHealth(snapshot, terminal, Partial(stopReason ?? "Observation coverage is not established."), ClockSnapshot());
        }
        catch (Exception error)
        {
            writerFailed = true;
            health.Report(new(IngestionIssue.WriterFailure, "Persistence failed: " + error.GetType().Name));
            lock (lifecycle) Seal(SessionStatus.Failed, error is SessionWriteException { SqliteErrorCode: 13 }
                ? "Storage limit/disk full (SQLITE_FULL); last durable boundary is preserved."
                : "Persistence failed; last durable boundary is preserved.");
            persistence.Complete();
            failureRecordError = error is SessionWriteException write ? write.FailureRecordError : store.RecordFailure(durable, error);
        }
        finally
        {
            writerReady.Set();
            ownerDone.Wait();
            acquisitionDone.Wait();
            lock (lifecycle) healthFinalizing = true;
            completion.TrySetResult(new(status, stopReason, durable, health.Snapshot(), Interlocked.Read(ref accepted), Interlocked.Read(ref processed), failureRecordError));
        }
    }
    private static DataQuality Partial(string reason) => new(MeasurementKind.Measured, CoverageState.Partial, reason);
    public async ValueTask DisposeAsync()
    {
        if (!Completion.IsCompleted) Complete(SourceCompletion.UserStop);
        await Completion.ConfigureAwait(false);
        ownerThread.Join(); writerThread.Join();
        stop.Dispose(); ownerDone.Dispose(); writerReady.Dispose(); acquisitionDone.Dispose();
    }
}
