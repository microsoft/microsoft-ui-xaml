// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Globalization;
using ModelContextProtocol;
using ModelContextProtocol.Server;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;

namespace WinUIProfiler.Mcp.Tools;

/// <summary>
/// A session's UI-thread ticks and regions of interest, as the app's frame processor and the session file's views give
/// them: Ticks, InterestingRows, and a tick's events with the objects they mention. Never the events' field values.
/// </summary>
[McpServerToolType]
internal static class FrameTools
{
    internal const string NoTicks = "No UI thread ticked while the session was recorded. Ticks come from " +
        "Microsoft-Windows-XAML's CoreServices_Frame events, which a UI thread writes when each tick starts and ends.";

    internal const string NoRegions = "The session has no regions of interest or interesting rows: it recorded no " +
        "navigation, PerfXamlEvent operation (such as a flyout or a dialog opening), window focus change or graphics " +
        "device creation.";

    internal static readonly string[] Views = ["summary", "slowest", "regions", "rows", "tick"];

    internal static readonly string[] RowTypes =
        ["Frame", "UpdateLayout", "Operation", "FocusChange", "CreateGraphicsDevice", "Navigating", "Navigated"];

    internal static readonly string[] Buckets = ["Xaml", "Composition", "Other"];

    private const int MaxLimit = 500;

    // The tick view lists at most this many event names and object types, so a busy tick's answer stays small.
    private const int TickNames = 30;
    private const int TickTypes = 20;

    // The tick view lists at most this many of one event's mentions. Paging by event can't bound one event's answer:
    // clearing a visual's children mentions every child, and a visual can have thousands.
    internal const int EventMentions = 10;

    // SessionFile names the field of a child whose parent's children were cleared "child of" the parent's field.
    private const string ClearedChildField = "child of ";

    private static readonly string[] SlowestOrders = ["duration", "events"];

    private static readonly string[] RegionOrders = ["duration", "time"];

    private static readonly string[] RowOrders = ["time", "duration"];

