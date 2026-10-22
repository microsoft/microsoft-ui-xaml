namespace WinUIProfiler.Contracts;

public enum TapMessageType : ushort { Handshake = 1, Capabilities = 2, Rundown = 3, LightweightSnapshot = 4, OwnershipGraph = 5, Cancel = 6, Disconnect = 7, Acknowledgement = 8, CapabilitiesResult = 9 }
public enum CommandStatus { Accepted, Completed, Unsupported, Rejected, Cancelled, Failed }
public static class TapProtocol
{
    public const ushort Version = 1;
    public const int HeaderBytes = 28;
    public const int MaximumBodyBytes = 1_048_576;
    public const int NonceBytes = 32;
}
public sealed record TapHeader(uint BodyLength, ushort Version, TapMessageType Type, Guid RequestId, uint SchemaVersion);
public sealed record TapMessage(TapHeader Header, object Body);
public interface ITapBody;
public sealed record HandshakeBody(Guid SessionId, ProcessInstanceKey Process, int SchemaVersion, string NonceBase64) : ITapBody;
public sealed record CapabilitiesRequestBody(int SchemaVersion) : ITapBody;
public sealed record CapabilitiesBody(Guid Epoch, int SchemaVersion, IReadOnlyList<CapabilityInfo> Capabilities) : ITapBody;
public sealed record RundownBody(Guid CaptureId, Guid? RootId, ulong? DispatcherId) : ITapBody;
public sealed record LightweightSnapshotBody(Guid CaptureId, Guid? RootId, ulong? DispatcherId) : ITapBody;
public sealed record OwnershipGraphBody(Guid CaptureId, int MaximumObjects, int MaximumEdges) : ITapBody;
public sealed record CancelBody(Guid TargetRequestId) : ITapBody;
public sealed record DisconnectBody(string Reason) : ITapBody;
public sealed record AcknowledgementBody(CommandStatus Status, Guid? CaptureId, string? Reason) : ITapBody;
