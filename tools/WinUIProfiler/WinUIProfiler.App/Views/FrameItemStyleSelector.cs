// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

// Gives each kind of row in the Frames tab its own container style, which tints the row: a tick, the tick a region of
// interest ends with, the region, and an interesting row that is not a tick. The list applies it each time it prepares a
// container, so a recycled container gets the style of its new row.
public sealed partial class FrameItemStyleSelector : StyleSelector
{
    public Style? TickStyle { get; set; }
    public Style? RegionTickStyle { get; set; }
    public Style? RegionStyle { get; set; }
    public Style? NonTickStyle { get; set; }

    protected override Style? SelectStyleCore(object item, DependencyObject container) => item switch
    {
        FrameRow { Region: not null } => RegionTickStyle,
        FrameRow => TickStyle,
        InterestingRowItem { IsRegion: true } => RegionStyle,
        _ => NonTickStyle
    };
}
