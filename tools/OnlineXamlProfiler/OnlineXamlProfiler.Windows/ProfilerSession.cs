// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Buffers.Binary;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Text.Json;
using Microsoft.Win32.SafeHandles;
using OnlineXamlProfiler.Core;

namespace OnlineXamlProfiler.Windows;

public sealed class ProfilerSession : IDisposable
{
    private readonly NamedPipeServerStream _pipe;
    private readonly Process _target;
    private readonly SemaphoreSlim _requestLock = new(1);
    private readonly CancellationTokenSource _lifetime = new();
    private ulong _requestId;
    private int _disposed;

    private ProfilerSession(NamedPipeServerStream pipe, Process target, string processStartTime, string runtimePath)
    {
        _pipe = pipe;
        _target = target;
        ProcessId = checked((uint)target.Id);
        ProcessStartTime = processStartTime;
        RuntimePath = runtimePath;
    }

    public uint ProcessId { get; }
    public string ProcessStartTime { get; }
    public string RuntimePath { get; }

    public static async Task<ProfilerSession> ConnectAsync(
        uint processId, string tapPath, CancellationToken cancellationToken)
    {
        tapPath = Path.GetFullPath(tapPath);
        if (!File.Exists(tapPath))
        {
            throw new FileNotFoundException("Build the matching-architecture OnlineXamlProfilerTap.dll first.", tapPath);
        }
        string injectorPath = Path.Combine(Path.GetDirectoryName(tapPath)!, "OnlineXamlProfiler.Injector.exe");
        if (!File.Exists(injectorPath))
        {
            throw new FileNotFoundException("Build OnlineXamlProfiler.Injector and keep it next to the matching TAP.", injectorPath);
        }
        var target = Process.GetProcessById(checked((int)processId));
        NamedPipeServerStream? pipe = null;
        try
        {
            EnsureMatchingArchitecture(target);
            string processStart = target.StartTime.ToUniversalTime().ToFileTimeUtc().ToString("X16", CultureInfo.InvariantCulture);
            var runtime = await WaitForRuntimeAsync(target, cancellationToken).ConfigureAwait(false);
            string pipeName = $"OnlineXamlProfiler.{Guid.NewGuid():N}";
            pipe = new NamedPipeServerStream(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Byte,
                PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
            using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
            timeout.CancelAfter(TimeSpan.FromSeconds(30));
            await InjectAsync(target, runtime, injectorPath, tapPath, pipeName, timeout.Token).ConfigureAwait(false);
            await pipe.WaitForConnectionAsync(timeout.Token).ConfigureAwait(false);

            if (!GetNamedPipeClientProcessId(pipe.SafePipeHandle, out uint clientId))
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }
            if (clientId != processId)
            {
                throw new InvalidDataException("The pipe client is not the selected target process.");
            }
            var frame = await FrameCodec.ReadAsync(pipe, timeout.Token).ConfigureAwait(false);
            ThrowIfError(frame);
            if (frame.Kind != MessageKind.Hello || frame.RequestId != 0)
            {
                throw new InvalidDataException("The TAP did not send a session handshake.");
            }
            var hello = JsonSerializer.Deserialize<SessionHello>(frame.Payload)
                ?? throw new InvalidDataException("The TAP handshake is null.");
            if (hello.ProcessId != processId || hello.SnapshotAbiVersion != 1 ||
                hello.ProcessStartTime != processStart || string.IsNullOrWhiteSpace(hello.RuntimePath) ||
                !string.Equals(hello.RuntimePath, runtime.XamlPath, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("The TAP runtime, process instance, or snapshot ABI does not match.");
            }
            return new(pipe, target, processStart, runtime.XamlPath);
        }
        catch
        {
            pipe?.Dispose();
            target.Dispose();
            throw;
        }
    }

    public async Task<SnapshotDocument> CaptureAsync(CancellationToken cancellationToken)
    {
        ObjectDisposedException.ThrowIf(_disposed != 0, this);
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, _lifetime.Token);
        timeout.CancelAfter(TimeSpan.FromMinutes(1));
        await _requestLock.WaitAsync(timeout.Token).ConfigureAwait(false);
        try
        {
            ObjectDisposedException.ThrowIf(_disposed != 0, this);
            if (_target.HasExited)
            {
                throw new InvalidOperationException("The target process has exited.");
            }
            ulong request = checked(++_requestId);
            byte[] payload = new byte[sizeof(uint)];
            BinaryPrimitives.WriteUInt32LittleEndian(payload, SnapshotDocument.MaximumNodeCount);
            await FrameCodec.WriteAsync(_pipe, new(MessageKind.Capture, request, payload), timeout.Token).ConfigureAwait(false);
            var frame = await FrameCodec.ReadAsync(_pipe, timeout.Token).ConfigureAwait(false);
            ThrowIfError(frame);
            if (frame.Kind != MessageKind.Snapshot || frame.RequestId != request)
            {
                throw new InvalidDataException("The TAP returned an unexpected snapshot response.");
            }
            var document = SnapshotDocument.Deserialize(frame.Payload);
            if (document.ProcessId != ProcessId || document.ProcessStartTime != ProcessStartTime ||
                document.RequestId != request.ToString(CultureInfo.InvariantCulture))
            {
                throw new InvalidDataException("The snapshot does not belong to this request and process instance.");
            }
            return document;
        }
        catch
        {
            // An interrupted frame cannot be reused for a subsequent request.
            Dispose();
            throw;
        }
        finally
        {
            _requestLock.Release();
        }
    }

