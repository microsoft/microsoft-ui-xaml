// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace WinUIProfiler.Engine.Storage;

// The kinds of interesting rows: the Type column of the InterestingRows view (SessionSchema).
public enum InterestingRowType
{
    Region,
    Frame,
    UpdateLayout,
    Operation,
    FocusChange,
    CreateGraphicsDevice,
    Navigating,
    Navigated
}

// One row of the InterestingRows view: a region of interest, or an interesting row. The view holds the rules (see the
// README). Name is the row's type: the kind, or for an operation the name PerfXamlEvent gives it. Trigger is a region's
// cause. A row's events are its thread's from StartSeq to StopSeq; a region that a focus change started begins at the
// focus event, which another thread can have written. Times are milliseconds from the start of the recording.
// FrameNumber is a tick row's. RegionId is the region the row starts in, by the region's StartSeq (a region's is its
// own). TickId is the tick a region ends with, or a tick row's tick.
public sealed record InterestingRow(InterestingRowType Type, string Name, int ThreadId, long StartSeq, long StopSeq,
    double StartMilliseconds, double StopMilliseconds, long? FrameNumber, string? Trigger = null, long? RegionId = null,
    long? TickId = null)
{
    public double DurationMilliseconds => StopMilliseconds - StartMilliseconds;
}