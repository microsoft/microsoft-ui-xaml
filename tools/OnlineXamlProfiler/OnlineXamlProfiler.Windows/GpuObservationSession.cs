// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using OnlineXamlProfiler.Core;

namespace OnlineXamlProfiler.Windows;

public sealed class GpuObservationSession : IAsyncDisposable, IDisposable
{
    private static readonly Guid Direct3D11 = new("db6f6ddb-ac77-4e88-8253-819df9bbf140");
    private static readonly Guid DxgKrnl = new("802ec45a-1e99-4b83-9920-87c98277ba9d");
    private static readonly Guid Dxgi = new("ca11c036-0102-4a2d-a6ad-f03cfed5d3c9");
    private readonly object _gate = new();
    private readonly GpuObservationLedger _ledger = new();
    private readonly long _startQpc = Stopwatch.GetTimestamp();
    private readonly bool _beforeLaunch;
    private TraceEventSession? _session;
    private Task _consumer = Task.CompletedTask;
    private uint? _target;
    private string _state = "Starting";
    private string _diagnostic = "";
    private long _invalidPayloads;
    private long _eventsLost;
    private int _disposed;

    private GpuObservationSession(bool beforeLaunch) => _beforeLaunch = beforeLaunch;

    public static async Task<GpuObservationSession> StartAsync(
        uint? processId, bool beforeLaunch, CancellationToken token)
    {
        var result = new GpuObservationSession(beforeLaunch) { _target = processId };
        try
        {
            await Task.Run(result.Start, token).ConfigureAwait(false);
            return result;
        }
        catch
        {
            await result.DisposeAsync().ConfigureAwait(false);
            throw;
        }
    }

    private void Start()
    {
        try
        {
            var session = new TraceEventSession($"OnlineXamlProfiler.Gpu.{Environment.ProcessId}.{Guid.NewGuid():N}");
            _session = session;
            session.StopOnDispose = true;
            session.BufferSizeMB = 32;
            session.Source.Dynamic.All += OnEvent;
            EnableProcessProvider(session, Direct3D11, 4, 12);
            EnableProcessProvider(session, Dxgi, 2, 42);
            session.EnableProvider(DxgKrnl, TraceEventLevel.Verbose, 0x40,
                new TraceEventProviderOptions { EventIDsToEnable = [33, 36] });
            lock (_gate) { _state = "Collecting"; }
            _consumer = Task.Run(() =>
            {
                try
                {
                    session.Source.Process();
                    lock (_gate)
                    {
                        Interlocked.Exchange(ref _eventsLost, session.EventsLost);
                        if (Volatile.Read(ref _disposed) == 0)
                        {
                            _state = "Stopped";
                            _diagnostic = "The GPU event consumer stopped before disconnect.";
                        }
                    }
                }
                catch (Exception exception) when (IsTraceFailure(exception))
                {
                    SetFailure("Failed", exception);
                }
            });
        }
        catch (Exception exception) when (IsTraceFailure(exception))
        {
            SetFailure("Unavailable", exception);
            _session?.Dispose();
            _session = null;
        }
    }

    private void EnableProcessProvider(TraceEventSession session, Guid provider, ulong keywords, int eventId)
    {
        var options = new TraceEventProviderOptions { EventIDsToEnable = [eventId] };
        if (_target is { } pid) { options.ProcessIDFilter = [checked((int)pid)]; }
        session.EnableProvider(provider, TraceEventLevel.Verbose, keywords, options);
    }

    public void SelectTarget(uint processId)
    {
        lock (_gate) { _target = processId; }
        _ledger.SelectTarget(processId);
    }

