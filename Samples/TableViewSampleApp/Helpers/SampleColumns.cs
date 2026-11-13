// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Markup;

namespace TableViewSampleApp.Helpers;

// Column factory for pages that build their columns in code (Hierarchy), so a column is just
// header, bound property and width. A text column is a TableViewTemplateColumn whose cell is a
// TextBlock bound to the property; SortMemberPath names the same property.
internal static class SampleColumns
{
    public static TableViewTemplateColumn Text(string header, string propertyPath, GridLength width) =>
        new TableViewTemplateColumn
        {
            Header = header,
            CellTemplate = TextCellTemplate(propertyPath),
            SortMemberPath = propertyPath,
            Width = width,
        };

    public static DataTemplate TextCellTemplate(string propertyPath) => (DataTemplate)XamlReader.Load(
        "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
        $"<TextBlock Text='{{Binding {propertyPath}}}' VerticalAlignment='Center' Padding='8,4,8,4' TextTrimming='CharacterEllipsis'/>" +
        "</DataTemplate>");

    public static GridLength Star(double factor = 1) => new GridLength(factor, GridUnitType.Star);
    public static GridLength Pixels(double px) => new GridLength(px, GridUnitType.Pixel);
}
