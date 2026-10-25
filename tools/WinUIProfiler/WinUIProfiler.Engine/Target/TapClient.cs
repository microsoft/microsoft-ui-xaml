using System.ComponentModel;
using System.Diagnostics;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Principal;
using Microsoft.Win32.SafeHandles;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.Engine.Target;

[SupportedOSPlatform("windows")]
public sealed class TapClient : IAsyncDisposable
{
    private const string Prefix = "WinUIProfiler-";
    private readonly NamedPipeClientStream pipe;
    private readonly ProcessInstanceKey target;
    private readonly SemaphoreSlim requests = new(1, 1);
    private readonly CancellationTokenSource closed = new();
    private int disposed;
    private bool authenticated;

    internal bool IsClosed => Volatile.Read(ref disposed) != 0;
    private TapClient(NamedPipeClientStream pipe, ProcessInstanceKey target) { this.pipe = pipe; this.target = target; }

    public static string CreatePipeName() => Prefix + Guid.NewGuid().ToString("N");

    public static async Task<TapClient> ConnectAsync(string pipeName, SessionScope scope, ReadOnlyMemory<byte> nonce,
        TimeSpan timeout, CancellationToken cancellationToken)
    {
        scope.Validate();
        ValidateTimeout(timeout);
        if (pipeName is null || !pipeName.StartsWith(Prefix, StringComparison.Ordinal) ||
            !Guid.TryParseExact(pipeName.AsSpan(Prefix.Length), "N", out _))
            throw new ArgumentException("A local unpredictable profiler pipe name is required.", nameof(pipeName));
        if (scope.Process.CreationTimeFileTime is null || nonce.Length != TapProtocol.NonceBytes)
            throw new ArgumentException("TAP authentication requires process creation time and a 32-byte nonce.");
        var ownedNonce = Convert.ToBase64String(nonce.Span);
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        deadline.CancelAfter(timeout);
        var pipe = new NamedPipeClientStream(".", pipeName, PipeDirection.InOut,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly, TokenImpersonationLevel.Identification);
        TapClient? client = null;
        var connected = false;
        try
        {
            await pipe.ConnectAsync(deadline.Token).ConfigureAwait(false);
            VerifyServer(pipe, scope.Process);
            client = new(pipe, scope.Process);
            var response = await client.Exchange(TapMessageType.Handshake,
                new HandshakeBody(scope.SessionId, scope.Process, ContractVersions.Schema, ownedNonce),
                timeout, deadline.Token).ConfigureAwait(false);
            if (response.Body is not AcknowledgementBody { Status: CommandStatus.Completed, CaptureId: null })
                throw new UnauthorizedAccessException("The TAP rejected the session/nonce handshake: " +
                    (response.Body is AcknowledgementBody rejected ? rejected.Reason : "Unexpected response."));
            client.authenticated = true;
            connected = true;
            return client;
        }
        catch (OperationCanceledException error) when (!cancellationToken.IsCancellationRequested && deadline.IsCancellationRequested)
        {
            throw new TimeoutException("TAP connection/handshake deadline expired.", error);
        }
        finally
        {
            if (!connected)
            {
                if (client is not null) await client.DisposeAsync().ConfigureAwait(false);
                else await pipe.DisposeAsync().ConfigureAwait(false);
            }
        }
    }

    public async Task<string> HelloAsync(TimeSpan timeout, CancellationToken cancellationToken)
    {
        var response = await SendAsync(TapMessageType.Hello, new HelloBody("hello"),
            timeout, cancellationToken).ConfigureAwait(false);
        return ((HelloBody)response.Body).Message;
    }