    private void OnEvent(TraceEvent data)
    {
        uint? target;
        lock (_gate) { target = _target; }
        if (data.ProviderGuid == Direct3D11 && (int)data.ID == 12)
        {
            if (data.ProcessID <= 0 || (target is { } pid && data.ProcessID != pid)) { return; }
            if (TryNumber(data, "pID3D11Resource", out ulong resource) &&
                TryNumber(data, "hKMResource", out ulong kernel))
            {
                if (resource == 0 || kernel == 0) { return; }
                _ledger.Add(new(GpuObservationKind.Resource, EventQpc(data), checked((uint)data.ProcessID),
                    ResourceId: resource, KernelResource: kernel));
                return;
            }
        }
        else if (data.ProviderGuid == DxgKrnl && (int)data.ID == 36)
        {
            if (!TryNumber(data, "hProcessId", out ulong pid) || pid > uint.MaxValue)
            {
                Interlocked.Increment(ref _invalidPayloads);
                return;
            }
            if (target is { } selected && pid != selected) { return; }
            if (TryNumber(data, "hThunkResource", out ulong kernel) &&
                TryNumber(data, "hVidMmGlobalAlloc", out ulong allocation))
            {
                if (kernel == 0 || allocation == 0) { return; }
                _ledger.Add(new(GpuObservationKind.DeviceAllocation, EventQpc(data), (uint)pid,
                    KernelResource: kernel, AllocationId: allocation));
                return;
            }
        }
        else if (data.ProviderGuid == DxgKrnl && (int)data.ID == 33)
        {
            if (TryNumber(data, "hVidMmGlobalAlloc", out ulong allocation) &&
                TryNumber(data, "allocSize", out ulong bytes) &&
                TryNumber(data, "hProcessId", out ulong owner) && owner <= uint.MaxValue && allocation != 0)
            {
                _ledger.Add(new(GpuObservationKind.AdapterAllocation, EventQpc(data), 0,
                    AllocationId: allocation, AllocationBytes: bytes, OwnerProcessId: (uint)owner));
                return;
            }
        }
        else if (data.ProviderGuid == Dxgi && (int)data.ID == 42)
        {
            if (data.ProcessID <= 0 || (target is { } pid && data.ProcessID != pid)) { return; }
            _ledger.Add(new(GpuObservationKind.Present, EventQpc(data), checked((uint)data.ProcessID)));
            return;
        }
        else { return; }
        Interlocked.Increment(ref _invalidPayloads);
    }

    private static bool TryNumber(TraceEvent data, string name, out ulong value)
    {
        value = 0;
        if (!data.PayloadNames.Contains(name, StringComparer.Ordinal)) { return false; }
        object? payload = data.PayloadByName(name);
        switch (payload)
        {
            case ulong number: value = number; return true;
            case long number: value = unchecked((ulong)number); return true;
            case uint number: value = number; return true;
            case int number: value = unchecked((uint)number); return true;
            case ushort number: value = number; return true;
            case byte number: value = number; return true;
            case IntPtr pointer: value = unchecked((ulong)pointer.ToInt64()); return true;
            case string text when text.StartsWith("0x", StringComparison.OrdinalIgnoreCase):
                return ulong.TryParse(text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out value);
            case string text:
                return ulong.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out value);
            default: return false;
        }
    }

#pragma warning disable CS0618 // Raw QPC is required to join the native draw observations without rounding.
    private static long EventQpc(TraceEvent data) => data.TimeStampQPC;
#pragma warning restore CS0618

    public GpuTraceSnapshot Freeze(SnapshotDocument document)
    {
        var session = _session;
        if (session is not null && Volatile.Read(ref _disposed) == 0)
        {
            try
            {
                session.Flush();
                Interlocked.Exchange(ref _eventsLost, session.EventsLost);
            }
            catch (Exception exception) when (IsTraceFailure(exception))
            {
                SetFailure("Failed", exception);
            }
        }
        lock (_gate)
        {
            return _ledger.Freeze(document, _state, _diagnostic, _beforeLaunch, _startQpc,
                Interlocked.Read(ref _eventsLost), Interlocked.Read(ref _invalidPayloads));
        }
    }

    private void SetFailure(string state, Exception exception)
    {
        lock (_gate)
        {
            _state = state;
            _diagnostic = $"{exception.Message} (0x{unchecked((uint)exception.HResult):X8})";
        }
    }

    private static bool IsTraceFailure(Exception exception) =>
        exception is UnauthorizedAccessException or Win32Exception or InvalidOperationException or
            NotSupportedException or IOException or System.Runtime.InteropServices.COMException;

    public async ValueTask DisposeAsync()
    {
        if (Interlocked.Exchange(ref _disposed, 1) != 0) { return; }
        await Task.Run(() =>
        {
            try { _session?.Dispose(); }
            catch (Exception exception) when (IsTraceFailure(exception)) { SetFailure("Failed", exception); }
        }).ConfigureAwait(false);
        await _consumer.ConfigureAwait(false);
        _session = null;
    }

    public void Dispose() => DisposeAsync().AsTask().GetAwaiter().GetResult();
}
