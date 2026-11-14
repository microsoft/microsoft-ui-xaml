// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;

namespace WinUIProfiler.App.ViewModels;

// A milestone in the Launch tab: its texts, and its bar in the waterfall, from the milestone before it to it. A record,
// so a refresh replaces only the rows that changed; its properties are get-only, as XAML's type information needs.
public sealed record LaunchMilestoneRow
{
    public LaunchMilestoneRow(string label, string time, string sincePrevious, string eventText, string thread, string count,
        Thickness barMargin, double barWidth, string accessibleName)
    {
        Label = label;
        Time = time;
        SincePrevious = sincePrevious;
        Event = eventText;
        Thread = thread;
        Count = count;
        BarMargin = barMargin;
        BarWidth = barWidth;
        AccessibleName = accessibleName;
    }

    public string Label { get; }
    public string Time { get; }
    public string SincePrevious { get; }
    public string Event { get; }
    public string Thread { get; }
    public string Count { get; }
    public Thickness BarMargin { get; }
    public double BarWidth { get; }
    public string AccessibleName { get; }

    public override string ToString() => AccessibleName;
}