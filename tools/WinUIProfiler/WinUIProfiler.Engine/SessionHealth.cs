using System.Security.Cryptography;
using System.Text;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine;

public enum IngestionIssue
{
    UnknownSchema, DecodeFailure, EtwEventsLost, EtwBuffersLost, ProducerLoss, SequenceGap,
    QueueOverflow, PersistenceBacklog, WriterFailure, SourceFailure, IdentityPartial, MissingMetadata, DiagnosticLimit
}

public sealed record IngestionDiagnostic
{
    public IngestionIssue Issue { get; }
    public string Reason { get; }
    public Guid? ProviderId { get; }
    public string? EventName { get; }
    public uint? ProducerProcessId { get; }
    public long? TimestampQpc { get; }
    public string? PayloadHex { get; }
    public IngestionDiagnostic(IngestionIssue issue, string reason, Guid? providerId = null, string? eventName = null,
        uint? producerProcessId = null, long? timestampQpc = null, string? payloadHex = null)
    {
        Issue = Require.Enum(issue); Reason = Require.Text(reason);
        if (reason.Length > 1024 || eventName?.Length > 1024 || payloadHex?.Length > 512 ||
            payloadHex is not null && (payloadHex.Length % 2 != 0 || !payloadHex.All(Uri.IsHexDigit)) || timestampQpc < 0)
            throw new ArgumentException("Diagnostic exceeds its bounded metadata/sample contract.");
        ProviderId = providerId; EventName = eventName; ProducerProcessId = producerProcessId; TimestampQpc = timestampQpc; PayloadHex = payloadHex;
    }
}

public sealed record HealthCount(IngestionIssue Issue, long Count, IngestionDiagnostic First);
public sealed record DiagnosticSample(string Key, long Count, IngestionDiagnostic Diagnostic);
public sealed record HealthSnapshot
{
    public IReadOnlyList<HealthCount> Issues { get; }
    public IReadOnlyList<DiagnosticSample> Samples { get; }
    public long Version => Issues.Sum(row => row.Count);
    public long Count(IngestionIssue issue) => Issues.SingleOrDefault(row => row.Issue == issue)?.Count ?? 0;
    public bool HasContinuityFailure => Issues.Any(row => row.Issue is not (IngestionIssue.IdentityPartial or IngestionIssue.MissingMetadata));
    public HealthSnapshot(IReadOnlyList<HealthCount> issues, IReadOnlyList<DiagnosticSample>? samples = null)
    {
        if (issues.Count > Enum.GetValues<IngestionIssue>().Length || issues.Select(r => r.Issue).Distinct().Count() != issues.Count)
            throw new ArgumentException("Health snapshot must have at most one aggregate per issue.");
        foreach (var row in issues)
        {
            Require.Enum(row.Issue); Require.Positive(row.Count); ArgumentNullException.ThrowIfNull(row.First);
            if (row.Issue != row.First.Issue) throw new ArgumentException("Health sample has a foreign category.");
        }
        Issues = Array.AsReadOnly(issues.ToArray());
        samples ??= [];
        if (samples.Count > SessionHealth.MaximumDiagnosticSamples || samples.Select(s => s.Key).Distinct().Count() != samples.Count)
            throw new ArgumentException("Diagnostic representative sample bound exceeded.");
        foreach (var sample in samples)
        {
            if (sample.Key.Length != 64 || !sample.Key.All(Uri.IsHexDigit)) throw new ArgumentException("Invalid diagnostic fingerprint.");
            Require.Positive(sample.Count); ArgumentNullException.ThrowIfNull(sample.Diagnostic);
        }
        Samples = Array.AsReadOnly(samples.ToArray());
    }
}

// Reserved, fixed-size control path. Overflow cannot displace its first diagnostic or counters.
public sealed class SessionHealth
{
    public const int MaximumDiagnosticSamples = 128;
    private readonly HealthCount?[] slots = new HealthCount?[Enum.GetValues<IngestionIssue>().Length];
    private readonly Dictionary<string, DiagnosticSample> samples = [];
    private readonly object gate = new();
    public bool Report(IngestionDiagnostic diagnostic, long count = 1)
    {
        ArgumentNullException.ThrowIfNull(diagnostic); Require.Positive(count);
        var key = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes($"{diagnostic.Issue}\0{diagnostic.ProviderId}\0{diagnostic.EventName}\0{diagnostic.Reason}")));
        lock (gate)
        {
            var index = (int)diagnostic.Issue;
            var prior = slots[index];
            slots[index] = new(diagnostic.Issue, checked((prior?.Count ?? 0) + count), prior?.First ?? diagnostic);
            if (diagnostic.Issue is not (IngestionIssue.UnknownSchema or IngestionIssue.DecodeFailure)) return true;
            if (samples.TryGetValue(key, out var existing))
                samples[key] = existing with { Count = checked(existing.Count + count) };
            else if (samples.Count < MaximumDiagnosticSamples) samples.Add(key, new(key, count, diagnostic));
            else
            {
                var overflow = (int)IngestionIssue.DiagnosticLimit;
                var old = slots[overflow];
                slots[overflow] = new(IngestionIssue.DiagnosticLimit, checked((old?.Count ?? 0) + 1), old?.First ??
                    new(IngestionIssue.DiagnosticLimit, "Distinct diagnostic sample limit reached; acquisition stopped instead of evicting schema evidence.",
                        diagnostic.ProviderId, diagnostic.EventName, diagnostic.ProducerProcessId, diagnostic.TimestampQpc, diagnostic.PayloadHex));
                return false;
            }
            return true;
        }
    }
    public HealthSnapshot Snapshot() { lock (gate) return new(slots.OfType<HealthCount>().ToArray(), samples.Values.ToArray()); }
}