    [McpServerTool(Name = "frames", ReadOnly = true, Idempotent = true, OpenWorld = false), Description(
        "Answers questions about a session's UI-thread ticks and regions of interest, from what the WinUI Profiler app's " +
        "frame processor stored. A tick is one pass of a UI thread's frame loop: it drew when the thread's next tick has a " +
        "higher frame number, and it's slow when it took more than 16.7 ms (a 60 Hz frame). A region of interest starts at " +
        "a navigation, a PerfXamlEvent operation (such as WinUI initializing, a flyout or a ContentDialog opening) or a " +
        "window focus change, and ends when that thread's next tick ends. Views: \"summary\" (the default): per UI thread, " +
        "how many ticks drew, their durations (p50, p90, max), how many were slow and their events by bucket (Xaml, " +
        "Composition, Other); the regions by trigger; the interesting rows by type; when the first frame drew. \"slowest\": " +
        "the longest ticks, or those with the most events, each with the region that ends with it. \"regions\": the regions, " +
        "longest first or in order, each with its trigger, duration, the tick it ends with and its rows by type, and the " +
        "rows outside every region. \"rows\": the interesting rows (ticks and UpdateLayout passes inside regions; " +
        "operations, navigations, focus changes and graphics device creations always), in order or longest first, " +
        "optionally of one region or one type. \"tick\": one tick: its events counted by provider and name (the 30 names " +
        "with the most events), the UI elements and visuals they mention counted by type (the 20 types with the most " +
        "objects), and a page of its events with their bucket and mentions (10 at most per event, the event's own fields " +
        "first, with omittedMentions counting the rest). Times are in ms (see the session's timeBase). " +
        "Events' field values aren't shown.")]
    public static Task<object> Frames(
        [Description("\"summary\", \"slowest\", \"regions\", \"rows\" or \"tick\". Omit it for the summary, or for the tick " +
            "view when tickId is given, or the rows view when regionId or type is given.")]
        string? view = null,
        [Description(SessionCatalog.SessionPathDescription)] string? sessionPath = null,
        [Description("The tick view: the tick to describe, by the tickId the slowest, regions and rows views give.")]
        long? tickId = null,
        [Description("The rows view: only the rows of this region, by the regionId the regions view gives.")]
        long? regionId = null,
        [Description("The rows view: only rows of this type: Frame, UpdateLayout, Operation, FocusChange, " +
            "CreateGraphicsDevice, Navigating or Navigated.")]
        string? type = null,
        [Description("The tick view: only the events of this bucket: Xaml, Composition or Other. The event names, the " +
            "objects and the page then cover that bucket; the tick's own counts still cover all its events.")]
        string? bucket = null,
        [Description("The slowest view: \"duration\" (the default) or \"events\". The regions view: \"duration\" (the " +
            "default, longest first) or \"time\". The rows view: \"time\" (the default) or \"duration\".")]
        string? orderBy = null,
        [Description("The slowest, regions, rows and tick views: how many items to skip, for the next page.")]
        int offset = 0,
        [Description("The slowest, regions, rows and tick views: the most items to return, from 1 to 500. Omit it for 20 " +
            "ticks, regions or events, or 50 rows.")]
        int? limit = null,
        CancellationToken cancellationToken = default)
    {
        var typeText = ToolArguments.Text(type);
        var chosen = ToolArguments.Text(view) is not null ? ToolArguments.View(view, Views, "frames")
            : tickId is not null ? "tick"
            : regionId is not null || typeText is not null ? "rows"
            : "summary";
        ToolArguments.OnlyFor(tickId is not null, nameof(tickId), "frames", chosen, "tick");
        ToolArguments.OnlyFor(regionId is not null, nameof(regionId), "frames", chosen, "rows");
        ToolArguments.OnlyFor(typeText is not null, nameof(type), "frames", chosen, "rows");
        ToolArguments.OnlyFor(ToolArguments.Text(bucket) is not null, nameof(bucket), "frames", chosen, "tick");
        ToolArguments.OnlyFor(ToolArguments.Text(orderBy) is not null, nameof(orderBy), "frames", chosen, "slowest", "regions", "rows");
        ToolArguments.OnlyFor(offset != 0, nameof(offset), "frames", chosen, "slowest", "regions", "rows", "tick");
        ToolArguments.OnlyFor(limit is not null, nameof(limit), "frames", chosen, "slowest", "regions", "rows", "tick");
        ToolArguments.NotNegative(offset, nameof(offset));
        var count = ToolArguments.Range(limit ?? (chosen == "rows" ? 50 : 20), 1, MaxLimit, nameof(limit));
        if (chosen == "tick" && tickId is null)
        {
            throw new McpException("The tick view needs tickId. The slowest, regions and rows views give tick IDs.");
        }

        InterestingRowType? rowType = null;
        if (typeText is not null)
        {
            if (string.Equals(typeText, "Region", StringComparison.OrdinalIgnoreCase))
            {
                throw new McpException("The regions view lists the regions. The rows view's type is one of: " +
                    string.Join(", ", RowTypes) + ".");
            }

            rowType = Enum.Parse<InterestingRowType>(ToolArguments.Choice(typeText, RowTypes, "row type")!);
        }

        var chosenBucket = ToolArguments.Choice(bucket, Buckets, "bucket");
        var order = chosen switch
        {
            "slowest" => ToolArguments.Choice(orderBy, SlowestOrders, "orderBy for the slowest view") ?? SlowestOrders[0],
            "regions" => ToolArguments.Choice(orderBy, RegionOrders, "orderBy for the regions view") ?? RegionOrders[0],
            "rows" => ToolArguments.Choice(orderBy, RowOrders, "orderBy for the rows view") ?? RowOrders[0],
            _ => null,
        };
        return SessionCatalog.RunAsync(sessionPath, path => chosen switch
        {
            "slowest" => Slowest(path, order!, offset, count),
            "regions" => Regions(path, order!, offset, count),
            "rows" => Rows(path, regionId, rowType, order!, offset, count),
            "tick" => Tick(path, tickId!.Value, chosenBucket, offset, count, cancellationToken),
            _ => Summary(path),
        }, cancellationToken);
    }

    private static object Summary(string path)
    {
        var info = SessionFile.ReadInfo(path);
        var ticks = SessionFile.ReadFrames(path);
        var rows = SessionFile.ReadInterestingRows(path);
        var launch = SessionFile.ReadLaunchSummary(path);
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (ticks.Count == 0)
        {
            notes.Add(NoTicks);
        }

        AddTickNotes(ticks, notes);
        var regions = rows.Where(row => row.Type == InterestingRowType.Region).ToList();
        var others = rows.Where(row => row.Type != InterestingRowType.Region).ToList();
        if (rows.Count == 0)
        {
            notes.Add(NoRegions);
        }

