using System.Buffers.Binary;
using System.Text;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Ingestion;

// Decoder layouts from event-contract.md, not an ETW producer or a fixture encoder.
internal static class NativeSchema
{
    private sealed record Node(char Kind, (string Name, Node Type)[]? Fields = null, Node? Element = null, int Maximum = 0);
    private static readonly Node U8 = new('b'), U16 = new('h'), U32 = new('u'), I32 = new('i'), I64 = new('l'),
        U64 = new('q'), F64 = new('d'), Bool = new('B'), Guid = new('g'), Text = new('s');
    private static Node S(params (string Name, Node Type)[] fields) => new('S', fields);
    private static Node A(Node element, int maximum = 4096) => new('A', Element: element, Maximum: maximum);
    private static Node O(Node value) => S(("Present", Bool), ("Value", value));
    private static readonly Node NullableText = O(Text);
    private static readonly Node Quality = S(("Kind", U8), ("Coverage", U8), ("Reason", NullableText));
    private static readonly Node Classification = S(("Kind", U8), ("Evidence", U8), ("Source", Text));
    private static readonly Node Object = S(("Domain", U8), ("ProducerEpoch", Guid), ("RuntimeObjectId", U64));
    private static readonly Node Attempt = S(("ProducerEpoch", Guid), ("AttemptId", U64), ("RootId", Guid), ("DispatcherId", U64));
    private static readonly Node Cause = S(("Kind", U8), ("Attempt", O(Attempt)), ("Reason", NullableText));
    private static readonly Node Value = S(("Kind", U8), ("Unit", U8), ("Integer", I64), ("Number", F64), ("Boolean", Bool), ("Text", Text), ("Quality", Quality));
    private static readonly Node Domain = S(("EventId", U16), ("EventVersion", U8));
    private static readonly Node Range = S(("Domain", Domain), ("BeforeSequence", O(U64)), ("ThroughSequence", O(U64)), ("Quality", Quality),
        ("SettledCut", O(S(("CutId", Guid), ("ReservedThrough", U64), ("SettledThrough", U64)))));
    private static readonly Node[] Bodies =
    [
        S(),
        S(("Epoch", Guid), ("Build", Text), ("Architecture", U32), ("QpcFrequency", I64), ("CapabilityCount", U32),
            ("Capabilities", A(S(("Capability", U8), ("Version", I32), ("Quality", Quality)), 128))),
        S(("Object", Object), ("Operation", U8), ("TypeId", O(U64)), ("Address", O(U64)), ("Quality", Quality), ("Classification", O(Classification)), ("Cause", Cause)),
        S(("TypeId", U64), ("Name", Text), ("Classification", O(Classification))),
        S(("Core", Object), ("Peer", Object), ("Linked", Bool), ("Cause", Cause)),
        S(("Parent", O(Object)), ("Child", Object), ("Relationship", U8), ("Order", I32), ("RootId", Guid), ("DispatcherId", U64), ("Connected", Bool), ("Cause", Cause), ("Membership", U8)),
        S(("Visual", Object), ("Property", U8), ("Value", Value), ("Cause", Cause)),
        S(("From", Object), ("To", Object), ("Kind", U8), ("Linked", Bool), ("Cause", Cause)),
        S(("Kind", U8), ("Phase", U8), ("OperationId", U64), ("HResult", O(I32)), ("Cause", Cause)),
        S(("Attempt", O(Attempt)), ("SubmittedFrameId", O(U64)), ("Outcome", U8), ("HResult", O(I32))),
        S(("DeviceEpoch", Guid), ("Operation", U8), ("Api", Text), ("Quality", Quality)),
        S(("Resource", Object), ("Operation", U8), ("DeviceEpoch", Guid), ("BackingAllocationId", O(Guid)), ("AllocationBytes", O(U64)),
            ("SizeProvenance", U8), ("Owner", O(Object)), ("WidthPixels", O(U32)), ("HeightPixels", O(U32)), ("Quality", Quality), ("Classification", Classification), ("Cause", Cause)),
        S(("AllocationId", U64), ("Operation", U8), ("PreviousAllocationId", O(U64)), ("RequestedBytes", U64), ("ActualBytes", O(U64)),
            ("AllocatorEpoch", Guid), ("BaselineCaptureId", O(Guid)), ("Quality", Quality), ("Cause", Cause)),
        S(("CaptureId", Guid), ("Kind", U8), ("Phase", U8), ("RootId", O(Guid)), ("DispatcherId", O(U64)), ("Domains", A(Range, 256)), ("ChunkCount", O(U32)), ("Quality", Quality)),
        S(("CaptureId", Guid), ("ChunkIndex", U32), ("RootId", Guid), ("DispatcherId", U64),
            ("Objects", A(S(("KeyIndex", U32), ("HasTypeName", Bool), ("TypeName", Text), ("HasAddress", Bool), ("Address", U64), ("SizeIndex", U32), ("ClassificationIndex", U32)))),
            ("Ownership", A(S(("FromIndex", U32), ("ToIndex", U32), ("Relationship", Text), ("Evidence", U8), ("QualityIndex", U32)), 8192)),
            ("Tree", A(S(("ParentIndex", U32), ("ChildIndex", U32), ("Relationship", U8), ("Order", I32), ("Connected", Bool), ("CauseIndex", U32), ("Membership", U8)))),
            ("Correlations", A(S(("FromIndex", U32), ("ToIndex", U32), ("Kind", U8), ("Linked", Bool), ("CauseIndex", U32)))),
            ("Resources", A(S(("ResourceIndex", U32), ("Operation", U8), ("DeviceEpoch", Guid), ("HasBackingAllocationId", Bool), ("BackingAllocationId", Guid),
                ("HasAllocationBytes", Bool), ("AllocationBytes", U64), ("SizeProvenance", U8), ("OwnerIndex", U32), ("HasWidthPixels", Bool), ("WidthPixels", U32),
                ("HasHeightPixels", Bool), ("HeightPixels", U32), ("QualityIndex", U32), ("ClassificationIndex", U32), ("CauseIndex", U32)))),
            ("Visuals", A(S(("VisualIndex", U32), ("Property", U8), ("ValueIndex", U32), ("CauseIndex", U32)))),
            ("Associations", A(S(("CoreIndex", U32), ("PeerIndex", U32), ("Linked", Bool), ("CauseIndex", U32)))),
            ("Types", A(S(("TypeId", U64), ("Name", Text), ("ClassificationIndex", U32)))),
            ("Allocations", A(S(("AllocationId", U64), ("Operation", U8), ("HasPreviousAllocationId", Bool), ("PreviousAllocationId", U64),
                ("RequestedBytes", U64), ("HasActualBytes", Bool), ("ActualBytes", U64), ("AllocatorEpoch", Guid), ("HasBaselineCaptureId", Bool), ("BaselineCaptureId", Guid),
                ("QualityIndex", U32), ("CauseIndex", U32)))),
            ("Keys", A(S(("Domain", U8), ("RuntimeObjectId", U64)))),
            ("Values", A(S(("Kind", U8), ("Unit", U8), ("Integer", I64), ("Number", F64), ("Boolean", Bool), ("Text", Text), ("QualityIndex", U32)))),
            ("Classifications", A(Classification)),
            ("Causes", A(S(("Kind", U8), ("HasAttempt", Bool), ("AttemptId", U64), ("RootId", Guid), ("DispatcherId", U64), ("HasReason", Bool), ("Reason", Text)))),
            ("Qualities", A(S(("Kind", U8), ("Coverage", U8), ("HasReason", Bool), ("Reason", Text))))),
        S(("Interval", S(("StartQpc", I64), ("EndQpc", I64))), ("LostEvents", O(U64)), ("Reason", Text), ("HasDomains", Bool), ("Domains", A(Domain, 256))),
        S(("Object", Object), ("Property", S(("PropertyId", U64), ("DeclaringType", Text), ("Name", Text))), ("Previous", Value), ("Current", Value), ("Cause", Cause))
    ];
    private static Node Layout(ushort id) => S(
        [("SemanticEventId", U16), ("EventSchemaVersion", U8), ("ProducerEpoch", Guid), ("SourceSequence", U64),
        ("TargetProcess", S(("ProcessId", U32), ("HasCreationTimeFileTime", Bool), ("CreationTimeFileTime", I64), ("InstanceId", Guid))),
        .. Bodies[id].Fields!]);

