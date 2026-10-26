// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;

namespace OnlineXamlProfiler.Core;

public enum GpuObservationKind { Resource, DeviceAllocation, AdapterAllocation, Present }

public readonly record struct GpuObservation(
    GpuObservationKind Kind, long Qpc, uint ProcessId, ulong ResourceId = 0,
    ulong KernelResource = 0, ulong AllocationId = 0, ulong AllocationBytes = 0, uint OwnerProcessId = 0);

public enum GpuEvidenceStatus { Matched, ResourceNotObserved, AllocationNotObserved, AmbiguousAllocation }

public sealed record GpuResourceEvidence(
    string ResourceId, long DrawObservedQpc, GpuEvidenceStatus Status,
    string AllocationId, long AllocationObservedQpc, ulong? AllocationBytes, uint? OwnerProcessId);

public sealed record GpuTraceSnapshot(
    string State, string Diagnostic, bool StartedBeforeLaunch,
    long StartQpc, long CaptureEndQpc, long LastDeliveredQpc,
    long EventsLost, long DroppedObservations, long InvalidPayloads,
    ulong? DistinctReportedAllocationBytes, long ObservedPresentCount,
    IReadOnlyList<GpuResourceEvidence> Resources)
{
    public static GpuTraceSnapshot Disabled { get; } = new(
        "Disabled", "", false, 0, 0, 0, 0, 0, 0, null, 0, []);
}

// This is a bounded evidence history, not an allocation/residency inventory.
// Start/Stop event opcodes are not assumed to describe resource destruction.
public sealed class GpuObservationLedger(int capacity = 32768)
{
    private readonly object _gate = new();
    private readonly Queue<GpuObservation> _events = new();
    private long _dropped;
    private long _lastDeliveredQpc;

    public void Add(GpuObservation observation)
    {
        if (capacity <= 0 || observation.Qpc <= 0 || !Enum.IsDefined(observation.Kind))
        {
            throw new ArgumentOutOfRangeException(nameof(observation));
        }
        lock (_gate)
        {
            if (_events.Count == capacity)
            {
                _events.Dequeue();
                ++_dropped;
            }
            _events.Enqueue(observation);
            _lastDeliveredQpc = Math.Max(_lastDeliveredQpc, observation.Qpc);
        }
    }

    public void SelectTarget(uint processId)
    {
        lock (_gate)
        {
            var retained = _events.Where(value => value.Kind == GpuObservationKind.AdapterAllocation ||
                value.ProcessId == processId).ToArray();
            _events.Clear();
            foreach (var value in retained) { _events.Enqueue(value); }
        }
    }

    public GpuTraceSnapshot Freeze(
        SnapshotDocument document, string state, string diagnostic, bool beforeLaunch,
        long startQpc, long eventsLost, long invalidPayloads)
    {
        document.Validate();
        long captureEnd = document.Threads.Where(thread => thread.Status >= 0)
            .Select(thread => long.Parse(thread.EndQpc, CultureInfo.InvariantCulture)).DefaultIfEmpty(startQpc).Max();
        GpuObservation[] events;
        long dropped, lastDelivered;
        lock (_gate)
        {
            events = _events.Where(value => value.Qpc <= captureEnd).OrderBy(value => value.Qpc).ToArray();
            dropped = _dropped;
            lastDelivered = _lastDeliveredQpc;
        }
        var resources = events.Where(value => value.Kind == GpuObservationKind.Resource &&
            value.ProcessId == document.ProcessId).GroupBy(value => value.ResourceId)
            .ToDictionary(group => group.Key, group => group.ToArray());
        var devices = events.Where(value => value.Kind == GpuObservationKind.DeviceAllocation &&
            value.ProcessId == document.ProcessId).GroupBy(value => value.KernelResource)
            .ToDictionary(group => group.Key, group => group.ToArray());
        var allocations = events.Where(value => value.Kind == GpuObservationKind.AdapterAllocation)
            .GroupBy(value => value.AllocationId).ToDictionary(group => group.Key, group => group.ToArray());

        var evidence = new List<GpuResourceEvidence>();
        foreach (var surface in document.Threads.Where(thread => thread.Status >= 0).SelectMany(thread => thread.Surfaces)
            .Where(surface => surface.Status >= 0 && surface.TextureStatus >= 0)
            .DistinctBy(surface => (surface.ResourceId, surface.ObservedQpc)))
        {
            ulong pointer = ulong.Parse(surface.ResourceId, NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture);
            GpuObservation? resource = Latest(resources.GetValueOrDefault(pointer), surface.ObservedQpc);
            if (resource is null)
            {
                evidence.Add(Missing(surface, GpuEvidenceStatus.ResourceNotObserved));
                continue;
            }
            // Multiple historical allocations for one kernel handle cannot be
            // safely interpreted as either planes or reused handle generations.
            var candidates = devices.GetValueOrDefault(resource.Value.KernelResource)?
                .Where(value => value.Qpc <= resource.Value.Qpc)
                .GroupBy(value => value.AllocationId).Select(group => group.MaxBy(value => value.Qpc)).ToArray() ?? [];
            if (candidates.Length != 1)
            {
                evidence.Add(Missing(surface, candidates.Length == 0 ?
                    GpuEvidenceStatus.AllocationNotObserved : GpuEvidenceStatus.AmbiguousAllocation));
                continue;
            }
            var device = candidates[0];
            GpuObservation? allocation = Latest(allocations.GetValueOrDefault(device.AllocationId), device.Qpc);
            if (allocation is null)
            {
                evidence.Add(Missing(surface, GpuEvidenceStatus.AllocationNotObserved));
                continue;
            }
            evidence.Add(new(surface.ResourceId, surface.ObservedQpc, GpuEvidenceStatus.Matched,
                allocation.Value.AllocationId.ToString("X16", CultureInfo.InvariantCulture),
                allocation.Value.Qpc, allocation.Value.AllocationBytes, allocation.Value.OwnerProcessId));
        }

        var matched = evidence.Where(value => value.Status == GpuEvidenceStatus.Matched).ToArray();
        ulong? reportedBytes = matched.Length == 0 ? null :
            matched.DistinctBy(value => (value.AllocationId, value.AllocationObservedQpc))
                .Aggregate(0UL, (total, value) => checked(total + value.AllocationBytes!.Value));
        long presents = events.LongCount(value => value.Kind == GpuObservationKind.Present &&
            value.ProcessId == document.ProcessId);
        return new(state, diagnostic, beforeLaunch, startQpc, captureEnd, lastDelivered, eventsLost,
            dropped, invalidPayloads, reportedBytes, presents, evidence.ToArray());
    }

    private static GpuResourceEvidence Missing(SnapshotSurface surface, GpuEvidenceStatus status) =>
        new(surface.ResourceId, surface.ObservedQpc, status, "", 0, null, null);

    private static GpuObservation? Latest(GpuObservation[]? events, long atQpc)
    {
        if (events is null) { return null; }
        for (int index = events.Length - 1; index >= 0; --index)
        {
            if (events[index].Qpc <= atQpc) { return events[index]; }
        }
        return null;
    }
}
