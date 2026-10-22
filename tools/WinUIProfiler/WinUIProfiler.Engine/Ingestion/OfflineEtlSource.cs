using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Ingestion;

public sealed record EtlMetadata(long QpcFrequency, int ClockType, long EventsLost, long BuffersLost, long ByteLength);

public sealed class OfflineEtlSource(string path, SessionScope scope, long sessionFrequency)
{
    private int started;
    public static EtlMetadata ReadMetadata(string path)
    {
        using var lease = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
        using var source = new ETWTraceEventSource(path);
        EtlMetadata? metadata = null;
        source.Kernel.EventTraceHeader += header =>
        {
            var clock = header.ReservedFlags & 3;
            if (clock != 1 || header.PerfFreq <= 0) throw new NotSupportedException("Only recorded raw-QPC ETLs are supported; no current-machine clock is substituted.");
            metadata = new(header.PerfFreq, clock, unchecked((uint)header.EventsLost), unchecked((uint)header.BuffersLost), lease.Length);
            source.StopProcessing();
        };
        source.Process();
        return metadata ?? throw new InvalidDataException("ETL has no supported clock header.");
    }

    public Task RunAsync(ProcessingWorker worker, CancellationToken cancellationToken = default)
    {
        if (Interlocked.Exchange(ref started, 1) != 0) throw new InvalidOperationException("A source can only be enumerated once.");
        var lifetime = worker.RegisterAcquisition();
        return Task.Factory.StartNew(() =>
        {
            using var acquisition = lifetime;
            try
            {
                using var lease = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
                var metadata = ReadMetadata(path);
                using var source = new ETWTraceEventSource(path);
                var decoder = new EventDecoder(scope, new(metadata.QpcFrequency, sessionFrequency));
                var loss = new EtwLossTracker(worker);
                loss.Update(metadata.EventsLost, metadata.BuffersLost);
                EtwAcquisition.Subscribe(source, decoder, worker);
                using var linked = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, worker.StopRequested);
                using var registration = linked.Token.Register(source.StopProcessing);
                var finished = !linked.IsCancellationRequested && source.Process();
                loss.Update(unchecked((uint)source.EventsLost), metadata.BuffersLost);
                worker.Complete(finished && !linked.IsCancellationRequested ? SourceCompletion.FiniteEnd : SourceCompletion.Cancelled);
            }
            catch (Exception error)
            {
                worker.Report(new(IngestionIssue.SourceFailure, "Offline ETL enumeration failed: " + error.GetType().Name));
                worker.Complete(SourceCompletion.Failed);
                throw;
            }
        }, CancellationToken.None, TaskCreationOptions.LongRunning, TaskScheduler.Default);
    }
}