    public void Dispose()
    {
        if (Interlocked.Exchange(ref _disposed, 1) == 0)
        {
            _lifetime.Cancel();
            _pipe.Dispose();
            _target.Dispose();
        }
    }

    private static void ThrowIfError(ProtocolFrame frame)
    {
        if (frame.Kind == MessageKind.Error)
        {
            var error = JsonSerializer.Deserialize<SessionError>(frame.Payload)
                ?? throw new InvalidDataException("The TAP error response is null.");
            throw new InvalidOperationException($"TAP 0x{unchecked((uint)error.Status):X8}: {error.Message}");
        }
    }

    private sealed record TargetRuntime(string XamlPath, string InitializerPath);

    private static async Task<TargetRuntime> WaitForRuntimeAsync(Process target, CancellationToken token)
    {
        var deadline = Stopwatch.StartNew();
        while (deadline.Elapsed < TimeSpan.FromSeconds(30))
        {
            token.ThrowIfCancellationRequested();
            if (target.HasExited)
            {
                throw new InvalidOperationException("The target exited before WinUI initialized. If it launched a different process, attach to that process's PID.");
            }
            target.Refresh();
            string? xamlPath = null;
            string? initializerPath = null;
            foreach (ProcessModule module in target.Modules)
            {
                if (string.Equals(module.ModuleName, "Microsoft.UI.Xaml.dll", StringComparison.OrdinalIgnoreCase))
                {
                    xamlPath = module.FileName;
                }
                else if (string.Equals(module.ModuleName, "Microsoft.Internal.FrameworkUdk.dll", StringComparison.OrdinalIgnoreCase))
                {
                    initializerPath = module.FileName;
                }
            }
            if (xamlPath is not null && initializerPath is not null)
            {
                return new(xamlPath, initializerPath);
            }
            await Task.Delay(200, token).ConfigureAwait(false);
        }
        throw new TimeoutException("The selected process did not load WinUI and its matching FrameworkUdk within 30 seconds.");
    }

    private static void EnsureMatchingArchitecture(Process target)
    {
        if (!IsWow64Process2(target.SafeHandle, out ushort processMachine, out ushort nativeMachine))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error());
        }
        ushort machine = processMachine == 0 ? nativeMachine : processMachine;
        Architecture architecture = machine switch
        {
            0x014c => Architecture.X86,
            0x8664 => Architecture.X64,
            0xaa64 => Architecture.Arm64,
            _ => throw new NotSupportedException($"Unsupported target machine 0x{machine:X4}."),
        };
        if (architecture != RuntimeInformation.ProcessArchitecture)
        {
            throw new NotSupportedException($"Run the {architecture} build of OnlineXamlProfiler and its TAP to attach to this process.");
        }
        if (target.Id == Environment.ProcessId)
        {
            throw new InvalidOperationException("Select a target other than the profiler itself.");
        }
    }

    private static async Task InjectAsync(
        Process target, TargetRuntime runtime, string injectorPath, string tapPath, string pipeName, CancellationToken token)
    {
        var start = new ProcessStartInfo(injectorPath)
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardError = true,
            RedirectStandardOutput = true,
        };
        foreach (string argument in new[] { target.Id.ToString(CultureInfo.InvariantCulture),
            runtime.InitializerPath, runtime.XamlPath, tapPath, pipeName })
        {
            start.ArgumentList.Add(argument);
        }
        using var injector = Process.Start(start) ?? throw new InvalidOperationException("The diagnostics injector did not start.");
        var error = injector.StandardError.ReadToEndAsync();
        var output = injector.StandardOutput.ReadToEndAsync();
        try
        {
            await injector.WaitForExitAsync(token).ConfigureAwait(false);
        }
        catch (OperationCanceledException)
        {
            if (!injector.HasExited)
            {
                injector.Kill();
            }
            await injector.WaitForExitAsync(CancellationToken.None).ConfigureAwait(false);
            await Task.WhenAll(error, output).ConfigureAwait(false);
            throw;
        }
        string diagnostic = (await error.ConfigureAwait(false)).Trim();
        await output.ConfigureAwait(false);
        if (injector.ExitCode != 0)
        {
            throw new COMException($"Diagnostics injector: {diagnostic} Exit code 0x{unchecked((uint)injector.ExitCode):X8}.", injector.ExitCode);
        }
    }

    private sealed record SessionHello(uint ProcessId, uint SnapshotAbiVersion, string ProcessStartTime, string RuntimePath);
    private sealed record SessionError(int Status, string Message);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetNamedPipeClientProcessId(SafePipeHandle pipe, out uint processId);

    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWow64Process2(SafeProcessHandle process, out ushort processMachine, out ushort nativeMachine);

}
