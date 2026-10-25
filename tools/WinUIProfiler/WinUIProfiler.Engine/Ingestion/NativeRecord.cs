using System.Collections.ObjectModel;
using System.Text;
using Microsoft.Diagnostics.Tracing;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

// No TraceEvent, StructValue or recycled ETW memory escapes the callback.
public sealed class NativeRecord
{
    public const int MaximumWireBytes = 48 * 1024;
    public Guid ProviderId { get; }
    public string Name { get; }
    public byte Level { get; }
    public ulong Keywords { get; }
    public uint ProducerProcessId { get; }
    public uint ThreadId { get; }
    public long TimestampQpc { get; }
    public Guid? ActivityId { get; }
    public ushort DescriptorId { get; }
    public byte DescriptorVersion { get; }
    public byte Opcode { get; }
    public ushort Task { get; }
    public IReadOnlyDictionary<string, object> Fields { get; }
    public long RetainedBytes { get; }
    private readonly byte[] sample;
    private readonly byte[] rawPayload;
    public ReadOnlyMemory<byte> PayloadSample => sample.ToArray();
    public ReadOnlyMemory<byte> RawPayload => rawPayload.ToArray();

    public NativeRecord(Guid providerId, string name, byte level, ulong keywords, uint producerProcessId,
        uint threadId, long timestampQpc, Guid? activityId, IReadOnlyDictionary<string, object> fields, ReadOnlyMemory<byte> payloadSample = default,
        ushort descriptorId = 0, byte descriptorVersion = 0, byte opcode = 0, ushort task = 0)
    {
        ProviderId = Require.Id(providerId); Name = Require.Text(name);
        if (Encoding.UTF8.GetByteCount(name) > 1024) throw new InvalidDataException("Event name exceeds its bound.");
        Level = level; Keywords = keywords; ProducerProcessId = Require.Positive(producerProcessId);
        ThreadId = Require.Positive(threadId); TimestampQpc = Require.Nonnegative(timestampQpc);
        ActivityId = activityId; sample = payloadSample.Span[..Math.Min(payloadSample.Length, 256)].ToArray();
        if (payloadSample.Length > MaximumWireBytes) throw new InvalidDataException("Native wire data exceeds 48 KiB.");
        rawPayload = payloadSample.ToArray();
        DescriptorId = descriptorId; DescriptorVersion = descriptorVersion; Opcode = opcode; Task = task;
        long retained = 128L + rawPayload.Length + sample.Length + name.Length * 2L;
        var remaining = 65536;
        IReadOnlyDictionary<string, object> CopyMap(IReadOnlyDictionary<string, object> map, int depth)
        {
            retained = checked(retained + 64 + map.Count * 32L);
            return new ReadOnlyDictionary<string, object>(map.ToDictionary(p =>
            {
                retained = checked(retained + p.Key.Length * 2L);
                return p.Key;
            }, p => Copy(p.Value, depth + 1)));
        }
        object Copy(object value, int depth)
        {
            if (--remaining < 0 || depth > 12) throw new InvalidDataException("Wire shape exceeds its bound.");
            retained = checked(retained + 32);
            if (value is IDictionary<string, object> map)
            {
                if (map.Count > 128) throw new InvalidDataException("Wire field count exceeds its bound.");
                return CopyMap(new ReadOnlyDictionary<string, object>(map), depth);
            }
            if (value is IReadOnlyDictionary<string, object> readOnly)
                return CopyMap(readOnly, depth);
            if (value is Array array)
            {
                if (array.Length > 8192) throw new InvalidDataException("Wire array exceeds its bound.");
                retained = checked(retained + array.Length * 8L);
                return Array.AsReadOnly(array.Cast<object>().Select(item => Copy(item, depth + 1)).ToArray());
            }
            if (value is IReadOnlyList<object> list)
            {
                retained = checked(retained + list.Count * 8L);
                return Array.AsReadOnly(list.Select(item => Copy(item, depth + 1)).ToArray());
            }
            if (value is string text)
            {
                if (Encoding.UTF8.GetByteCount(text) > 1024 || text.Contains('\0'))
                    throw new InvalidDataException("Wire text exceeds its bound or contains embedded NUL.");
                retained = checked(retained + text.Length * 2L);
                return text;
            }
            return value is byte or short or ushort or int or uint or long or ulong or bool or double or Guid or DateTime
                ? value : throw new InvalidDataException("Unsupported wire primitive.");
        }
        Fields = (IReadOnlyDictionary<string, object>)Copy(fields, 0);
        RetainedBytes = retained;
    }

