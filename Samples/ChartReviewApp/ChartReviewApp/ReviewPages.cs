// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.ApplicationModel.DataTransfer;
using Windows.UI.ViewManagement;

namespace ChartsSample
{
    // Review-only pages: Design reference, Accessibility and About. Kept out of the upstream
    // ChartApp files so the review app stays a small, readable delta on the engineering sample.
    public sealed partial class MainWindow
    {
        internal const string ReviewAppName = "Charts Review App";
        internal const string ReviewAppVersion = "1.5";

        private readonly AccessibilitySettings _accessibility = new();
        private readonly UISettings _uiSettings = new();
        private TextBlock _environmentText;

        private static readonly string[] Week = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        private static readonly string[] WorkWeek = { "Mon", "Tue", "Wed", "Thu", "Fri" };

        internal static string BuildLabel => $"{ReviewAppName} {ReviewAppVersion}";

        private void BuildReviewPages()
        {
            BuildDesignReference();
            BuildAccessibilityCases();
            BuildScenariosPage();
            BuildAbout();
            RootGrid.Loaded += OnReviewRootLoaded;
            // AccessibilitySettings.HighContrastChanged cannot be subscribed in an unpackaged desktop
            // app (0x80070490); contrast changes raise ActualThemeChanged, handled in BuildAbout.
            try
            {
                _uiSettings.TextScaleFactorChanged += (_, _) => DispatcherQueue.TryEnqueue(() => { UpdateEnvironmentText(); ApplyTextScaleLayout(); });
            }
            catch (Exception)
            {
            }
        }

        // Size the window in DIPs (the upstream sample sizes it in physical pixels, which is small at
        // 150% display scale), clamped to the work area.
        private void OnReviewRootLoaded(object sender, RoutedEventArgs e)
        {
            RootGrid.Loaded -= OnReviewRootLoaded;
            double scale = RootGrid.XamlRoot?.RasterizationScale ?? 1.0;
            var work = Microsoft.UI.Windowing.DisplayArea.GetFromWindowId(AppWindow.Id, Microsoft.UI.Windowing.DisplayAreaFallback.Primary).WorkArea;
            int width = (int)Math.Min(1440 * scale, work.Width * 0.95);
            int height = (int)Math.Min(960 * scale, work.Height * 0.95);
            if (!ApplyWindowSizeIfRequested())
            {
                AppWindow.MoveAndResize(new Windows.Graphics.RectInt32(work.X + (work.Width - width) / 2, work.Y + (work.Height - height) / 2, width, height));
            }
            UpdateEnvironmentText();
            ApplyTextScaleLayout();
            StartFocusWalkIfRequested();
            DumpDescriptionsIfRequested();
            CheckClippingIfRequested();
        }

        // Fixed-width app chrome grows with the Windows text size so its labels are not cut off.
        private void ApplyTextScaleLayout()
        {
            double textScale = 1.0;
            try { textScale = Math.Max(1.0, _uiSettings.TextScaleFactor); } catch (Exception) { }
            _textScale = textScale;
            ScenarioNavigation.OpenPaneLength = Math.Min(224 * textScale, 440);
            OnViewportSizeChanged(this, null);
        }

        private double _textScale = 1.0;

        private readonly List<Chart> _a11yCharts = new();

        private Chart TrackA11yChart(Chart chart)
        {
            _a11yCharts.Add(chart);
            return chart;
        }

        // Chart heights follow the visible height, so large windows are not left mostly empty.
        private void UpdateResponsiveHeights()
        {
            double view = ScenarioScroll?.ActualHeight ?? 0;
            bool known = view > 0;
            // Large text needs more room before a chart's plot area collapses.
            double textMin = 160 * Math.Min(_textScale, 2.25);
            double a11y = known ? Math.Max(view * 0.3, Math.Max(220, textMin)) : 220;
            foreach (var chart in _a11yCharts) chart.Height = a11y;
            double date = known ? Math.Max(view * 0.4, Math.Max(240, textMin)) : 240;
            DtChartHostA.Height = date;
            DtChartHostB.Height = date;
            if (_scenarioList != null) _scenarioList.MaxHeight = known ? Math.Max(300, view - 230) : 520;
            if (_scenarioPickerColumn != null)
            {
                // Titles wrap, so the list never takes more than a third of the page from the chart.
                double width = ScenarioScroll?.ActualWidth ?? 0;
                double picker = Math.Min(300 * _textScale, 520);
                if (width > 0) picker = Math.Max(240, Math.Min(picker, width * 0.34));
                _scenarioPickerColumn.Width = new GridLength(picker);
            }
            int count = _scenarioCharts.Count;
            if (count > 0)
            {
                double available = known ? Math.Max(300, view - 260) : 380;
                double height = count > 1 ? Math.Max(available / count - 12, Math.Max(240, textMin)) : Math.Max(available, Math.Max(300, textMin));
                foreach (var chart in _scenarioCharts) chart.Height = height;
            }
        }

