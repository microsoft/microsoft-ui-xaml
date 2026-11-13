// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

// The template of each kind of row in the Frames tab: a tick, a region of interest, and an interesting row that is not a
// tick.
public sealed partial class FrameItemTemplateSelector : DataTemplateSelector
{
    public DataTemplate? TickTemplate { get; set; }
    public DataTemplate? RegionTemplate { get; set; }
    public DataTemplate? NonTickTemplate { get; set; }

    protected override DataTemplate? SelectTemplateCore(object item) => item switch
    {
        FrameRow => TickTemplate,
        InterestingRowItem { IsRegion: true } => RegionTemplate,
        _ => NonTickTemplate
    };

    protected override DataTemplate? SelectTemplateCore(object item, DependencyObject container) => SelectTemplateCore(item);
}
