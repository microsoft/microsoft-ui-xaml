// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Charts;
using Windows.ApplicationModel.DataTransfer;

namespace ChartsSample
{
    // The 30 accessibility assessment scenarios used for the Chart accessibility review. Data comes
    // unchanged from scenarios.json; the chart settings that file does not carry (series kinds, axis labels and
    // ranges, legend titles, app-set names, the replaced label) match the assessment app's UIA tree.
    public sealed partial class MainWindow
    {
        private sealed record ScenarioInfo(string Id, string Title, string Instructions, JsonElement Expectations, bool Live);

        private readonly List<ScenarioInfo> _scenarios = new();
        private readonly ObservableCollection<ListViewItem> _scenarioItems = new();
        private readonly DispatcherTimer _scenarioTimer = new();
        private ListView _scenarioList;
        private TextBox _scenarioSearch;
        private TextBlock _scenarioCount;
        private TextBlock _scenarioOpenText;
        private TextBlock _scenarioTitle;
        private TextBlock _scenarioInstructions;
        private TextBlock _scenarioIdText;
        private StackPanel _scenarioLivePanel;
        private ToggleButton _scenarioPauseButton;
        private TextBlock _scenarioLiveStatus;
        private StackPanel _scenarioHost;
        private ScenarioInfo _openScenario;
        private Action _scenarioStep;
        private Action _scenarioReset;
        private int _scenarioUpdates;
        private readonly List<Chart> _scenarioCharts = new();
        private ColumnDefinition _scenarioPickerColumn;
        private Button _scenarioOpenButton;