        // Test hook (not for reviewers): --focus-walk=<count>[:<AutomationId>] moves focus forward the way
        // Tab does, logs each stop to %TEMP%\ChartsReviewApp-focus.txt, then gives the named element
        // keyboard focus so a screenshot shows its focus visual.
        private async void StartFocusWalkIfRequested()
        {
            string spec = null;
            foreach (var arg in Environment.GetCommandLineArgs())
                if (arg.StartsWith("--focus-walk=", StringComparison.OrdinalIgnoreCase)) spec = arg.Substring("--focus-walk=".Length);
            if (spec == null) return;
            string[] parts = spec.Split(':');
            int count = int.TryParse(parts[0], out int n) ? n : 20;
            string target = parts.Length > 1 ? parts[1] : null;
            var log = new StringBuilder();
            await System.Threading.Tasks.Task.Delay(1500);
            var options = new FindNextElementOptions { SearchRoot = RootGrid.XamlRoot.Content };
            ScenarioNavigation.Focus(FocusState.Keyboard);
            for (int i = 1; i <= count; i++)
            {
                await FocusManager.TryMoveFocusAsync(FocusNavigationDirection.Next, options);
                await System.Threading.Tasks.Task.Delay(150);
                var focused = FocusManager.GetFocusedElement(RootGrid.XamlRoot) as DependencyObject;
                string name = focused == null ? "" : AutomationProperties.GetName(focused);
                string id = focused == null ? "" : AutomationProperties.GetAutomationId(focused);
                string type = focused?.GetType().Name ?? "(none)";
                string state = (focused as Control)?.FocusState.ToString() ?? "";
                string visuals = (focused as Control)?.UseSystemFocusVisuals.ToString() ?? "";
                if (focused is FrameworkElement fe && string.IsNullOrEmpty(id)) id = fe.Name;
                log.AppendLine($"{i,2} {type} name='{name}' id='{id}' focusState={state} useSystemFocusVisuals={visuals}");
            }
            if (!string.IsNullOrEmpty(target))
            {
                var element = FindByAutomationId(RootGrid, target) as Control;
                bool ok = element?.Focus(FocusState.Keyboard) ?? false;
                log.AppendLine($"Focus({target}, Keyboard) -> {ok}; FocusState={element?.FocusState}; UseSystemFocusVisuals={element?.UseSystemFocusVisuals}");
                if (element != null) element.StartBringIntoView();
            }
            File.WriteAllText(Path.Combine(Path.GetTempPath(), "ChartsReviewApp-focus.txt"), log.ToString());
        }

        private static DependencyObject FindByAutomationId(DependencyObject root, string id)
        {
            if (AutomationProperties.GetAutomationId(root) == id) return root;
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var found = FindByAutomationId(VisualTreeHelper.GetChild(root, i), id);
                if (found != null) return found;
            }
            return null;
        }

        // ---------------------------------------------------------------- Design reference
        // The eight chart frames from the Fluent Windows Visual Library chart pages, built with
        // product defaults only (no brushes, sizes or thicknesses set by the app).

