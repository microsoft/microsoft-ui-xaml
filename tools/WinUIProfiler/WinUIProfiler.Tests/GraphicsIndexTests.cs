// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

internal static class GraphicsIndexTests
{
    private const ulong Device = 0xfedcba9876543210;
    private const ulong Adapter = 0xffffa00000001234;

    internal static void Run()
    {
        UnsignedHandleSearch();
        KindChangingStops();
        HistoricalObservations();
        PromotedDeviceBounds();
        ImplicitDeviceReferences();
        ImplicitDeviceLifetimeBounds();
        PartialDeviceBounds();
        RundownStartPromotion();
        HistoricalRundownEvidence();
        HistoricalDeviceReferences();
        HistoricalStartBounds();
        StoppedPartialHistoryBounds();
        ResourceDeviceReuse();
        StaleMappings();
        DelayedGlobalStops();
        ReusedGlobalBridges();
        DelayedGlobalSources();
        PromotedGlobalEvidence();
        StopOnlyGlobalBridges();
        ClearedBridges();
        IndependentAllocationLinks();
        PartialAllocationIntervals();
        CompleteLifetimes(reverse: false);
        CompleteLifetimes(reverse: true);
        IncompleteLifetimes();
        ReusedHandles();
        DelayedObservations();
        AmbiguousAndChangingBridges();
        DisjointLifetimes();
        PromotedDisjointLifetimes();
        RefinedGlobalEnds();
        SinglePassGraphicsTrim();
        TrimmedLifetimes();
    }

    private static void UnsignedHandleSearch()
    {
        ProfilerEvent Value(string name, object value) => new(0, 0, 10, 7, XamlProvider.Xaml, false, name, 0, [new("Value", value)]);
        ProfilerEvent[] events =
        [
            Direct3D(12, ulong.MaxValue),
            Direct3D(12, ulong.MaxValue - 1),
            Direct3D(12, 1UL << 63),
            Value("SignedAddress", -1L),
            Value("SignedMinimum", long.MinValue),
            Value("NumericText", ulong.MaxValue.ToString(CultureInfo.InvariantCulture)),
            Value("RealValue", 64.0)
        ];
        WithRecording(events, connection =>
        {
            (string Filter, int[] Matches)[] cases =
            [
                (Hex(ulong.MaxValue), [0, 3]),
                ("0XFFFFFFFFFFFFFFFF", [0, 3]),
                (ulong.MaxValue.ToString(CultureInfo.InvariantCulture), [0]),
                (" +018446744073709551615 ", [0]),
                (Hex(ulong.MaxValue - 1), [1]),
                ((ulong.MaxValue - 1).ToString(CultureInfo.InvariantCulture), [1]),
                ((ulong.MaxValue - 2).ToString(CultureInfo.InvariantCulture), []),
                (Hex(1UL << 63), [2, 4]),
                ((1UL << 63).ToString(CultureInfo.InvariantCulture), [2]),
                ("-1", [3]),
                (long.MinValue.ToString(CultureInfo.InvariantCulture), [4]),
                ("NumericText", [5]),
                ("64", [6])
            ];
            foreach (var (filter, expected) in cases)
            {
                var matches = new List<ProfilerEvent>();
                var count = SessionFile.CountSpanEvents(connection.DataSource, 7, 1, events.Length, null, filter);
                var read = SessionFile.ReadSpanEvents(connection.DataSource, 7, 1, events.Length, null, filter, 0, -1,
                    (data, _, _) => matches.Add(data));
                Expect(count == expected.Length && read == count &&
                    matches.Select(data => data.Fields[0].Value).SequenceEqual(expected.Select(index => events[index].Fields[0].Value)),
                    $"Numeric search '{filter}' must distinguish unsigned neighbors, preserve signed addresses and exclude numeric strings.");
            }
        });
        Console.WriteLine("Passed: hex/decimal searches preserve full-width unsigned handles, signed addresses and numeric precision.");
    }