        private void BuildScenariosPage()
        {
            LoadScenarios();

            var root = new Grid { ColumnSpacing = 24 };
            _scenarioPickerColumn = new ColumnDefinition { Width = new GridLength(300) };
            root.ColumnDefinitions.Add(_scenarioPickerColumn);
            root.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            // Picker: search, count, list and Open. Selecting does not replace the open chart, so a screen
            // reader browsing the list is not interrupted; Open (or Enter / double-click) does.
            var picker = new StackPanel { Spacing = 8 };
            picker.Children.Add(Heading("Scenarios", AutomationHeadingLevel.Level2));
            _scenarioSearch = new TextBox { PlaceholderText = "Find a scenario" };
            AutomationProperties.SetName(_scenarioSearch, "Find a scenario");
            AutomationProperties.SetAutomationId(_scenarioSearch, "ScenarioSearch");
            _scenarioSearch.TextChanged += (_, _) => FilterScenarios(announce: true);
            picker.Children.Add(_scenarioSearch);
            _scenarioCount = new TextBlock { Style = (Style)RootGrid.Resources["DescriptionStyle"] };
            AutomationProperties.SetAutomationId(_scenarioCount, "ScenarioCount");
            AutomationProperties.SetLiveSetting(_scenarioCount, AutomationLiveSetting.Polite);
            picker.Children.Add(_scenarioCount);
            _scenarioList = new ListView { ItemsSource = _scenarioItems, SelectionMode = ListViewSelectionMode.Single, MaxHeight = 520 }; // height follows the window: UpdateResponsiveHeights
            AutomationProperties.SetName(_scenarioList, "Scenarios");
            AutomationProperties.SetAutomationId(_scenarioList, "ScenarioPicker");
            _scenarioList.DoubleTapped += (_, _) => OpenSelectedScenario();
            _scenarioList.SelectionChanged += (_, _) => UpdateOpenScenarioEnabled();
            _scenarioList.KeyDown += (_, e) =>
            {
                if (e.Key == Windows.System.VirtualKey.Enter) { OpenSelectedScenario(); e.Handled = true; }
            };
            picker.Children.Add(_scenarioList);
            _scenarioOpenText = new TextBlock { Style = (Style)RootGrid.Resources["DescriptionStyle"] };
            picker.Children.Add(_scenarioOpenText);
            var open = new Button { Content = "Open scenario", Style = (Style)Application.Current.Resources["AccentButtonStyle"], HorizontalAlignment = HorizontalAlignment.Stretch };
            AutomationProperties.SetName(open, "Open selected scenario");
            AutomationProperties.SetAutomationId(open, "OpenScenario");
            open.Click += (_, _) => OpenSelectedScenario();
            _scenarioOpenButton = open;
            picker.Children.Add(open);
            root.Children.Add(picker);

            // Workspace: title, instructions, live controls, the chart(s) and Copy context.
            var workspace = new StackPanel { Spacing = 12 };
            Grid.SetColumn(workspace, 1);
            _scenarioTitle = new TextBlock { Style = (Style)Application.Current.Resources["SubtitleTextBlockStyle"], TextWrapping = TextWrapping.Wrap };
            AutomationProperties.SetHeadingLevel(_scenarioTitle, AutomationHeadingLevel.Level2);
            AutomationProperties.SetAutomationId(_scenarioTitle, "ScenarioTitle");
            workspace.Children.Add(_scenarioTitle);
            _scenarioInstructions = new TextBlock { Style = (Style)RootGrid.Resources["DescriptionStyle"] };
            AutomationProperties.SetAutomationId(_scenarioInstructions, "ScenarioInstructions");
            workspace.Children.Add(_scenarioInstructions);
            _scenarioIdText = new TextBlock { Style = (Style)RootGrid.Resources["DescriptionStyle"], IsTextSelectionEnabled = true };
            AutomationProperties.SetAutomationId(_scenarioIdText, "ScenarioId");
            workspace.Children.Add(_scenarioIdText);

            _scenarioLivePanel = new StackPanel { Orientation = Orientation.Horizontal, Spacing = 8, Visibility = Visibility.Collapsed };
            // A toggle with a fixed label: pressed means live updates are running.
            _scenarioPauseButton = new ToggleButton { Content = "Live updates" };
            AutomationProperties.SetAutomationId(_scenarioPauseButton, "ScenarioPauseResume");
            _scenarioPauseButton.Click += (_, _) => SetScenarioRunning(_scenarioPauseButton.IsChecked == true, announce: true);
            var step = new Button { Content = "Step" };
            AutomationProperties.SetName(step, "Step once");
            AutomationProperties.SetAutomationId(step, "ScenarioStep");
            step.Click += (_, _) => { SetScenarioRunning(false, announce: false); StepScenario(); AnnounceScenarioStatus(); };
            var reset = new Button { Content = "Reset" };
            AutomationProperties.SetAutomationId(reset, "ScenarioReset");
            reset.Click += (_, _) => { SetScenarioRunning(false, announce: false); _scenarioUpdates = 0; _scenarioReset?.Invoke(); AnnounceScenarioStatus(); };
            _scenarioLiveStatus = new TextBlock { VerticalAlignment = VerticalAlignment.Center, Style = (Style)RootGrid.Resources["DescriptionStyle"] };
            AutomationProperties.SetAutomationId(_scenarioLiveStatus, "ScenarioLiveStatus");
            AutomationProperties.SetLiveSetting(_scenarioLiveStatus, AutomationLiveSetting.Polite);
            _scenarioLivePanel.Children.Add(_scenarioPauseButton);
            _scenarioLivePanel.Children.Add(step);
            _scenarioLivePanel.Children.Add(reset);
            _scenarioLivePanel.Children.Add(_scenarioLiveStatus);
            workspace.Children.Add(_scenarioLivePanel);

            _scenarioHost = new StackPanel { Spacing = 12 };
            workspace.Children.Add(new Border { Style = (Style)RootGrid.Resources["CardStyle"], Child = _scenarioHost });

            var copy = new Button { Content = "Copy context" };
            AutomationProperties.SetAutomationId(copy, "ScenarioCopyContext");
            copy.Click += (_, _) => CopyScenarioContext();
            workspace.Children.Add(copy);
            root.Children.Add(workspace);
            ScenariosScenario.Children.Add(root);

            _scenarioTimer.Tick += (_, _) => { StepScenario(); };
            FilterScenarios(announce: false);
            if (_scenarios.Count > 0)
            {
                _scenarioList.SelectedIndex = 0;
                OpenScenario(_scenarios[0]);
            }
        }

        private void LoadScenarios()
        {
            try
            {
                using var stream = typeof(MainWindow).Assembly.GetManifestResourceStream("scenarios.json");
                using var doc = JsonDocument.Parse(stream);
                foreach (var s in doc.RootElement.EnumerateArray())
                {
                    _scenarios.Add(new ScenarioInfo(
                        s.GetProperty("id").GetString(),
                        s.GetProperty("title").GetString(),
                        s.GetProperty("instructions").GetString(),
                        s.GetProperty("expectations").Clone(),
                        s.TryGetProperty("hasLiveProducer", out var live) && live.GetBoolean()));
                }
            }
            catch (Exception ex)
            {
                StatusText.Text = "Could not load scenarios.json: " + ex.Message;
            }
        }