    public async Task<TreeSnapshotDocument> TakeSnapshotAsync(TimeSpan timeout, CancellationToken cancellationToken)
    {
        ValidateTimeout(timeout);
        ObjectDisposedException.ThrowIf(IsClosed, this);
        if (!authenticated) throw new InvalidOperationException("The TAP session is not authenticated.");
        var requestId = Guid.NewGuid();
        var captureId = Guid.NewGuid();
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, closed.Token);
        deadline.CancelAfter(timeout);
        var acquired = false;
        var completed = false;
        try
        {
            await requests.WaitAsync(deadline.Token).ConfigureAwait(false);
            acquired = true;
            ObjectDisposedException.ThrowIf(IsClosed, this);
            await TapFraming.WriteAsync(pipe, TapMessageType.TakeSnapshot, requestId,
                new TakeSnapshotBody(captureId), deadline.Token).ConfigureAwait(false);
            using var payload = new MemoryStream();
            var chunks = 0;
            while (true)
            {
                var response = await TapFraming.ReadAsync(pipe, deadline.Token).ConfigureAwait(false);
                if (response.Header.RequestId != requestId)
                    throw new InvalidDataException("Snapshot response belongs to another request.");
                if (response.Body is AcknowledgementBody rejection)
                {
                    if (chunks != 0 || rejection.CaptureId != captureId ||
                        rejection.Status is not (CommandStatus.Unsupported or CommandStatus.Failed or CommandStatus.Cancelled or CommandStatus.Rejected))
                        throw new InvalidDataException("Snapshot rejection has invalid capture/status/chunk correlation.");
                    completed = true;
                    if (rejection.Status == CommandStatus.Unsupported)
                        throw new NotSupportedException("The target TAP cannot capture these trees: " + rejection.Reason);
                    throw new IOException($"Target snapshot {rejection.Status}: {rejection.Reason}");
                }
                if (response.Body is SnapshotChunkBody chunk)
                {
                    if (chunk.CaptureId != captureId || chunk.Index != chunks || chunks >= TapProtocol.MaximumSnapshotChunks)
                        throw new InvalidDataException("Snapshot chunks are foreign, duplicated or out of order.");
                    var bytes = Convert.FromBase64String(chunk.DataBase64);
                    if (payload.Length + bytes.Length > TapProtocol.MaximumSnapshotBytes)
                        throw new InvalidDataException("Snapshot exceeds its transfer bound.");
                    payload.Write(bytes);
                    chunks++;
                    continue;
                }
                if (response.Body is not SnapshotResultBody result || result.CaptureId != captureId ||
                    result.ChunkCount != chunks || result.ByteCount != payload.Length || chunks == 0)
                    throw new InvalidDataException("Snapshot is missing its exact correlated terminal count/length.");
                var document = TreeSnapshotCodec.Decode(payload.GetBuffer().AsMemory(0, checked((int)payload.Length)), target, captureId);
                completed = true;
                return document;
            }
        }
        catch (OperationCanceledException error) when (!cancellationToken.IsCancellationRequested &&
            !closed.IsCancellationRequested && deadline.IsCancellationRequested)
        {
            throw new TimeoutException("TAP snapshot deadline expired; the partial transport cannot be reused.", error);
        }
        finally
        {
            if (acquired)
            {
                if (!completed) Close();
                requests.Release();
            }
        }
    }

    public Task<TapMessage> SendAsync(TapMessageType type, ITapBody body, TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        if (Volatile.Read(ref disposed) != 0) return Task.FromException<TapMessage>(new ObjectDisposedException(nameof(TapClient)));
        if (!authenticated) throw new InvalidOperationException("The TAP session is not authenticated.");
        if (type is not (TapMessageType.Capabilities or TapMessageType.Rundown or TapMessageType.LightweightSnapshot or
            TapMessageType.OwnershipGraph or TapMessageType.Cancel or TapMessageType.Disconnect or TapMessageType.Hello))
            throw new ArgumentException("Only typed target commands may be sent by a TAP client.", nameof(type));
        return Exchange(type, body, timeout, cancellationToken);
    }

    private async Task<TapMessage> Exchange(TapMessageType type, ITapBody body, TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        ValidateTimeout(timeout);
        ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
        var requestId = Guid.NewGuid();
        var frame = TapCodec.Encode(type, requestId, body);
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, closed.Token);
        deadline.CancelAfter(timeout);
        var acquired = false;
        var completed = false;
        try
        {
            await requests.WaitAsync(deadline.Token).ConfigureAwait(false);
            acquired = true;
            ObjectDisposedException.ThrowIf(Volatile.Read(ref disposed) != 0, this);
            await pipe.WriteAsync(frame, deadline.Token).ConfigureAwait(false);
            await pipe.FlushAsync(deadline.Token).ConfigureAwait(false);
            var response = await TapFraming.ReadAsync(pipe, deadline.Token).ConfigureAwait(false);
            if (response.Header.RequestId != requestId)
                throw new InvalidDataException("TAP response belongs to another request.");
            if (type == TapMessageType.Hello)
            {
                if (response.Header.Type != TapMessageType.Hello || response.Body is not HelloBody { Message: "hello" })
                    throw new InvalidDataException("TAP hello requires an exact Hello reply, not an acknowledgement or another response.");
            }
            else if (response.Body is AcknowledgementBody acknowledgement)
            {
                var capture = body switch
                {
                    RundownBody request => (Guid?)request.CaptureId,
                    LightweightSnapshotBody request => request.CaptureId,
                    OwnershipGraphBody request => request.CaptureId,
                    _ => null
                };
                if (acknowledgement.CaptureId is { } actual && actual != capture ||
                    capture.HasValue && acknowledgement.Status is CommandStatus.Accepted or CommandStatus.Completed &&
                    acknowledgement.CaptureId != capture)
                    throw new InvalidDataException("TAP acknowledgement does not identify the requested capture.");
                if (type == TapMessageType.Capabilities && acknowledgement.Status is CommandStatus.Accepted or CommandStatus.Completed)
                    throw new InvalidDataException("Successful capability discovery requires a capability payload.");
            }
            else if (type != TapMessageType.Capabilities || response.Body is not CapabilitiesBody)
                throw new InvalidDataException("Unexpected TAP response type.");
            completed = true;
            if (type == TapMessageType.Disconnect) Close();
            return response;
        }
        catch (OperationCanceledException error) when (!cancellationToken.IsCancellationRequested &&
            !closed.IsCancellationRequested && deadline.IsCancellationRequested)
        {
            throw new TimeoutException("TAP command deadline expired; the transport cannot be reused.", error);
        }
        finally
        {
            if (acquired)
            {
                if (!completed) Close();
                requests.Release();
            }
        }
    }

    private static void ValidateTimeout(TimeSpan timeout)
    {
        if (timeout < TimeSpan.FromMilliseconds(1) || timeout > TimeSpan.FromMinutes(2))
            throw new ArgumentOutOfRangeException(nameof(timeout), "TAP deadlines must be between 1 ms and 2 minutes.");
    }

    private static void VerifyServer(NamedPipeClientStream pipe, ProcessInstanceKey expected)
    {
        if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle, out var pid))
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Cannot verify the TAP pipe server.");
        if (pid != expected.ProcessId)
            throw new UnauthorizedAccessException("The TAP pipe belongs to another process.");
        using var process = Process.GetProcessById(checked((int)pid));
        if (process.StartTime.ToFileTimeUtc() != expected.CreationTimeFileTime)
            throw new UnauthorizedAccessException("The TAP process creation time does not match; PID reuse is not trusted.");
    }

    private void Close()
    {
        if (Interlocked.Exchange(ref disposed, 1) == 0)
        {
            closed.Cancel();
            pipe.Dispose();
        }
    }

    public async ValueTask DisposeAsync()
    {
        Close();
        await requests.WaitAsync().ConfigureAwait(false);
        try { closed.Dispose(); }
        finally { requests.Release(); }
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetNamedPipeServerProcessId(SafePipeHandle pipe, out uint serverProcessId);
}
