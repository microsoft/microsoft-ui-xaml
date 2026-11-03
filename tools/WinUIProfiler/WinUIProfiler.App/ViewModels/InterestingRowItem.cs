// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.ViewModels;

// A region of interest, or another interesting row, in the Frames tab. Text describes it in the list: a region's cause
// and span, or another row's name, thread and span.
public sealed class InterestingRowItem(InterestingRow row, string text)
{
    public InterestingRow Row { get; } = row;
    public string Text { get; } = text;
    public bool IsRegion => Row.Type == InterestingRowType.Region;
    // The tick a region ends with, when it is listed.
    public FrameRow? Tick { get; set; }
    public string Start => Row.StartMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Duration => Row.DurationMilliseconds.ToString("N1", CultureInfo.CurrentCulture);
    public string Frame => Row.FrameNumber?.ToString("N0", CultureInfo.CurrentCulture) ?? "";
    public string Name => Row.Name;

    public override string ToString() => IsRegion ? $"{Name} | {Text}" : Text;
}
