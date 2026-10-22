using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Ingestion;

internal sealed class EtwLossTracker(ProcessingWorker worker)
{
    private readonly object gate = new();
    private long events;
    private long buffers;
    internal void Update(long lostEvents, long lostBuffers)
    {
        Require.Nonnegative(lostEvents); Require.Nonnegative(lostBuffers);
        lock (gate)
        {
            if (lostEvents > events) worker.Report(new(IngestionIssue.EtwEventsLost, "ETW reported transport event loss; delivery closure is invalid."), lostEvents - events);
            if (lostBuffers > buffers) worker.Report(new(IngestionIssue.EtwBuffersLost, "ETW reported lost buffers; affected observations are incomplete."), lostBuffers - buffers);
            events = Math.Max(events, lostEvents); buffers = Math.Max(buffers, lostBuffers);
        }
    }
}

internal static class EtwAcquisition
{
    internal static void Subscribe(ETWTraceEventSource source, EventDecoder decoder, ProcessingWorker worker)
    {
        bool IsForeign(TraceEvent data)
        {
            var pid = data.ProcessID > 0 ? (uint)data.ProcessID : 0;
            return decoder.IsProvablyForeignRuntime(data.ProviderGuid, pid, []) ||
                data.ProviderGuid == EventCatalog.RuntimeProviderId &&
                decoder.IsProvablyForeignRuntime(data.ProviderGuid, pid, data.EventData());
        }
        void Receive(TraceEvent data)
        {
            if (worker.StopRequested.IsCancellationRequested) return;
            try
            {
                if (IsForeign(data)) return;
                var native = NativeRecord.Capture(data);
                if (decoder.Decode(native) is { } decoded)
                {
                    if (decoded.Event.Envelope.Process.CreationTimeFileTime is null)
                        worker.Report(new(IngestionIssue.IdentityPartial, "Trace-local process identity has no creation evidence; it is not merged across loss/reuse."));
                    worker.TryEnqueue(decoded);
                }
            }
            catch (Exception error) when (error is ArgumentException or InvalidDataException or NotSupportedException or OverflowException or System.Text.Json.JsonException)
            {
                var issue = error is NotSupportedException ? IngestionIssue.UnknownSchema : IngestionIssue.DecodeFailure;
                var name = data.EventName.Length <= 1024 ? data.EventName : data.EventName[..1024];
                var raw = data.EventData();
                worker.Report(new(issue, error.GetType().Name + ": " + error.Message[..Math.Min(error.Message.Length, 768)],
                    data.ProviderGuid, name, data.ProcessID > 0 ? checked((uint)data.ProcessID) : null,
                    RawTimestamp(data), Convert.ToHexString(raw.AsSpan(0, Math.Min(raw.Length, 256)))));
            }
        }
        source.Dynamic.All += Receive;
        source.UnhandledEvents += data =>
        {
            if (worker.StopRequested.IsCancellationRequested) return;
            if (IsForeign(data)) return;
            var bytes = data.EventData();
            worker.Report(new(IngestionIssue.UnknownSchema, $"Unsupported ETW event: descriptor {(int)data.ID}, version {data.Version}, level {(int)data.Level}, keywords {(ulong)data.Keywords:X16}.",
                data.ProviderGuid, data.EventName[..Math.Min(data.EventName.Length, 1024)], data.ProcessID > 0 ? (uint)data.ProcessID : null,
                RawTimestamp(data), Convert.ToHexString(bytes.AsSpan(0, Math.Min(bytes.Length, 256)))));
        };
    }
    private static long? RawTimestamp(TraceEvent data)
    {
#pragma warning disable CS0618 // Diagnostic provenance is in the same raw clock as accepted observations.
        return data.TimeStampQPC >= 0 ? data.TimeStampQPC : null;
#pragma warning restore CS0618
    }
}

internal static class EtwSessionStatistics
{
    // Public Windows SDK EVENT_TRACE_PROPERTIES, queried only for this source's uniquely owned name.
    [StructLayout(LayoutKind.Sequential)]
    private struct Wnode
    {
        internal uint BufferSize, ProviderId;
        internal ulong HistoricalContext, Timestamp;
        internal Guid Guid;
        internal uint ClientContext, Flags;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct Properties
    {
        internal Wnode Wnode;
        internal uint BufferSize, MinimumBuffers, MaximumBuffers, MaximumFileSize, LogFileMode, FlushTimer, EnableFlags;
        internal int AgeLimit;
        internal uint NumberOfBuffers, FreeBuffers, EventsLost, BuffersWritten, LogBuffersLost, RealTimeBuffersLost;
        internal IntPtr LoggerThreadId;
        internal uint LogFileNameOffset, LoggerNameOffset;
    }
    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
    private static extern uint ControlTraceW(ulong handle, string sessionName, IntPtr properties, uint control);
    internal static (long Events, long Buffers) Query(string ownedSessionName) => Control(ownedSessionName, 0);
    internal static (long Events, long Buffers) Stop(string ownedSessionName) => Control(ownedSessionName, 1);
    private static (long Events, long Buffers) Control(string ownedSessionName, uint control)
    {
        const int bytes = 4096;
        var memory = Marshal.AllocHGlobal(bytes);
        try
        {
            Marshal.Copy(new byte[bytes], 0, memory, bytes);
            Marshal.StructureToPtr(new Properties { Wnode = new() { BufferSize = bytes }, LoggerNameOffset = (uint)Marshal.SizeOf<Properties>() }, memory, false);
            var status = ControlTraceW(0, ownedSessionName, memory, control);
            if (status != 0) throw new Win32Exception(checked((int)status), "Controlling owned ETW transport/loss failed.");
            var result = Marshal.PtrToStructure<Properties>(memory);
            return (result.EventsLost, (long)result.LogBuffersLost + result.RealTimeBuffersLost);
        }
        finally { Marshal.FreeHGlobal(memory); }
    }
}
