using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Target;

public static class TapFraming
{
    public static async ValueTask<TapMessage> ReadAsync(Stream stream, CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(stream);
        cancellationToken.ThrowIfCancellationRequested();
        var headerBytes = new byte[TapProtocol.HeaderBytes];
        await stream.ReadExactlyAsync(headerBytes, cancellationToken).ConfigureAwait(false);
        var header = TapCodec.DecodeHeader(headerBytes);
        var frame = new byte[TapProtocol.HeaderBytes + header.BodyLength];
        headerBytes.CopyTo(frame, 0);
        await stream.ReadExactlyAsync(frame.AsMemory(TapProtocol.HeaderBytes), cancellationToken).ConfigureAwait(false);
        return TapCodec.Decode(frame);
    }

    public static async ValueTask WriteAsync(Stream stream, TapMessageType type, Guid requestId, ITapBody body,
        CancellationToken cancellationToken)
    {
        ArgumentNullException.ThrowIfNull(stream);
        cancellationToken.ThrowIfCancellationRequested();
        var frame = TapCodec.Encode(type, requestId, body);
        await stream.WriteAsync(frame, cancellationToken).ConfigureAwait(false);
        await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
    }
}
