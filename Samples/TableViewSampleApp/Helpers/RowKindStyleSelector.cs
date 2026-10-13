// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;

namespace TableViewSampleApp.Helpers;

// <snippet Hierarchy>
// One Style per row kind. TableView asks again whenever a row's hierarchy state changes, so a row
// that gains its first child switches to ExpandableRow without being re-created.
public sealed partial class RowKindStyleSelector : StyleSelector
{
    public Style? LeafRow { get; set; }
    public Style? ExpandableRow { get; set; }
    public Style? GroupHeader { get; set; }

    protected override Style? SelectStyleCore(object item, DependencyObject container) => container switch
    {
        TableViewGroupHeader => GroupHeader,
        TableViewRow { IsExpandable: true } => ExpandableRow,
        TableViewRow => LeafRow,
        _ => null,
    };
}
// </snippet>
