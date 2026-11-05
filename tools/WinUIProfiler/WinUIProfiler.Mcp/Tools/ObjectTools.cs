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
/// The UI elements and Composition visuals a session's events mention, as the app's object processor stored them
/// (Objects and ObjectEvents): how many of each type, which ones, and what happened to one. Never the events' field values.
/// </summary>
[McpServerToolType]
internal static class ObjectTools
{
    internal const string NoObjects = "No event in the session mentions a UI element or a Composition visual. Elements come " +
        "from Microsoft-Windows-XAML's TraceLogging events, such as DependencyObject_Enter, and from the XAML-Profiler " +
        "provider's events; visuals come only from the XAML-Profiler provider's visual and visual surface events.";

    internal const string NoVisuals = "No event in the session mentions a Composition visual. Visuals come only from the " +
        "XAML-Profiler provider's visual and visual surface events, and most of those need a WinUI build with " +
        "XamlProfilerEnabled, such as a Debug build from main.";

    internal const string NoElements = "No event in the session mentions a UI element. Elements come from " +
        "Microsoft-Windows-XAML's TraceLogging events, such as DependencyObject_Enter, and from the XAML-Profiler provider's events.";

    internal static readonly string[] Views = ["types", "list", "history"];

    private const int MaxLimit = 500;

    private static readonly string[] Kinds = ["Element", "Visual"];

    private static readonly string[] Orders = ["time", "events"];

    [McpServerTool(Name = "objects", ReadOnly = true, Idempotent = true, OpenWorld = false), Description(
        "Answers questions about the UI elements and Composition visuals a session's events mention, from what the WinUI " +
        "Profiler app's object processor stored (its Objects tab shows the same). An object lives in the file from the first " +
        "to the last event that mentions it; its type is the nearest built-in type; a visual's name is its owner element's " +
        "label. Views: \"types\" (the default): how many elements and visuals of each type, and how many events mention " +
        "them, most objects first, with totals per kind. \"list\": the objects of one type (its full or short name, such as " +
        "Button), or whose name contains some text, in the order they were first mentioned or most events first, each with " +
        "its objectId, address, type, name, how many events mention it, and when it was first and last mentioned. " +
        "\"history\": what happened to one object, by objectId or address: its events by provider and name, and a page of " +
        "them in order, each with its time, thread, the fields that mention the object and the number of the frame it " +
        "belongs to. Times are in ms (see the session's timeBase). Events' field values aren't shown.")]
    public static Task<object> Objects(
        [Description("\"types\", \"list\" or \"history\". Omit it for the types view, or for the history view when objectId " +
            "or address is given, or the list view when type or name is given.")]
        string? view = null,
        [Description(SessionCatalog.SessionPathDescription)] string? sessionPath = null,
        [Description("The types and list views: only Element (UI elements) or Visual (Composition visuals).")]
        string? kind = null,
        [Description("The list view: only objects of this type, by its full name, such as Microsoft.UI.Xaml.Controls.Button, " +
            "or its short name, such as Button, ignoring case.")]
        string? type = null,
        [Description("The list view: only objects whose name (an element's x:Name, or a visual's owner element's label) " +
            "contains this text, ignoring case.")]
        string? name = null,
        [Description("The history view: the object, by the objectId the list view gives.")] long? objectId = null,
        [Description("The history view: the object, by its address, in hex with 0x as the tools show it, or as a decimal " +
            "number. When several objects had the address during the session, they're listed instead.")]
        string? address = null,
        [Description("The list view: \"time\" (the default, first mentioned first) or \"events\" (most events first).")]
        string? orderBy = null,
        [Description("How many types, objects or events to skip, for the next page.")] int offset = 0,
        [Description("The most types, objects or events to return, from 1 to 500. Omit it for 50.")] int? limit = null,
        CancellationToken cancellationToken = default)
    {
        var typeText = ToolArguments.Text(type);
        var nameText = ToolArguments.Text(name);
        var addressText = ToolArguments.Text(address);
        var chosen = ToolArguments.Text(view) is not null ? ToolArguments.View(view, Views, "objects")
            : objectId is not null || addressText is not null ? "history"
            : typeText is not null || nameText is not null ? "list"
            : "types";
        ToolArguments.OnlyFor(objectId is not null, nameof(objectId), "objects", chosen, "history");
        ToolArguments.OnlyFor(addressText is not null, nameof(address), "objects", chosen, "history");
        ToolArguments.OnlyFor(typeText is not null, nameof(type), "objects", chosen, "list");
        ToolArguments.OnlyFor(nameText is not null, nameof(name), "objects", chosen, "list");
        ToolArguments.OnlyFor(ToolArguments.Text(kind) is not null, nameof(kind), "objects", chosen, "types", "list");
        ToolArguments.OnlyFor(ToolArguments.Text(orderBy) is not null, nameof(orderBy), "objects", chosen, "list");
        ToolArguments.NotNegative(offset, nameof(offset));
        var count = ToolArguments.Range(limit ?? 50, 1, MaxLimit, nameof(limit));
        ObjectKind? chosenKind = ToolArguments.Choice(kind, Kinds, "kind") is { } kindName ? Enum.Parse<ObjectKind>(kindName) : null;
        var byEvents = ToolArguments.Choice(orderBy, Orders, "orderBy") == "events";
        long? wantedAddress = null;
        if (chosen == "history")
        {
            if ((objectId is null) == (addressText is null))
            {
                throw new McpException("The history view needs one of objectId and address, not both. The list view gives both.");
            }

            wantedAddress = addressText is null ? null : ToolArguments.Address(addressText);
        }

        return SessionCatalog.RunAsync(sessionPath, path => chosen switch
        {
            "list" => List(path, chosenKind, typeText, nameText, byEvents, offset, count),
            "history" => History(path, objectId, wantedAddress, offset, count, cancellationToken),
            _ => Types(path, chosenKind, offset, count),
        }, cancellationToken);
    }