    internal static void Validate(NativeRecord record, ushort semanticId)
    {
        var schema = Layout(semanticId);
        var raw = record.RawPayload;
        var reader = new WireReader(raw.Span);
        Check(schema, record.Fields, ref reader, raw.Length != 0);
        if (raw.Length != 0 && !reader.AtEnd) throw new InvalidDataException("Unexpected trailing native payload bytes.");
        var metadata = 3 + Encoding.UTF8.GetByteCount(record.Name) + 1 + FieldMetadata(schema) +
            2 + Encoding.UTF8.GetByteCount(EventCatalog.RuntimeProviderName) + 1;
        if (raw.Length + metadata > NativeRecord.MaximumWireBytes)
            throw new InvalidDataException("Approved native schema metadata plus data exceeds 48 KiB.");
    }
    private static int FieldMetadata(Node schema) => schema.Fields!.Sum(field =>
        Encoding.UTF8.GetByteCount(field.Name) + 1 + (field.Type.Kind is 'B' or 's' or 'S' or 'A' ? 2 : 1) +
        (field.Type.Kind == 'S' ? FieldMetadata(field.Type) : field.Type.Kind == 'A' ? FieldMetadata(field.Type.Element!) : 0));

    private static void Check(Node schema, object value, ref WireReader reader, bool hasRaw)
    {
        if (schema.Kind == 'S')
        {
            if (value is not IReadOnlyDictionary<string, object> map || !map.Keys.SequenceEqual(schema.Fields!.Select(f => f.Name)))
                throw new InvalidDataException("Native field names/order do not match the exact supported layout.");
            foreach (var field in schema.Fields!) Check(field.Type, map[field.Name], ref reader, hasRaw);
            return;
        }
        if (schema.Kind == 'A')
        {
            if (value is not IReadOnlyList<object> rows || rows.Count > schema.Maximum)
                throw new InvalidDataException("Native vector shape/count is invalid.");
            if (hasRaw && reader.U16() != rows.Count) throw new InvalidDataException("Typed array count and wire bytes disagree.");
            foreach (var row in rows) Check(schema.Element!, row, ref reader, hasRaw);
            return;
        }
        var valid = schema.Kind switch
        {
            'b' => value is byte, 'h' => value is int i && i is >= 0 and <= ushort.MaxValue || value is ushort,
            'u' => value is int or uint, 'i' => value is int, 'l' => value is long, 'q' => value is long or ulong,
            'd' => value is double d && double.IsFinite(d), 'B' => value is bool, 'g' => value is System.Guid, 's' => value is string,
            _ => false
        };
        if (!valid) throw new InvalidDataException("Native primitive type does not match the supported layout.");
        if (hasRaw) reader.Skip(schema.Kind);
    }
    private ref struct WireReader(ReadOnlySpan<byte> bytes)
    {
        private ReadOnlySpan<byte> remaining = bytes;
        internal bool AtEnd => remaining.IsEmpty;
        private ReadOnlySpan<byte> Take(int count)
        {
            if (remaining.Length < count) throw new InvalidDataException("Truncated native primitive.");
            var value = remaining[..count]; remaining = remaining[count..]; return value;
        }
        internal ushort U16() => BinaryPrimitives.ReadUInt16LittleEndian(Take(2));
        internal void Skip(char kind)
        {
            switch (kind)
            {
                case 'b': Take(1); break;
                case 'B': if (Take(1)[0] > 1) throw new InvalidDataException("Invalid bool8."); break;
                case 'h': Take(2); break;
                case 'u': case 'i': Take(4); break;
                case 'l': case 'q': Take(8); break;
                case 'd':
                    if (!double.IsFinite(BitConverter.Int64BitsToDouble(BinaryPrimitives.ReadInt64LittleEndian(Take(8)))))
                        throw new InvalidDataException("Non-finite native number.");
                    break;
                case 'g': Take(16); break;
                case 's':
                    var end = remaining.IndexOf((byte)0);
                    if (end is < 0 or > 1024) throw new InvalidDataException("Unterminated or oversized UTF-8 field.");
                    try { _ = new UTF8Encoding(false, true).GetCharCount(Take(end)); }
                    catch (DecoderFallbackException error) { throw new InvalidDataException("Invalid native UTF-8.", error); }
                    Take(1); break;
                default: throw new InvalidDataException("Unsupported wire primitive.");
            }
        }
    }
}
