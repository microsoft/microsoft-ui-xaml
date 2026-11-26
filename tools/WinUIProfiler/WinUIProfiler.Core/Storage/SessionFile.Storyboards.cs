// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Events;

namespace WinUIProfiler.Engine.Storage;

// The most recently observed run of a storyboard lifetime, not proof it remains active after LastMilliseconds.
public sealed record SessionStoryboard(ulong StoryboardId, ulong ObjectAddress, ulong RunId, string Name,
    string Operation, string State, bool IsPaused, bool HasIndependentAnimation, bool IsInTimeManager,
    uint? ChildCount, int ThreadId, double FirstMilliseconds, double LastMilliseconds, bool BeginObserved, bool BeginFailed);

public sealed record StoryboardSnapshot(SessionFileInfo Info, IReadOnlyList<SessionStoryboard> Storyboards,
    long Observations, bool Limited)
{
    public bool HistoryIncomplete => Info.CompleteFromSeq is > 1 || Info.LostByEtw is > 0
        || Info.DroppedByProfiler is > 0 || Limited || Storyboards.Any(row => !row.BeginObserved && !row.BeginFailed);
}

public static partial class SessionFile
{
    // Reads only StoryboardActivity records. At a selected time, later observations cannot affect the result.
    // Keeps a bounded, most recently observed set; an evicted lifetime that returns has partial history.
    public static StoryboardSnapshot ReadStoryboards(string path, double? atMilliseconds = null, int limit = 500,
        CancellationToken cancellationToken = default)
    {
        if (atMilliseconds is { } time && (!double.IsFinite(time) || time < 0))
        {
            throw new ArgumentOutOfRangeException(nameof(atMilliseconds));
        }
        ArgumentOutOfRangeException.ThrowIfLessThan(limit, 1);
        ArgumentOutOfRangeException.ThrowIfGreaterThan(limit, 10_000);

        using var snapshot = OpenSnapshot(path);
        var rows = new Dictionary<ulong, LinkedListNode<SessionStoryboard>>();
        var recent = new LinkedList<SessionStoryboard>();
        long observations = 0;
        var limited = false;
        snapshot.ReadEvents([(XamlProviders.XamlProfiler, "StoryboardActivity")], null, value =>
        {
            if (atMilliseconds is { } at && value.RelativeMilliseconds > at)
            {
                return;
            }

            var next = ReadStoryboard(value);
            observations++;
            if (rows.TryGetValue(next.StoryboardId, out var node))
            {
                var previous = node.Value;
                if (previous.ObjectAddress != next.ObjectAddress)
                {
                    throw new InvalidDataException("A StoryboardActivity lifetime changed its object address.");
                }
                if (next.RunId < previous.RunId)
                {
                    return;
                }
                if (next.RunId == previous.RunId)
                {
                    var latest = next.LastMilliseconds < previous.LastMilliseconds ? previous : next;
                    var earlier = next.LastMilliseconds < previous.LastMilliseconds ? next : previous;
                    next = latest with
                    {
                        FirstMilliseconds = Math.Min(previous.FirstMilliseconds, next.FirstMilliseconds),
                        BeginObserved = previous.BeginObserved || next.BeginObserved,
                        BeginFailed = previous.BeginFailed || next.BeginFailed,
                        ChildCount = latest.ChildCount ?? earlier.ChildCount,
                        Name = latest.Name.Length == 0 ? earlier.Name : latest.Name
                    };
                }
                node.Value = next;
                recent.Remove(node);
                recent.AddLast(node);
            }
            else
            {
                if (rows.Count == limit)
                {
                    rows.Remove(recent.First!.Value.StoryboardId);
                    recent.RemoveFirst();
                    limited = true;
                }
                rows.Add(next.StoryboardId, recent.AddLast(next));
            }
        }, cancellationToken);

        return new(snapshot.Info, recent.OrderByDescending(row => row.LastMilliseconds).ToArray(), observations, limited);
    }

    private static SessionStoryboard ReadStoryboard(ProfilerEvent value)
    {
        var fields = new Dictionary<string, object?>(StringComparer.Ordinal);
        foreach (var field in value.Fields)
        {
            if (!fields.TryAdd(field.Name, field.Value))
            {
                throw InvalidStoryboardField(field.Name);
            }
        }

        object Required(string name) => fields.TryGetValue(name, out var item) && item is not null
            ? item : throw InvalidStoryboardField(name);
        string Text(string name) => Required(name) is string text ? text : throw InvalidStoryboardField(name);
        bool Flag(string name) => Required(name) is bool flag ? flag : throw InvalidStoryboardField(name);
        ulong Number(string name) => Required(name) switch
        {
            ulong number => number,
            long number when number >= 0 => (ulong)number,
            uint number => number,
            int number when number >= 0 => (ulong)number,
            _ => throw InvalidStoryboardField(name)
        };

        var id = Number("StoryboardId");
        var address = Number("ObjectAddress");
        var run = Number("RunId");
        var operation = Text("Operation");
        var state = Text("State");
        var children = Number("ChildCount");
        if (id == 0) throw InvalidStoryboardField("StoryboardId");
        if (address == 0) throw InvalidStoryboardField("ObjectAddress");
        if (children > uint.MaxValue) throw InvalidStoryboardField("ChildCount");
        if (operation is not ("BeginRequested" or "Begin" or "BeginFailed" or "Registered" or "Unregistered"
            or "PauseRequested" or "ResumeRequested" or "StopRequested" or "Observed" or "Destroyed"))
        {
            throw InvalidStoryboardField("Operation");
        }
        if (state is not ("NotStarted" or "Stopped" or "Active" or "Filling" or "Paused" or "Destroyed"))
        {
            throw InvalidStoryboardField("State");
        }

        return new(id, address, run, Text("Name"), operation, state, Flag("IsPaused"),
            Flag("HasIndependentAnimation"), Flag("IsInTimeManager"), operation == "Destroyed" ? null : (uint)children, value.ThreadId,
            value.RelativeMilliseconds, value.RelativeMilliseconds, operation == "Begin", operation == "BeginFailed");
    }

    private static InvalidDataException InvalidStoryboardField(string field) =>
        new($"Missing, invalid or unsupported StoryboardActivity field '{field}'.");
}