    private static object Types(string path, ObjectKind? kind, int offset, int count)
    {
        var info = SessionFile.ReadInfo(path);
        var objects = SessionFile.ReadObjects(path, kind);
        var groups = objects.GroupBy(item => (item.Kind, item.Type))
            .Select(group => (group.Key.Kind, group.Key.Type, Objects: group.Count(), Events: group.Sum(item => item.Events)))
            .OrderByDescending(group => group.Objects).ThenByDescending(group => group.Events).ThenBy(group => group.Kind)
            .ThenBy(group => group.Type, StringComparer.Ordinal)
            .ToList();
        var page = groups.Skip(offset).Take(count).Select(group => new
        {
            kind = group.Kind.ToString(),
            type = group.Type,
            typeUnknown = group.Type is null ? true : (bool?)null,
            objects = group.Objects,
            events = group.Events,
        }).ToList();
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "types",
            kind = kind?.ToString(),
            totals = objects.GroupBy(item => item.Kind).OrderBy(group => group.Key).Select(group => new
            {
                kind = group.Key.ToString(),
                objects = group.Count(),
                types = group.Where(item => item.Type is not null).Select(item => item.Type).Distinct(StringComparer.Ordinal).Count(),
                withoutType = group.Count(item => item.Type is null),
                named = group.Count(item => item.Name is not null),
            }).ToList(),
            total = groups.Count,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, groups.Count),
            types = page,
            notes = Notes(info, objects, kind),
        };
    }

    private static object List(string path, ObjectKind? kind, string? type, string? name, bool byEvents, int offset, int count)
    {
        var info = SessionFile.ReadInfo(path);
        var objects = SessionFile.ReadObjects(path, kind);
        var ofType = type is null ? objects : objects.Where(item => IsType(item.Type, type)).ToList();
        var matching = name is null ? ofType : ofType.Where(item => item.Name?.Contains(name, StringComparison.OrdinalIgnoreCase) == true);
        var ordered = (byEvents
            ? matching.OrderByDescending(item => item.Events).ThenBy(item => item.FirstMilliseconds).ThenBy(item => item.ObjectId)
            : matching).ToList();
        var page = ordered.Skip(offset).Take(count).Select(Row).ToList();
        var notes = Notes(info, objects, kind);
        if (type is not null && ofType.Count == 0 && objects.Count > 0)
        {
            var similar = objects.Select(item => item.Type).OfType<string>().Distinct(StringComparer.Ordinal)
                .Where(itemType => itemType.Contains(type, StringComparison.OrdinalIgnoreCase))
                .Order(StringComparer.OrdinalIgnoreCase).Take(10).ToList();
            notes.Add(similar.Count == 0
                ? $"No object has the type {type}. The types view lists the types."
                : $"No object has the type {type}. These types contain it: {string.Join(", ", similar)}.");
        }

        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "list",
            kind = kind?.ToString(),
            type,
            name,
            orderBy = byEvents ? "events" : "time",
            total = ordered.Count,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, ordered.Count),
            objects = page,
            notes,
        };
    }

    private static object History(string path, long? objectId, long? address, int offset, int count,
        CancellationToken cancellationToken)
    {
        var info = SessionFile.ReadInfo(path);
        var matches = SessionFile.ReadObjects(path, objectId: objectId, address: address);
        var what = objectId is { } id
            ? string.Create(CultureInfo.InvariantCulture, $"objectId {id}")
            : $"the address {EventJson.Hex(address!.Value)}";
        if (matches.Count == 0)
        {
            throw new McpException($"No object in {path} has {what}" +
                (info.CompleteFromSeq is null ? "" : ", or its events were deleted to keep the file within its size limit") +
                ". The list view lists the objects.");
        }

        if (matches.Count > 1)
        {
            return new
            {
                session = SessionCatalog.Header(path, info),
                view = "history",
                ambiguous = true,
                message = string.Create(CultureInfo.InvariantCulture,
                    $"{matches.Count} objects had {what} during the session: the runtime reuses an address once its " +
                    $"object is freed. Call the history view again with one's objectId."),
                objects = matches.Select(Row).ToList(),
                notes = SessionCatalog.Notes(info),
            };
        }

        var item = matches[0];
        var kinds = new Dictionary<(string Provider, string Name), (long Count, double FirstMs)>();
        var page = new List<object>();
        long index = 0;
        var total = SessionFile.ReadTimeline(path, item.ObjectId, 0, -1, (profilerEvent, fields, frameNumber) =>
        {
            var provider = EventJson.Provider(profilerEvent.Provider).Name;
            var key = (provider, profilerEvent.Name);
            kinds[key] = kinds.TryGetValue(key, out var seen)
                ? (seen.Count + 1, seen.FirstMs)
                : (1, profilerEvent.RelativeMilliseconds);
            if (index >= offset && index < (long)offset + count)
            {
                page.Add(new
                {
                    ms = EventJson.Ms(profilerEvent.RelativeMilliseconds),
                    threadId = profilerEvent.ThreadId,
                    provider,
                    name = profilerEvent.Name,
                    mentionedIn = fields,
                    frameNumber,
                });
            }

            index++;
        }, cancellationToken);
        return new
        {
            session = SessionCatalog.Header(path, info),
            view = "history",
            @object = Row(item),
            events = kinds.OrderByDescending(pair => pair.Value.Count).ThenBy(pair => pair.Value.FirstMs)
                .ThenBy(pair => pair.Key.Provider, StringComparer.Ordinal).ThenBy(pair => pair.Key.Name, StringComparer.Ordinal)
                .Select(pair => new
                {
                    provider = pair.Key.Provider,
                    name = pair.Key.Name,
                    count = pair.Value.Count,
                    firstMs = EventJson.Ms(pair.Value.FirstMs),
                }).ToList(),
            total,
            offset,
            returned = page.Count,
            nextOffset = ToolArguments.NextOffset(offset, page.Count, total),
            page,
            notes = SessionCatalog.Notes(info),
        };
    }

    /// <summary>Whether an object's type is the given one, by its full name or its short name (after the last dot).</summary>
    private static bool IsType(string? itemType, string type) => itemType is not null &&
        (string.Equals(itemType, type, StringComparison.OrdinalIgnoreCase) ||
            string.Equals(itemType[(itemType.LastIndexOf('.') + 1)..], type, StringComparison.OrdinalIgnoreCase));

    private static List<string> Notes(SessionFileInfo info, IReadOnlyList<SessionObject> objects, ObjectKind? kind)
    {
        var notes = new List<string>(SessionCatalog.Notes(info));
        if (objects.Count == 0)
        {
            notes.Add(kind switch
            {
                ObjectKind.Element => NoElements,
                ObjectKind.Visual => NoVisuals,
                _ => NoObjects,
            });
        }
        else if (kind is null && !objects.Any(item => item.Kind == ObjectKind.Visual))
        {
            notes.Add(NoVisuals);
        }

        var untyped = objects.Count(item => item.Type is null);
        if (untyped > 0)
        {
            notes.Add(string.Create(CultureInfo.InvariantCulture,
                $"{untyped} objects have no type: none of the events that mention them gave it. An element's type comes from " +
                $"DependencyObject_Enter, a visual's from its insertion."));
        }

        return notes;
    }

    private static object Row(SessionObject item) => new
    {
        objectId = item.ObjectId,
        kind = item.Kind.ToString(),
        address = EventJson.Hex(item.Address),
        type = item.Type,
        name = item.Name,
        events = item.Events,
        firstMs = EventJson.Ms(item.FirstMilliseconds),
        lastMs = EventJson.Ms(item.LastMilliseconds),
    };
}
