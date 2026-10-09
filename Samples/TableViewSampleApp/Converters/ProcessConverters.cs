// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Converters;

// ConverterParameter names the metric: Cpu, Memory, Disk or Network.
public sealed partial class ProcessHeatBrushConverter : IValueConverter
{
    private static SolidColorBrush[]? s_levels;

    private static readonly double[] s_cpu = { 0.5, 2, 5, 10 };
    private static readonly double[] s_memory = { 50, 200, 500, 1000 };
    private static readonly double[] s_disk = { 0.1, 0.5, 2, 5 };
    private static readonly double[] s_network = { 0.05, 0.3, 1, 5 };

    public object Convert(object value, Type targetType, object parameter, string language)
    {
        // Created on first use on the UI thread; ChipBrushes recolours them under a Contrast theme.
        s_levels ??= new[] { 22, 38, 58, 82, 110 }
            .Select(alpha => ChipBrushes.CreateTint(96, 160, 240, (byte)alpha))
            .ToArray();

        var thresholds = (parameter as string) switch
        {
            "Memory" => s_memory,
            "Disk" => s_disk,
            "Network" => s_network,
            _ => s_cpu,
        };

        var load = value is double d ? d : 0;
        return s_levels[thresholds.Count(threshold => load > threshold)];
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}
