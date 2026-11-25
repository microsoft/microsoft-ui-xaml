// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

// Gives an element's row and a visual's row their own container style, which tints the row. The list applies it each time
// it prepares a container, so a recycled container gets the style of its new row.
public sealed partial class ObjectRowStyleSelector : StyleSelector
{
    public Style? ElementStyle { get; set; }
    public Style? VisualStyle { get; set; }

    protected override Style? SelectStyleCore(object item, DependencyObject container) =>
        item is ObjectRow { Item.Kind: ObjectKind.Visual } ? VisualStyle : ElementStyle;
}