        private void BuildDesignReference()
        {
            DesignScenario.Children.Add(Note(
                "Each chart below matches a frame on the Figma chart pages (430 × 193) and uses product defaults only: " +
                "the app sets data, axes and orientation, never colors, sizes or line thickness. " +
                "Compare them with Figma in Light and Dark using the theme selector."));

            // Two frames per row when the window is wide enough, one otherwise.
            var grid = new VariableSizedWrapGrid { Orientation = Orientation.Horizontal, ItemWidth = 462, ItemHeight = 256 };

            double[] line = { 220, 570, 420, 1000, 110, 740, 560 };
            double[][] multiLine =
            {
                new double[] { 140, 490, 350, 920, 40, 660, 600 },
                new double[] { 330, 640, 410, 480, 140, 390, 420 },
                new double[] { 540, 540, 380, 580, 210, 470, 500 },
                new double[] { 210, 640, 190, 730, 480, 840, 700 },
                new double[] { 270, 690, 250, 780, 530, 900, 820 },
                new double[] { 410, 600, 340, 660, 210, 470, 380 },
            };
            double[] bar = { 180, 690, 430, 80, 460, 780, 560 };
            double[][] multiBar =
            {
                new double[] { 180, 690, 430, 80, 460, 780, 560 },
                new double[] { 260, 500, 100, 170, 610, 580, 890 },
                new double[] { 200, 380, 80, 130, 470, 450, 690 },
            };
            double[][] horizontal =
            {
                new double[] { 250, 580, 410, 190, 1180 },
                new double[] { 120, 680, 290, 410, 960 },
                new double[] { 90, 540, 230, 330, 760 },
            };

            Place(grid, Frame("Line", "Frame 1: weekly values", Week, 1000, f => AddSeries(f, new LineSeries(), line)), 0, 0);
            Place(grid, Frame("Area", "Frame 2: weekly values", Week, 1000, f => AddSeries(f, new AreaSeries(), line)), 0, 1);
            Place(grid, Frame("Multiple lines with nodes", "Frame 3: six weekly series with nodes", Week, 1000, f =>
            {
                for (int i = 0; i < multiLine.Length; i++)
                    AddSeries(f, new LineSeries { ShowDataMarkers = true, Title = $"Series {i + 1}" }, multiLine[i]);
            }), 1, 0);
            Place(grid, Frame("Line with nodes", "Frame 4: weekly values with nodes", Week, 1000, f => AddSeries(f, new LineSeries { ShowDataMarkers = true }, line)), 1, 1);
            Place(grid, Frame("Column", "Frame 5: weekly values, vertical bars", Week, 1000, f => AddSeries(f, new BarSeries { Orientation = BarOrientation.Vertical }, bar)), 2, 0);
            Place(grid, Frame("Clustered column", "Frame 6: three weekly series, vertical bars", Week, 1000, f =>
            {
                for (int i = 0; i < multiBar.Length; i++)
                    AddSeries(f, new BarSeries { Orientation = BarOrientation.Vertical, Title = $"Series {i + 1}" }, multiBar[i]);
            }), 2, 1);
            Place(grid, Frame("Bar", "Frame 7: work-week values, horizontal bars", WorkWeek, 1500, f => AddSeries(f, new BarSeries { Orientation = BarOrientation.Horizontal }, horizontal[0])), 3, 0);
            Place(grid, Frame("Clustered bar", "Frame 8: three work-week series, horizontal bars", WorkWeek, 1500, f =>
            {
                for (int i = 0; i < horizontal.Length; i++)
                    AddSeries(f, new BarSeries { Orientation = BarOrientation.Horizontal, Title = $"Series {i + 1}" }, horizontal[i]);
            }), 3, 1);
            DesignScenario.Children.Add(grid);
        }

        private sealed class FrameParts
        {
            public Chart Chart;
            public Samples Categories;
            public CategoryAxis X;
            public LinearAxis Y;
        }

        private static StackPanel Frame(string title, string automationName, string[] categories, double maximum, Action<FrameParts> addSeries)
        {
            var x = new CategoryAxis();
            var y = new LinearAxis { Minimum = 0, Maximum = maximum, Spacing = 250, GridLines = GridLines.Major };
            var names = new Samples { ItemsSource = new ObservableCollection<string>(categories) };
            var chart = new Chart { Width = 430, Height = 193 };
            chart.Axes.Add(x);
            chart.Axes.Add(y);
            chart.Data.Add(names);
            AutomationProperties.SetName(chart, automationName);
            AutomationProperties.SetAutomationId(chart, "Design" + title.Replace(" ", string.Empty));
            addSeries(new FrameParts { Chart = chart, Categories = names, X = x, Y = y });

            var panel = new StackPanel { Spacing = 8 };
            panel.Children.Add(Heading(title, AutomationHeadingLevel.Level2));
            panel.Children.Add(chart);
            return panel;
        }