        private void FilterScenarios(bool announce)
        {
            string query = _scenarioSearch?.Text?.Trim() ?? "";
            string selected = (_scenarioList?.SelectedItem as ListViewItem)?.Tag as string;
            _scenarioItems.Clear();
            foreach (var s in _scenarios)
            {
                if (query.Length > 0 &&
                    s.Title.IndexOf(query, StringComparison.OrdinalIgnoreCase) < 0 &&
                    s.Id.IndexOf(query, StringComparison.OrdinalIgnoreCase) < 0)
                {
                    continue;
                }
                // Titles wrap rather than clip at large text sizes.
                var item = new ListViewItem { Content = new TextBlock { Text = s.Title, TextWrapping = TextWrapping.Wrap }, Tag = s.Id };
                AutomationProperties.SetName(item, s.Title);
                AutomationProperties.SetAutomationId(item, "Scenario-" + s.Id);
                _scenarioItems.Add(item);
                if (s.Id == selected) _scenarioList.SelectedItem = item;
            }
            string count = _scenarioItems.Count switch
            {
                0 => $"No scenarios match \"{query}\".",
                1 => "1 scenario",
                _ => $"{_scenarioItems.Count} scenarios"
            };
            // Typing in the search box announces the new count, including "no results".
            if (announce) AnnounceStatus(_scenarioCount, count);
            else _scenarioCount.Text = count;
            UpdateOpenScenarioEnabled();
        }

        // Open is available only when a scenario is selected.
        private void UpdateOpenScenarioEnabled()
        {
            if (_scenarioOpenButton != null) _scenarioOpenButton.IsEnabled = _scenarioList?.SelectedItem != null;
        }

        private void OpenSelectedScenario()
        {
            if ((_scenarioList.SelectedItem as ListViewItem)?.Tag is string id && OpenScenarioById(id))
                AnnounceStatus(StatusText, $"Opened scenario {_openScenario.Title}. {_openScenario.Instructions}");
        }

        private bool OpenScenarioById(string id)
        {
            var s = _scenarios.FirstOrDefault(x => string.Equals(x.Id, id, StringComparison.OrdinalIgnoreCase));
            if (s == null) return false;
            OpenScenario(s);
            foreach (var item in _scenarioItems)
                if ((string)item.Tag == s.Id) _scenarioList.SelectedItem = item;
            return true;
        }

        private void OpenScenario(ScenarioInfo s)
        {
            SetScenarioRunning(false, announce: false);
            _scenarioStep = null;
            _scenarioReset = null;
            _scenarioUpdates = 0;
            _scenarioCharts.Clear();
            _scenarioHost.Children.Clear();
            _openScenario = s;
            _scenarioTitle.Text = s.Title;
            _scenarioInstructions.Text = s.Instructions;
            _scenarioIdText.Text = "Scenario ID: " + s.Id;
            _scenarioOpenText.Text = "Currently open: " + s.Title;

            var charts = s.Expectations.GetProperty("charts").EnumerateArray().ToList();
            if (s.Id == "keyboard-before-after-chart")
            {
                var before = new Button { Content = "Before the chart" };
                AutomationProperties.SetAutomationId(before, "BeforeChart");
                _scenarioHost.Children.Add(before);
            }
            for (int i = 0; i < charts.Count; i++)
            {
                var chart = BuildScenarioChart(s.Id, i, charts[i]);
                AutomationProperties.SetAutomationId(chart, $"ScenarioChart{i}");
                _scenarioCharts.Add(chart);
                _scenarioHost.Children.Add(chart);
            }
            if (s.Id == "keyboard-before-after-chart")
            {
                var after = new Button { Content = "After the chart" };
                AutomationProperties.SetAutomationId(after, "AfterChart");
                _scenarioHost.Children.Add(after);
            }

            if (s.Live) ConfigureLiveScenario(s);
            _scenarioLivePanel.Visibility = s.Live ? Visibility.Visible : Visibility.Collapsed;
            UpdateScenarioStatus();
            UpdateResponsiveHeights();
            if (ScenariosScenario.Visibility == Visibility.Visible) StatusText.Text = $"Opened scenario {s.Title}.";
        }

