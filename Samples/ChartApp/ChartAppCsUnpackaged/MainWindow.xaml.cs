using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Media;
using Windows.Globalization.DateTimeFormatting;
using Windows.Graphics;
using Windows.UI;

namespace ChartsSample
{
    public sealed partial class MainWindow : Window
    {
        private static readonly Color OriginalAreaFill = Color.FromArgb(0x60, 0x4F, 0x6B, 0xED);
        private static readonly Color OriginalAreaStroke = Color.FromArgb(0xFF, 0x30, 0x47, 0xB8);
        private static readonly Color OriginalBarFill = Color.FromArgb(0xFF, 0x0F, 0x6C, 0xBD);
        private static readonly Color OriginalBarStroke = Color.FromArgb(0xFF, 0x07, 0x3B, 0x66);
        private static readonly Color[] ExampleColors =
        {
            Color.FromArgb(0xFF, 0x0F, 0x6C, 0xBD),
            Color.FromArgb(0xFF, 0x00, 0x82, 0x72),
            Color.FromArgb(0xFF, 0xFF, 0xB9, 0x00),
            Color.FromArgb(0xFF, 0x87, 0x64, 0xB8),
            Color.FromArgb(0xFF, 0xE3, 0x00, 0x8C),
            Color.FromArgb(0xFF, 0x69, 0x79, 0x7E),
            Color.FromArgb(0xFF, 0x10, 0x7C, 0x10),
            Color.FromArgb(0xFF, 0xD8, 0x3B, 0x01)
        };
        private static readonly double[] ProfitSeed = { 18, 27, 22, 41, 36, 52 };
        private static readonly double[] ExpensesSeed = { 31, 25, 29, 24, 32, 28 };
        private static readonly double[] AreaSeed = { 8, 18, 14, 29, 24, 37 };
        private static readonly double[] BarSeed = { 12, 20, 17, 31, 26, 39 };
        private static readonly double[] LineWeights = { 1, 2, 3, 5 };

        private readonly ObservableCollection<string> _months = new() { "Jan", "Feb", "Mar", "Apr", "May", "Jun" };
        private readonly ObservableVector<double> _profit = new(ProfitSeed);
        private readonly ObservableVector<double> _expenses = new(ExpensesSeed);
        private readonly ObservableVector<double> _area = new(AreaSeed);
        private readonly ObservableVector<double> _bars = new(BarSeed);
        private readonly Dictionary<CartesianSeries, ObservableCollection<double>> _seriesValues = new();
        private readonly ObservableCollection<string> _codeCategories = new() { "Alpha", "Beta", "Gamma", "Delta", "Epsilon" };
        private readonly ObservableVector<double> _codeValues = new() { 12, 38, 21, 47, 34 };
        private readonly CategoryAxis _xAxis = new() { Label = "Month" };
        private readonly LinearAxis _yAxis = new() { Label = "Profit" };
        private readonly DateTimeAxis _dtAxisA = new();
        private readonly DateTimeAxis _dtAxisB = new();
        private readonly DispatcherTimer _updates = new() { Interval = TimeSpan.FromSeconds(1) };
        private readonly AppWindow _appWindow;
        private SecondaryChartWindow _secondary;
        private int _updateIndex;
        private int _nextLineSeries = 3;
        private int _nextAreaSeries = 2;
        private int _nextBarSeries = 2;
        private bool _ready;
        private bool _syncing;
        private bool _closing;
        private bool _lastEditSucceeded;
        private bool _allowClose;

        public MainWindow(string variant)
        {
            InitializeComponent();
            _appWindow = AppWindow;
            _appWindow.Resize(new SizeInt32(1280, 900));
            VariantText.Text = variant;

            Month.ItemsSource = _months;
            Profit.ItemsSource = _profit;
            Expenses.ItemsSource = _expenses;
            AreaMonth.ItemsSource = new ObservableCollection<string>(_months);
            AreaValues.ItemsSource = _area;
            BarMonth.ItemsSource = new ObservableCollection<string>(_months);
            BarValues.ItemsSource = _bars;
            _seriesValues.Add(ProfitSeries, _profit);
            _seriesValues.Add(ExpensesSeries, _expenses);
            _seriesValues.Add(MarkupAreaSeries, _area);
            _seriesValues.Add(MarkupBarSeries, _bars);

            MarkupChart.Axes.Add(_xAxis);
            MarkupChart.Axes.Add(_yAxis);
            ProfitSeries.XAxis = _xAxis;
            ProfitSeries.YAxis = _yAxis;

            ConnectBarAxes(MarkupBarSeries);
            RebuildSeriesChoice(MarkupChart, PresentationSeriesComboBox, 0);
            RebuildSeriesChoice(AreaMarkupChart, AreaSeriesChoice, 0);
            RebuildSeriesChoice(BarMarkupChart, BarSeriesChoice, 0);

            CreateCodeChart();
            CreateDateTimeCharts();
            _ready = true;
            ScenarioNavigation.SelectedItem = NavLine;
            Synchronize(SyncPresentationKnobs);
            Synchronize(SyncAxes);
            Synchronize(SyncAreaOptions);
            Synchronize(SyncBarOptions);
            UpdateDataText();
            _updates.Tick += OnUpdateTick;
            _updates.Start();
            _appWindow.Closing += OnAppWindowClosing;
            Closed += OnClosed;
        }

        private void CreateCodeChart()
        {
            var yValues = new Samples();
            var line = new LineSeries { YValues = yValues };
            var chart = new Chart { ShowLegend = true };
            chart.Series.Add(line);
            CodeChartHost.Child = chart;

            // Connect the handles before supplying their observable collections.
            var xValues = new Samples();
            line.XValues = xValues;
            chart.Data.Add(xValues);
            chart.Data.Add(yValues);
            xValues.ItemsSource = _codeCategories;
            yValues.ItemsSource = _codeValues;
            line.Title = "Initialized out of order";
            line.StrokeDashStyle = StrokeDashStyle.DashDot;
            line.StrokeThickness = 3;
            line.Stroke = ColorBrush(0, 120, 212);
            line.DataLabelOverrides[1] = new DataLabelOverride("Beta", ColorBrush(16, 124, 16));
            line.DataMarkerOverrides[3] = new DataMarkerOverride(MarkerShape.Diamond, ColorBrush(136, 23, 152));
            AutomationProperties.SetName(chart, "Code-created line chart");
            AutomationProperties.SetHelpText(chart, "Five categories, a dash-dot line, a custom Beta label and a diamond at Delta. Exact values are in Current data.");
        }

        private void CreateDateTimeCharts()
        {
            var days = new ObservableCollection<DateTimeOffset>();
            var dailyValues = new ObservableCollection<double>();
            var dailyTargets = new ObservableCollection<double>();
            var start = new DateTimeOffset(2024, 1, 1, 0, 0, 0, TimeSpan.Zero);
            for (int day = 0; day < 75; day++)
            {
                days.Add(start.AddDays(day));
                double t = day / 74.0;
                dailyValues.Add(50 + 20 * Math.Sin(t * 6.283185) + 4 * Math.Sin(t * 25.13274));
                dailyTargets.Add(50);
            }

            DtChartHostA.Child = CreateDateTimeChart(days, dailyValues, dailyTargets, _dtAxisA, "Daily value");
            DtDataTextA.Text = DateDataText(days, dailyValues, dailyTargets);

            var months = new ObservableCollection<DateTimeOffset>();
            var monthlyValues = new ObservableCollection<double>
            {
                42, 38, 47, 53, 61, 58, 72, 68, 55, 49, 63, 70,
                45, 41, 52, 60, 67, 64, 79, 74, 61, 54, 68, 76,
                48, 44, 55, 63, 71, 68, 83, 78, 65, 58, 72, 80
            };
            var monthlyTargets = new ObservableCollection<double>();
            for (int i = 0; i < monthlyValues.Count; i++)
            {
                months.Add(new DateTimeOffset(2022 + i / 12, i % 12 + 1, 1, 0, 0, 0, TimeSpan.Zero));
                monthlyTargets.Add(60 + i / 12 * 5);
            }

            DtChartHostB.Child = CreateDateTimeChart(months, monthlyValues, monthlyTargets, _dtAxisB, "Monthly value");
            DtDataTextB.Text = DateDataText(months, monthlyValues, monthlyTargets);
        }