        private static void AddSeries(FrameParts frame, CartesianSeries series, double[] values)
        {
            var samples = new Samples { ItemsSource = new ObservableVector<double>(values) };
            frame.Chart.Data.Add(samples);
            series.XValues = frame.Categories;
            series.YValues = samples;
            series.XAxis = frame.X;
            series.YAxis = frame.Y;
            frame.Chart.Series.Add(series);
        }

        // ---------------------------------------------------------------- Accessibility

        private void BuildAccessibilityCases()
        {
            A11yScenario.Children.Add(Note(
                "Explore these charts with Narrator, Inspect or Accessibility Insights. Each card says what to check. " +
                "The case numbers match the UI scope document."));

            var grid = new Grid { ColumnSpacing = 16, RowSpacing = 16 };
            grid.ColumnDefinitions.Add(new ColumnDefinition());
            grid.ColumnDefinitions.Add(new ColumnDefinition());
            for (int i = 0; i < 3; i++) grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

            // A1: generated name and description.
            {
                var chart = TrackA11yChart(new Chart { Height = 220, ShowLegend = true, LegendTitle = "Quarterly totals" });
                var q = Strings("Q1", "Q2", "Q3", "Q4");
                chart.Data.Add(q);
                AddLine(chart, q, new LineSeries { Title = "Revenue", ShowDataMarkers = true }, 18, 27, 31, 34);
                AddLine(chart, q, new LineSeries { Title = "Cost", ShowDataLabels = true }, 12, 15, 19, 21);
                AutomationProperties.SetAutomationId(chart, "A11yGeneratedChart");
                Place(grid, Case("A1. Generated name and description",
                    "No name is set, so the chart generates its name, “2 series”, and its description from the data.", chart), 0, 0);
            }
            // A2: name and description from the app.
            {
                var chart = TrackA11yChart(new Chart { Height = 220 });
                var regions = Strings("North", "East", "South", "West");
                chart.Data.Add(regions);
                AddLine(chart, regions, new BarSeries { Title = "Open tickets", Orientation = BarOrientation.Vertical }, 34, 21, 9, 17);
                AutomationProperties.SetName(chart, "Open support tickets by region");
                AutomationProperties.SetFullDescription(chart, "North has the most open tickets, 34. South has the fewest, 9.");
                AutomationProperties.SetAutomationId(chart, "A11yNamedChart");
                Place(grid, Case("A2. Name and description from the app",
                    "AutomationProperties.Name and FullDescription replace the generated name and description.", chart), 0, 1);
            }
            // A3: axis title used in point values.
            {
                var chart = TrackA11yChart(new Chart { Height = 220 });
                var months = Strings("Jan", "Feb", "Mar", "Apr", "May", "Jun");
                var monthAxis = new CategoryAxis { Label = "Month" };
                var dollarsAxis = new LinearAxis { Label = "Dollars" };
                chart.Axes.Add(monthAxis);
                chart.Axes.Add(dollarsAxis);
                chart.Data.Add(months);
                AddLine(chart, months, new AreaSeries { Title = "Profit", XAxis = monthAxis, YAxis = dollarsAxis }, 18, 27, 22, 41, 36, 52);
                AutomationProperties.SetAutomationId(chart, "A11yAxisTitleChart");
                Place(grid, Case("A3. Axis title in point values",
                    "The value axis is titled Dollars, so points read “Jan, Dollars=18” rather than “Jan, Value=18”.", chart), 1, 0);
            }
            // A4: dates along the axis (known issue).
            {
                var chart = TrackA11yChart(new Chart { Height = 220 });
                var start = new DateTimeOffset(2024, 1, 8, 0, 0, 0, TimeSpan.Zero);
                var days = new Samples { ItemsSource = new ObservableCollection<DateTimeOffset> { start, start.AddDays(1), start.AddDays(2), start.AddDays(3), start.AddDays(4) } };
                var dayAxis = new DateTimeAxis();
                var visitorsAxis = new LinearAxis();
                chart.Axes.Add(dayAxis);
                chart.Axes.Add(visitorsAxis);
                chart.Data.Add(days);
                AddLine(chart, days, new LineSeries { Title = "Daily visitors", ShowDataMarkers = true, XAxis = dayAxis, YAxis = visitorsAxis }, 120, 135, 128, 150, 142);
                AutomationProperties.SetAutomationId(chart, "A11yDateChart");
                Place(grid, Case("A4. Dates along the axis (known issue)",
                    "Points are named by position, such as “1, Value=120”, not by their dates.", chart), 1, 1);
            }
            // A5: hidden series.
            {
                var chart = TrackA11yChart(new Chart { Height = 220 });
                var months = Strings("Jan", "Feb", "Mar", "Apr", "May", "Jun");
                chart.Data.Add(months);
                AddLine(chart, months, new LineSeries { Title = "Actual" }, 30, 34, 29, 41, 45, 52);
                AddLine(chart, months, new LineSeries { Title = "Target", IsVisible = false }, 35, 35, 40, 40, 45, 45);
                AutomationProperties.SetAutomationId(chart, "A11yHiddenSeriesChart");
                Place(grid, Case("A5. Hidden series",
                    "Target has IsVisible set to False. It is not drawn, and the chart neither counts nor exposes it.", chart), 2, 0);
            }
            // A6: disabled chart.
            {
                var chart = TrackA11yChart(new Chart { Height = 220, IsEnabled = false });
                var weeks = Strings("Week 1", "Week 2", "Week 3", "Week 4");
                chart.Data.Add(weeks);
                AddLine(chart, weeks, new AreaSeries { Title = "Visits" }, 120, 180, 150, 210);
                AutomationProperties.SetAutomationId(chart, "A11yDisabledChart");
                Place(grid, Case("A6. Disabled chart",
                    "IsEnabled is False, so the chart and all of its parts report IsEnabled as false.", chart), 2, 1);
            }
            A11yScenario.Children.Add(grid);
        }