        private Chart BuildScenarioChart(string id, int chartIndex, JsonElement chartData)
        {
            var chart = new Chart();
            var series = chartData.TryGetProperty("series", out var sa) ? sa.EnumerateArray().ToList() : new List<JsonElement>();

            // Shared X values: categories, dates, or none when the data has neither.
            Samples x = null;
            var first = series.FirstOrDefault(sr => sr.GetProperty("points").GetArrayLength() > 0);
            if (first.ValueKind != JsonValueKind.Undefined)
            {
                var points = first.GetProperty("points").EnumerateArray().ToList();
                if (points[0].TryGetProperty("dateUtc", out _))
                    x = new Samples { ItemsSource = new ObservableCollection<DateTimeOffset>(points.Select(p => DateTimeOffset.Parse(p.GetProperty("dateUtc").GetString()))) };
                else if (points.Any(p => !string.IsNullOrEmpty(p.GetProperty("category").GetString())))
                    x = new Samples { ItemsSource = new ObservableCollection<string>(points.Select(p => p.GetProperty("category").GetString())) };
            }
            if (x != null) chart.Data.Add(x);

            CartesianAxis xAxis = null;
            LinearAxis yAxis = null;
            switch (id)
            {
                case "labelled-axes":
                    xAxis = new CategoryAxis { Label = "Month" }; yAxis = new LinearAxis { Label = "Dollars" }; break;
                case "axis-range-set-by-app":
                    xAxis = new CategoryAxis { Label = "Month" }; yAxis = new LinearAxis { Label = "Percent", Minimum = 0, Maximum = 100 }; break;
                case "dates-along-axis":
                    xAxis = new DateTimeAxis { Label = "Date" }; yAxis = new LinearAxis { Label = "Value" }; break;
                case "hidden-axis":
                    xAxis = new CategoryAxis { Label = "Month" }; yAxis = new LinearAxis { Label = "Dollars", IsVisible = false }; break;
            }
            if (xAxis != null) chart.Axes.Add(xAxis);
            if (yAxis != null) chart.Axes.Add(yAxis);

            for (int i = 0; i < series.Count; i++)
                chart.Series.Add(BuildScenarioSeries(id, i, series[i], chart, x, xAxis, yAxis));

            switch (id)
            {
                case "legend-shown": chart.ShowLegend = true; chart.LegendTitle = "Accounts"; break;
                case "legend-with-hidden-series": chart.ShowLegend = true; chart.LegendTitle = "Regions"; break;
                case "series-distinguished-only-by-colour": chart.ShowLegend = true; break;
                case "named-by-app": AutomationProperties.SetName(chart, "Processor utilization"); break;
                case "two-charts-one-page": AutomationProperties.SetName(chart, chartIndex == 0 ? "Requests per month" : "Errors per month"); break;
                case "disabled-chart": chart.IsEnabled = false; break;
            }
            return chart;
        }

        private static CartesianSeries BuildScenarioSeries(string id, int index, JsonElement data, Chart chart, Samples x, CartesianAxis xAxis, LinearAxis yAxis)
        {
            CartesianSeries series = id switch
            {
                "bar-chart" => new BarSeries(),
                "area-chart" => new AreaSeries(),
                "mixed-line-and-bar" when index == 0 => new BarSeries(),
                _ => new LineSeries()
            };
            string title = data.GetProperty("title").GetString();
            if (!string.IsNullOrEmpty(title)) series.Title = title;
            series.IsVisible = data.GetProperty("visible").GetBoolean();

            var y = new Samples { ItemsSource = new ObservableVector<double>(data.GetProperty("points").EnumerateArray().Select(p => p.GetProperty("value").GetDouble())) };
            chart.Data.Add(y);
            if (x != null) series.XValues = x;
            series.YValues = y;
            if (xAxis != null) series.XAxis = xAxis;
            if (yAxis != null) series.YAxis = yAxis;

            if (id == "data-labels-and-markers") { series.ShowDataLabels = true; series.ShowDataMarkers = true; }
            if (id == "replaced-data-label-text")
            {
                series.ShowDataLabels = true;
                series.DataLabelOverrides[2] = new DataLabelOverride("Quarter peak", null);
            }
            return series;
        }

        // ---------------------------------------------------------------- Live scenarios