        var first = launch.Count == 0 ? null : launch[0];
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "summary",
            ticks = ticks.Count,
            uiThreads = ticks.GroupBy(tick => tick.ThreadId).Select(thread => DescribeThread([.. thread])).ToList(),
            regions = new { count = regions.Count, byTrigger = ByTrigger(regions) },
            interestingRows = others.GroupBy(row => row.Type).OrderBy(group => group.Key).Select(group => new
            {
                type = group.Key.ToString(),
                count = group.Count(),
                outsideRegions = group.Count(row => row.RegionId is null),
                totalMs = EventJson.Ms(group.Sum(row => row.DurationMilliseconds)),
                maxMs = EventJson.Ms(group.Max(row => row.DurationMilliseconds)),
            }).ToList(),
            firstFrame = first is null ? null : new
            {
                frameNumber = first.FrameNumber,
                ms = EventJson.Ms(first.Milliseconds),
                measuredFrom = first.FromProcessStart ? "ProcessStart" : "RecordingStart",
            },
            notes,
        };
    }

    private static object DescribeThread(IReadOnlyList<SessionFrame> ticks)
    {
        var starts = ticks.Where(tick => tick.StartMilliseconds is not null).Select(tick => tick.StartMilliseconds!.Value).ToList();
        var ends = ticks.Where(tick => tick.EndMilliseconds is not null).Select(tick => tick.EndMilliseconds!.Value).ToList();
        return new
        {
            threadId = ticks[0].ThreadId,
            ticks = ticks.Count,
            drew = ticks.Count(tick => tick.Drawn == true),
            didNotDraw = ticks.Count(tick => tick.Drawn == false),
            drewUnknown = ticks.Count(tick => tick.Drawn is null),
            slow = ticks.Count(tick => tick.Slow),
            tickMs = Durations(ticks.Where(tick => tick.TickMilliseconds is not null)),
            drawingTickMs = Durations(ticks.Where(tick => tick.Drawn == true && tick.TickMilliseconds is not null)),
            events = new
            {
                total = ticks.Sum(tick => tick.Events),
                xaml = ticks.Sum(tick => tick.Xaml),
                composition = ticks.Sum(tick => tick.Composition),
                other = ticks.Sum(tick => tick.Other),
            },
            firstStartMs = starts.Count == 0 ? null : EventJson.Ms(starts.Min()),
            lastEndMs = ends.Count == 0 ? null : EventJson.Ms(ends.Max()),
        };
    }

    /// <summary>How many ticks have a known duration, and their total, nearest-rank p50 and p90, and maximum.</summary>
    private static object? Durations(IEnumerable<SessionFrame> ticks)
    {
        var values = ticks.Select(tick => tick.TickMilliseconds!.Value).Order().ToList();
        if (values.Count == 0)
        {
            return null;
        }

        return new
        {
            count = values.Count,
            totalMs = EventJson.Ms(values.Sum()),
            p50Ms = EventJson.Ms(Rank(values, 50)),
            p90Ms = EventJson.Ms(Rank(values, 90)),
            maxMs = EventJson.Ms(values[^1]),
        };
    }

    /// <summary>The nearest-rank percentile of sorted values.</summary>
    internal static double Rank(IReadOnlyList<double> sorted, int percent) =>
        sorted[Math.Max(0, ((percent * sorted.Count) + 99) / 100 - 1)];

    private static void AddTickNotes(IReadOnlyList<SessionFrame> ticks, List<string> notes)
    {
        var unstarted = ticks.Count(tick => tick.StartMilliseconds is null);
        if (unstarted > 0)
        {
            notes.Add(unstarted == 1
                ? "1 tick started before the recording, or its start was deleted, so its duration isn't known."
                : $"{EventJson.Count(unstarted, "tick", "ticks")} started before the recording, or their starts were deleted, " +
                    "so their durations aren't known.");
        }

        var running = ticks.Count(tick => tick.EndMilliseconds is null);
        if (running > 0)
        {
            notes.Add(running == 1
                ? "1 tick hadn't ended when the recording stopped, so its duration isn't known."
                : $"{EventJson.Count(running, "tick", "ticks")} hadn't ended when the recording stopped, so their durations " +
                    "aren't known.");
        }

        if (ticks.Any(tick => tick.Drawn is null))
        {
            notes.Add("Whether a UI thread's latest tick drew isn't known: the thread's next tick would tell.");
        }
    }

    private static object Slowest(string path, string order, int offset, int limit)
    {
        var info = SessionFile.ReadInfo(path);
        var ticks = SessionFile.ReadFrames(path);
        var regions = RegionsByTick(SessionFile.ReadInterestingRows(path));
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (ticks.Count == 0)
        {
            notes.Add(NoTicks);
        }

        List<SessionFrame> ranked;
        if (order == "events")
        {
            ranked = [.. ticks.OrderByDescending(tick => tick.Events).ThenBy(tick => tick.FrameId)];
        }
        else
        {
            ranked = [.. ticks.Where(tick => tick.TickMilliseconds is not null)
                .OrderByDescending(tick => tick.TickMilliseconds).ThenBy(tick => tick.FrameId)];
            var unranked = ticks.Count - ranked.Count;
            if (unranked > 0)
            {
                notes.Add(unranked == 1
                    ? "1 tick isn't ranked because its duration isn't known: it started before the recording, or hadn't " +
                        "ended when it stopped."
                    : $"{EventJson.Count(unranked, "tick", "ticks")} aren't ranked because their durations aren't known: " +
                        "they started before the recording, or hadn't ended when it stopped.");
            }
        }

        var page = ranked.Skip(offset).Take(limit).Select(tick => DescribeTick(tick, regions.GetValueOrDefault(tick.FrameId))).ToList();
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "slowest",
            orderBy = order,
            total = ranked.Count,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, ranked.Count),
            ticks = page,
            notes,
        };
    }

    private static object Regions(string path, string order, int offset, int limit)
    {
        var info = SessionFile.ReadInfo(path);
        var rows = SessionFile.ReadInterestingRows(path);
        var ticks = SessionFile.ReadFrames(path).ToDictionary(tick => tick.FrameId);
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (rows.Count == 0)
        {
            notes.Add(NoRegions);
        }

        var regions = rows.Where(row => row.Type == InterestingRowType.Region).ToList();
        var members = rows.Where(row => row.Type != InterestingRowType.Region && row.RegionId is not null)
            .ToLookup(row => row.RegionId!.Value);
        IEnumerable<InterestingRow> ordered = order == "time"
            ? regions
            : regions.OrderByDescending(row => row.DurationMilliseconds).ThenBy(row => row.StartSeq);
        var page = ordered.Skip(offset).Take(limit).Select(region => new
        {
            regionId = RegionId(region),
            trigger = region.Trigger,
            threadId = region.ThreadId,
            startMs = EventJson.Ms(region.StartMilliseconds),
            stopMs = EventJson.Ms(region.StopMilliseconds),
            durationMs = EventJson.Ms(region.DurationMilliseconds),
            endsWithTick = EndTick(region, ticks),
            rows = members[RegionId(region)].GroupBy(row => row.Type).OrderBy(group => group.Key)
                .ToDictionary(group => group.Key.ToString(), group => group.Count()),
        }).ToList();
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "regions",
            orderBy = order,
            total = regions.Count,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, regions.Count),
            byTrigger = ByTrigger(regions),
            regions = page,
            outsideRegions = rows.Where(row => row.Type != InterestingRowType.Region && row.RegionId is null)
                .GroupBy(row => row.Type).OrderBy(group => group.Key).Select(group => new
                {
                    type = group.Key.ToString(),
                    count = group.Count(),
                    totalMs = EventJson.Ms(group.Sum(row => row.DurationMilliseconds)),
                    maxMs = EventJson.Ms(group.Max(row => row.DurationMilliseconds)),
                }).ToList(),
            notes,
        };
    }

    private static object Rows(string path, long? regionId, InterestingRowType? type, string order, int offset, int limit)
    {
        var info = SessionFile.ReadInfo(path);
        var rows = SessionFile.ReadInterestingRows(path, regionId);
        var notes = new List<string>(SessionCatalog.Notes(info));
        InterestingRow? region = null;
        if (regionId is { } id)
        {
            region = rows.FirstOrDefault(row => row.Type == InterestingRowType.Region && RegionId(row) == id)
                ?? throw new McpException(string.Create(CultureInfo.InvariantCulture,
                    $"There's no region {id} in {path}. The regions view lists the regions with their regionId."));
        }
        else if (rows.Count == 0)
        {
            notes.Add(NoRegions);
        }

        var matching = rows.Where(row => row.Type != InterestingRowType.Region && (type is null || row.Type == type)).ToList();
        if (type is InterestingRowType.Frame or InterestingRowType.UpdateLayout && region is null)
        {
            notes.Add($"{type} rows are listed only inside regions of interest; the slowest view ranks every tick.");
        }

        IEnumerable<InterestingRow> ordered = order == "duration"
            ? matching.OrderByDescending(row => row.DurationMilliseconds).ThenBy(row => row.StartSeq)
            : matching;
        var page = ordered.Skip(offset).Take(limit).Select(DescribeRow).ToList();
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "rows",
            region = region is null ? null : DescribeRegion(region),
            type = type?.ToString(),
            orderBy = order,
            total = matching.Count,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, matching.Count),
            rows = page,
            notes,
        };
    }

    private static object Tick(string path, long tickId, string? bucket, int offset, int limit, CancellationToken cancellationToken)
    {
        var info = SessionFile.ReadInfo(path);
        var tick = SessionFile.ReadFrames(path, tickId).FirstOrDefault() ?? throw new McpException(string.Create(
            CultureInfo.InvariantCulture, $"There's no tick {tickId} in {path}. The slowest, regions and rows views give tick IDs."));
        var region = RegionsByTick(SessionFile.ReadInterestingRows(path)).GetValueOrDefault(tickId);
        var kinds = new Dictionary<(string Provider, string Name, string Bucket), long>();
        var objects = new Dictionary<long, ObjectMention>();
        var page = new List<object>();
        long matching = 0;
        var capped = 0;
        var total = SessionFile.ReadFrameEvents(path, tickId, null, null, 0, -1, (profilerEvent, eventBucket, mentions) =>
        {
            if (bucket is not null && eventBucket != bucket)
            {
                return;
            }

            var provider = EventJson.Provider(profilerEvent.Provider).Name;
            var key = (provider, profilerEvent.Name, eventBucket);
            kinds[key] = kinds.GetValueOrDefault(key) + 1;
            foreach (var mention in mentions)
            {
                objects.TryAdd(mention.ObjectId, mention);
            }

            if (matching >= offset && matching < (long)offset + limit)
            {
                var omitted = Math.Max(mentions.Count - EventMentions, 0);
                capped += omitted > 0 ? 1 : 0;
                page.Add(new
                {
                    ms = EventJson.Ms(profilerEvent.RelativeMilliseconds),
                    threadId = profilerEvent.ThreadId,
                    provider,
                    name = profilerEvent.Name,
                    bucket = eventBucket,
                    // The objects in the event's own fields first: an event's mentions come by object, so a cleared
                    // parent can come after its first child. OrderBy is stable, so each group stays in that order.
                    mentions = mentions.OrderBy(mention => mention.Field.StartsWith(ClearedChildField, StringComparison.Ordinal))
                        .Take(EventMentions).Select(mention => new
                        {
                            objectId = mention.ObjectId,
                            kind = mention.Kind.ToString(),
                            field = mention.Field,
                            type = mention.Type,
                            name = mention.Name,
                        }).ToList(),
                    omittedMentions = omitted > 0 ? omitted : (int?)null,
                });
            }

            matching++;
        }, cancellationToken);
        var names = kinds.OrderByDescending(pair => pair.Value).ThenBy(pair => pair.Key.Provider, StringComparer.Ordinal)
            .ThenBy(pair => pair.Key.Name, StringComparer.Ordinal).ToList();
        var types = objects.Values.GroupBy(mention => (mention.Kind, mention.Type))
            .OrderByDescending(group => group.Count()).ThenBy(group => group.Key.Kind)
            .ThenBy(group => group.Key.Type, StringComparer.Ordinal).ToList();
        var notes = new List<string>(SessionCatalog.Notes(info));
        AddTickNotes([tick], notes);
        if (names.Count > TickNames)
        {
            var unlisted = names.Skip(TickNames).ToList();
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"Only the {TickNames} of {names.Count:N0} event names with the most events are listed; the other " +
                $"{EventJson.Count(unlisted.Count, "name has", "names have")} " +
                $"{EventJson.Count(unlisted.Sum(pair => pair.Value), "event", "events")}.") +
                (bucket is null ? " Pass bucket (Xaml, Composition or Other) to list one bucket's names." : ""));
        }

        if (types.Count > TickTypes)
        {
            var unlisted = types.Skip(TickTypes).ToList();
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"Only the {TickTypes} of {types.Count:N0} types with the most objects are listed; the other " +
                $"{EventJson.Count(unlisted.Count, "type has", "types have")} " +
                $"{EventJson.Count(unlisted.Sum(group => group.Count()), "object", "objects")}."));
        }

        var untyped = objects.Values.Count(mention => mention.Type is null);
        if (untyped > 0)
        {
            notes.Add(ObjectTools.UntypedNote(untyped));
        }

        if (capped > 0)
        {
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"{EventJson.Count(capped, "event on this page mentions", "events on this page mention")} more than " +
                $"{EventMentions} objects (clearing a visual's children mentions every child). Each such event lists " +
                $"{EventMentions} of them, those in its own fields first, and its omittedMentions says how many more it " +
                $"mentions. The objects counted by type include them all."));
        }

        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "tick",
            tick = DescribeTick(tick, region),
            region = region is null ? null : DescribeRegion(region),
            eventNames = names.Count,
            events = names.Take(TickNames).Select(pair => new
            {
                provider = pair.Key.Provider,
                name = pair.Key.Name,
                bucket = pair.Key.Bucket,
                count = pair.Value,
            }).ToList(),
            objectTypes = types.Count,
            objects = types.Take(TickTypes).Select(group => new
            {
                kind = group.Key.Kind.ToString(),
                type = group.Key.Type,
                typeUnknown = group.Key.Type is null ? true : (bool?)null,
                objects = group.Count(),
            }).ToList(),
            total,
            bucket,
            matching,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, matching),
            page,
            notes,
        };
    }

    /// <summary>A region's ID: its StartSeq, by which its rows name it.</summary>
    internal static long RegionId(InterestingRow region) => region.RegionId ?? region.StartSeq;

    /// <summary>The regions by the tick each ends with.</summary>
    private static Dictionary<long, InterestingRow> RegionsByTick(IReadOnlyList<InterestingRow> rows) => rows
        .Where(row => row.Type == InterestingRowType.Region && row.TickId is not null)
        .GroupBy(row => row.TickId!.Value)
        .ToDictionary(group => group.Key, group => group.First());

    private static List<object> ByTrigger(IReadOnlyList<InterestingRow> regions) => [.. regions
        .GroupBy(region => region.Trigger ?? "")
        .OrderByDescending(group => group.Count()).ThenBy(group => group.Key, StringComparer.Ordinal)
        .Select(group => (object)new
        {
            trigger = group.Key,
            count = group.Count(),
            totalMs = EventJson.Ms(group.Sum(region => region.DurationMilliseconds)),
            maxMs = EventJson.Ms(group.Max(region => region.DurationMilliseconds)),
        })];

    private static object DescribeTick(SessionFrame tick, InterestingRow? region) => new
    {
        tickId = tick.FrameId,
        threadId = tick.ThreadId,
        frameNumber = tick.FrameNumber,
        drew = tick.Drawn,
        startMs = EventJson.Ms(tick.StartMilliseconds),
        endMs = EventJson.Ms(tick.EndMilliseconds),
        tickMs = EventJson.Ms(tick.TickMilliseconds),
        slow = tick.Slow,
        events = tick.Events,
        xaml = tick.Xaml,
        composition = tick.Composition,
        other = tick.Other,
        endsRegion = region is null ? null : new { regionId = RegionId(region), trigger = region.Trigger },
    };

    private static object? EndTick(InterestingRow region, Dictionary<long, SessionFrame> ticks)
    {
        if (region.TickId is not { } id || !ticks.TryGetValue(id, out var tick))
        {
            return null;
        }

        return new
        {
            tickId = tick.FrameId,
            frameNumber = tick.FrameNumber,
            drew = tick.Drawn,
            tickMs = EventJson.Ms(tick.TickMilliseconds),
            slow = tick.Slow,
        };
    }

    private static object DescribeRegion(InterestingRow region) => new
    {
        regionId = RegionId(region),
        trigger = region.Trigger,
        threadId = region.ThreadId,
        startMs = EventJson.Ms(region.StartMilliseconds),
        stopMs = EventJson.Ms(region.StopMilliseconds),
        durationMs = EventJson.Ms(region.DurationMilliseconds),
        endsWithTickId = region.TickId,
    };

    private static object DescribeRow(InterestingRow row) => new
    {
        type = row.Type.ToString(),
        name = row.Name,
        threadId = row.ThreadId,
        startMs = EventJson.Ms(row.StartMilliseconds),
        stopMs = EventJson.Ms(row.StopMilliseconds),
        durationMs = EventJson.Ms(row.DurationMilliseconds),
        frameNumber = row.FrameNumber,
        tickId = row.TickId,
        regionId = row.RegionId,
    };
}
