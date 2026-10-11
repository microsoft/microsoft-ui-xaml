// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;

namespace TableViewSampleApp.Helpers;

// Column factory for pages that build their columns in code (Hierarchy and the self-checks), so a
// column is just header, bound property and width.
internal static class SampleColumns
{
    public static TableViewTextColumn Text(string header, string propertyPath, GridLength width) =>
        new TableViewTextColumn
        {
            Header = header,
            Binding = new Binding { Path = new PropertyPath(propertyPath) },
            Width = width,
        };

    public static GridLength Star(double factor = 1) => new GridLength(factor, GridUnitType.Star);
    public static GridLength Pixels(double px) => new GridLength(px, GridUnitType.Pixel);
}