        private static Chart CreateDateTimeChart(ObservableCollection<DateTimeOffset> dates,
            ObservableCollection<double> values, ObservableCollection<double> targets, DateTimeAxis xAxis, string title)
        {
            var x = new Samples { ItemsSource = dates };
            var actual = new Samples { ItemsSource = values };
            var target = new Samples { ItemsSource = targets };
            var yAxis = new LinearAxis();
            var chart = new Chart { ShowLegend = true };
            chart.Axes.Add(xAxis);
            chart.Axes.Add(yAxis);
            chart.Data.Add(x);
            chart.Data.Add(actual);
            chart.Data.Add(target);
            chart.Series.Add(new LineSeries { Title = title, XValues = x, YValues = actual, XAxis = xAxis, YAxis = yAxis, StrokeThickness = 3 });
            chart.Series.Add(new LineSeries { Title = "Target", XValues = x, YValues = target, XAxis = xAxis, YAxis = yAxis, StrokeDashStyle = StrokeDashStyle.Dash });
            AutomationProperties.SetName(chart, title + " and target by date");
            AutomationProperties.SetHelpText(chart, "Actual is solid and target is dashed. Expand the data section for all dates and exact values.");
            return chart;
        }

        private static string DateDataText(ObservableCollection<DateTimeOffset> dates,
            ObservableCollection<double> values, ObservableCollection<double> targets)
        {
            var text = new StringBuilder();
            for (int i = 0; i < dates.Count; i++)
            {
                text.AppendLine($"{dates[i]:yyyy-MM-dd}: actual {values[i]:0.##}, target {targets[i]:0.##}");
            }
            return text.ToString();
        }

        private void OnUpdateTick(object sender, object e)
        {
            UpdateValues(_codeValues, 0);
            UpdateValues(_profit, 1);
            UpdateValues(_expenses, 2);
            _updateIndex = (_updateIndex + 1) % 30;
            UpdateDataText();
        }

        private void UpdateValues(ObservableCollection<double> values, int bias)
        {
            int index = (_updateIndex + bias) % values.Count;
            double value = values[index];
            values[index] = value >= 48 ? value - 29 : value + 7 + bias;
        }

        private void UpdateDataText()
        {
            var text = new StringBuilder();
            foreach (var series in MarkupChart.Series)
            {
                text.AppendLine(SeriesDataText(series));
            }
            text.AppendLine("Code-created line:");
            for (int i = 0; i < _codeValues.Count; i++)
            {
                text.AppendLine($"{_codeCategories[i]}: {_codeValues[i]:0}");
            }
            DataText.Text = text.ToString();
            AreaDataText.Text = SeriesDataText(SelectedAreaSeries());
            BarDataText.Text = SeriesDataText(SelectedBarSeries());
        }

        private string SeriesDataText(CartesianSeries series)
        {
            var values = _seriesValues[series];
            var text = new StringBuilder();
            text.AppendLine(series.Title + (series.IsVisible ? ":" : " (hidden):"));
            for (int i = 0; i < values.Count; i++)
                text.AppendLine($"{_months[i]}: {values[i]:0.##}");
            return text.ToString().TrimEnd();
        }

