using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Engine.Processing;

public sealed class BaselineProjectionPages
{
    public NormalizedEvent End { get; }
    public SessionInfo Info { get; }
    public DataQuality Quality { get; }
    public IReadOnlyList<IReadOnlyList<ProjectionChange>> Pages { get; }
    public SourceProvenance? Provenance { get; }
    public int RecordCount { get; }
    public long AccountedBytes { get; }

    internal BaselineProjectionPages(NormalizedEvent end, SessionInfo info, DataQuality quality,
        IReadOnlyList<IReadOnlyList<ProjectionChange>> pages, SourceProvenance? provenance, int recordCount, long accountedBytes)
    {
        End = end; Info = info; Quality = quality; Pages = pages; Provenance = provenance;
        RecordCount = recordCount; AccountedBytes = accountedBytes;
    }
}

public sealed class BaselinePagePreparer
{
    private readonly SessionScope scope;
    private readonly CommittedWatermark previous;
    private readonly IEventProcessor[] processors;
    private readonly int maximumRecords;
    private readonly long maximumBytes;
    private readonly int ownerThread = Environment.CurrentManagedThreadId;
    private bool attempted;

    public BaselinePagePreparer(SessionScope scope, CommittedWatermark previousBoundary,
        IReadOnlyList<IEventProcessor> processors, int maximumRecords = SessionWriter.MaximumBaselineRecords,
        long maximumBytes = SessionWriter.MaximumBaselineBytes)
    {
        scope.Validate(); Require.NotNull(processors);
        if (maximumRecords is < 1 or > SessionWriter.MaximumBaselineRecords ||
            maximumBytes is < 1 or > SessionWriter.MaximumBaselineBytes)
            throw new ArgumentOutOfRangeException(nameof(maximumRecords));
        if (processors.Count > 5 || processors.Any(p => p is null)) throw new ArgumentException("At most five owner processors are supported.");
        this.scope = scope; previous = previousBoundary;
        this.processors = processors.ToArray();
        this.maximumRecords = maximumRecords; this.maximumBytes = maximumBytes;
    }

    // Caller must supply the actual next-canonical validated End and already-reconciled owned chunks.
    // This is not reconciliation or publication permission. Any failure requires discarding these processors.
    public BaselineProjectionPages Prepare(NormalizedEvent validatedEnd, SessionInfo info, DataQuality quality,
        IReadOnlyList<CaptureChunkPayload> reconciledChunks, SourceProvenance? provenance = null,
        CancellationToken cancellationToken = default)
    {
        if (Environment.CurrentManagedThreadId != ownerThread) throw new InvalidOperationException("Preparation must execute synchronously on its state-owner thread.");
        if (attempted) throw new InvalidOperationException("A baseline preparation attempt is terminal; discard its processor state rather than retry.");
        attempted = true;
        cancellationToken.ThrowIfCancellationRequested();
        Require.NotNull(reconciledChunks);
        var header = SessionWriter.PrepareBaselineHeader(validatedEnd, scope, previous, info, quality, provenance);
        if (header.Bytes > maximumBytes) throw new ProcessingLimitException();
        var budget = new ProjectionBudget(maximumBytes - header.Bytes, maximumRecords);
        var context = new ProcessingContext(scope, header.Context.Watermark, quality, budget);
        ProjectionSupport.ValidatePublication(header.Source, reconciledChunks, context);
        var pages = new List<IReadOnlyList<ProjectionChange>>();
        var page = new List<ProjectionChange>();
        var keys = new HashSet<(string Dataset, string Key)>();
        long pageBytes = 0;
        foreach (var processor in processors)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var changes = processor is IVerifiedBaselineProcessor baseline
                ? baseline.PublishBaseline(header.Source, reconciledChunks, context)
                : processor.Process([header.Source], context);
            using var output = changes.GetEnumerator();
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();
                if (!output.MoveNext()) break;
                cancellationToken.ThrowIfCancellationRequested();
                var frozen = SessionWriter.FreezeBaselineChange(output.Current, header.Source, header.Context);
                budget.Consume(frozen.Bytes);
                if (!keys.Add(frozen.Key)) throw new ArgumentException("A baseline End may contain only one revision per projection key.");
                if (page.Count == SessionWriter.MaximumBatchRecords || pageBytes + frozen.Bytes > SessionWriter.MaximumBatchBytes)
                {
                    pages.Add(Array.AsReadOnly(page.ToArray()));
                    page.Clear(); pageBytes = 0;
                }
                page.Add(frozen.Change); pageBytes += frozen.Bytes;
            }
        }
        cancellationToken.ThrowIfCancellationRequested();
        if (page.Count != 0) pages.Add(Array.AsReadOnly(page.ToArray()));
        return new(header.Source, header.Info, quality, Array.AsReadOnly(pages.ToArray()), provenance,
            maximumRecords - budget.RemainingRecords, maximumBytes - budget.RemainingBytes);
    }
}