    public static NativeRecord Capture(TraceEvent data)
    {
        if (data.EventDataLength > MaximumWireBytes) throw new InvalidDataException("Native wire exceeds 48 KiB.");
        var fields = data.ProviderGuid == EventCatalog.StandardXamlProviderId ? new Dictionary<string, object>() :
            data.PayloadNames.Select((name, index) => (name, value: data.PayloadValue(index)))
                .ToDictionary(p => p.name, p => p.value);
#pragma warning disable CS0618 // Raw QPC is required to preserve native interval and source clock semantics.
        return new(data.ProviderGuid, data.EventName, (byte)data.Level, (ulong)data.Keywords,
            checked((uint)data.ProcessID), checked((uint)data.ThreadID), data.TimeStampQPC,
            data.ActivityID == Guid.Empty ? null : data.ActivityID, fields, data.EventData(), (ushort)data.ID,
            checked((byte)data.Version), checked((byte)data.Opcode), checked((ushort)data.Task));
#pragma warning restore CS0618
    }

    public NativeRecord WithFields(IReadOnlyDictionary<string, object> fields) =>
        new(ProviderId, Name, Level, Keywords, ProducerProcessId, ThreadId, TimestampQpc, ActivityId, fields, rawPayload, DescriptorId, DescriptorVersion, Opcode, Task);

    public ProcessInstanceKey ReadRuntimeTarget()
    {
        var target = new NativeFields(Fields).Struct("TargetProcess");
        return new(target.U32("ProcessId"), target.Bool("HasCreationTimeFileTime") ? target.I64("CreationTimeFileTime") : null,
            target.Guid("InstanceId"));
    }
}

internal sealed class NativeFields(IReadOnlyDictionary<string, object> fields)
{
    internal object Get(string name) => fields.TryGetValue(name, out var value) ? value : throw new InvalidDataException($"Missing wire field: {name}.");
    internal NativeFields Struct(string name) => Get(name) is IReadOnlyDictionary<string, object> map
        ? new(map) : throw new InvalidDataException($"Expected struct: {name}.");
    internal NativeFields[] Array(string name, int maximum = 4096)
    {
        if (Get(name) is not IReadOnlyList<object> values || values.Count > maximum)
            throw new InvalidDataException($"Invalid bounded array: {name}.");
        return values.Select(v => v is IReadOnlyDictionary<string, object> map
            ? new NativeFields(map) : throw new InvalidDataException($"Expected struct array: {name}.")).ToArray();
    }
    internal byte U8(string n) => Get(n) is byte value ? value : throw Type(n);
    internal ushort U16(string n) => Get(n) switch { int v when v is >= 0 and <= ushort.MaxValue => (ushort)v, ushort v => v, _ => throw Type(n) };
    internal uint U32(string n) => Get(n) switch { int v => unchecked((uint)v), uint v => v, _ => throw Type(n) };
    internal int I32(string n) => Get(n) is int v ? v : throw Type(n);
    internal long I64(string n) => Get(n) is long v ? v : throw Type(n);
    internal ulong U64(string n) => Get(n) switch { long v => unchecked((ulong)v), ulong v => v, _ => throw Type(n) };
    internal double F64(string n) => Get(n) is double v && double.IsFinite(v) ? v : throw Type(n);
    internal bool Bool(string n) => Get(n) is bool v ? v : throw Type(n);
    internal Guid Guid(string n) => Get(n) is Guid v ? v : throw Type(n);
    internal string Text(string n) => Get(n) is string v ? v : throw Type(n);
    internal T Enum<T>(string n) where T : struct, Enum => Require.Enum((T)System.Enum.ToObject(typeof(T), U8(n)));
    internal T? Optional<T>(string n, Func<NativeFields, string, T> read) where T : struct
    {
        var f = Struct(n);
        var present = f.Bool("Present");
        var value = read(f, "Value");
        return present ? value : null;
    }
    internal string? NullableText(string n)
    {
        var f = Struct(n); var present = f.Bool("Present"); var text = f.Text("Value");
        return present ? text : null;
    }
    private static InvalidDataException Type(string name) => new($"Invalid wire type/value for {name}.");
}

public sealed record QpcMapping(long SourceFrequency, long SessionFrequency, long OriginQpc = 0)
{
    public long Convert(long qpc)
    {
        Require.Positive(SourceFrequency); Require.Positive(SessionFrequency); Require.Nonnegative(OriginQpc);
        if (qpc < OriginQpc) throw new InvalidDataException("QPC precedes the selected origin.");
        return checked((long)((Int128)(qpc - OriginQpc) * SessionFrequency / SourceFrequency));
    }
}

public sealed record SourceClockMetadata(Guid SourceId, int ClockType, QpcMapping Mapping)
{
    public const int MaximumSources = 16;
    public void Validate()
    {
        Require.Id(SourceId); ArgumentNullException.ThrowIfNull(Mapping);
        if (ClockType != 1) throw new NotSupportedException("Only recorded raw-QPC clock mappings are supported.");
        _ = Mapping.Convert(Mapping.OriginQpc);
    }
}
public sealed record SourceProvenance(ProcessInstanceKey OriginalTarget, long OriginalTimestampQpc, SourceClockMetadata? Clock = null);
public sealed record DecodedEvent(NormalizedEvent Event, SourceProvenance Provenance);