        private void OnThemeChoiceChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_ready) return;
            RootGrid.RequestedTheme = ThemeChoice.SelectedIndex switch
            {
                1 => ElementTheme.Light,
                2 => ElementTheme.Dark,
                _ => ElementTheme.Default
            };
        }

        private void OnScenarioSelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            if (!_ready || args.SelectedItem is not NavigationViewItem item) return;
            string scenario = item.Tag as string;
            bool sharedLine = scenario is "line" or "axes" or "presentation";
            LineScenario.Visibility = sharedLine ? Visibility.Visible : Visibility.Collapsed;
            AreaScenario.Visibility = scenario == "area" ? Visibility.Visible : Visibility.Collapsed;
            BarScenario.Visibility = scenario == "bar" ? Visibility.Visible : Visibility.Collapsed;
            DateTimeScenario.Visibility = scenario == "datetime" ? Visibility.Visible : Visibility.Collapsed;
            LiveScenario.Visibility = scenario == "live" ? Visibility.Visible : Visibility.Collapsed;
            SeriesEditors.Visibility = scenario is "line" or "presentation" ? Visibility.Visible : Visibility.Collapsed;
            LineSeriesActions.Visibility = scenario == "line" ? Visibility.Visible : Visibility.Collapsed;
            LineStyleEditors.Visibility = scenario == "line" ? Visibility.Visible : Visibility.Collapsed;
            LineResetButton.Visibility = scenario == "line" ? Visibility.Visible : Visibility.Collapsed;
            PointOverrideEditors.Visibility = scenario == "presentation" ? Visibility.Visible : Visibility.Collapsed;
            PresentationKnobStatusText.Visibility = scenario is "line" or "presentation" ? Visibility.Visible : Visibility.Collapsed;
            AxisEditors.Visibility = scenario == "axes" ? Visibility.Visible : Visibility.Collapsed;
            SyncAxisAvailability();
            (ScenarioHeading.Text, ScenarioDescription.Text) = scenario switch
            {
                "area" => ("Area charts", "Explore fill styles, colors, labels and markers using six monthly values."),
                "bar" => ("Bar charts", "Compare monthly values with configurable colors, labels and orientation."),
                "datetime" => ("Date & time", "Explore daily and monthly timelines, intervals and date label formats."),
                "axes" => ("Axes & ordering", "Adjust profit bounds, category ordering, ticks and grid lines."),
                "presentation" => ("Labels & markers", "Customize series defaults and indexed point labels and markers."),
                "live" => ("Live data", "Watch values update and exercise an independent secondary UI thread."),
                _ => ("Line charts", "Compare monthly profit and expenses. Explore series defaults and the legend.")
            };
            ScenarioScroll.ChangeView(null, 0, null, true);
            if (sender.IsLoaded && sender.DisplayMode != NavigationViewDisplayMode.Expanded)
                sender.IsPaneOpen = false;
        }

        private void OnHeaderSizeChanged(object sender, SizeChangedEventArgs e) =>
            UpdateHeaderLayout();

        private void OnNavigationDisplayModeChanged(NavigationView sender, NavigationViewDisplayModeChangedEventArgs args) =>
            UpdateHeaderLayout();

        private void UpdateHeaderLayout()
        {
            if (ThemeChoice == null || PageHeader == null || ScenarioNavigation == null) return;
            bool stacked = PageHeader.ActualWidth < 640;
            PageHeader.Margin = new Thickness(16,
                ScenarioNavigation.DisplayMode == NavigationViewDisplayMode.Minimal ? 56 : 16, 16, 12);
            Grid.SetRow(ThemeChoice, stacked ? 1 : 0);
            Grid.SetColumn(ThemeChoice, stacked ? 0 : 1);
            Grid.SetColumnSpan(HeadingPanel, stacked ? 2 : 1);
            ThemeChoice.HorizontalAlignment = stacked ? HorizontalAlignment.Left : HorizontalAlignment.Right;
        }

        private void OnEditorSizeChanged(object sender, SizeChangedEventArgs e)
        {
            var grid = (Grid)sender;
            double threshold = grid == DateTimeScenario ? 900 : grid.Tag switch
            {
                "Choices" => 300,
                "Actions" => 180,
                _ => 360
            };
            int columns = e.NewSize.Width >= threshold ? 2 : 1;
            if (grid.Tag as string == "Bounds" && e.NewSize.Width >= 360) columns = 3;
            int rows = (grid.Children.Count + columns - 1) / columns;
            if (grid.ColumnDefinitions.Count == columns && grid.RowDefinitions.Count == rows) return;
            grid.ColumnDefinitions.Clear();
            grid.RowDefinitions.Clear();
            for (int i = 0; i < columns; i++)
                grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            for (int i = 0; i < rows; i++)
                grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            for (int i = 0; i < grid.Children.Count; i++)
            {
                var field = (FrameworkElement)grid.Children[i];
                Grid.SetRow(field, i / columns);
                Grid.SetColumn(field, i % columns);
            }
        }

        private void OnWorkspaceSizeChanged(object sender, SizeChangedEventArgs e) =>
            UpdateWorkspaceLayout((Grid)sender);

        private void OnViewportSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (!_ready || _closing) return;
            UpdateWorkspaceLayout(LineScenario);
            UpdateWorkspaceLayout(AreaScenario);
            UpdateWorkspaceLayout(BarScenario);
            UpdateWorkspaceLayout(LiveScenario);
        }

        private void UpdateWorkspaceLayout(Grid workspace)
        {
            if (!_ready || _closing) return;
            bool wide = workspace.ActualWidth >= 900;
            var preview = (Border)workspace.Children[0];
            var editors = (ScrollViewer)workspace.Children[1];
            workspace.ColumnDefinitions[1].Width = new GridLength(wide ? 440 : 0);
            workspace.ColumnSpacing = wide ? 16 : 0;
            workspace.RowSpacing = wide ? 0 : 12;
            Grid.SetColumn(editors, wide ? 1 : 0);
            Grid.SetRow(editors, wide ? 0 : 1);
            // Bound only the wide inspector so its controls scroll without moving the preview.
            editors.MaxHeight = wide ? Math.Max(1, ScenarioScroll.ActualHeight - 16) : double.PositiveInfinity;
            editors.VerticalScrollMode = wide ? ScrollMode.Enabled : ScrollMode.Disabled;
            editors.VerticalScrollBarVisibility = wide ? ScrollBarVisibility.Auto : ScrollBarVisibility.Disabled;
            ((FrameworkElement)preview.Child).Height = wide
                ? Math.Clamp(ScenarioScroll.ActualHeight - 50, 240, 420)
                : Math.Clamp(ScenarioScroll.ActualHeight * 0.35, 180, 240);
        }

        private void OnToggleUpdatesClick(object sender, RoutedEventArgs e)
        {
            if (_updates.IsEnabled)
            {
                _updates.Stop();
                UpdatesButton.Content = "Resume updates";
            }
            else
            {
                _updates.Start();
                UpdatesButton.Content = "Pause updates";
            }
        }

        private void OnToggleBarOrientationClick(object sender, RoutedEventArgs e)
        {
            EditBar(() =>
            {
                var series = SelectedBarSeries();
                series.Orientation = series.Orientation == BarOrientation.Horizontal
                    ? BarOrientation.Vertical : BarOrientation.Horizontal;
            }, "Bar orientation changed. X remains categories; Y remains values.");
        }

        private LineSeries SelectedPresentationSeries() =>
            (LineSeries)MarkupChart.Series[PresentationSeriesComboBox.SelectedIndex];
        private AreaSeries SelectedAreaSeries() =>
            (AreaSeries)AreaMarkupChart.Series[AreaSeriesChoice.SelectedIndex];
        private BarSeries SelectedBarSeries() =>
            (BarSeries)BarMarkupChart.Series[BarSeriesChoice.SelectedIndex];

        private void RebuildSeriesChoice(Chart chart, ComboBox choice, int selectedIndex)
        {
            Synchronize(() =>
            {
                choice.Items.Clear();
                foreach (var series in chart.Series)
                    choice.Items.Add(series.Title);
                choice.SelectedIndex = Math.Clamp(selectedIndex, 0, chart.Series.Count - 1);
            });
        }

        private void ConnectBarAxes(BarSeries series)
        {
            var xAxis = series.XAxis ?? new CategoryAxis { Label = "Month" };
            var yAxis = series.YAxis ?? new LinearAxis { Label = "Value" };
            if (!BarMarkupChart.Axes.Contains(xAxis)) BarMarkupChart.Axes.Add(xAxis);
            if (!BarMarkupChart.Axes.Contains(yAxis)) BarMarkupChart.Axes.Add(yAxis);
            series.XAxis = xAxis;
            series.YAxis = yAxis;
        }

        private void AddExampleSeries(Chart chart, ComboBox choice, CartesianSeries series,
            Samples categories, double[] seed, int number)
        {
            var values = new ObservableVector<double>();
            foreach (double value in seed)
                values.Add(value * (0.65 + 0.05 * (number % 4)) + number * 3);
            var samples = new Samples { ItemsSource = values };
            series.Title = $"Series {number}";
            series.XValues = categories;
            series.YValues = samples;
            chart.Data.Add(samples);
            chart.Series.Add(series);
            _seriesValues.Add(series, values);
            chart.ShowLegend = true;
            RebuildSeriesChoice(chart, choice, chart.Series.Count - 1);
        }

        private void RemoveSelectedSeries(Chart chart, ComboBox choice, Action sync, TextBlock status)
        {
            if (!_ready || _syncing || _closing || chart.Series.Count <= 1) return;
            int index = choice.SelectedIndex;
            var series = chart.Series[index];
            ApplyEdit(() =>
            {
                chart.Series.RemoveAt(index);
                chart.Data.Remove(series.YValues);
                if (series.XAxis != null) chart.Axes.Remove(series.XAxis);
                if (series.YAxis != null) chart.Axes.Remove(series.YAxis);
                _seriesValues.Remove(series);
                RebuildSeriesChoice(chart, choice, Math.Min(index, chart.Series.Count - 1));
            }, sync, status, $"Removed {series.Title}.", chart: chart);
        }

        private void ClearExample(Chart chart)
        {
            foreach (var series in chart.Series)
                _seriesValues.Remove(series);
            chart.Series.Clear();
            chart.Data.Clear();
            chart.Axes.Clear();
        }

        private static void RestoreValues(ObservableCollection<double> values, double[] seed)
        {
            for (int i = 0; i < seed.Length; i++)
                values[i] = seed[i];
        }

        private static void ResetSeriesPresentation(CartesianSeries series)
        {
            series.IsVisible = true;
            series.ShowDataLabels = false;
            series.ShowDataMarkers = false;
            series.ClearValue(CartesianSeries.MarkerShapeProperty);
            series.StrokeDashStyle = StrokeDashStyle.Solid;
            series.Stroke = null;
            series.DataLabelBrush = null;
            series.DataMarkerBrush = null;
            series.DataLabelOverrides.Clear();
            series.DataMarkerOverrides.Clear();
        }

        private void OnLineAddSeriesClick(object sender, RoutedEventArgs e)
        {
            int number = _nextLineSeries;
            EditLine(() =>
            {
                var series = new LineSeries
                {
                    IsVisible = true, StrokeThickness = 3, StrokeDashStyle = StrokeDashStyle.Solid,
                    Stroke = new SolidColorBrush(ExampleColors[(number - 1) % 6]),
                    MarkerShape = MarkerShape.Circle, ShowDataMarkers = true, ShowDataLabels = false
                };
                AddExampleSeries(MarkupChart, PresentationSeriesComboBox, series, Month, ProfitSeed, number);
                _nextLineSeries++;
            }, $"Added Series {number}.");
        }

        private void OnAreaAddSeriesClick(object sender, RoutedEventArgs e)
        {
            int number = _nextAreaSeries;
            EditArea(() =>
            {
                Color color = ExampleColors[(number - 1) % 6];
                Color fill = color;
                fill.A = 0x60;
                var series = new AreaSeries
                {
                    IsVisible = true, StrokeThickness = 2, Stroke = new SolidColorBrush(color),
                    Fill = new SolidColorBrush(fill), MarkerShape = MarkerShape.Circle,
                    ShowDataMarkers = false, ShowDataLabels = false
                };
                AddExampleSeries(AreaMarkupChart, AreaSeriesChoice, series, AreaMonth, AreaSeed, number);
                _nextAreaSeries++;
            }, $"Added Series {number}.");
        }

        private void OnBarAddSeriesClick(object sender, RoutedEventArgs e)
        {
            int number = _nextBarSeries;
            EditBar(() =>
            {
                Color color = ExampleColors[(number - 1) % 6];
                var series = new BarSeries
                {
                    IsVisible = true, StrokeThickness = 1.5, Stroke = new SolidColorBrush(color),
                    Fill = new SolidColorBrush(color), MarkerShape = MarkerShape.Circle,
                    ShowDataMarkers = false, ShowDataLabels = false,
                    Orientation = SelectedBarSeries().Orientation
                };
                ConnectBarAxes(series);
                AddExampleSeries(BarMarkupChart, BarSeriesChoice, series, BarMonth, BarSeed, number);
                _nextBarSeries++;
            }, $"Added Series {number}.");
        }

        private void OnLineRemoveSeriesClick(object sender, RoutedEventArgs e) =>
            RemoveSelectedSeries(MarkupChart, PresentationSeriesComboBox, SyncPresentationKnobs, PresentationKnobStatusText);
        private void OnAreaRemoveSeriesClick(object sender, RoutedEventArgs e) =>
            RemoveSelectedSeries(AreaMarkupChart, AreaSeriesChoice, SyncAreaOptions, AreaStatusText);
        private void OnBarRemoveSeriesClick(object sender, RoutedEventArgs e) =>
            RemoveSelectedSeries(BarMarkupChart, BarSeriesChoice, SyncBarOptions, BarStatusText);

        private static Color SelectedExampleColor(ComboBox choice, Color original)
        {
            int index = choice.SelectedIndex;
            if (index == 0) return original;
            if (index > 0 && index <= ExampleColors.Length) return ExampleColors[index - 1];
            throw new ArgumentException("Choose an available series color.");
        }

        private static int ExampleColorIndex(Brush brush, Color original)
        {
            if (brush is not SolidColorBrush solid) return -1;
            if (solid.Color == original) return 0;
            int index = Array.IndexOf(ExampleColors, solid.Color);
            return index < 0 ? -1 : index + 1;
        }

        private static MarkerShape SelectedMarkerShape(ComboBox choice)
        {
            int index = choice.SelectedIndex;
            if (index < 0 || index > (int)MarkerShape.Plus)
                throw new ArgumentException("Choose an available marker shape.");
            return (MarkerShape)index;
        }

        private static void SyncVisibility(CheckBox checkBox, CartesianSeries series)
        {
            checkBox.IsChecked = series.IsVisible;
            checkBox.Content = series.IsVisible ? "Visible" : "Hidden";
        }

        private void SyncAreaOptions()
        {
            var series = SelectedAreaSeries();
            AreaColorChoice.SelectedIndex = ExampleColorIndex(series.Stroke, OriginalAreaStroke);
            AreaFillChoice.SelectedIndex = series.Fill is SolidColorBrush fill
                ? fill.Color.A switch { 0x60 => 0, 0xFF => 1, 0 => 2, _ => -1 }
                : -1;
            AreaMarkerChoice.SelectedIndex = (int)series.MarkerShape;
            SyncVisibility(AreaVisibleCheckBox, series);
            AreaValuesCheckBox.IsChecked = series.ShowDataLabels;
            AreaMarkersCheckBox.IsChecked = series.ShowDataMarkers;
            AreaLegendCheckBox.IsChecked = AreaMarkupChart.ShowLegend;
            AreaRemoveSeriesButton.IsEnabled = AreaMarkupChart.Series.Count > 1;
            UpdateDataText();
        }

        private void SyncBarOptions()
        {
            var series = SelectedBarSeries();
            BarColorChoice.SelectedIndex = ExampleColorIndex(series.Stroke, OriginalBarStroke);
            BarMarkerChoice.SelectedIndex = (int)series.MarkerShape;
            SyncVisibility(BarVisibleCheckBox, series);
            BarValuesCheckBox.IsChecked = series.ShowDataLabels;
            BarMarkersCheckBox.IsChecked = series.ShowDataMarkers;
            BarLegendCheckBox.IsChecked = BarMarkupChart.ShowLegend;
            BarOrientationChoice.SelectedIndex = (int)series.Orientation;
            BarOrientationText.Text = $"Orientation: {series.Orientation}";
            BarRemoveSeriesButton.IsEnabled = BarMarkupChart.Series.Count > 1;
            UpdateDataText();
        }

        private void OnAreaSeriesChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_ready || _syncing || _closing || AreaSeriesChoice.SelectedIndex < 0) return;
            Synchronize(SyncAreaOptions);
            AreaStatusText.Text = $"Editing {SelectedAreaSeries().Title}.";
        }

        private void OnBarSeriesChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_ready || _syncing || _closing || BarSeriesChoice.SelectedIndex < 0) return;
            Synchronize(SyncBarOptions);
            BarStatusText.Text = $"Editing {SelectedBarSeries().Title}.";
        }

        private void EditLine(Action edit, string message) =>
            EditPresentation(edit, SyncPresentationKnobs, message);

        private void EditArea(Action edit, string message)
        {
            if (_closing) return;
            ApplyEdit(edit, SyncAreaOptions, AreaStatusText, message, chart: AreaMarkupChart);
        }

        private void EditBar(Action edit, string message)
        {
            if (_closing) return;
            ApplyEdit(edit, SyncBarOptions, BarStatusText, message, chart: BarMarkupChart);
        }

        private void OnAreaStyleChanged(object sender, SelectionChangedEventArgs e)
        {
            EditArea(() =>
            {
                var series = SelectedAreaSeries();
                Color fill = SelectedExampleColor(AreaColorChoice, OriginalAreaFill);
                Color stroke = SelectedExampleColor(AreaColorChoice, OriginalAreaStroke);
                fill.A = AreaFillChoice.SelectedIndex switch
                {
                    0 => 0x60,
                    1 => 0xFF,
                    2 => 0,
                    _ => throw new ArgumentException("Choose an available area fill.")
                };
                // Null selects the palette; an explicit transparent color keeps outline-only mode.
                series.Fill = new SolidColorBrush(fill);
                series.Stroke = new SolidColorBrush(stroke);
            }, "Area color and fill updated. The source data is unchanged.");
        }

        private void OnAreaVisibilityClick(object sender, RoutedEventArgs e) =>
            EditArea(() => SelectedAreaSeries().IsVisible = AreaVisibleCheckBox.IsChecked == true, "Area series visibility updated.");

        private void OnAreaValuesClick(object sender, RoutedEventArgs e) =>
            EditArea(() => SelectedAreaSeries().ShowDataLabels = AreaValuesCheckBox.IsChecked == true, "Area value labels updated.");

        private void OnAreaMarkersClick(object sender, RoutedEventArgs e) =>
            EditArea(() => SelectedAreaSeries().ShowDataMarkers = AreaMarkersCheckBox.IsChecked == true, "Area point markers updated.");

        private void OnAreaMarkerChanged(object sender, SelectionChangedEventArgs e) =>
            EditArea(() => SelectedAreaSeries().MarkerShape = SelectedMarkerShape(AreaMarkerChoice), "Area marker shape updated.");

        private void OnAreaLegendClick(object sender, RoutedEventArgs e) =>
            EditArea(() => AreaMarkupChart.ShowLegend = AreaLegendCheckBox.IsChecked == true, "Area legend updated.");

        private void OnResetAreaClick(object sender, RoutedEventArgs e)
        {
            EditArea(() =>
            {
                ClearExample(AreaMarkupChart);
                RestoreValues(_area, AreaSeed);
                ResetSeriesPresentation(MarkupAreaSeries);
                MarkupAreaSeries.Title = "Monthly area";
                MarkupAreaSeries.XValues = AreaMonth;
                MarkupAreaSeries.YValues = AreaValues;
                MarkupAreaSeries.Fill = new SolidColorBrush(OriginalAreaFill);
                MarkupAreaSeries.Stroke = new SolidColorBrush(OriginalAreaStroke);
                MarkupAreaSeries.StrokeThickness = 2;
                AreaMarkupChart.Data.Add(AreaMonth);
                AreaMarkupChart.Data.Add(AreaValues);
                AreaMarkupChart.Series.Add(MarkupAreaSeries);
                _seriesValues.Add(MarkupAreaSeries, _area);
                AreaMarkupChart.ShowLegend = true;
                _nextAreaSeries = 2;
                RebuildSeriesChoice(AreaMarkupChart, AreaSeriesChoice, 0);
            }, "Area example reset. Other charts and the application theme are unchanged.");
        }

        private void OnBarColorChanged(object sender, SelectionChangedEventArgs e)
        {
            EditBar(() =>
            {
                Color fill = SelectedExampleColor(BarColorChoice, OriginalBarFill);
                Color stroke = SelectedExampleColor(BarColorChoice, OriginalBarStroke);
                SelectedBarSeries().Fill = new SolidColorBrush(fill);
                SelectedBarSeries().Stroke = new SolidColorBrush(stroke);
            }, "Bar color updated. The source data is unchanged.");
        }

        private void OnBarVisibilityClick(object sender, RoutedEventArgs e) =>
            EditBar(() => SelectedBarSeries().IsVisible = BarVisibleCheckBox.IsChecked == true, "Bar series visibility updated.");

        private void OnBarValuesClick(object sender, RoutedEventArgs e) =>
            EditBar(() => SelectedBarSeries().ShowDataLabels = BarValuesCheckBox.IsChecked == true, "Bar value labels updated.");

        private void OnBarMarkersClick(object sender, RoutedEventArgs e) =>
            EditBar(() => SelectedBarSeries().ShowDataMarkers = BarMarkersCheckBox.IsChecked == true, "Bar point markers updated.");

        private void OnBarMarkerChanged(object sender, SelectionChangedEventArgs e) =>
            EditBar(() => SelectedBarSeries().MarkerShape = SelectedMarkerShape(BarMarkerChoice), "Bar marker shape updated.");

        private void OnBarOrientationChanged(object sender, SelectionChangedEventArgs e) =>
            EditBar(() =>
            {
                if (BarOrientationChoice.SelectedIndex is < 0 or > 1)
                    throw new ArgumentException("Choose an available bar orientation.");
                SelectedBarSeries().Orientation = (BarOrientation)BarOrientationChoice.SelectedIndex;
            }, "Bar orientation changed. The selected series keeps its own explicit axes.");

        private void OnBarLegendClick(object sender, RoutedEventArgs e) =>
            EditBar(() => BarMarkupChart.ShowLegend = BarLegendCheckBox.IsChecked == true, "Bar legend updated.");

        private void OnResetBarClick(object sender, RoutedEventArgs e)
        {
            EditBar(() =>
            {
                ClearExample(BarMarkupChart);
                RestoreValues(_bars, BarSeed);
                ResetSeriesPresentation(MarkupBarSeries);
                MarkupBarSeries.Title = "Monthly bars";
                MarkupBarSeries.XValues = BarMonth;
                MarkupBarSeries.YValues = BarValues;
                MarkupBarSeries.Fill = new SolidColorBrush(OriginalBarFill);
                MarkupBarSeries.Stroke = new SolidColorBrush(OriginalBarStroke);
                MarkupBarSeries.StrokeThickness = 1.5;
                MarkupBarSeries.Orientation = BarOrientation.Horizontal;
                ConnectBarAxes(MarkupBarSeries);
                BarMarkupChart.Data.Add(BarMonth);
                BarMarkupChart.Data.Add(BarValues);
                BarMarkupChart.Series.Add(MarkupBarSeries);
                _seriesValues.Add(MarkupBarSeries, _bars);
                BarMarkupChart.ShowLegend = true;
                _nextBarSeries = 2;
                RebuildSeriesChoice(BarMarkupChart, BarSeriesChoice, 0);
            }, "Bar example reset to horizontal. Other charts and the application theme are unchanged.");
        }

        private void OnLineVisibilityClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().IsVisible = LineVisibleCheckBox.IsChecked == true,
                SyncLineOptions, "Line series visibility updated.");

        private void OnLineWeightChanged(object sender, SelectionChangedEventArgs e) =>
            EditPresentation(() =>
            {
                int index = LineWeightChoice.SelectedIndex;
                if (index < 0 || index >= LineWeights.Length)
                    throw new ArgumentException("Choose an available line weight.");
                SelectedPresentationSeries().StrokeThickness = LineWeights[index];
            }, SyncLineOptions, "Line weight updated.");

        private void OnLineStyleChanged(object sender, SelectionChangedEventArgs e) =>
            EditPresentation(() =>
            {
                int index = LineStyleChoice.SelectedIndex;
                if (index < 0 || index > (int)StrokeDashStyle.DashDotDot)
                    throw new ArgumentException("Choose an available line style.");
                SelectedPresentationSeries().StrokeDashStyle = (StrokeDashStyle)index;
            }, SyncLineOptions, "Line style updated.");

        private void OnLineColorChanged(object sender, SelectionChangedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().Stroke = LineColorChoice.SelectedIndex == 0
                ? null : new SolidColorBrush(SelectedExampleColor(LineColorChoice, default)), SyncLineOptions, "Line color updated.");

        private void OnLineMarkerChanged(object sender, SelectionChangedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().MarkerShape = SelectedMarkerShape(LineMarkerChoice),
                SyncLineOptions, "Line marker shape updated.");

        private void OnResetLineClick(object sender, RoutedEventArgs e)
        {
            EditLine(() =>
            {
                ClearExample(MarkupChart);
                RestoreValues(_profit, ProfitSeed);
                RestoreValues(_expenses, ExpensesSeed);
                ResetSeriesPresentation(ProfitSeries);
                ResetSeriesPresentation(ExpensesSeries);
                ProfitSeries.Title = "Monthly profit";
                ProfitSeries.XValues = Month;
                ProfitSeries.YValues = Profit;
                ProfitSeries.StrokeThickness = 3;
                ProfitSeries.MarkerShape = MarkerShape.Circle;
                ProfitSeries.ShowDataMarkers = true;
                ExpensesSeries.Title = "Monthly expenses";
                ExpensesSeries.XValues = Month;
                ExpensesSeries.YValues = Expenses;
                ExpensesSeries.StrokeThickness = 2;
                ExpensesSeries.StrokeDashStyle = StrokeDashStyle.Dash;
                ExpensesSeries.ShowDataLabels = true;
                ExpensesSeries.XAxis = null;
                ExpensesSeries.YAxis = null;
                ResetProfitAxes();
                ProfitSeries.XAxis = _xAxis;
                ProfitSeries.YAxis = _yAxis;
                MarkupChart.Axes.Add(_xAxis);
                MarkupChart.Axes.Add(_yAxis);
                MarkupChart.Data.Add(Month);
                MarkupChart.Data.Add(Profit);
                MarkupChart.Data.Add(Expenses);
                MarkupChart.Series.Add(ProfitSeries);
                MarkupChart.Series.Add(ExpensesSeries);
                _seriesValues.Add(ProfitSeries, _profit);
                _seriesValues.Add(ExpensesSeries, _expenses);
                MarkupChart.ShowLegend = true;
                MarkupChart.LegendTitle = "Monthly totals";
                _nextLineSeries = 3;
                RebuildSeriesChoice(MarkupChart, PresentationSeriesComboBox, 0);
                Synchronize(() =>
                {
                    LegendTitleTextBox.Text = MarkupChart.LegendTitle;
                    OverrideIndexNumberBox.Value = 0;
                    AxisStatusText.Text = "";
                    SyncAxes();
                });
            }, "Line example reset. Other charts, updates and the application theme are unchanged.");
        }

        private void ResetProfitAxes()
        {
            _yAxis.Minimum = null;
            _yAxis.Maximum = null;
            _yAxis.Spacing = null;
            _xAxis.SortKey = CategorySortKey.Index;
            _xAxis.SortOrder = SortOrder.Ascending;
            foreach (CartesianAxis axis in new CartesianAxis[] { _xAxis, _yAxis })
            {
                axis.IsVisible = true;
                axis.ShowTickLabels = true;
                axis.ShowTickMarks = false;
                axis.GridLines = GridLines.None;
                axis.GridLineMajorBrush = null;
                axis.TickBrush = null;
                axis.TickLabelBrush = null;
                axis.AxisLineBrush = null;
            }
        }

        private void SyncAxisAvailability()
        {
            bool available = MarkupChart.Series.Contains(ProfitSeries);
            AxisControls.IsEnabled = available;
            AxisSeriesWarning.Visibility = !available && AxisEditors.Visibility == Visibility.Visible
                ? Visibility.Visible : Visibility.Collapsed;
        }

        private uint? SelectedOverrideIndex()
        {
            double value = OverrideIndexNumberBox.Value;
            if (!double.IsFinite(value) || value < 0 || value >= _months.Count || value != Math.Truncate(value))
            {
                AnnounceStatus(PresentationKnobStatusText, "Point index must be a whole number from 0 to 5.");
                return null;
            }
            return (uint)value;
        }

        private void SyncPresentationKnobs()
        {
            SyncLineOptions();
            SyncPresentationOverrideEditors();
        }

        private void SyncLineOptions()
        {
            var series = SelectedPresentationSeries();
            SyncVisibility(LineVisibleCheckBox, series);
            LineWeightChoice.SelectedIndex = Array.IndexOf(LineWeights, series.StrokeThickness);
            LineStyleChoice.SelectedIndex = (int)series.StrokeDashStyle;
            LineColorChoice.SelectedIndex = series.Stroke == null ? 0 : ExampleColorIndex(series.Stroke, default);
            LineMarkerChoice.SelectedIndex = (int)series.MarkerShape;
            ShowDataLabelsCheckBox.IsChecked = series.ShowDataLabels;
            ShowDataMarkersCheckBox.IsChecked = series.ShowDataMarkers;
            DataLabelBrushCheckBox.IsChecked = series.DataLabelBrush != null;
            DataMarkerBrushCheckBox.IsChecked = series.DataMarkerBrush != null;
            LegendVisibilityCheckBox.IsChecked = MarkupChart.ShowLegend;
            LineRemoveSeriesButton.IsEnabled = MarkupChart.Series.Count > 1;
            SyncAxisAvailability();
            UpdateDataText();
        }

        private void SyncPresentationOverrideEditors()
        {
            var index = SelectedOverrideIndex();
            if (!index.HasValue)
            {
                LabelOverrideTextBox.Text = "";
                LabelOverrideBrushCheckBox.IsChecked = false;
                MarkerShapeComboBox.SelectedIndex = -1;
                MarkerOverrideBrushCheckBox.IsChecked = false;
                return;
            }
            SyncLabelOverrideEditor(index.Value);
            SyncMarkerOverrideEditor(index.Value);
        }

        private void SyncLabelOverrideEditor(uint index)
        {
            SelectedPresentationSeries().DataLabelOverrides.TryGetValue(index, out var label);
            LabelOverrideTextBox.Text = label?.Text ?? "";
            LabelOverrideBrushCheckBox.IsChecked = label?.Brush != null;
        }

        private void SyncMarkerOverrideEditor(uint index)
        {
            SelectedPresentationSeries().DataMarkerOverrides.TryGetValue(index, out var marker);
            MarkerShapeComboBox.SelectedIndex = (int)(marker?.Shape ?? MarkerShape.Circle);
            MarkerOverrideBrushCheckBox.IsChecked = marker?.Brush != null;
        }

        private void OnPresentationSeriesChanged(object sender, SelectionChangedEventArgs e)
        {
            if (!_ready || _syncing || _closing || PresentationSeriesComboBox.SelectedIndex < 0) return;
            Synchronize(SyncPresentationKnobs);
            if (SelectedOverrideIndex().HasValue)
                PresentationKnobStatusText.Text = $"Editing {SelectedPresentationSeries().Title}.";
        }

        private void OnOverrideIndexChanged(NumberBox sender, NumberBoxValueChangedEventArgs e)
        {
            if (!_ready || _syncing) return;
            Synchronize(SyncPresentationOverrideEditors);
            if (SelectedOverrideIndex() is uint index)
                PresentationKnobStatusText.Text = $"Editing point {index} ({_months[(int)index]}).";
        }

        private void EditPresentation(Action edit, Action sync, string message) =>
            ApplyEdit(edit, sync, PresentationKnobStatusText, message, chart: MarkupChart);

        private void OnLegendVisibilityClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => MarkupChart.ShowLegend = LegendVisibilityCheckBox.IsChecked == true,
                () => LegendVisibilityCheckBox.IsChecked = MarkupChart.ShowLegend, "Legend visibility updated.");
        private void OnApplyLegendTitleClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => MarkupChart.LegendTitle = LegendTitleTextBox.Text,
                () => LegendTitleTextBox.Text = MarkupChart.LegendTitle, "Legend title applied.");
        private void OnShowDataLabelsClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().ShowDataLabels = ShowDataLabelsCheckBox.IsChecked == true,
                () => ShowDataLabelsCheckBox.IsChecked = SelectedPresentationSeries().ShowDataLabels, "Default labels updated.");
        private void OnShowDataMarkersClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().ShowDataMarkers = ShowDataMarkersCheckBox.IsChecked == true,
                () => ShowDataMarkersCheckBox.IsChecked = SelectedPresentationSeries().ShowDataMarkers, "Default markers updated.");
        private void OnDataLabelBrushClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().DataLabelBrush = DataLabelBrushCheckBox.IsChecked == true ? ColorBrush(0, 99, 177) : null,
                () => DataLabelBrushCheckBox.IsChecked = SelectedPresentationSeries().DataLabelBrush != null, "Series label brush updated.");
        private void OnDataMarkerBrushClick(object sender, RoutedEventArgs e) =>
            EditPresentation(() => SelectedPresentationSeries().DataMarkerBrush = DataMarkerBrushCheckBox.IsChecked == true ? ColorBrush(216, 59, 1) : null,
                () => DataMarkerBrushCheckBox.IsChecked = SelectedPresentationSeries().DataMarkerBrush != null, "Series marker brush updated.");

        private void OnApplyLabelOverrideClick(object sender, RoutedEventArgs e)
        {
            if (SelectedOverrideIndex() is uint index)
                EditPresentation(() => SelectedPresentationSeries().DataLabelOverrides[index] =
                    new DataLabelOverride(LabelOverrideTextBox.Text, LabelOverrideBrushCheckBox.IsChecked == true ? ColorBrush(16, 124, 16) : null),
                    () => SyncLabelOverrideEditor(index),
                    $"Label override applied at index {index}.");
        }

        private void OnRemoveLabelOverrideClick(object sender, RoutedEventArgs e)
        {
            if (SelectedOverrideIndex() is uint index)
                EditPresentation(() => SelectedPresentationSeries().DataLabelOverrides.Remove(index),
                    () => SyncLabelOverrideEditor(index), $"Label override removed at index {index}, if present.");
        }

        private void OnApplyMarkerOverrideClick(object sender, RoutedEventArgs e)
        {
            if (SelectedOverrideIndex() is not uint index) return;
            int shape = MarkerShapeComboBox.SelectedIndex;
            if (shape < 0 || shape > (int)MarkerShape.Plus)
            {
                PresentationKnobStatusText.Text = "Choose a marker shape.";
                return;
            }
            EditPresentation(() => SelectedPresentationSeries().DataMarkerOverrides[index] =
                new DataMarkerOverride((MarkerShape)shape, MarkerOverrideBrushCheckBox.IsChecked == true ? ColorBrush(136, 23, 152) : null),
                () => SyncMarkerOverrideEditor(index),
                $"Marker override applied at index {index}.");
        }

        private void OnRemoveMarkerOverrideClick(object sender, RoutedEventArgs e)
        {
            if (SelectedOverrideIndex() is uint index)
                EditPresentation(() => SelectedPresentationSeries().DataMarkerOverrides.Remove(index),
                    () => SyncMarkerOverrideEditor(index), $"Marker override removed at index {index}, if present.");
        }

        private void OnClearOverridesClick(object sender, RoutedEventArgs e) => EditPresentation(() =>
        {
            SelectedPresentationSeries().DataLabelOverrides.Clear();
            SelectedPresentationSeries().DataMarkerOverrides.Clear();
        }, SyncPresentationOverrideEditors, "All overrides cleared for the selected series.");

        private void Synchronize(Action sync)
        {
            bool wasSyncing = _syncing;
            _syncing = true;
            try { sync(); }
            finally { _syncing = wasSyncing; }
        }

        private void ApplyEdit(Action edit, Action sync, TextBlock status, string success,
            string invalid = "That value is not supported. The current setting has been restored.", Chart chart = null)
        {
            if (!_ready || _syncing || _closing) return;
            _lastEditSucceeded = false;
            try
            {
                edit();
                // Brush changes alone can leave the rendered plot at its previous appearance.
                chart?.InvalidateArrange();
                AnnounceStatus(status, success);
                _lastEditSucceeded = true;
            }
            catch (ArgumentException)
            {
                AnnounceStatus(status, invalid);
            }
            catch (COMException ex) when (ex.HResult == unchecked((int)0x80070057))
            {
                AnnounceStatus(status, invalid);
            }
            finally
            {
                Synchronize(sync);
            }
        }

        // Validation errors are usually raised while focus leaves the edited field, and a Polite
        // live-region update is dropped when it coincides with that focus change. Set the status
        // text and raise an explicit UIA notification so it is announced without moving focus.
        // Removing a series also disables the focused button (another focus change), so defer the
        // notification to a low-priority dispatch that runs after focus has settled; otherwise the
        // focus-change announcement cuts it off.
        private void AnnounceStatus(TextBlock target, string message)
        {
            target.Text = message;
            if (string.IsNullOrEmpty(message)) return;
            DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
            {
                AutomationPeer peer = FrameworkElementAutomationPeer.FromElement(target)
                    ?? FrameworkElementAutomationPeer.CreatePeerForElement(target);
                peer?.RaiseNotificationEvent(
                    AutomationNotificationKind.Other,
                    AutomationNotificationProcessing.MostRecent,
                    message,
                    "ChartsSampleStatus");
            });
        }

        private void SyncAxes()
        {
            SyncAxisAvailability();
            LinearMinBox.Value = _yAxis.Minimum ?? double.NaN;
            LinearMaxBox.Value = _yAxis.Maximum ?? double.NaN;
            LinearSpacingBox.Value = _yAxis.Spacing ?? double.NaN;
            SortKeyBox.SelectedIndex = (int)_xAxis.SortKey;
            SortOrderBox.SelectedIndex = (int)_xAxis.SortOrder;
            AxisVisibleCheck.IsChecked = _yAxis.IsVisible;
            TickLabelsCheck.IsChecked = _yAxis.ShowTickLabels;
            TickMarksCheck.IsChecked = _yAxis.ShowTickMarks;
            GridLinesBox.SelectedIndex = (int)_yAxis.GridLines;
            GridLineBrushBox.SelectedIndex = BrushIndex(_yAxis.GridLineMajorBrush);
            TickBrushBox.SelectedIndex = BrushIndex(_yAxis.TickBrush);
            TickLabelBrushBox.SelectedIndex = BrushIndex(_yAxis.TickLabelBrush);
            AxisLineBrushBox.SelectedIndex = BrushIndex(_yAxis.AxisLineBrush);
        }

        private void EditAxis(Action edit, Action sync)
        {
            if (!_ready || _syncing || _closing || !MarkupChart.Series.Contains(ProfitSeries)) return;
            ApplyEdit(edit, sync, AxisStatusText, "Profit axes updated.",
                "Invalid axis value. Use finite bounds with minimum below maximum, and positive spacing, or leave blank for Auto. Current settings restored.",
                MarkupChart);
        }
        private static double? OptionalNumber(NumberBox box) => double.IsNaN(box.Value) ? null : box.Value;
        private void OnLinearMinChanged(NumberBox sender, NumberBoxValueChangedEventArgs e) =>
            EditAxis(() => _yAxis.Minimum = OptionalNumber(sender), () => sender.Value = _yAxis.Minimum ?? double.NaN);
        private void OnLinearMaxChanged(NumberBox sender, NumberBoxValueChangedEventArgs e) =>
            EditAxis(() => _yAxis.Maximum = OptionalNumber(sender), () => sender.Value = _yAxis.Maximum ?? double.NaN);
        private void OnLinearSpacingChanged(NumberBox sender, NumberBoxValueChangedEventArgs e) =>
            EditAxis(() => _yAxis.Spacing = OptionalNumber(sender), () => sender.Value = _yAxis.Spacing ?? double.NaN);
        private void OnSortKeyChanged(object sender, SelectionChangedEventArgs e) =>
            EditAxis(() => _xAxis.SortKey = (CategorySortKey)SortKeyBox.SelectedIndex, () => SortKeyBox.SelectedIndex = (int)_xAxis.SortKey);
        private void OnSortOrderChanged(object sender, SelectionChangedEventArgs e) =>
            EditAxis(() => _xAxis.SortOrder = (SortOrder)SortOrderBox.SelectedIndex, () => SortOrderBox.SelectedIndex = (int)_xAxis.SortOrder);

        private void EditBothAxes(Action<CartesianAxis> edit, Action sync) => EditAxis(() => { edit(_xAxis); edit(_yAxis); }, sync);
        private void OnAxisVisibleChanged(object sender, RoutedEventArgs e) =>
            EditBothAxes(axis => axis.IsVisible = AxisVisibleCheck.IsChecked == true, () => AxisVisibleCheck.IsChecked = _yAxis.IsVisible);
        private void OnTickLabelsChanged(object sender, RoutedEventArgs e) =>
            EditBothAxes(axis => axis.ShowTickLabels = TickLabelsCheck.IsChecked == true, () => TickLabelsCheck.IsChecked = _yAxis.ShowTickLabels);
        private void OnTickMarksChanged(object sender, RoutedEventArgs e) =>
            EditBothAxes(axis => axis.ShowTickMarks = TickMarksCheck.IsChecked == true, () => TickMarksCheck.IsChecked = _yAxis.ShowTickMarks);
        private void OnGridLinesChanged(object sender, SelectionChangedEventArgs e) =>
            EditBothAxes(axis => axis.GridLines = (GridLines)GridLinesBox.SelectedIndex, () => GridLinesBox.SelectedIndex = (int)_yAxis.GridLines);
        private void OnGridLineBrushChanged(object sender, SelectionChangedEventArgs e) =>
            EditBothAxes(axis => axis.GridLineMajorBrush = BrushFromIndex(GridLineBrushBox.SelectedIndex), () => GridLineBrushBox.SelectedIndex = BrushIndex(_yAxis.GridLineMajorBrush));
        private void OnTickBrushChanged(object sender, SelectionChangedEventArgs e) =>
            EditBothAxes(axis => axis.TickBrush = BrushFromIndex(TickBrushBox.SelectedIndex), () => TickBrushBox.SelectedIndex = BrushIndex(_yAxis.TickBrush));
        private void OnTickLabelBrushChanged(object sender, SelectionChangedEventArgs e) =>
            EditBothAxes(axis => axis.TickLabelBrush = BrushFromIndex(TickLabelBrushBox.SelectedIndex), () => TickLabelBrushBox.SelectedIndex = BrushIndex(_yAxis.TickLabelBrush));
        private void OnAxisLineBrushChanged(object sender, SelectionChangedEventArgs e) =>
            EditBothAxes(axis => axis.AxisLineBrush = BrushFromIndex(AxisLineBrushBox.SelectedIndex), () => AxisLineBrushBox.SelectedIndex = BrushIndex(_yAxis.AxisLineBrush));

        private static SolidColorBrush ColorBrush(byte red, byte green, byte blue) => new(Color.FromArgb(255, red, green, blue));
        private static Brush BrushFromIndex(int index) => index switch
        {
            1 => ColorBrush(220, 40, 40),
            2 => ColorBrush(40, 80, 220),
            3 => ColorBrush(40, 180, 60),
            _ => null
        };

        private static int BrushIndex(Brush brush)
        {
            if (brush is not SolidColorBrush solid) return 0;
            if (solid.Color == Color.FromArgb(255, 220, 40, 40)) return 1;
            if (solid.Color == Color.FromArgb(255, 40, 80, 220)) return 2;
            if (solid.Color == Color.FromArgb(255, 40, 180, 60)) return 3;
            return 0;
        }

        private void OnDtIntervalTypeAChanged(object sender, SelectionChangedEventArgs e) =>
            SetDateInterval(_dtAxisA, DtIntervalTypeBoxA, DtWarningA, DtFormatStatusA, true);
        private void OnDtIntervalTypeBChanged(object sender, SelectionChangedEventArgs e) =>
            SetDateInterval(_dtAxisB, DtIntervalTypeBoxB, DtWarningB, DtFormatStatusB, false);

        private void SetDateInterval(DateTimeAxis axis, ComboBox box, TextBlock warning, TextBlock status, bool daily)
        {
            ApplyEdit(() => axis.IntervalType = (DateTimeIntervalType)box.SelectedIndex, () =>
            {
                box.SelectedIndex = (int)axis.IntervalType;
                bool isAuto = axis.IntervalType == DateTimeIntervalType.Auto;
                bool incompatible = daily ? axis.IntervalType == DateTimeIntervalType.Year :
                    axis.IntervalType == DateTimeIntervalType.Day || axis.IntervalType == DateTimeIntervalType.Week;
                if (isAuto)
                {
                    // Known open issue: switching back to Auto can keep the previously plotted
                    // positions. Surface the limitation and the documented workaround instead of
                    // silently reporting success.
                    warning.Text = "Known issue: switching back to Auto can keep the previous plotted positions. Select a specific interval (for example Day) and then Auto again to restore the curve. This is an open Charts control integration issue.";
                    warning.Visibility = Visibility.Visible;
                }
                else
                {
                    warning.Text = daily ? "Warning: Year is not meaningful for a 75-day range." :
                        "Warning: Day/Week ticks are too dense for a three-year range.";
                    warning.Visibility = incompatible ? Visibility.Visible : Visibility.Collapsed;
                }
            }, status, "Date interval updated.");
        }

        private void OnDtLabelFormatAApply(object sender, RoutedEventArgs e) => SetDateFormat(_dtAxisA, DtLabelFormatBoxA, DtFormatStatusA);
        private void OnDtLabelFormatBApply(object sender, RoutedEventArgs e) => SetDateFormat(_dtAxisB, DtLabelFormatBoxB, DtFormatStatusB);

        private void SetDateFormat(DateTimeAxis axis, TextBox box, TextBlock status)
        {
            string preview = "";
            ApplyEdit(() =>
            {
                string template = box.Text.Trim();
                // DateTimeFormatter validates templates before they reach the axis.
                var formatter = new DateTimeFormatter(template.Length == 0 ? "shortdate" : template);
                preview = formatter.Format(new DateTimeOffset(2024, 1, 15, 0, 0, 0, TimeSpan.Zero));
                axis.LabelFormat = template;
            }, () =>
            {
                box.Text = axis.LabelFormat ?? "";
                if (preview.Length > 0 && _lastEditSucceeded)
                    status.Text += $" Example: {preview}";
            }, status, "Label format applied.",
                "Invalid date template. Try shortdate or month day year; blank uses the default. Current format restored.");
        }

        private async void OnToggleSecondaryClick(object sender, RoutedEventArgs e)
        {
            SecondaryButton.IsEnabled = false;
            try
            {
                if (_secondary != null)
                {
                    bool wasRunning = _secondary.IsRunning;
                    await _secondary.StopAsync();
                    _secondary = null;
                    if (_closing) return;
                    if (wasRunning)
                    {
                        AnnounceStatus(StatusText, "Secondary window closed and its UI thread stopped.");
                        return;
                    }
                }

                if (_closing) return;
                var secondary = new SecondaryChartWindow();
                _secondary = secondary;
                string error = await secondary.Ready;
                if (_closing) return;
                if (error != null)
                {
                    await secondary.StopAsync();
                    _secondary = null;
                    AnnounceStatus(StatusText, error);
                }
                else
                {
                    AnnounceStatus(StatusText, "Secondary window updates on its own UI thread. Toggle again to close it.");
                }
            }
            finally
            {
                if (!_closing) SecondaryButton.IsEnabled = true;
            }
        }

        private async void OnAppWindowClosing(AppWindow sender, AppWindowClosingEventArgs args)
        {
            if (_allowClose || _secondary == null) return;
            args.Cancel = true;
            if (_closing) return;
            _closing = true;
            _updates.Stop();
            UpdatesButton.IsEnabled = false;
            SecondaryButton.IsEnabled = false;
            // Keep the primary dispatcher pumping until the other STA has finished.
            await _secondary.StopAsync();
            _secondary = null;
            _allowClose = true;
            Close();
        }

        private void OnClosed(object sender, WindowEventArgs args)
        {
            _closing = true;
            _updates.Stop();
            _updates.Tick -= OnUpdateTick;
            _appWindow.Closing -= OnAppWindowClosing;
            Closed -= OnClosed;
        }
    }
}