        private void ConfigureLiveScenario(ScenarioInfo s)
        {
            var live = s.Expectations.GetProperty("live");
            string kind = live.GetProperty("kind").GetString();
            var chart = _scenarioCharts[0];
            if (kind == "value-cycle")
            {
                // Jan goes 20, 21, ... 59, then back to 20 (period 40, increment 1). Every two seconds when running.
                int seriesIndex = live.GetProperty("seriesIndex").GetInt32();
                int pointIndex = live.GetProperty("pointIndex").GetInt32();
                int period = live.GetProperty("period").GetInt32();
                double increment = live.GetProperty("increment").GetDouble();
                var values = (ObservableVector<double>)chart.Series[seriesIndex].YValues.ItemsSource;
                double start = values[pointIndex];
                _scenarioTimer.Interval = TimeSpan.FromSeconds(2);
                _scenarioStep = () => values[pointIndex] = start + (_scenarioUpdates % period) * increment;
                _scenarioReset = () => values[pointIndex] = start;
            }
            else if (kind == "series-toggle")
            {
                // Alternates one and two series. Every three seconds when running.
                var alternate = live.GetProperty("alternateCharts")[0].GetProperty("series");
                var extra = alternate[1];
                var x = chart.Series[0].XValues;
                CartesianSeries added = null;
                _scenarioTimer.Interval = TimeSpan.FromSeconds(3);
                void Remove()
                {
                    if (added == null) return;
                    chart.Series.Remove(added);
                    chart.Data.Remove(added.YValues);
                    added = null;
                }
                _scenarioStep = () =>
                {
                    if (added != null) { Remove(); return; }
                    added = BuildScenarioSeries(s.Id, 1, extra, chart, x, null, null);
                    chart.Series.Add(added);
                };
                _scenarioReset = Remove;
            }
        }

        private void StepScenario()
        {
            if (_scenarioStep == null) return;
            _scenarioUpdates++;
            _scenarioStep();
            UpdateScenarioStatus();
        }

        private void SetScenarioRunning(bool running, bool announce)
        {
            if (running && _scenarioStep != null) _scenarioTimer.Start(); else _scenarioTimer.Stop();
            if (_scenarioPauseButton != null) _scenarioPauseButton.IsChecked = _scenarioTimer.IsEnabled;
            UpdateScenarioStatus();
            if (announce) AnnounceScenarioStatus();
        }

        private void UpdateScenarioStatus()
        {
            if (_scenarioLiveStatus != null)
                _scenarioLiveStatus.Text = $"{(_scenarioTimer.IsEnabled ? "Running" : "Paused")}. Updates: {_scenarioUpdates}.";
        }

        // Announces only on user actions (Resume, Pause, Step, Reset), never on timer ticks.
        private void AnnounceScenarioStatus()
        {
            UpdateScenarioStatus();
            FrameworkElementAutomationPeer.FromElement(_scenarioLiveStatus)?.RaiseAutomationEvent(AutomationEvents.LiveRegionChanged);
        }

        private void PauseScenariosOnLeave(string scenario)
        {
            if (scenario != "scenarios") SetScenarioRunning(false, announce: false);
        }

        // Test hook (not for reviewers): --dump-descriptions=<path> opens every scenario and writes each chart's
        // UIA name, localized control type and FullDescription (read from the chart's automation peer), then exits.
        private async void DumpDescriptionsIfRequested()
        {
            string path = Environment.GetCommandLineArgs()
                .FirstOrDefault(a => a.StartsWith("--dump-descriptions=", StringComparison.OrdinalIgnoreCase))?
                .Substring("--dump-descriptions=".Length);
            if (string.IsNullOrEmpty(path)) return;
            ScenarioNavigation.SelectedItem = NavScenarios;
            var output = new StringBuilder();
            foreach (var s in _scenarios)
            {
                OpenScenario(s);
                await System.Threading.Tasks.Task.Delay(900);
                foreach (var chart in _scenarioCharts)
                {
                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(chart);
                    output.AppendLine($"{s.Id}\t{peer.GetName()}\t{peer.GetLocalizedControlType()}\t{peer.GetFullDescription()}");
                }
            }
            File.WriteAllText(path, output.ToString());
            Application.Current.Exit();
        }

        private void CopyScenarioContext()
        {
            var text = new StringBuilder();
            if (_openScenario != null)
            {
                text.AppendLine($"Scenario: {_openScenario.Id} ({_openScenario.Title})");
                if (_openScenario.Live) text.AppendLine($"Live data: {(_scenarioTimer.IsEnabled ? "running" : "paused")}, updates {_scenarioUpdates}");
            }
            text.AppendLine(BuildInfoText(ReadBuildInfo()));
            text.Append(EnvironmentText());
            var package = new DataPackage();
            package.SetText(text.ToString());
            Clipboard.SetContent(package);
            AnnounceStatus(StatusText, "Scenario context copied.");
        }
    }
}