        private static Samples Strings(params string[] values) =>
            new() { ItemsSource = new ObservableCollection<string>(values) };

        private static void AddLine(Chart chart, Samples x, CartesianSeries series, params double[] values)
        {
            var y = new Samples { ItemsSource = new ObservableVector<double>(values) };
            chart.Data.Add(y);
            series.XValues = x;
            series.YValues = y;
            chart.Series.Add(series);
        }

        private Border Case(string title, string note, Chart chart)
        {
            var panel = new StackPanel { Spacing = 8 };
            panel.Children.Add(Heading(title, AutomationHeadingLevel.Level2));
            panel.Children.Add(new TextBlock { Text = note, Style = (Style)RootGrid.Resources["DescriptionStyle"] });
            panel.Children.Add(chart);
            return new Border { Style = (Style)RootGrid.Resources["CardStyle"], Child = panel };
        }

        // ---------------------------------------------------------------- About

        private void BuildAbout()
        {
            var info = ReadBuildInfo();
            AboutScenario.Children.Add(Heading(BuildLabel, AutomationHeadingLevel.Level2));
            AboutScenario.Children.Add(Note(
                "Quote the review build below in every bug, together with the page, the theme, the text size and the display scale."));

            AboutScenario.Children.Add(Heading("Build information", AutomationHeadingLevel.Level2));
            var build = new TextBlock { IsTextSelectionEnabled = true, Text = BuildInfoText(info) };
            AboutScenario.Children.Add(new Border { Style = (Style)RootGrid.Resources["CardStyle"], Child = build });

            AboutScenario.Children.Add(Heading("Environment", AutomationHeadingLevel.Level2));
            _environmentText = new TextBlock { IsTextSelectionEnabled = true };
            AboutScenario.Children.Add(new Border { Style = (Style)RootGrid.Resources["CardStyle"], Child = _environmentText });

            var copy = new Button { Content = "Copy build and environment" };
            copy.Click += (_, _) =>
            {
                var package = new DataPackage();
                package.SetText(BuildInfoText(info) + Environment.NewLine + EnvironmentText());
                Clipboard.SetContent(package);
                AnnounceStatus(StatusText, "Build and environment copied.");
            };
            AboutScenario.Children.Add(copy);
            RootGrid.ActualThemeChanged += (_, _) => UpdateEnvironmentText();
            UpdateEnvironmentText();
        }

        private static Dictionary<string, string> ReadBuildInfo()
        {
            try
            {
                var path = Path.Combine(AppContext.BaseDirectory, "BuildInfo.json");
                return JsonSerializer.Deserialize<Dictionary<string, string>>(File.ReadAllText(path)) ?? new();
            }
            catch (Exception)
            {
                return new();
            }
        }

