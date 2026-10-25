using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;

namespace WinUIProfiler.Engine.Processing;

public sealed class ProjectionBudget
{
    public const int MaximumRecordBytes = 64 * 1024;
    public long RemainingBytes { get; private set; }
    public int RemainingRecords { get; private set; }
    internal ProjectionBudget(long remainingBytes, int remainingRecords = Storage.SessionWriter.MaximumBatchRecords)
    {
        RemainingBytes = remainingBytes;
        RemainingRecords = remainingRecords;
    }
    internal void Consume(long bytes)
    {
        if (bytes > RemainingBytes || RemainingRecords == 0) throw new ProcessingLimitException();
        RemainingBytes -= bytes; RemainingRecords--;
    }
}
internal sealed class ProcessingLimitException : Exception;

internal static class BatchAccounting
{
    internal const int HeaderBytes = 32;
    internal const int ChangeHeaderBytes = 16;
    internal static long SourceBytes(NormalizedEvent source, SourceProvenance? provenance) =>
        EventCodec.Encode(source.Envelope).LongLength + (provenance is null ? 0 : JsonBytes(provenance, 4096));
    internal static long MetadataBytes(SessionInfo info, DataQuality quality, int limit) =>
        HeaderBytes + JsonBytes(info, limit) + JsonBytes(quality, limit);

    internal static long JsonBytes(object value, int limit)
    {
        if (limit <= 0) throw new ProcessingLimitException();
        using var sink = new CappedCounter(limit);
        JsonSerializer.Serialize(sink, value, value.GetType(), WireJson.Options);
        return sink.Bytes;
    }
    // Never retains a serialized oversized candidate or accumulates every output before checking.
    private sealed class CappedCounter(int limit) : Stream
    {
        internal long Bytes { get; private set; }
        public override void Write(byte[] buffer, int offset, int count) => Write(buffer.AsSpan(offset, count));
        public override void Write(ReadOnlySpan<byte> buffer)
        {
            if (buffer.Length > limit - Bytes) throw new ProcessingLimitException();
            Bytes += buffer.Length;
        }
        public override bool CanRead => false;
        public override bool CanSeek => false;
        public override bool CanWrite => true;
        public override long Length => Bytes;
        public override long Position { get => Bytes; set => throw new NotSupportedException(); }
        public override void Flush() { }
        public override int Read(byte[] buffer, int offset, int count) => throw new NotSupportedException();
        public override long Seek(long offset, SeekOrigin origin) => throw new NotSupportedException();
        public override void SetLength(long value) => throw new NotSupportedException();
    }
}
