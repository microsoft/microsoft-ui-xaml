using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Target;
using Xunit;

namespace WinUIProfiler.Engine.Tests;

public sealed class TapTreeSnapshotTests
{
    [Fact]
    public void Snapshot_preserves_explicit_many_to_one_object_visual_correlators()
    {
        var decoded = DecodeSnapshot(SnapshotJson());
        using var json = JsonDocument.Parse(JsonSerializer.Serialize(decoded, WireJson.Options));
        Assert.Equal(2, json.RootElement.GetProperty("correlations").GetArrayLength());
        Assert.All(json.RootElement.GetProperty("correlations").EnumerateArray(),
            link => Assert.Equal(10UL, link.GetProperty("visualNodeId").GetUInt64()));
    }

    [Theory]
    [InlineData("foreign-process")]
    [InlineData("foreign-creation")]
    [InlineData("dangling-link")]
    [InlineData("parent-cycle")]
    [InlineData("duplicate-node")]
    [InlineData("zero-address")]
    [InlineData("cross-root")]
    [InlineData("invalid-interval")]
    [InlineData("duplicate-link")]
    public void Snapshot_rejects_unproven_or_inconsistent_object_correlations(string invalid)
    {
        var json = SnapshotJson();
        json = invalid switch
        {
            "foreign-process" => json.Replace("\"processId\":42", "\"processId\":43"),
            "foreign-creation" => json.Replace("\"creationTimeFileTime\":123", "\"creationTimeFileTime\":124"),
            "dangling-link" => json.Replace("\"visualNodeId\":10", "\"visualNodeId\":11"),
            "parent-cycle" => json.Replace("\"parentId\":null", "\"parentId\":2"),
            "duplicate-node" => json.Replace("\"id\":2", "\"id\":1"),
            "zero-address" => json.Replace("\"objectAddress\":\"0x100\"", "\"objectAddress\":\"0x0\""),
            "cross-root" => json.Replace("\"rootId\":1", "\"rootId\":2"),
            "invalid-interval" => json.Replace("\"startQpc\":101", "\"startQpc\":99"),
            "duplicate-link" => json.Replace("\"xamlNodeId\":2", "\"xamlNodeId\":1"),
            _ => throw new ArgumentOutOfRangeException(nameof(invalid))
        };
        Assert.Throws<InvalidDataException>(() => DecodeSnapshot(json));
    }

    [Theory]
    [InlineData("duplicate-property")]
    [InlineData("unknown-property")]
    [InlineData("missing-nullable-property")]
    public void Snapshot_requires_strict_complete_document_fields(string invalid)
    {
        var json = SnapshotJson();
        json = invalid switch {
            "duplicate-property" => json.Replace("\"schemaVersion\":1", "\"schemaVersion\":1,\"schemaVersion\":1"),
            "unknown-property" => json.Replace("\"schemaVersion\":1", "\"schemaVersion\":1,\"unexpected\":true"),
            "missing-nullable-property" => json.Replace("\"name\":\"Root\",", ""),
            _ => throw new ArgumentOutOfRangeException(nameof(invalid))
        };
        Assert.Throws<JsonException>(() => DecodeSnapshot(json));
    }

    private static TreeSnapshotDocument DecodeSnapshot(string json) =>
        TreeSnapshotCodec.Decode(Encoding.UTF8.GetBytes(json), new(42, 123, CoreTests.Session), CoreTests.Session);

    internal static string SnapshotJson(uint processId = 42, long creation = 123, Guid? capture = null) =>
        JsonSerializer.Serialize(new {
            schemaVersion = 1, captureId = capture ?? CoreTests.Session, processId,
            creationTimeFileTime = creation, qpcFrequency = 10, startQpc = 100, endQpc = 120,
            roots = new[] { new { id = 1, threadId = 17, startQpc = 101, endQpc = 119, xamlRootAddress = "0x123" } },
            xamlNodes = new[] {
                new { id = 1, rootId = 1, parentId = (int?)null, childIndex = 0, type = "Grid", name = "Root",
                    objectAddress = "0x100", diagnosticsHandle = "0x201", properties = new { opacity = 1.0 } },
                new { id = 2, rootId = 1, parentId = (int?)1, childIndex = 0, type = "Border", name = "Child",
                    objectAddress = "0x101", diagnosticsHandle = "0x202", properties = new { opacity = 1.0 } } },
            compositionNodes = new[] { new { id = 10, rootId = 1, parentId = (int?)null, childIndex = 0,
                type = "ContainerVisual", objectAddress = "0x301", visualAddress = "0x401", properties = new { opacity = 1.0 } } },
            correlations = new[] { new { xamlNodeId = 1, visualNodeId = 10 }, new { xamlNodeId = 2, visualNodeId = 10 } },
            limitations = new[] { "Snapshot-local public tree evidence; no private lifetime identity." }
        });

    [Theory]
    [InlineData(11, "TakeSnapshot", "{\"captureId\":\"c2e8b781-a39c-409f-9298-2298013dcd97\"}")]
    [InlineData(12, "SnapshotChunk", "{\"captureId\":\"c2e8b781-a39c-409f-9298-2298013dcd97\",\"index\":0,\"dataBase64\":\"e30=\"}")]
    [InlineData(13, "SnapshotResult", "{\"captureId\":\"c2e8b781-a39c-409f-9298-2298013dcd97\",\"chunkCount\":1,\"byteCount\":2}")]
    public void Snapshot_frames_have_explicit_typed_bounded_contracts(ushort type, string name, string json)
    {
        var decoded = TapCodec.Decode(Frame(type, json));
        Assert.Equal(name, decoded.Header.Type.ToString());
        Assert.Equal(Guid.Parse("c2e8b781-a39c-409f-9298-2298013dcd97"),
            decoded.Body.GetType().GetProperty("CaptureId")!.GetValue(decoded.Body));
        Assert.Equal(Frame(type, json)[..8], TapCodec.Encode(decoded.Header.Type, decoded.Header.RequestId,
            (ITapBody)decoded.Body)[..8]);
    }

    internal static byte[] Frame(ushort type, string json)
    {
        var body = Encoding.UTF8.GetBytes(json);
        var bytes = new byte[TapProtocol.HeaderBytes + body.Length];
        BinaryPrimitives.WriteUInt32LittleEndian(bytes, (uint)body.Length);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(4), TapProtocol.Version);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.AsSpan(6), type);
        CoreTests.Session.TryWriteBytes(bytes.AsSpan(8, 16), bigEndian: true, out _);
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(24), ContractVersions.Schema);
        body.CopyTo(bytes, TapProtocol.HeaderBytes);
        return bytes;
    }
}