    private static void KindChangingStops()
    {
        WithRecording(
        [
            Direct3D(12, 101, kernelResource: 700),
            Allocation(36, 201, 900, 700),
            Direct3D(6, 102, kernelResource: 700),
            Direct3D(13, 102, kernelResource: 700),
            new(0, 0, 10, 7, XamlProvider.Xaml, false, "AfterGraphicsStop", 0, [])
        ], connection =>
        {
            Expect(Count(connection, "Events") == 5 && Count(connection, "EventDetails", "Name = 'AfterGraphicsStop'") == 1,
                "A kind-changing Stop must not abort the raw batch or stop subsequent XAML recording.");
            Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 4 AND EndSeq IS NULL") == 1 &&
                Count(connection, "GraphicsLinks", "EndSeq = StartSeq") == 0,
                "Relationships must reflect final membership once per event, without transient zero-length intervals.");
        });
        Console.WriteLine("Passed: ambiguous, kind-changing Stops preserve graphics links and subsequent XAML recording.");
    }

    private static void HistoricalObservations()
    {
        WithRecording(
        [
            Direct3D(12, 1, kernelResource: 100),
            Direct3D(12, 1, kernelResource: 101) with { TimestampQpc = 3_000 },
            Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 2_000 },
            Allocation(36, 2, 3, 101)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == 2 &&
                Count(connection, "GraphicsLifetimes", "StartSeq = 1 AND StopSeq = 3 AND DurationMs = 1") == 1,
                "A delayed Stop must complete its unique historical lifetime instead of creating a third object.");
            Expect(Count(connection, "GraphicsObjects", "StartSeq = 2 AND EndSeq IS NULL") == 1 &&
                Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 4 AND EndSeq IS NULL") == 1,
                "Completing historical evidence must not close or remap the replacement lifetime.");
        });
        foreach (var phase in new[] { 12, 14 })
        {
            WithRecording(
            [
                Direct3D(12, 1, kernelResource: 100),
                Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 3_000 },
                Direct3D(phase, 1, kernelResource: 100) with { TimestampQpc = phase == 12 ? 1_000 : 2_000 },
                Allocation(36, 2, 3, 100),
                Direct3D(12, 4, kernelResource: 100)
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects", $"Kind = 'Texture2D' AND Handle = '{Hex(1)}'") == 1 &&
                    Count(connection, "GraphicsObjectEvents", """
                        Seq = 3 AND GraphicsObjectId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE StartSeq = 1)
                        """) == 1,
                    "Late Start/rundown must attach to retained closed history, not resurrect an open resource.");
                Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 5 AND EndSeq IS NULL") == 1,
                    "Old evidence must not make a replacement resource's bridge ambiguous.");
            });
        }
        WithRecording(
        [
            Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 3_000 },
            Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsLifetimes", "StartSeq = 2 AND StopSeq = 1 AND DurationMs = 2") == 1 &&
                Count(connection, "GraphicsObjects") == 1,
                "A delayed creation can complete a unique stop-only lifetime without reopening it.");
        });
        WithRecording(
        [
            Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 3_000 },
            Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 4_000 },
            Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_000 },
            Allocation(36, 2, 3, 100)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == 3 &&
                Count(connection, "GraphicsObjects", "Kind = 'Texture2D' AND EndSeq IS NULL") == 0 &&
                Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation'") == 0,
                "An observation with multiple possible historical matches must remain separate and non-correlatable.");
        });
        Console.WriteLine("Passed: delayed endpoints match unique retained lifetimes; ambiguous history never resurrects mappings.");
    }

    private static void PromotedDeviceBounds()
    {
        WithRecording(
        [
            Direct3D(12, 1, Device),
            Direct3D(3, Device) with { TimestampQpc = 500 },
            Direct3D(6, 2, Device) with { TimestampQpc = 700 },
            Direct3D(7, 2, Device) with { TimestampQpc = 800 },
            Direct3D(13, 1, Device),
            Direct3D(4, Device)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Buffer' AND DeviceId IS NOT NULL AND DurationMs = 0.1") == 1,
                "Resources between a promoted device's actual Start and its first reference must use that device lifetime.");
        });
        Console.WriteLine("Passed: promoted device creation bounds apply to earlier resource lifetimes.");
    }

    private static void ImplicitDeviceReferences()
    {
        foreach (var explicitParent in new[] { false, true })
        {
            foreach (var stopQpc in new[] { 2_000L, 4_000L })
            {
                var resourceStop = Direct3D(13, 1, Device) with { TimestampQpc = 3_000 };
                if (!explicitParent)
                    resourceStop = resourceStop with
                    {
                        Fields = resourceStop.Fields.Where(field => field.Name != "pID3D11Device").ToArray()
                    };
                WithRecording(
                [
                    Direct3D(3, Device) with { TimestampQpc = 100 },
                    Direct3D(12, 1, Device) with { TimestampQpc = 1_000 },
                    resourceStop,
                    Direct3D(4, Device) with { TimestampQpc = stopQpc }
                ], connection =>
                {
                    var complete = stopQpc > 3_000;
                    Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == (complete ? 1 : 2) &&
                        Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND DurationMs IS NOT NULL") == (complete ? 1 : 0),
                        "A device Stop must respect its latest resource reference even when the owning-device field is omitted.");
                    Expect(Count(connection, "GraphicsObjects",
                        $"Kind = 'Device' AND StartQpc = 100 AND LastQpc = {(complete ? stopQpc : 3_000)}") == 1 &&
                        Count(connection, "GraphicsObjectEvents", """
                            Seq = 3 AND Role = 'Reference' AND GraphicsObjectId =
                                (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'Device' AND StartQpc = 100)
                            """) == 1 &&
                        Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND DurationMs = 2") == 1 &&
                        Count(connection, "Events") == 4,
                        "Implicit references must preserve parent evidence, complete resources and raw events without suppressing valid device Stops.");
                });
            }
        }
        foreach (var replacement in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Direct3D(3, Device) with { TimestampQpc = 100 },
                Direct3D(4, Device) with { TimestampQpc = 4_000 }
            ];
            if (replacement) events.Add(Direct3D(3, Device) with { TimestampQpc = 5_000 });
            var resourceStop = Direct3D(13, 1) with { TimestampQpc = 3_000 };
            events.AddRange(
            [
                Direct3D(12, 1, Device) with { TimestampQpc = 2_000 },
                resourceStop with { Fields = resourceStop.Fields.Where(field => field.Name != "pID3D11Device").ToArray() }
            ]);
            WithRecording(events, connection =>
            {
                Expect(Sql.Scalar(connection, """
                    SELECT COUNT(*) FROM GraphicsLifetimes r JOIN GraphicsObjects d ON d.GraphicsObjectId = r.DeviceId
                    WHERE r.Kind = 'Texture2D' AND r.StartQpc = 2000 AND r.StopQpc = 3000
                        AND d.StartQpc = 100 AND d.StopQpc = 4000 AND d.LastQpc = 4000
                    """) == 1 &&
                    Count(connection, "GraphicsObjects", "Kind = 'Device'") == (replacement ? 2 : 1) &&
                    (!replacement || Count(connection, "GraphicsObjects",
                        "Kind = 'Device' AND StartQpc = 5000 AND KnownFromQpc = 5000 AND LastQpc = 5000 AND EndSeq IS NULL") == 1),
                    "Implicit historical references must stay on the original device and leave its active replacement untouched.");
            });
        }
        Console.WriteLine("Passed: implicit resource references maintain device bounds without changing historical owners or replacements.");
    }

    private static void ImplicitDeviceLifetimeBounds()
    {
        foreach (var startKnown in new[] { false, true })
        {
            foreach (var stopped in new[] { false, true })
            {
                foreach (var (phase, qpc) in new[] { (13, 4_000L), (13, 3_000L), (12, 500L), (12, 1_000L) })
                {
                    List<ProfilerEvent> events =
                    [
                        Direct3D(startKnown ? 3 : 5, Device) with { TimestampQpc = 1_000 },
                        Direct3D(14, 1, Device) with { TimestampQpc = 2_000 },
                        Direct3D(stopped ? 4 : 3, Device) with { TimestampQpc = 3_000 }
                    ];
                    var observation = Direct3D(phase, 1) with { TimestampQpc = qpc };
                    events.Add(observation with
                    {
                        Fields = observation.Fields.Where(field => field.Name != "pID3D11Device").ToArray()
                    });
                    WithRecording(events, connection =>
                    {
                        var valid = qpc is >= 1_000 and <= 3_000;
                        var last = Math.Max(stopped ? 3_000 : 2_000, valid ? qpc : 0);
                        Expect(Count(connection, "GraphicsObjects", $"""
                            Kind = 'Device' AND FirstQpc = 1000 AND KnownFromQpc = 1000 AND LastQpc = {last}
                                AND EndQpc = 3000 AND {(startKnown ? "StartQpc = 1000" : "StartQpc IS NULL")}
                            """) == 1 &&
                            Count(connection, "GraphicsObjectEvents", """
                                Seq = 4 AND Role = 'Reference' AND GraphicsObjectId =
                                    (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'Device' AND FirstQpc = 1000)
                                """) == (valid ? 1 : 0),
                            "Implicit references must stay within the retained parent's known bounds, including exact endpoint timestamps.");
                        Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == (stopped ? 1 : 2) &&
                            (stopped || Count(connection, "GraphicsObjects",
                                "Kind = 'Device' AND StartQpc = 3000 AND KnownFromQpc = 3000 AND LastQpc = 3000 AND EndSeq IS NULL") == 1) &&
                            Count(connection, "Events") == events.Count,
                            "Rejected parent references must preserve the resource event and leave any reused-pointer device untouched.");
                    });
                }
            }
        }
        Console.WriteLine("Passed: implicit references respect stopped/reused device bounds without extending partial or complete parents.");
    }

    private static void HistoricalDeviceReferences()
    {
        foreach (var replacement in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Direct3D(3, Device) with { TimestampQpc = 1_000 },
                Direct3D(4, Device) with { TimestampQpc = 3_000 }
            ];
            if (replacement) events.Add(Direct3D(3, Device) with { TimestampQpc = 4_000 });
            events.AddRange(
            [
                Direct3D(12, 1, Device) with { TimestampQpc = 2_000 },
                Direct3D(13, 1, Device) with { TimestampQpc = 2_500 }
            ]);
            WithRecording(events, connection =>
            {
                Expect(Sql.Scalar(connection, """
                    SELECT COUNT(*) FROM GraphicsLifetimes r JOIN GraphicsObjects d ON d.GraphicsObjectId = r.DeviceId
                    WHERE r.Kind = 'Texture2D' AND r.StartQpc = 2000 AND r.StopQpc = 2500
                        AND d.StartQpc = 1000 AND d.StopQpc = 3000 AND d.LastQpc = 3000
                    """) == 1,
                    "Delayed resource events must resolve their unique historical device even when its pointer has been reused.");
                Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == (replacement ? 2 : 1) &&
                    (!replacement || Count(connection, "GraphicsObjects", "Kind = 'Device' AND StartQpc = 4000 AND LastQpc = 4000 AND EndSeq IS NULL") == 1),
                    "Historical resource evidence must not create another device or modify the active replacement.");
            });
        }
        Console.WriteLine("Passed: delayed resources retain their historical device across device-pointer reuse.");
    }

    private static void PartialDeviceBounds()
    {
        foreach (var closed in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Direct3D(12, 1, Device) with { TimestampQpc = 1_000 },
                Direct3D(5, Device) with { TimestampQpc = 500 }
            ];
            if (closed) events.Add(Direct3D(4, Device) with { TimestampQpc = 2_000 });
            events.AddRange(
            [
                Direct3D(6, 2, Device) with { TimestampQpc = 700 },
                Direct3D(7, 2, Device) with { TimestampQpc = 800 },
                Direct3D(13, 1, Device) with { TimestampQpc = 1_500 }
            ]);
            if (!closed) events.Add(Direct3D(4, Device) with { TimestampQpc = 2_000 });
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1 &&
                    Count(connection, "GraphicsObjects", "DeviceId IS NOT NULL") == 2 &&
                    Count(connection, "GraphicsLifetimes", "Kind = 'Buffer' AND DurationMs = 0.1") == 1,
                    "Earlier device rundown must extend the accepted observation bound for active and historical references.");
                Expect(Count(connection, "GraphicsObjects", "Kind = 'Device' AND FirstQpc = 1000 AND StartQpc IS NULL AND StopQpc = 2000") == 1,
                    "Accepting earlier existence evidence must preserve the first-arrival timestamp and unknown creation.");
            });
        }
        List<ProfilerEvent> retained =
        [
            Direct3D(12, 1, Device) with { TimestampQpc = 1_000 },
            Direct3D(5, Device) with { TimestampQpc = 500 },
            Direct3D(4, Device) with { TimestampQpc = 2_000 }
        ];
        retained.AddRange(Enumerable.Range(0, 6_000).Select(_ =>
            new ProfilerEvent(0, 0, 10, 7, XamlProvider.Xaml, false, "UnrelatedEvent", 0, [new("Value", new string('x', 256))])));
        retained.Add(Direct3D(6, 2, Device) with { TimestampQpc = 700 });
        retained.Add(Direct3D(7, 2, Device) with { TimestampQpc = 800 });
        WithRecording(retained, connection =>
        {
            Expect(Sql.Scalar(connection, "SELECT CompleteFromSeq FROM SessionInfo") > 3 &&
                    Count(connection, "GraphicsObjects", "Kind = 'Device' AND FirstQpc = 1000 AND KnownFromSeq = 2 AND KnownFromQpc = 500 AND Trimmed") == 1 &&
                    Count(connection, "GraphicsObjects", "DeviceId IS NOT NULL") == 2,
                    "Historical device observation bounds must survive trimming of their original raw evidence.");
        }, limitBytes: 1024 * 1024);
        WithRecording(
        [
            Direct3D(12, 1, Device) with { TimestampQpc = 1_000 },
            Direct3D(3, Device) with { TimestampQpc = 2_000 },
            Direct3D(6, 2, Device) with { TimestampQpc = 2_500 },
            Direct3D(13, 1, Device) with { TimestampQpc = 1_500 },
            Direct3D(7, 2, Device) with { TimestampQpc = 2_600 },
            Direct3D(4, Device) with { TimestampQpc = 3_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 2 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND FirstObservation = 'Reference' AND StartQpc IS NULL AND EndQpc = 2000") == 1,
                "A Start later than a device's reference must create a replacement, not promote the earlier placeholder.");
            Expect(Sql.Scalar(connection, """
                SELECT COUNT(*) FROM GraphicsObjects r JOIN GraphicsObjects d ON d.GraphicsObjectId = r.DeviceId
                WHERE (r.Kind = 'Texture2D' AND d.FirstObservation = 'Reference' AND d.StartQpc IS NULL)
                    OR (r.Kind = 'Buffer' AND d.StartQpc = 2000 AND d.StopQpc = 3000)
                """) == 2,
                "Resources before and after a later device Start must retain their distinct owning device lifetimes.");
        });
        Console.WriteLine("Passed: partial device bounds accept earlier rundown but reject later creation as placeholder promotion.");
    }

    private static void RundownStartPromotion()
    {
        (ProfilerEvent Start, ProfilerEvent Stop, ProfilerEvent Rundown)[] identities =
        [
            (Direct3D(3, Device), Direct3D(4, Device), Direct3D(5, Device)),
            (Direct3D(12, 1), Direct3D(13, 1), Direct3D(14, 1)),
            (Allocation(33, 2, 3, 100), Allocation(34, 2, 3, 100), Allocation(35, 2, 3, 100)),
            (Allocation(36, 2, 3, 100), Allocation(37, 2, 3, 100), Allocation(38, 2, 3, 100))
        ];
        foreach (var (start, stop, rundown) in identities)
        {
            WithRecording(
            [
                rundown with { TimestampQpc = 2_000 },
                start with { TimestampQpc = 1_000 },
                stop with { TimestampQpc = 3_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 1 &&
                    Count(connection, "GraphicsLifetimes", "FirstObservation = 'Rundown' AND StartSeq = 2 AND StopSeq = 3 AND DurationMs = 2") == 1 &&
                    Count(connection, "GraphicsObjectEvents") == 3,
                    "An earlier Start must complete the unique compatible active rundown lifetime.");
            });
            WithRecording(
            [
                start with { TimestampQpc = 1_000 },
                stop with { TimestampQpc = 2_000 },
                rundown with { TimestampQpc = 4_000 },
                start with { TimestampQpc = 500 },
                stop with { TimestampQpc = 5_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 3 &&
                    Count(connection, "GraphicsLifetimes", "StartQpc = 500 AND StopQpc IS NULL AND EndQpc = 1000") == 1 &&
                    Count(connection, "GraphicsLifetimes", "FirstQpc = 4000 AND StartQpc IS NULL AND StopQpc = 5000") == 1,
                    "An earlier Start must not promote active rundown across an intervening known lifetime.");
            });
            WithRecording(
            [
                stop with { TimestampQpc = 2_000 },
                rundown with { TimestampQpc = 4_000 },
                start with { TimestampQpc = 1_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 3 &&
                    Count(connection, "GraphicsLifetimes", "DurationMs IS NOT NULL") == 0,
                    "An earlier Start with both closed and active partial candidates must remain ambiguous.");
            });
        }
        Console.WriteLine("Passed: earlier Starts promote only unique rundown lifetimes within proven reuse bounds.");
    }

    private static void HistoricalRundownEvidence()
    {
        (ProfilerEvent Start, ProfilerEvent Stop, ProfilerEvent Rundown)[] identities =
        [
            (Direct3D(3, Device), Direct3D(4, Device), Direct3D(5, Device)),
            (Direct3D(12, 1), Direct3D(13, 1), Direct3D(14, 1)),
            (Allocation(33, 2, 3, 100), Allocation(34, 2, 3, 100), Allocation(35, 2, 3, 100)),
            (Allocation(36, 2, 3, 100), Allocation(37, 2, 3, 100), Allocation(38, 2, 3, 100))
        ];
        foreach (var (start, stop, rundown) in identities)
        {
            WithRecording(
            [
                stop with { TimestampQpc = 3_000 },
                rundown with { TimestampQpc = 2_000 },
                start with { TimestampQpc = 1_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 1 &&
                    Count(connection, "GraphicsLifetimes", "FirstObservation = 'Stop' AND StartSeq = 3 AND StopSeq = 1 AND DurationMs = 2") == 1 &&
                    Count(connection, "GraphicsObjectEvents") == 3,
                    "Earlier rundown must join a unique stop-only lifetime without fabricating a reuse boundary.");
            });
            WithRecording(
            [
                start with { TimestampQpc = 1_000 },
                stop with { TimestampQpc = 2_000 },
                stop with { TimestampQpc = 4_000 },
                rundown with { TimestampQpc = 500 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 3 &&
                    Count(connection, "GraphicsLifetimes", "FirstQpc = 500 AND StopQpc IS NULL AND EndQpc = 1000") == 1,
                    "Earlier rundown must not cross an intervening known lifetime to match a later stop-only row.");
            });
            WithRecording(
            [
                start with { TimestampQpc = 3_000 },
                stop with { TimestampQpc = 4_000 },
                start with { TimestampQpc = 1_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects") == 2 &&
                    Count(connection, "GraphicsLifetimes", "StartQpc = 1000 AND StopQpc IS NULL AND EndQpc = 3000 AND EndSeq = 1") == 1,
                    "A detached earlier lifetime must end at the replacement's beginning, not its later Stop.");
            });
        }
        WithRecording(
        [
            Direct3D(4, Device) with { TimestampQpc = 3_000 },
            Direct3D(12, 1, Device) with { TimestampQpc = 2_000 },
            Direct3D(3, Device) with { TimestampQpc = 1_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND StartSeq = 3 AND StopSeq = 1 AND DurationMs = 2") == 1 &&
                Count(connection, "GraphicsObjects", "Kind = 'Texture2D' AND DeviceId IS NOT NULL") == 1,
                "An earlier resource reference must match its unique stop-only parent device.");
        });
        WithRecording(
        [
            Direct3D(12, 1, Device) with { TimestampQpc = 1_000 },
            Direct3D(3, Device) with { TimestampQpc = 500 },
            Direct3D(4, Device) with { TimestampQpc = 2_000 },
            Direct3D(3, Device) with { TimestampQpc = 100 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == 2 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Device' AND StartQpc = 100 AND StopQpc IS NULL AND EndQpc = 500 AND EndSeq = 2") == 1,
                "A historical reuse bound must use the sequence of the earliest accepted observation, not the first arrival.");
        });
        Console.WriteLine("Passed: stop-only lifetimes accept unique earlier existence evidence within historical bounds.");
    }

    private static void HistoricalStartBounds()
    {
        (ProfilerEvent Start, ProfilerEvent Stop)[] identities =
        [
            (Direct3D(3, Device), Direct3D(4, Device)),
            (Direct3D(12, 1), Direct3D(13, 1)),
            (Allocation(33, 2, 3, 100), Allocation(34, 2, 3, 100)),
            (Allocation(36, 2, 3, 100), Allocation(37, 2, 3, 100))
        ];
        foreach (var (start, stop) in identities)
        {
            foreach (var qpc in new[] { 500L, 2_000L, 3_000L })
            {
                WithRecording(
                [
                    start with { TimestampQpc = 1_000 },
                    stop with { TimestampQpc = 2_000 },
                    stop with { TimestampQpc = 4_000 },
                    start with { TimestampQpc = qpc }
                ], connection =>
                {
                    if (qpc < 2_000)
                    {
                        Expect(Count(connection, "GraphicsObjects") == 3 &&
                            Count(connection, "GraphicsLifetimes", "StartQpc = 500 AND StopQpc IS NULL AND EndQpc = 1000 AND DurationMs IS NULL") == 1 &&
                            Count(connection, "GraphicsLifetimes", "StartQpc IS NULL AND StopQpc = 4000") == 1,
                            "A delayed Start must not complete a later Stop across an intervening known lifetime.");
                    }
                    else
                    {
                        Expect(Count(connection, "GraphicsObjects") == 2 &&
                            Count(connection, "GraphicsLifetimes", $"StartQpc = {qpc} AND StopQpc = 4000 AND DurationMs IS NOT NULL") == 1,
                            "A delayed Start after the preceding lifetime, including its exact Stop timestamp, must still complete its unique Stop.");
                    }
                    Expect(Count(connection, "GraphicsLifetimes", "StartQpc = 1000 AND StopQpc = 2000 AND DurationMs = 1") == 1,
                        "A delayed Start must preserve the intervening lifetime's original evidence.");
                });
            }
        }
        WithRecording(
        [
            Direct3D(8, 1) with { TimestampQpc = 1_000 },
            Direct3D(7, 1) with { TimestampQpc = 2_000 },
            Direct3D(13, 1) with { TimestampQpc = 4_000 },
            Direct3D(12, 1, kernelResource: 101) with { TimestampQpc = 6_000 },
            Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 500 },
            Allocation(36, 2, 3, 101) with { TimestampQpc = 7_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind IN ('Buffer', 'Texture2D')") == 4 &&
                Count(connection, "GraphicsLifetimes", "StartQpc = 500 AND EndQpc = 1000 AND StopQpc IS NULL") == 1,
                "Historical Start bounds must span all resource kinds and use the earliest boundary, not the active replacement.");
            Expect(Count(connection, "GraphicsObjects", "StartQpc = 6000 AND EndSeq IS NULL") == 1 &&
                Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 6 AND EndSeq IS NULL") == 1,
                "Detached historical evidence must not close or remap the active replacement.");
        });
        WithRecording(
        [
            Direct3D(3, Device + 1) with { TimestampQpc = 100 },
            Direct3D(3, Device) with { TimestampQpc = 1_000 },
            Direct3D(14, 1, Device) with { TimestampQpc = 1_500 },
            Direct3D(13, 1, Device) with { TimestampQpc = 2_000 },
            Direct3D(13, 1, Device + 1) with { TimestampQpc = 5_000 },
            Direct3D(12, 1, Device + 1) with { TimestampQpc = 500 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == 3 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND StartQpc = 500 AND StopQpc IS NULL AND EndQpc = 1500") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND StartQpc IS NULL AND StopQpc = 5000") == 1,
                "An intervening resource with another known owner must bound a delayed Start even when its own creation was not observed.");
        });
        WithRecording(
        [
            Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
            Allocation(34, 2, 3, 100) with { TimestampQpc = 2_000 },
            Allocation(34, 2, 3, 100, Adapter + 1) with { TimestampQpc = 4_000 },
            Allocation(33, 2, 3, 100, Adapter + 1) with { TimestampQpc = 500 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects") == 2 &&
                Count(connection, "GraphicsLifetimes", $"Qualifier = '{Hex(Adapter + 1)}' AND StartQpc = 500 AND StopQpc = 4000") == 1,
                "A matching pointer on another adapter must not impose a reuse bound on this allocation.");
        });
        Console.WriteLine("Passed: delayed Starts respect intervening lifetimes, resource kinds and qualified identity spaces.");
    }

    private static void StoppedPartialHistoryBounds()
    {
        (ProfilerEvent Start, ProfilerEvent Stop, ProfilerEvent Rundown)[] identities =
        [
            (Direct3D(3, Device), Direct3D(4, Device), Direct3D(5, Device)),
            (Direct3D(12, 1), Direct3D(13, 1), Direct3D(14, 1)),
            (Allocation(33, 2, 3, 100), Allocation(34, 2, 3, 100), Allocation(35, 2, 3, 100)),
            (Allocation(36, 2, 3, 100), Allocation(37, 2, 3, 100), Allocation(38, 2, 3, 100))
        ];
        foreach (var (start, stop, rundown) in identities)
        {
            foreach (var closed in new[] { false, true })
            {
                foreach (var qpc in new[] { 500L, 2_000L, 5_000L, 5_500L })
                {
                    List<ProfilerEvent> events =
                    [
                        rundown with { TimestampQpc = 1_000 },
                        stop with { TimestampQpc = 5_000 },
                        rundown with { TimestampQpc = 6_000 }
                    ];
                    if (closed) events.Add(stop with { TimestampQpc = 7_000 });
                    events.Add(start with { TimestampQpc = qpc });
                    if (!closed) events.Add(stop with { TimestampQpc = 7_000 });
                    WithRecording(events, connection =>
                    {
                        var blocked = qpc < 5_000;
                        Expect(Count(connection, "GraphicsObjects") == (blocked ? 3 : 2) &&
                            Count(connection, "GraphicsLifetimes", "FirstQpc = 1000 AND StartQpc IS NULL AND StopQpc = 5000") == 1 &&
                            Count(connection, "GraphicsLifetimes",
                                $"FirstQpc = 6000 AND {(blocked ? "StartQpc IS NULL" : $"StartQpc = {qpc}")} AND StopQpc = 7000") == 1 &&
                            Count(connection, "GraphicsLifetimes", "DurationMs IS NOT NULL") == (blocked ? 0 : 1),
                            "A delayed Start must not cross a stopped partial lifetime, but may follow its Stop, including the exact timestamp.");
                        Expect(Count(connection, "Events") == events.Count &&
                            Count(connection, "GraphicsObjectEvents", "Role <> 'Reference'") == events.Count,
                            "Partial-history bounds must preserve every raw event and its lifecycle evidence.");
                    });
                }
            }
        }
        Console.WriteLine("Passed: stopped partial history bounds active and closed candidates without rejecting valid late creation.");
    }

    private static void ResourceDeviceReuse()
    {
        foreach (var samePointer in new[] { false, true })
        {
            var replacement = samePointer ? Device : Device + 1;
            WithRecording(
            [
                Direct3D(3, Device) with { TimestampQpc = 1_000 },
                Direct3D(12, 1, Device, 100) with { TimestampQpc = 2_000 },
                Allocation(36, 2, 3, 100) with { TimestampQpc = 2_500 },
                Direct3D(4, Device) with { TimestampQpc = 3_000 },
                Direct3D(3, replacement) with { TimestampQpc = 4_000 },
                Direct3D(14, 1, replacement, 101) with { TimestampQpc = 5_000 },
                Direct3D(14, 1, Device, 100) with { TimestampQpc = 2_600 },
                Allocation(36, 4, 5, 101) with { TimestampQpc = 5_500 },
                Direct3D(13, 1, replacement, 101) with { TimestampQpc = 6_000 }
            ], connection =>
            {
                Expect(Count(connection, "Events") == 9 &&
                    Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND DurationMs IS NULL") == 2 &&
                    Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND StartQpc = 2000 AND StopQpc IS NULL AND EndReason = 'HandleReuse'") == 1,
                    "A changed owning device must split a reused resource without fabricating a complete lifetime.");
                Expect(Sql.Scalar(connection, """
                    SELECT COUNT(*) FROM GraphicsLifetimes r JOIN GraphicsObjects d ON d.GraphicsObjectId = r.DeviceId
                    WHERE r.Kind = 'Texture2D' AND r.FirstObservation = 'Rundown' AND r.FirstQpc = 5000
                        AND NOT r.StartKnown AND r.StopQpc = 6000 AND d.StartQpc = 4000
                    """) == 1 &&
                    Count(connection, "GraphicsObjects", "Kind = 'Device' AND StartQpc = 1000 AND StopQpc = 3000 AND LastQpc = 3000") == 1,
                    "The replacement must use its actual device lifetime, without extending the destroyed parent.");
                Expect(Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == 2 &&
                    Count(connection, "GraphicsLinks", $"Kind = 'ResourceAllocation' AND Handle = '{Hex(100)}' AND EndSeq = 6") == 1 &&
                    Count(connection, "GraphicsLinks", $"Kind = 'ResourceAllocation' AND Handle = '{Hex(101)}' AND StartSeq = 8 AND EndSeq = 9") == 1,
                    "Late evidence for the old owner must not reopen old bridges or interrupt the replacement's links.");
            });
        }
        WithRecording(
        [
            Direct3D(3, Device) with { TimestampQpc = 1_000 },
            Direct3D(12, 1, Device) with { TimestampQpc = 2_000 },
            Direct3D(4, Device) with { TimestampQpc = 3_000 },
            Direct3D(3, Device + 1) with { TimestampQpc = 4_000 },
            Direct3D(13, 1, Device + 1) with { TimestampQpc = 6_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == 2 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND FirstObservation = 'Stop' AND NOT StartKnown AND StopQpc = 6000") == 1 &&
                Count(connection, "GraphicsLifetimes", "Kind = 'Texture2D' AND DurationMs IS NOT NULL") == 0,
                "Even a stop-only replacement must not complete an older resource owned by a different device.");
        });
        Console.WriteLine("Passed: device changes split reused resources and preserve historical owners and allocation links.");
    }

    private static void StaleMappings()
    {
        WithRecording(
        [
            Direct3D(12, 1, kernelResource: 100),
            Allocation(36, 2, 3, 100),
            Allocation(38, 2, 4, 101) with { TimestampQpc = 4_000 },
            Direct3D(14, 1, kernelResource: 101) with { TimestampQpc = 5_000 },
            Allocation(38, 2, 3, 100) with { TimestampQpc = 3_000 },
            Direct3D(14, 1, kernelResource: 100) with { TimestampQpc = 2_500 },
            Allocation(33, 2, 4, 101)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", $"Kind = 'Texture2D' AND hKMResource = '{Hex(101)}' AND LastQpc = 5000") == 1 &&
                Count(connection, "GraphicsObjects", $"Kind = 'DeviceAllocation' AND hThunkResource = '{Hex(101)}' AND hVidMmGlobalAlloc = '{Hex(4)}'") == 1,
                "Older rundown must not overwrite newer resource/allocation metadata or move last observation backwards.");
            Expect(Count(connection, "GraphicsLinks", $"Kind = 'ResourceAllocation' AND Handle = '{Hex(101)}' AND StartSeq = 4 AND EndSeq IS NULL") == 1 &&
                Count(connection, "GraphicsResourceAllocations", $"GlobalAllocation = '{Hex(4)}' AND EndSeq IS NULL") == 1,
                "Older raw evidence must not revert the currently observed allocation chain.");
        });
        foreach (var stopFirst in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Allocation(33, 2, 3, 100),
                Allocation(36, 2, 3, 100),
                Direct3D(12, 1, kernelResource: 100)
            ];
            if (stopFirst) events.Add(Allocation(34, 2, 3, 100));
            events.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 6_000 });
            events.Add(Allocation(38, 2, 3, 100) with { TimestampQpc = 5_000 });
            events.Add(Allocation(38, 2, 3, 100) with { TimestampQpc = 7_000 });
            WithRecording(events, connection =>
            {
                var replacement = stopFirst ? 5 : 4;
                Expect(Count(connection, "GraphicsLinks", $"""
                    Kind = 'AllocationGlobal'
                    AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'AdapterAllocation' AND StartSeq = {replacement})
                    AND StartSeq = {events.Count} AND EndSeq IS NULL
                    """) == 1 &&
                    Count(connection, "GraphicsLinks", $"Kind = 'AllocationGlobal' AND StartSeq = {events.Count - 1}") == 0,
                    "Rundown older than global reuse must not re-enable a mapping; a newer confirmation may do so.");
                Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 3 AND EndSeq IS NULL") == 1,
                    "Global invalidation must not interrupt the independent resource bridge.");
            });
        }
        Console.WriteLine("Passed: stale rundown preserves newer metadata and cannot bypass global-reuse invalidation.");
    }

    private static void ClearedBridges()
    {
        foreach (var field in new[] { "hKMResource", "hThunkResource", "hVidMmGlobalAlloc", "pDxgAdapter" })
        {
            var runtime = field == "hKMResource";
            var observation = runtime ? Direct3D(14, 1, kernelResource: 100) : Allocation(38, 2, 3, 100);
            var missing = observation with { Fields = observation.Fields.Where(item => item.Name != field).ToArray() };
            var unknown = observation with
            {
                Fields = observation.Fields.Select(item => item.Name == field ? new EventField(field, null) : item).ToArray()
            };
            var cleared = observation with
            {
                Fields = observation.Fields.Select(item => item.Name == field ? new EventField(field, 0UL) : item).ToArray()
            };
            WithRecording(
            [
                Direct3D(12, 1, kernelResource: 100),
                Allocation(36, 2, 3, 100),
                Allocation(33, 2, 3, 100),
                missing,
                unknown,
                cleared,
                missing
            ], connection =>
            {
                var link = field is "hKMResource" or "hThunkResource" ? "ResourceAllocation" : "AllocationGlobal";
                var kind = runtime ? "Texture2D" : "DeviceAllocation";
                Expect(Count(connection, "GraphicsLinks", $"Kind = '{link}' AND EndSeq = 6") == 1 &&
                    Count(connection, "GraphicsLinks", $"Kind = '{link}' AND EndSeq IS NULL") == 0 &&
                    Count(connection, "GraphicsObjects", $"Kind = '{kind}' AND {field} IS NULL") == 1,
                    $"A missing {field} must preserve its bridge, but explicit zero must close it and clear latest metadata.");
                Expect(Count(connection, "GraphicsLinks", $"Kind <> '{link}' AND EndSeq IS NULL") == 1,
                    "Clearing one bridge must preserve the allocation's independent relationship.");
            });
        }
        Console.WriteLine("Passed: missing bridge fields preserve mappings while explicit zero clears only the reported bridge.");
    }

    private static void DelayedGlobalStops()
    {
        foreach (var confirmationFirst in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 },
                Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 2_500 },
                Allocation(38, 2, 3, 100) with { TimestampQpc = 4_000 },
                Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 }
            ];
            var replacement = Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 };
            var stale = Allocation(38, 2, 3, 100) with { TimestampQpc = 4_500 };
            events.AddRange(confirmationFirst ? [stale, replacement] : [replacement, stale]);
            events.Add(Allocation(38, 2, 3, 100) with { TimestampQpc = 6_000 });
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal' AND StartSeq IN (6, 7)") == 0 &&
                    Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal' AND StartSeq = 8 AND EndSeq IS NULL") == 1,
                    "A delayed adapter Stop must invalidate every old bridge until confirmation newer than the replacement.");
                Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND StartSeq = 3 AND EndSeq IS NULL") == 1 &&
                    Count(connection, "GraphicsResourceAllocations", "StartSeq = 5 AND EndSeq = 8 AND AdapterAllocationId IS NULL") == 1,
                    "Global bridge invalidation must preserve the independent resource link and its unknown-adapter interval.");
            });
        }
        Console.WriteLine("Passed: delayed adapter Stops cannot carry stale allocation bridges into replacement targets.");
    }

    private static void ReusedGlobalBridges()
    {
        foreach (var field in new[] { "pDxgAdapter", "hVidMmGlobalAlloc" })
        {
            foreach (var clear in new[] { false, true })
            {
                foreach (var awayFirst in new[] { false, true })
                {
                    foreach (var stopped in new[] { false, true })
                    {
                        var original = field == "pDxgAdapter" ? Adapter : 3;
                        ProfilerEvent Bridge(ulong value, long qpc)
                        {
                            var data = Allocation(38, 2, 3, 100) with { TimestampQpc = qpc };
                            return data with
                            {
                                Fields = data.Fields.Where(item => field != "pDxgAdapter" || item.Name != "hVidMmGlobalAlloc")
                                    .Select(item => item.Name == field ? new EventField(field, value) : item).ToArray()
                            };
                        }
                        List<ProfilerEvent> events =
                        [
                            Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 500 },
                            Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                            Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 }
                        ];
                        if (awayFirst) events.Add(Bridge(clear ? 0 : original + 1, 2_500));
                        if (stopped) events.Add(Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 });
                        events.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 6_000 });
                        if (!awayFirst) events.Add(Bridge(clear ? 0 : original + 1, 4_000));
                        events.AddRange(
                        [
                            Bridge(original, 5_000),
                            Allocation(38, 2, 3, 100) with { TimestampQpc = 5_500 },
                            Allocation(38, 2, 3, 100) with { TimestampQpc = 6_000 },
                            Allocation(38, 2, 3, 100) with { TimestampQpc = 7_000 }
                        ]);
                        WithRecording(events, connection =>
                        {
                            Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == 2 &&
                                Count(connection, "GraphicsLinks", $"""
                                    Kind = 'AllocationGlobal' AND StartSeq = {events.Count} AND EndSeq IS NULL
                                    AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects
                                        WHERE Kind = 'AdapterAllocation' AND StartQpc = 6000)
                                    """) == 1,
                                "Bridge changes must retain reuse protection until explicit confirmation newer than the replacement.");
                            Expect(Count(connection, "GraphicsLinks",
                                "Kind = 'ResourceAllocation' AND StartSeq = 3 AND EndSeq IS NULL") == 1 &&
                                Count(connection, "Events") == events.Count,
                                "Global bridge changes must preserve independent resource links and every raw observation.");
                        });
                    }
                }
            }
        }
        WithRecording(
        [
            Allocation(36, 2, 3, 100),
            Allocation(34, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Allocation(38, 2, 3, 100, Adapter + 1) with
            {
                Fields = Allocation(38, 2, 3, 100, Adapter + 1).Fields.Where(field => field.Name != "hVidMmGlobalAlloc").ToArray()
            },
            Allocation(38, 2, 3, 100) with
            {
                Fields = Allocation(38, 2, 3, 100).Fields.Where(field => field.Name != "hVidMmGlobalAlloc").ToArray()
            },
            Allocation(38, 2, 3, 100)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == 1 &&
                Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal' AND StartSeq = 6 AND EndSeq IS NULL") == 1,
                "A Stop-only global lifetime must retain its invalidation guard even when no earlier link could be created.");
        });
        WithRecording(
        [
            Allocation(33, 2, 3, 100),
            Allocation(36, 2, 3, 100),
            Allocation(38, 2, 3, 100, Adapter + 1) with
            {
                Fields = Allocation(38, 2, 3, 100, Adapter + 1).Fields.Where(field => field.Name != "hVidMmGlobalAlloc").ToArray()
            },
            Allocation(38, 2, 3, 100) with
            {
                Fields = Allocation(38, 2, 3, 100).Fields.Where(field => field.Name != "hVidMmGlobalAlloc").ToArray()
            }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal' AND StartSeq = 4 AND EndSeq IS NULL") == 1,
                "Returning to the same unended global allocation must not require an unnecessary new handle confirmation.");
        });
        foreach (var targetFirst in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 },
                Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 }
            ];
            var source = Allocation(38, 2, 4, 100) with { TimestampQpc = 4_000 };
            var target = Allocation(33, 2, 4, 100) with { TimestampQpc = 5_000 };
            events.AddRange(targetFirst ? [target, source] : [source, target]);
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsLinks", $"""
                    Kind = 'AllocationGlobal' AND Handle = '{Hex(4)}' AND StartSeq = 5 AND EndSeq IS NULL
                    """) == 1,
                    "Invalidation of one identity must not block a genuinely new global allocation in either arrival order.");
            });
        }
        Console.WriteLine("Passed: global reuse guards survive adapter/zero-handle changes while healthy and new identities remain correlatable.");
    }

    private static void DelayedGlobalSources()
    {
        foreach (var stopped in new[] { false, true })
        {
            foreach (var sourceFirst in new[] { false, true })
            {
                foreach (var phase in new[] { 36, 38 })
                {
                    foreach (var confirmed in new[] { false, true })
                    {
                        List<ProfilerEvent> events =
                        [
                            Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 }
                        ];
                        if (stopped) events.Add(Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 });
                        events.Add(Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_500 });
                        var source = Allocation(phase, 2, 3, 100) with { TimestampQpc = 2_000 };
                        if (sourceFirst) events.Add(source);
                        var replacement = Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 };
                        events.Add(replacement with
                        {
                            Fields = replacement.Fields.Select(field =>
                                field.Name == "allocSize" ? new EventField(field.Name, 8192UL) : field).ToArray()
                        });
                        if (!sourceFirst) events.Add(source);
                        if (confirmed) events.Add(Allocation(38, 2, 3, 100) with { TimestampQpc = 6_000 });
                        var confirmation = events.Count;
                        events.Add(Allocation(37, 2, 3, 100) with { TimestampQpc = confirmed ? 6_500 : 2_500 });
                        events.Add(Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = confirmed ? 7_000 : 2_800 });
                        WithRecording(events, connection =>
                        {
                            var replacementLinks = """
                                Kind = 'AllocationGlobal' AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects
                                    WHERE Kind = 'AdapterAllocation' AND StartQpc = 5000)
                                """;
                            Expect(Count(connection, "GraphicsLinks", replacementLinks) == (confirmed ? 1 : 0) &&
                                Count(connection, "GraphicsLinks", $"{replacementLinks} AND StartSeq < {confirmation}") == 0,
                                "A delayed allocation must not borrow a reused global target before fresh explicit confirmation.");
                            Expect(Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NULL AND SizeBytes IS NULL") >= 1 &&
                                Count(connection, "GraphicsResourceAllocations", "SizeBytes = '8192'") == (confirmed ? 1 : 0) &&
                                Count(connection, "Events") == events.Count,
                                "Delayed global attribution must retain raw evidence and independent resource intervals with unknown sizes.");
                        });
                    }
                }
            }
        }
        foreach (var sourceFirst in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
                Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 3_500 }
            ];
            var source = Allocation(36, 2, 3, 100) with { TimestampQpc = 4_000 };
            var target = Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 };
            events.AddRange(sourceFirst ? [source, target] : [target, source]);
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NOT NULL AND StartSeq = 5") == 1,
                    "A genuinely new allocation created after the previous global end must correlate in either observation order.");
            });
        }
        Console.WriteLine("Passed: delayed Start/rundown allocations cannot borrow reused global targets in either observation order.");
    }

    private static void PromotedGlobalEvidence()
    {
        foreach (var sourceFirst in new[] { false, true })
        {
            foreach (var stale in new[] { false, true })
            {
                List<ProfilerEvent> events =
                [
                    Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                    Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
                    Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 3_500 }
                ];
                var source = Allocation(38, 2, 3, 100) with { TimestampQpc = 4_500 };
                var target = Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 };
                events.AddRange(sourceFirst ? [source, target] : [target, source]);
                var start = stale ? 2_000 : 4_000;
                events.Add(Allocation(36, 2, 3, 101) with { TimestampQpc = start });
                WithRecording(events, connection =>
                {
                    Expect(Count(connection, "GraphicsObjects", $"""
                        Kind = 'DeviceAllocation' AND FirstObservation = 'Rundown' AND StartQpc = {start}
                            AND hThunkResource = '{Hex(100)}'
                        """) == 1 &&
                        Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND EndSeq IS NULL") == 1,
                        "A delayed allocation Start must promote the same lifetime without replacing newer resource metadata.");
                    Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == (stale ? 0 : 1) &&
                        Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NOT NULL AND StartSeq = 6") == (stale ? 0 : 1),
                        "Promoted creation must refresh global eligibility in either order, without admitting creation before the previous end.");
                });
            }
        }
        foreach (var phase in new[] { 33, 35 })
        {
            var target = Allocation(35, 2, 3, 100) with { TimestampQpc = 6_000 };
            WithRecording(
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_500 },
                Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 },
                Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
                target with
                {
                    Fields = target.Fields.Select(field =>
                        field.Name == "allocSize" ? new EventField(field.Name, 8192UL) : field).ToArray()
                },
                Allocation(38, 2, 3, 100) with { TimestampQpc = 5_000 },
                Allocation(phase, 2, 3, 100) with { TimestampQpc = 4_000 }
            ], connection =>
            {
                Expect(Count(connection, "GraphicsObjects", """
                    Kind = 'AdapterAllocation' AND FirstObservation = 'Rundown' AND FirstQpc = 6000
                        AND KnownFromQpc = 4000 AND SizeBytes = '8192'
                    """) == 1 &&
                    Count(connection, "GraphicsResourceAllocations", "StartSeq = 7 AND SizeBytes = '8192'") == 1,
                    "Earlier target Start or rundown evidence must refresh global links without overwriting newer size metadata.");
                Expect(Count(connection, "GraphicsLinks", """
                    Kind = 'AllocationGlobal' AND StartSeq < 7 AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects
                        WHERE Kind = 'AdapterAllocation' AND FirstQpc = 6000)
                    """) == 0,
                    "Confirmation before the target's known existence must remain unattributed until earlier evidence arrives.");
            });
        }
        var incomplete = Allocation(38, 2, 3, 100) with { TimestampQpc = 6_000 };
        WithRecording(
        [
            Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
            Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_500 },
            Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 },
            Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
            Allocation(33, 2, 3, 100) with { TimestampQpc = 4_000 },
            incomplete with { Fields = incomplete.Fields.Where(field => field.Name != "hVidMmGlobalAlloc").ToArray() },
            Allocation(38, 2, 3, 101) with { TimestampQpc = 5_000 }
        ], connection =>
        {
            Expect(Count(connection, "GraphicsObjects", $"Kind = 'DeviceAllocation' AND hThunkResource = '{Hex(100)}'") == 1 &&
                Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NOT NULL AND StartSeq = 7") == 1,
                "Accepted delayed confirmation must refresh global evidence without rolling back newer resource bridges.");
        });
        foreach (var returning in new[] { false, true })
        {
            List<ProfilerEvent> events =
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 },
                Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 3_500 },
                Allocation(38, 2, 3, 100) with { TimestampQpc = 4_500 },
                Allocation(38, 2, 3, 100, Adapter + 1) with { TimestampQpc = 4_700 },
                Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 },
                Allocation(36, 2, 3, 101) with { TimestampQpc = 4_000 }
            ];
            if (returning) events.Add(Allocation(38, 2, 3, 100) with { TimestampQpc = 4_800 });
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsObjects", $"""
                    Kind = 'DeviceAllocation' AND pDxgAdapter = '{Hex(returning ? Adapter : Adapter + 1)}'
                        AND hThunkResource = '{Hex(100)}'
                    """) == 1 &&
                    Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == (returning ? 1 : 0),
                    "Delayed creation evidence must not restore an older adapter, but must remain usable after a genuine return.");
            });
        }
        Console.WriteLine("Passed: delayed source/target evidence refreshes global links without replacing current bridges.");
    }

    private static void StopOnlyGlobalBridges()
    {
        foreach (var field in new[] { "pDxgAdapter", "hVidMmGlobalAlloc" })
        {
            foreach (var observed in new[] { false, true })
            {
                foreach (var awayFirst in new[] { false, true })
                {
                    foreach (var clear in new[] { false, true })
                    {
                        var original = field == "pDxgAdapter" ? Adapter : 3;
                        ProfilerEvent Bridge(ulong value, long qpc)
                        {
                            var data = Allocation(38, 2, 3, 100) with { TimestampQpc = qpc };
                            return data with
                            {
                                Fields = data.Fields.Select(item =>
                                    item.Name == field ? new EventField(field, value) : item).ToArray()
                            };
                        }
                        List<ProfilerEvent> events = [Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 500 }];
                        if (observed) events.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 500 });
                        events.Add(Allocation(36, 2, 3, 100) with { TimestampQpc = 1_000 });
                        if (awayFirst) events.Add(Bridge(clear ? 0 : original + 1, 2_000));
                        events.Add(Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 });
                        events.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 4_000 });
                        if (!awayFirst) events.Add(Bridge(clear ? 0 : original + 1, 2_000));
                        events.Add(Bridge(original, 2_500));
                        events.Add(Bridge(original, 4_000));
                        events.Add(Bridge(original, 5_000));
                        var confirmation = events.Count;
                        events.Add(Bridge(clear ? 0 : original + 1, 6_000));
                        var returning = Bridge(original, 7_000);
                        events.Add(field == "pDxgAdapter" ? returning with
                        {
                            Fields = returning.Fields.Where(item => item.Name != "hVidMmGlobalAlloc").ToArray()
                        } : returning);
                        WithRecording(events, connection =>
                        {
                            var replacementLinks = """
                                Kind = 'AllocationGlobal' AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects
                                    WHERE Kind = 'AdapterAllocation' AND StartQpc = 4000)
                                """;
                            Expect(Count(connection, "GraphicsLinks", replacementLinks) == 2 &&
                                Count(connection, "GraphicsLinks", $"{replacementLinks} AND StartSeq < {confirmation}") == 0 &&
                                Count(connection, "GraphicsLinks", $"{replacementLinks} AND StartSeq = {events.Count} AND EndSeq IS NULL") == 1,
                                "Stop-only reuse must survive an absent source; only newer confirmation may rejoin, and that proof must survive a healthy return.");
                            Expect(Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND EndSeq IS NULL") == 1 &&
                                Count(connection, "Events") == events.Count,
                                "Stop-only global invalidation must preserve resource links and every raw event.");
                        });
                    }
                }
            }
        }
        Console.WriteLine("Passed: Stop-only global reuse is retained across adapter, global-handle and cleared-bridge excursions.");
    }

    private static void IndependentAllocationLinks()
    {
        WithRecording(
        [
            Direct3D(12, 1, kernelResource: 100),
            Allocation(36, 2, 3, 100),
            Allocation(33, 2, 3, 100),
            Allocation(34, 2, 3, 100),
            Allocation(37, 2, 3, 100)
        ], connection =>
        {
            Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal' AND EndSeq = 4") == 1 &&
                Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation' AND EndSeq = 5") == 1,
                "An adapter Stop must end only the global link, leaving the resource bridge until its own endpoint ends.");
            Expect(Count(connection, "GraphicsResourceAllocations", "StartSeq = 4 AND EndSeq = 5 AND AdapterAllocationId IS NULL") == 1,
                "The remaining resource interval must stay queryable with an unknown adapter.");
        });
        Console.WriteLine("Passed: resource and global allocation links close independently.");
    }

    private static void PartialAllocationIntervals()
    {
        foreach (var open in new[] { false, true })
        {
            WithRecording(
            [
                Direct3D(12, 1, kernelResource: 100),
                Allocation(36, 2, 3, 100),
                Allocation(33, 2, 3, 100)
            ], connection =>
            {
                Sql.Execute(connection, $"""
                    UPDATE GraphicsLinks SET EndSeq = {(open ? "NULL" : "12")} WHERE Kind = 'ResourceAllocation';
                    UPDATE GraphicsLinks SET StartSeq = 5, EndSeq = 7 WHERE Kind = 'AllocationGlobal';
                    INSERT INTO GraphicsLinks (SourceId, TargetId, Kind, Handle, Qualifier, StartSeq, EndSeq)
                    SELECT SourceId, TargetId, Kind, Handle, Qualifier, 9, 11 FROM GraphicsLinks WHERE Kind = 'AllocationGlobal';
                    """);
                using var command = connection.CreateCommand();
                command.CommandText = """
                    SELECT StartSeq, EndSeq, AdapterAllocationId, GlobalAllocation, Adapter, SizeBytes
                    FROM GraphicsResourceAllocations ORDER BY StartSeq
                    """;
                using var rows = command.ExecuteReader();
                (long Start, long? End, bool Known)[] expected = [(2, 5, false), (5, 7, true), (7, 9, false), (9, 11, true), (11, open ? null : 12, false)];
                foreach (var item in expected)
                {
                    Expect(rows.Read() && rows.GetInt64(0) == item.Start &&
                        (rows.IsDBNull(1) ? (long?)null : rows.GetInt64(1)) == item.End &&
                        Enumerable.Range(2, 4).All(column => rows.IsDBNull(column) != item.Known),
                        "Allocation views must preserve leading, middle and trailing unknown-adapter intervals, including an open tail.");
                }
                Expect(!rows.Read(), "Allocation interval partitioning must not duplicate or overlap segments.");
            });
        }
        Console.WriteLine("Passed: allocation views retain complete coverage across known chains and unknown adapter gaps.");
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

    private static void DisjointLifetimes()
    {
        foreach (var global in new[] { false, true })
        foreach (var sourceEnds in new[] { false, true })
        foreach (var alreadyClosed in new[] { false, true })
        foreach (var gap in new[] { 0, 1_000 })
        {
            ProfilerEvent Source(int phase, long qpc) =>
                (global ? Allocation(36 + phase, 2, 3, 100) : Direct3D(12 + phase, 1, kernelResource: 100))
                    with { TimestampQpc = qpc };
            ProfilerEvent Target(int phase, long qpc) =>
                Allocation((global ? 33 : 36) + phase, 2, 3, 100) with { TimestampQpc = qpc };
            List<ProfilerEvent> events =
            [
                Source(0, sourceEnds ? 1_000 : 2_000 + gap),
                Target(0, sourceEnds ? 2_000 + gap : 1_000)
            ];
            if (alreadyClosed) events.Add(sourceEnds ? Target(1, 4_000) : Source(1, 4_000));
            events.Add(sourceEnds ? Source(1, 2_000) : Target(1, 2_000));
            WithRecording(events, connection =>
            {
                var kind = global ? "AllocationGlobal" : "ResourceAllocation";
                Expect(Count(connection, "GraphicsLinks", $"Kind = '{kind}'") == (gap == 0 ? 1 : 0),
                    $"A delayed Stop must retract only provably disjoint {kind} links, including closed intervals " +
                    $"(source ends: {sourceEnds}; closed: {alreadyClosed}; gap: {gap}).");
                Expect(Count(connection, "Events") == events.Count &&
                    Count(connection, "GraphicsObjects", "StopQpc = 2000 AND EndReason = 'Stop'") == 1,
                    "Retracting a derived association must preserve every raw observation and its actual lifetime end.");
                if (!global)
                    Expect(Count(connection, "GraphicsResourceAllocations") == (gap == 0 ? 1 : 0),
                        "Disproved resource associations must disappear from the public allocation view too.");
            });
        }
        foreach (var allocationFirst in new[] { false, true })
        {
            List<ProfilerEvent> events = [Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 1_000 }];
            var allocation = Allocation(36, 2, 3, 100) with { TimestampQpc = 2_000 };
            if (allocationFirst) events.Add(allocation);
            events.Add(Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 3_000 });
            events.Add(Direct3D(12, 4, kernelResource: 100) with { TimestampQpc = 5_000 });
            if (!allocationFirst) events.Add(allocation);
            events.Add(Allocation(37, 2, 3, 100) with { TimestampQpc = 2_500 });
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsResourceAllocations", """
                    ResourceId = (SELECT GraphicsObjectId FROM GraphicsObjects WHERE Kind = 'Texture2D' AND StartQpc = 5000)
                    """) == 0 &&
                    Count(connection, "GraphicsLinks", "Kind = 'ResourceAllocation'") == (allocationFirst ? 1 : 0),
                    "A delayed allocation Stop must invalidate a reused resource bridge without deleting a valid older association.");
            });
        }
        Console.WriteLine("Passed: delayed endpoint Stops retract disjoint and reused resource/global links without rejecting equal-time bounds.");
    }

    private static void PromotedDisjointLifetimes()
    {
        foreach (var global in new[] { false, true })
        foreach (var sourceUnknown in new[] { false, true })
        foreach (var promote in new[] { false, true })
        {
            ProfilerEvent Source(int phase, long qpc) =>
                (global ? Allocation(36 + phase, 2, 3, 100) : Direct3D(12 + phase, 1, kernelResource: 100))
                    with { TimestampQpc = qpc };
            ProfilerEvent Target(int phase, long qpc) =>
                Allocation((global ? 33 : 36) + phase, 2, 3, 100) with { TimestampQpc = qpc };
            List<ProfilerEvent> events =
            [
                sourceUnknown ? Source(2, 5_000) : Target(2, 5_000),
                sourceUnknown ? Target(0, 1_000) : Source(0, 1_000),
                sourceUnknown ? Target(1, 2_000) : Source(1, 2_000)
            ];
            if (promote) events.Add(sourceUnknown ? Source(0, 3_000) : Target(0, 3_000));
            WithRecording(events, connection =>
            {
                var kind = global ? "AllocationGlobal" : "ResourceAllocation";
                Expect(Count(connection, "GraphicsLinks", $"Kind = '{kind}'") == (promote ? 0 : 1),
                    $"Unknown creation must permit a partial {kind} association until a delayed Start proves it impossible " +
                    $"(source unknown: {sourceUnknown}; promoted: {promote}).");
                Expect(Count(connection, "Events") == events.Count &&
                    Count(connection, "GraphicsObjects", "FirstObservation = 'Rundown'") == 1,
                    "Creation promotion must correct derived links without replacing the original partial lifetime.");
            });
        }
        Console.WriteLine("Passed: partial resource/global links survive unknown creation and retract after disjoint Start promotion.");
    }

    private static void RefinedGlobalEnds()
    {
        foreach (var (otherLifetime, trim, keepRefined) in new[]
        {
            (false, false, false), (true, false, false), (true, true, false), (false, true, true)
        })
        {
            List<ProfilerEvent> events = [];
            if (otherLifetime)
                events.AddRange(
                [
                    Allocation(33, 2, 3, 100) with { TimestampQpc = 3_500 },
                    Allocation(34, 2, 3, 100) with { TimestampQpc = 4_500 }
                ]);
            events.AddRange(
            [
                Allocation(33, 2, 3, 100) with { TimestampQpc = 1_000 },
                Allocation(36, 2, 3, 100) with { TimestampQpc = 4_000 },
                Allocation(33, 2, 3, 100) with { TimestampQpc = 5_000 }
            ]);
            if (trim)
                for (var i = 0; i < 6_000; i++)
                {
                    events.Add(new(0, 0, 10, 7, XamlProvider.Xaml, false, "UnrelatedEvent", 0,
                        [new("Value", new string('x', 256))]));
                    if (keepRefined && i % 64 == 0)
                        events.Add(Allocation(35, 2, 3, 100) with { TimestampQpc = 1_500 });
                }
            events.Add(Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 });
            WithRecording(events, connection =>
            {
                Expect(Count(connection, "GraphicsLinks", """
                    Kind = 'AllocationGlobal' AND EndSeq IS NULL
                    AND TargetId = (SELECT GraphicsObjectId FROM GraphicsObjects
                        WHERE Kind = 'AdapterAllocation' AND StartQpc = 5000)
                    """) == (otherLifetime ? 0 : 1),
                    "Refining a reuse end must remove its superseded bound, but preserve independent retained and trimmed ends.");
                if (trim)
                {
                    Expect(Sql.Scalar(connection, "SELECT CompleteFromSeq FROM SessionInfo") > 5,
                        "The refined-end fixture must really discard the original graphics events.");
                    Expect(Count(connection, "GraphicsObjects", "StopQpc = 4500") == 0,
                        "The independent closed bound must survive even after its lifetime row is trimmed.");
                }
                if (!trim || keepRefined)
                    Expect(Count(connection, "GraphicsObjects", "StartQpc = 1000 AND StopQpc = 3000 AND EndQpc = 3000") == 1,
                        "Retained lifetime ends must remain refinable rather than becoming permanent trimmed-history bounds.");
            }, limitBytes: trim ? 1024 * 1024 : SessionWriter.DefaultLimitBytes);
        }
        Console.WriteLine("Passed: refined global ends preserve independent history without freezing retained reuse boundaries.");
    }

    private static void SinglePassGraphicsTrim()
    {
        foreach (var (closedChild, cutoff) in new[] { (true, 5L), (false, 4L), (true, 3L) })
        {
            List<ProfilerEvent> events = [Direct3D(3, Device), Direct3D(12, 1, Device, 100)];
            if (closedChild) events.Add(Direct3D(13, 1, Device, 100));
            events.Add(Direct3D(4, Device));
            WithRecording(events, connection =>
            {
                using (var graphics = new GraphicsIndex(connection))
                using (var transaction = connection.BeginTransaction())
                {
                    graphics.Trim(cutoff, transaction);
                    transaction.Commit();
                }
                var retained = !closedChild || cutoff == 3;
                Expect(Count(connection, "GraphicsObjects", "Kind = 'Device'") == (retained ? 1 : 0) &&
                    Count(connection, "GraphicsObjects", "Kind = 'Texture2D'") == (retained ? 1 : 0),
                    "One trim must remove an eligible closed child and parent, but retain open children and cutoff-boundary evidence.");
            });
        }
        Console.WriteLine("Passed: one graphics trim removes eligible child/parent pairs and retains referenced or boundary lifetimes.");
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
        var resourceStop = Direct3D(7, 4, Device, 101);
        events.Add(resourceStop with
        {
            Fields = resourceStop.Fields.Where(field => field.Name != "pID3D11Device").ToArray()
        });
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
        foreach (var sourceObserved in new[] { false, true })
        {
            List<ProfilerEvent> delayed =
            [
                Direct3D(12, 1, kernelResource: 100) with { TimestampQpc = 500 }
            ];
            if (sourceObserved)
            {
                delayed.Add(Allocation(36, 2, 3, 100) with { TimestampQpc = 1_000 });
                delayed.Add(Allocation(38, 2, 3, 100, Adapter + 1) with { TimestampQpc = 2_000 });
            }
            delayed.Add(Allocation(34, 2, 3, 100) with { TimestampQpc = 3_000 });
            if (!sourceObserved) delayed.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 4_000 });
            delayed.AddRange(Enumerable.Range(0, 6_000).Select(_ =>
                new ProfilerEvent(0, 0, 10, 7, XamlProvider.Xaml, false, "UnrelatedEvent", 0, [new("Value", new string('x', 256))])));
            if (sourceObserved) delayed.Add(Allocation(33, 2, 3, 100) with { TimestampQpc = 4_000 });
            delayed.Add(Allocation(sourceObserved ? 38 : 36, 2, 3, 100) with { TimestampQpc = 2_500 });
            delayed.Add(Allocation(37, 2, 3, 100) with { TimestampQpc = 6_000 });
            delayed.Add(Direct3D(13, 1, kernelResource: 100) with { TimestampQpc = 7_000 });
            WithRecording(delayed, connection =>
            {
                Expect(Sql.Scalar(connection, "SELECT CompleteFromSeq FROM SessionInfo") > 5 &&
                    Count(connection, "GraphicsObjects", "Kind = 'AdapterAllocation' AND StopQpc = 3000") == 0,
                    "The global-retention fixture must discard the Stop-only adapter lifetime.");
                Expect(Count(connection, "GraphicsLinks", "Kind = 'AllocationGlobal'") == 0 &&
                    Count(connection, "GraphicsResourceAllocations", "AdapterAllocationId IS NULL AND SizeBytes IS NULL") == 1,
                    "Trimming must retain reuse protection for an active target or a surviving source, including while its bridge is away.");
            }, limitBytes: 1024 * 1024);
        }
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
