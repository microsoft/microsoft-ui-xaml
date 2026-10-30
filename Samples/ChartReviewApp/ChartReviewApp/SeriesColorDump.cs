// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.IO;
using System.Linq;
using System.Text;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Media;

namespace ChartsSample
{
    // Test hook: --dump-series-colors=<path> rewrites <path> twice a second with the app-set series colors
    // of the Line, Area and Bar charts and the current surface, so contrast fixes can be checked through
    // UI Automation when no screen capture is possible (for example a disconnected remote session).
    public sealed partial class MainWindow
    {
        private DispatcherTimer _seriesColorDump;

        private void StartSeriesColorDumpIfRequested()
        {
            string path = Environment.GetCommandLineArgs()
                .FirstOrDefault(a => a.StartsWith("--dump-series-colors=", StringComparison.OrdinalIgnoreCase))?
                .Substring("--dump-series-colors=".Length);
            if (string.IsNullOrEmpty(path)) return;
            _seriesColorDump = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(500) };
            _seriesColorDump.Tick += (_, _) =>
            {
                var text = new StringBuilder();
                text.AppendLine($"theme={RootGrid.ActualTheme} surface={(_darkSurface ? "dark" : "light")}");
                foreach (var (name, chart) in new[] { ("Line", MarkupChart), ("Area", AreaMarkupChart), ("Bar", BarMarkupChart) })
                {
                    foreach (var series in chart.Series)
                    {
                        Brush fill = series is AreaSeries area ? area.Fill : series is BarSeries bar ? bar.Fill : null;
                        text.AppendLine($"{name}|{series.Title}|stroke={Hex(series.Stroke)}|fill={Hex(fill)}");
                    }
                }
                try
                {
                    File.WriteAllText(path, text.ToString());
                }
                catch (IOException)
                {
                }
            };
            _seriesColorDump.Start();
        }

        private static string Hex(Brush brush) =>
            brush is SolidColorBrush solid ? $"#{solid.Color.A:X2}{solid.Color.R:X2}{solid.Color.G:X2}{solid.Color.B:X2}" : "none";
    }
}
