// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

internal static class GraphicsIndexTests
{
    private const ulong Device = 0xfedcba9876543210;
    private const ulong Adapter = 0xffffa00000001234;

    internal static void Run()
    {
        CompleteLifetimes(reverse: false);
        CompleteLifetimes(reverse: true);
        IncompleteLifetimes();
        ReusedHandles();
        DelayedObservations();
        AmbiguousAndChangingBridges();
        TrimmedLifetimes();
    }

    private static void CompleteLifetimes(bool reverse)
    {
        List<ProfilerEvent> events = [Direct3D(3, Device)];
        foreach (var id in Enumerable.Range(20, 6))
        {
            events.Add(new(0, 0, 10, 7, XamlProvider.Direct3D11, false, "CreationPhase", id % 2 + 1, [],
                EventId: id, Version: 0));
        }
        for (var i = 0; i < 4; i++)
        {
            var handle = Device + (ulong)i;
            var kernelResource = 0xf1234567U + (uint)i;
            ProfilerEvent[] starts =
            [
                Direct3D(6 + i * 3, handle, Device, kernelResource),
                Allocation(36, handle, handle, kernelResource),
                Allocation(33, handle, handle, kernelResource)
            ];
            ProfilerEvent[] stops =
            [
                Direct3D(7 + i * 3, handle, Device, kernelResource),
                Allocation(37, handle, handle, kernelResource),
                Allocation(34, handle, handle, kernelResource)
            ];
            events.AddRange(reverse ? starts.Reverse() : starts);
            events.AddRange(reverse ? stops.Reverse() : stops);
        }
        events.Add(Direct3D(4, Device));
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "GraphicsLifetimes") == 13 &&
                Count(connection, "GraphicsLifetimes", "StartKnown AND StopKnown AND DurationMs > 0") == 13,
                "Device, buffer, all texture dimensions and both allocation kinds need actual Start/Stop lifetimes.");
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1 &&
                Count(connection, "GraphicsObjects", "Kind IN ('Buffer', 'Texture1D', 'Texture2D', 'Texture3D')") == 4 &&
                Count(connection, "GraphicsObjects", "Kind IN ('DeviceAllocation', 'AdapterAllocation')") == 8,
                "Creation-call phases must not create devices or duplicate resource/allocation rows.");
            Expect(Sql.Scalar(connection, "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name GLOB 'Graphics*'") == 3,
                "The processed schema must use only GraphicsObjects, GraphicsObjectEvents and GraphicsLinks.");
            Expect(Count(connection, "GraphicsLifetimes", $"Handle = '{Hex(Device)}'") == 4,
                "Equal numeric handles in runtime-device, runtime-resource and allocation domains must remain distinct.");
            Expect(Count(connection, "GraphicsObjects", """
                DeviceId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'Device') AND hKMResource IS NOT NULL
                """) == 4,
                "Every runtime resource must reference its observed runtime device.");
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device' AND hKMDevice = '0x0000000000000043' AND hDevice IS NULL") == 1 &&
                Count(connection, "GraphicsObjects", $"hDevice = '{Hex(Device)}' AND hKMDevice IS NULL") == 8,
                "D3D11 hKMDevice and DxgKrnl hDevice must retain their separate identities.");
            Expect(Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NOT NULL AND EndSeq > StartSeq") == 4 &&
                Count(connection, "GraphicsResourceAllocations", "ProcessId = 10 AND SizeBytes = '18446744073709551615'") == 4,
                "Both observation orders must resolve full resource/allocation chains without losing unsigned sizes.");
        });
        Console.WriteLine($"Passed: complete DirectX lifetimes and exact allocation chains ({(reverse ? "kernel first" : "runtime first")}).");
    }

    private static void IncompleteLifetimes()
    {
        ProfilerEvent[] events =
        [
            Direct3D(5, Device),
            new(0, 0, 10, 7, XamlProvider.Direct3D11, false, "D3D11CoreCreateDevice/Stop", 2, [], EventId: 21, Version: 0),
            Direct3D(8, 11, Device, 100),
            Direct3D(7, 11, Device, 100),
            Direct3D(13, 12),
            Direct3D(9, 13),
            Allocation(38, 14, 15, 101),
            Allocation(35, 14, 15, 101),
            Direct3D(4, Device)
        ];
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "GraphicsLifetimes") == 6 &&
                Count(connection, "GraphicsLifetimes", "DurationMs IS NOT NULL") == 0,
                "Rundown, stop-only and unclosed objects must not acquire fabricated complete durations.");
            Expect(Count(connection, "GraphicsLifetimes",
                "Kind IN ('Device', 'Buffer') AND FirstObservation = 'Rundown' AND NOT StartKnown AND StopKnown") == 2,
                "A rundown reports an existing object, not its creation time.");
            Expect(Count(connection, "GraphicsLifetimes", "FirstObservation = 'Stop' AND NOT StartKnown AND StopKnown") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture1D' AND StartKnown AND NOT StopKnown AND EndReason IS NULL") == 1,
                "Missing starts and missing stops must remain distinct from zero and from each other.");
            Expect(Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND StopSeq = 9") == 1,
                "A device-creation operation Stop must not destroy the device.");
            Expect(Count(connection, "GraphicsLifetimes", "LostByEtw = 7 AND DroppedByProfiler = 3") == 6,
                "Derived lifetime rows must disclose capture losses.");
        });
        WithRecording(
        [
            Direct3D(12, 21, kernelResource: 102),
            Allocation(36, 22, 23, 102),
            Direct3D(13, 21, kernelResource: 102),
            Allocation(37, 22, 23, 102)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NULL AND SizeBytes IS NULL") == 1,
                "A missing shared/global allocation must remain unknown rather than inventing ownership or zero bytes.");
        });
        WithRecording(
        [
            Direct3D(12, 1),
            Direct3D(6, 2),
            Allocation(36, 3, 4, 0),
            Allocation(33, 3, 4, 0),
            Direct3D(12, 0, kernelResource: 100),
            Allocation(36, 0, 4, 100),
            Allocation(33, 3, 0, 100)
        ], connection =>
        {
            Expect(Count(connection, "Events") == 7 && Count(connection, "GraphicsObjects") == 4,
                "Zero primary handles must remain raw events rather than fabricate keyed lifetimes.");
            Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation'") == 0 &&
                Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == 1,
                "Zero resource bridges must not correlate private resources, even when their allocation chain is known.");
        });
        Console.WriteLine("Passed: rundown, missing endpoints, zero handles, unobserved shared allocations and capture losses remain explicit.");
    }

    private static void ReusedHandles()
    {
        ProfilerEvent[] events =
        [
            Direct3D(6, 1, kernelResource: 100),
            Allocation(36, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Direct3D(12, 1, kernelResource: 101),
            Allocation(37, 2, 3, 100),
            Allocation(34, 2, 3, 100),
            Allocation(36, 2, 3, 101),
            Allocation(33, 2, 3, 101),
            Direct3D(13, 1, kernelResource: 101),
            Allocation(37, 2, 3, 101),
            Allocation(34, 2, 3, 101)
        ];
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "GraphicsLifetimes", $"Handle = '{Hex(1)}'") == 2 &&
                Count(connection, "GraphicsLifetimes",
                    "Kind = 'Buffer' AND EndReason = 'HandleReuse' AND EndSeq = 4 AND NOT StopKnown AND DurationMs IS NULL") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND StartKnown AND StopSeq = 9") == 1,
                "Pointer reuse across resource kinds must split lifetimes without pretending a destruction was observed.");
            Expect(Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NOT NULL") == 2 &&
                Count(connection, "GraphicsObjects", "Kind IN ('DeviceAllocation', 'AdapterAllocation')") == 4,
                "Reused allocation/global handles need new surrogate IDs and correctly bounded relationships.");
        });
        Console.WriteLine("Passed: reused resource and allocation handles produce separate, bounded lifetimes.");
    }

    private static void DelayedObservations()
    {
        ProfilerEvent[] events =
        [
            Direct3D(12, 1, kernelResource: 100),
            Allocation(36, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Allocation(36, 2, 3, 100),
            Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
            Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 500 },
            Direct3D(14, 1, kernelResource: 999) with { TimestampQpc = 750 },
            Allocation(37, 2, 3, 100),
            Direct3D(13, 1, kernelResource: 100),
            Allocation(34, 2, 3, 100)
        ];
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "GraphicsLinks", """
                Kind = 'AllocationGlobal'
                AND SourceId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'DeviceAllocation' AND StartSeq = 2)
                AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'AdapterAllocation' AND StartSeq = 4)
                """) == 0,
                "An unclosed device allocation must not automatically attach to a reused global pointer.");
            Expect(Count(connection, "GraphicsLinks", """
                Kind = 'AllocationGlobal' AND StartSeq = 5 AND EndSeq = 9
                AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'AdapterAllocation' AND StartSeq = 4)
                """) == 1,
                "A delayed old adapter Stop must not unlink the newer instance's allocation.");
            Expect(Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND StartSeq = 1 AND StopSeq = 10") == 1 &&
                Count(connection, "GraphicsLinks", $"Handle = '{Hex(999)}'") == 0,
                "Delayed Stops and rundown must not end or change a newer resource's mapping.");
        });
        WithRecording(
        [
            Direct3D(12, 1, Device),
            Direct3D(3, Device) with { TimestampQpc = 500 },
            Direct3D(13, 1, Device),
            Direct3D(4, Device)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND StartQpc = 500 AND StopSeq = 4") == 1 &&
                Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1,
                "An earlier device Start must promote a reference placeholder, not fabricate a second device.");
        });
        Console.WriteLine("Passed: delayed observations and global-pointer reuse do not corrupt newer lifetimes.");
    }

    private static void AmbiguousAndChangingBridges()
    {
        ProfilerEvent[] events =
        [
            Direct3D(12, 101, kernelResource: 700),
            Allocation(36, 201, 900, 700),
            Allocation(33, 201, 900, 700),
            Allocation(36, 202, 900, 700),
            Direct3D(6, 102, kernelResource: 700),
            Direct3D(7, 102, kernelResource: 700),
            Allocation(38, 201, 901, 701),
            Direct3D(14, 101, kernelResource: 701),
            Allocation(33, 201, 901, 701, Adapter + 1),
            Allocation(33, 201, 901, 701),
            Allocation(37, 201, 901, 701),
            Allocation(37, 202, 900, 700),
            Direct3D(13, 101, kernelResource: 701),
            Allocation(34, 201, 901, 701),
            Allocation(34, 201, 901, 701, Adapter + 1),
            Allocation(34, 201, 900, 700)
        ];
        WithRecording(events, connection =>
        {
            Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND EndReason = 'AmbiguousIdentity'") == 2 &&
                Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 6") == 2,
                "Ambiguous kernel-resource claims must close links, then resume only once ownership is unique.");
            Expect(Count(connection, "GraphicsResourceAllocations", $"KernelResource = '{Hex(700)}' AND GlobalAllocation = '{Hex(900)}'") == 4 &&
                Count(connection, "GraphicsResourceAllocations", $"KernelResource = '{Hex(701)}' AND GlobalAllocation = '{Hex(901)}'") == 1,
                "Multiple allocations and changed bridge handles must preserve the identity of every historical interval.");
            Expect(Count(connection, "GraphicsLinks", $"""
                Kind = 'AllocationGlobal' AND TargetId IN
                    (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'AdapterAllocation' AND Qualifier = '{Hex(Adapter + 1)}')
                """) == 0,
                "A matching global pointer on another adapter must not correlate.");
        });
        WithRecording(
        [
            Direct3D(12, 101, kernelResource: 700),
            Allocation(36, 201, 900, 700),
            Allocation(38, 201, 901, 701),
            Direct3D(14, 101, kernelResource: 701)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsResourceAllocations",
                "AdapterAllocationId IS NULL AND GlobalAllocation IS NULL AND Adapter IS NULL") == 2,
                "Missing adapter lifetimes must not borrow mutable latest metadata for historical allocation chains.");
            Expect(Count(connection, "GraphicsObjects", $"Kind = 'DeviceAllocation' AND hVidMmGlobalAlloc = '{Hex(901)}'") == 1,
                "The allocation's latest declared handles must still remain available separately from observed links.");
        });
        Console.WriteLine("Passed: multiple allocations, ambiguous claims, changing bridges and adapter scoping stay correct.");
    }

    private static void TrimmedLifetimes()
    {
        List<ProfilerEvent> events =
        [
            Direct3D(3, Device),
            Direct3D(12, 1, Device, 100),
            Allocation(36, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Direct3D(13, 1, Device, 100),
            Allocation(37, 2, 3, 100),
            Allocation(34, 2, 3, 100),
            Direct3D(6, 4, Device, 101)
        ];
        events.AddRange(Enumerable.Range(0, 6_000).Select(_ =>
            new ProfilerEvent(0, 0, 10, 7, XamlProvider.Xaml, false, "UnrelatedEvent", 0, [new("Value", new string('x', 256))])));
        events.Add(Direct3D(7, 4, Device, 101));
        events.Add(Direct3D(4, Device));
        WithRecording(events, connection =>
        {
            Expect(Sql.Scalar(connection, "SELECT CompleteFromSeq FROM SessionInfo") > 8,
                "The retention fixture must really discard the original graphics events.");
            Expect(Count(connection, "GraphicsLifetimes") == 2 &&
                Count(connection, "GraphicsLifetimes", "Trimmed AND StartKnown AND StopKnown") == 2 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Buffer' AND StartSeq = 8 AND StopSeq = 6009") == 1,
                "Trimming must retain open lifetimes and referenced parent devices, including their original creation evidence flags.");
            Expect(Count(connection, "GraphicsObjects", "Kind IN ('DeviceAllocation', 'AdapterAllocation')") == 0 &&
                Count(connection, "GraphicsLinks") == 0 && Count(connection, "GraphicsObjects", "Kind = 'Buffer'") == 1 &&
                Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1,
                "Closed graphics history and its metadata must leave together, without dangling device references.");
        }, limitBytes: 1024 * 1024);
        Console.WriteLine("Passed: rolling retention prunes closed graphics history and preserves open lifetime continuity.");
    }

    private static ProfilerEvent Direct3D(int id, ulong handle, ulong parent = 0, uint kernelResource = 0)
    {
        var kind = (id - 3) / 3;
        string[] names = ["Device", "Buffer", "Texture1D", "Texture2D", "Texture3D"];
        string[] phases = ["Start", "Stop", "DCStart"];
        EventField[] fields = kind == 0
            ? [new("pID3D11Device", handle), new("hKMDevice", 0x43U), new("pIDXGIDevice", Device + 20)]
            : [new("pID3D11Resource", handle), new("pID3D11Device", parent), new("hKMResource", kernelResource)];
        return new(0, 0, 10, 7, XamlProvider.Direct3D11, false, $"{names[kind]}/{phases[(id - 3) % 3]}",
            (id - 3) % 3 + 1, fields, EventId: id, Version: 0);
    }

    private static ProfilerEvent Allocation(int id, ulong handle, ulong global, uint resource, ulong adapter = Adapter) =>
        new(0, 0, 10, 70, XamlProvider.DxgKrnl, false, id < 36 ? "AdapterAllocation" : "DeviceAllocation", (id - 33) % 3 + 1,
        [
            new("hProcessId", 10UL), new("hVidMmAlloc", handle), new("hVidMmGlobalAlloc", global),
            new("hDevice", Device), new("pDxgAdapter", adapter), new("hThunkResource", (ulong)resource),
            new("hThunkAllocation", handle + 10), new("hDxgResource", handle + 20),
            new("hDxgSharedResource", handle + 30), new("allocSize", ulong.MaxValue)
        ], EventId: id, Version: id < 36 ? 3 : 2);

    private static void WithRecording(IEnumerable<ProfilerEvent> events, Action<SqliteConnection> inspect,
        long limitBytes = SessionWriter.DefaultLimitBytes)
    {
        var path = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-GraphicsIndex-{Guid.NewGuid():N}.sqlite");
        try
        {
            using (var writer = SessionWriter.Create(path, new(10, "GraphicsIndexTests", null, 1_000_000, 1_000, SessionCoverage.Trace), limitBytes))
            {
                long sequence = 0;
                foreach (var item in events)
                {
                    sequence++;
                    var qpc = item.TimestampQpc == 0 ? sequence * 1_000 : item.TimestampQpc;
                    writer.Process(sequence, item with { TimestampQpc = qpc, RelativeMilliseconds = qpc / 1_000.0 });
                }
                writer.Close("Test complete", lostByEtw: 7, droppedByProfiler: 3);
                if (writer.Failure is { } failure) throw new InvalidOperationException("Graphics recording failed.", failure);
            }
            using var connection = new SqliteConnection($"Data Source={path};Pooling=False");
            connection.Open();
            inspect(connection);
            Expect(Sql.Scalar(connection, "SELECT COUNT(*) FROM pragma_foreign_key_check") == 0,
                "Graphics indexing and trimming must preserve every declared foreign key.");
        }
        finally
        {
            foreach (var file in new[] { path, path + "-wal", path + "-shm" }) File.Delete(file);
        }
    }

    private static long Count(SqliteConnection connection, string table, string predicate = "1") =>
        Sql.Scalar(connection, $"SELECT COUNT(*) FROM {table} WHERE {predicate}");

    private static string Hex(ulong value) => $"0x{value:x16}";

    private static void Expect(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
}
