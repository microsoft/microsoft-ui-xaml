namespace WinUIProfiler.Contracts;

public enum TapMessageType : ushort
{
    Handshake = 1, Disconnect = 7, Acknowledgement = 8, Hello = 10
}
public enum CommandStatus
{
    Accepted, Completed, Unsupported, Rejected, Cancelled, Failed
}
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
public sealed record HelloBody(string Message) : ITapBody;
public sealed record HandshakeBody(Guid SessionId, ProcessInstanceKey Process, int SchemaVersion, string NonceBase64) : ITapBody;
public sealed record DisconnectBody(string Reason) : ITapBody;
public sealed record AcknowledgementBody(CommandStatus Status, Guid? CaptureId, string? Reason) : ITapBody;