        private static string BuildInfoText(Dictionary<string, string> info)
        {
            string Get(string key) => info.TryGetValue(key, out var value) ? value : "unknown";
            var text = new StringBuilder();
            text.AppendLine($"App: {BuildLabel} ({Get("ReviewBuild")})");
            text.AppendLine($"Charts: {Get("ChartsSource")}");
            text.AppendLine($"Charts DLL SHA-256 (first 12): {Get("ChartsDllSha")}, built {Get("ChartsBuilt")}");
            text.AppendLine($"Loaded Charts DLL: {LoadedChartsPath()}");
            text.AppendLine($"Windows App SDK: {Get("WinAppSdk")}");
            text.Append($"App source: {Get("AppSource")}");
            return text.ToString();
        }

        private void UpdateEnvironmentText()
        {
            if (_environmentText != null) _environmentText.Text = EnvironmentText();
        }

        private string EnvironmentText()
        {
            var text = new StringBuilder();
            text.AppendLine($"Windows: {Environment.OSVersion.Version}");
            text.AppendLine($"App theme: {RootGrid.ActualTheme} (selector: {(ThemeChoice.SelectedItem as ComboBoxItem)?.Content})");
            text.AppendLine($"Contrast theme: {(_accessibility.HighContrast ? "On, " + _accessibility.HighContrastScheme : "Off")}");
            text.AppendLine($"Text size: {Math.Round(_uiSettings.TextScaleFactor * 100)}%");
            text.Append($"Display scale: {(RootGrid.XamlRoot != null ? Math.Round(RootGrid.XamlRoot.RasterizationScale * 100) + "%" : "unknown")}");
            return text.ToString();
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr GetModuleHandle(string name);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern uint GetModuleFileName(IntPtr module, StringBuilder path, uint size);

        private static string LoadedChartsPath()
        {
            var module = GetModuleHandle("Microsoft.UI.Xaml.Controls.Charts.dll");
            if (module == IntPtr.Zero) return "not loaded yet";
            var path = new StringBuilder(1024);
            return GetModuleFileName(module, path, (uint)path.Capacity) > 0 ? path.ToString() : "unknown";
        }

        // ---------------------------------------------------------------- Helpers

        private TextBlock Note(string text) =>
            new() { Text = text, Style = (Style)RootGrid.Resources["DescriptionStyle"] };

        private static TextBlock Heading(string text, AutomationHeadingLevel level)
        {
            var heading = new TextBlock { Text = text, Style = (Style)Application.Current.Resources["BodyStrongTextBlockStyle"] };
            AutomationProperties.SetHeadingLevel(heading, level);
            return heading;
        }

        private static void Place(Panel panel, FrameworkElement element, int row, int column)
        {
            Grid.SetRow(element, row);
            Grid.SetColumn(element, column);
            panel.Children.Add(element);
        }

        // --page=<tag> opens a page directly (tags are the NavigationViewItem Tag values);
        // --theme=light|dark|system sets the theme selector.
        private void SelectStartPage()
        {
            foreach (var arg in Environment.GetCommandLineArgs())
            {
                if (arg.StartsWith("--scenario=", StringComparison.OrdinalIgnoreCase))
                {
                    ScenarioNavigation.SelectedItem = NavScenarios;
                    OpenScenarioById(arg.Substring("--scenario=".Length));
                    continue;
                }
                if (arg.StartsWith("--theme=", StringComparison.OrdinalIgnoreCase))
                {
                    ThemeChoice.SelectedIndex = arg.Substring("--theme=".Length).ToLowerInvariant() switch
                    {
                        "light" => 1,
                        "dark" => 2,
                        _ => 0
                    };
                    continue;
                }
                if (!arg.StartsWith("--page=", StringComparison.OrdinalIgnoreCase)) continue;
                string tag = arg.Substring("--page=".Length);
                foreach (var item in EnumerateNavItems())
                {
                    if (string.Equals(item.Tag as string, tag, StringComparison.OrdinalIgnoreCase))
                    {
                        ScenarioNavigation.SelectedItem = item;
                        break;
                    }
                }
            }
        }

        private IEnumerable<NavigationViewItem> EnumerateNavItems()
        {
            foreach (var item in ScenarioNavigation.MenuItems) if (item is NavigationViewItem n) yield return n;
            foreach (var item in ScenarioNavigation.FooterMenuItems) if (item is NavigationViewItem n) yield return n;
        }
    }
}
