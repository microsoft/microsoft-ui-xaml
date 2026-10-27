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
    private readonly SemaphoreSlim requests = new(1, 1);
    private readonly CancellationTokenSource closed = new();
    private int disposed;
    private bool authenticated;
    private TapClient(NamedPipeClientStream pipe) => this.pipe = pipe;
    public static string CreatePipeName() => Prefix + Guid.NewGuid().ToString("N");

    public static async Task<TapClient> ConnectAsync(string pipeName,
SessionScope scope,
ReadOnlyMemory<byte> nonce,
        TimeSpan timeout,
CancellationToken cancellationToken)
    {
        scope.Validate();
        ValidateTimeout(timeout);
        if (pipeName is null ||
!pipeName.StartsWith(Prefix, StringComparison.Ordinal) ||
            !Guid.TryParseExact(pipeName.AsSpan(Prefix.Length), "N", out _))
        {
            throw new ArgumentException("A local unpredictable profiler pipe name is required.", nameof(pipeName));
        }

        if (scope.Process.CreationTimeFileTime is not > 0 || nonce.Length != TapProtocol.NonceBytes)
        {
            throw new ArgumentException("TAP authentication requires process creation time and a 32-byte nonce.");
        }

        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        deadline.CancelAfter(timeout);
        var pipe = new NamedPipeClientStream(".",
pipeName,
PipeDirection.InOut,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly,
TokenImpersonationLevel.Identification);
        TapClient? client = null;
        var connected = false;
        try
        {
            await pipe.ConnectAsync(deadline.Token).ConfigureAwait(false);
            VerifyServer(pipe, scope.Process);
            client = new(pipe);
            await client.Exchange(TapMessageType.Handshake,
                new HandshakeBody(scope.SessionId, scope.Process, ContractVersions.Schema, Convert.ToBase64String(nonce.Span)),
                timeout,
deadline.Token).ConfigureAwait(false);
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
                if (client is not null)
                {
                    await client.DisposeAsync().ConfigureAwait(false);
                }
                else
                {
                    await pipe.DisposeAsync().ConfigureAwait(false);
                }
            }
        }
    }

    public async Task<string> HelloAsync(TimeSpan timeout, CancellationToken cancellationToken)
    {
        if (!authenticated)
        {
            throw new InvalidOperationException("The TAP session is not authenticated.");
        }

        var response = await Exchange(TapMessageType.Hello, new HelloBody("hello"), timeout, cancellationToken).ConfigureAwait(false);
        return ((HelloBody)response.Body).Message;
    }

    public async Task DisconnectAsync(CancellationToken cancellationToken)
    {
        if (!authenticated)
        {
            throw new InvalidOperationException("The TAP session is not authenticated.");
        }

        await Exchange(TapMessageType.Disconnect,
        new DisconnectBody("Profiler disconnected."),
                    TimeSpan.FromSeconds(5),
        cancellationToken).ConfigureAwait(false);
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
            {
                throw new InvalidDataException("TAP response belongs to another request.");
            }

            if (type == TapMessageType.Hello)
            {
                if (response.Header.Type != TapMessageType.Hello || response.Body is not HelloBody { Message: "hello" })
                {
                    throw new InvalidDataException("TAP hello requires an exact Hello reply.");
                }
            }
            else if (response.Header.Type != TapMessageType.Acknowledgement ||
                response.Body is not AcknowledgementBody { Status: CommandStatus.Completed, CaptureId: null })
            {
                throw new InvalidDataException("TAP rejected the command: " +
                                (response.Body is AcknowledgementBody rejected ? rejected.Reason : "Unexpected response."));
            }

            completed = true;
            if (type == TapMessageType.Disconnect)
            {
                Close();
            }

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
                if (!completed)
                {
                    Close();
                }

                requests.Release();
            }
        }
    }

    private static void ValidateTimeout(TimeSpan timeout)
    {
        if (timeout < TimeSpan.FromMilliseconds(1) || timeout > TimeSpan.FromMinutes(2))
        {
            throw new ArgumentOutOfRangeException(nameof(timeout));
        }
    }

    private static void VerifyServer(NamedPipeClientStream pipe, ProcessInstanceKey expected)
    {
        if (!GetNamedPipeServerProcessId(pipe.SafePipeHandle, out var pid))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Cannot verify the TAP pipe server.");
        }

        if (pid != expected.ProcessId)
        {
            throw new UnauthorizedAccessException("The TAP pipe belongs to another process.");
        }

        using var process = Process.GetProcessById(checked((int)pid));
        if (process.StartTime.ToFileTimeUtc() != expected.CreationTimeFileTime)
        {
            throw new UnauthorizedAccessException("The TAP process creation time does not match; PID reuse is not trusted.");
        }
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
        try
        {
            closed.Dispose();

        }
        finally
        {
            requests.Release();

        }
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    [DefaultDllImportSearchPaths(DllImportSearchPath.System32)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetNamedPipeServerProcessId(SafePipeHandle pipe, out uint serverProcessId);
}
