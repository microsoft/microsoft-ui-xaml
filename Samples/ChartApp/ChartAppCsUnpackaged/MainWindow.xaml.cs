using System;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
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
            Color.FromArgb(0xFF, 0x10, 0x7C, 0x10),
            Color.FromArgb(0xFF, 0xD8, 0x3B, 0x01)
        };

        private readonly ObservableCollection<string> _months = new() { "Jan", "Feb", "Mar", "Apr", "May", "Jun" };
        private readonly ObservableVector<double> _profit = new() { 18, 27, 22, 41, 36, 52 };
        private readonly ObservableVector<double> _expenses = new() { 31, 25, 29, 24, 32, 28 };
        private readonly ObservableCollection<double> _area = new() { 8, 18, 14, 29, 24, 37 };
        private readonly ObservableCollection<double> _bars = new() { 12, 20, 17, 31, 26, 39 };
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
        private bool _ready;
        private bool _syncing;
        private bool _closing;
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

            MarkupChart.Axes.Add(_xAxis);
            MarkupChart.Axes.Add(_yAxis);
            ProfitSeries.XAxis = _xAxis;
            ProfitSeries.YAxis = _yAxis;

            var barXAxis = new CategoryAxis { Label = "Month" };
            var barYAxis = new LinearAxis { Label = "Value" };
            BarMarkupChart.Axes.Add(barXAxis);
            BarMarkupChart.Axes.Add(barYAxis);
            MarkupBarSeries.XAxis = barXAxis;
            MarkupBarSeries.YAxis = barYAxis;

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
            var areaText = new StringBuilder();
            var barText = new StringBuilder();
            for (int i = 0; i < _months.Count; i++)
            {
                text.AppendLine($"{_months[i]}: profit {_profit[i]:0}, expenses {_expenses[i]:0}, area {_area[i]:0}, bars {_bars[i]:0}");
                areaText.AppendLine($"{_months[i]}: {_area[i]:0}");
                barText.AppendLine($"{_months[i]}: {_bars[i]:0}");
            }
            text.AppendLine("Code-created line:");
            for (int i = 0; i < _codeValues.Count; i++)
            {
                text.AppendLine($"{_codeCategories[i]}: {_codeValues[i]:0}");
            }
            DataText.Text = text.ToString();
            AreaDataText.Text = areaText.ToString().TrimEnd();
            BarDataText.Text = barText.ToString().TrimEnd();
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
            PointOverrideEditors.Visibility = scenario == "presentation" ? Visibility.Visible : Visibility.Collapsed;
            PresentationKnobStatusText.Visibility = scenario is "line" or "presentation" ? Visibility.Visible : Visibility.Collapsed;
            AxisEditors.Visibility = scenario == "axes" ? Visibility.Visible : Visibility.Collapsed;
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

        private void OnHeaderSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (ThemeChoice == null) return;
            bool stacked = e.NewSize.Width < 640;
            Grid.SetRow(ThemeChoice, stacked ? 1 : 0);
            Grid.SetColumn(ThemeChoice, stacked ? 0 : 1);
            Grid.SetColumnSpan(HeadingPanel, stacked ? 2 : 1);
            ThemeChoice.HorizontalAlignment = stacked ? HorizontalAlignment.Left : HorizontalAlignment.Right;
        }

        private void OnEditorSizeChanged(object sender, SizeChangedEventArgs e)
        {
            var grid = (Grid)sender;
            int columns = e.NewSize.Width >= (grid == DateTimeScenario ? 900 : 600) ? 2 : 1;
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

        private void OnToggleUpdatesClick(object sender, RoutedEventArgs e)
        {
            if (_updates.IsEnabled)
            {
                _updates.Stop();
                UpdatesButton.Content = "Resume updates";
                StatusText.Text = "Primary chart updates paused. The secondary window updates independently.";
            }
            else
            {
                _updates.Start();
                UpdatesButton.Content = "Pause updates";
                StatusText.Text = "Primary chart updates resumed, once a second.";
            }
        }

        private void OnToggleBarOrientationClick(object sender, RoutedEventArgs e)
        {
            EditBar(() =>
            {
                MarkupBarSeries.Orientation = MarkupBarSeries.Orientation == BarOrientation.Horizontal
                    ? BarOrientation.Vertical : BarOrientation.Horizontal;
                StatusText.Text = $"Bar orientation: {MarkupBarSeries.Orientation}. The same explicit axes remain connected.";
            }, "Bar orientation changed. X remains categories; Y remains values.");
        }

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

        private void SyncAreaOptions()
        {
            AreaColorChoice.SelectedIndex = ExampleColorIndex(MarkupAreaSeries.Stroke, OriginalAreaStroke);
            AreaFillChoice.SelectedIndex = MarkupAreaSeries.Fill is SolidColorBrush fill
                ? fill.Color.A switch { 0x60 => 0, 0xFF => 1, 0 => 2, _ => -1 }
                : -1;
            AreaVisibleCheckBox.IsChecked = MarkupAreaSeries.IsVisible;
            AreaValuesCheckBox.IsChecked = MarkupAreaSeries.ShowDataLabels;
            AreaMarkersCheckBox.IsChecked = MarkupAreaSeries.ShowDataMarkers;
            AreaLegendCheckBox.IsChecked = AreaMarkupChart.ShowLegend;
        }

        private void SyncBarOptions()
        {
            BarColorChoice.SelectedIndex = ExampleColorIndex(MarkupBarSeries.Stroke, OriginalBarStroke);
            BarVisibleCheckBox.IsChecked = MarkupBarSeries.IsVisible;
            BarValuesCheckBox.IsChecked = MarkupBarSeries.ShowDataLabels;
            BarLegendCheckBox.IsChecked = BarMarkupChart.ShowLegend;
            BarOrientationText.Text = $"Orientation: {MarkupBarSeries.Orientation}";
        }

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
                MarkupAreaSeries.Fill = new SolidColorBrush(fill);
                MarkupAreaSeries.Stroke = new SolidColorBrush(stroke);
                if (MarkupAreaSeries.ShowDataMarkers)
                    MarkupAreaSeries.DataMarkerBrush = MarkupAreaSeries.Stroke;
            }, "Area color and fill updated. The source data is unchanged.");
        }

        private void OnAreaVisibilityClick(object sender, RoutedEventArgs e) =>
            EditArea(() => MarkupAreaSeries.IsVisible = AreaVisibleCheckBox.IsChecked == true, "Area series visibility updated.");

        private void OnAreaValuesClick(object sender, RoutedEventArgs e) =>
            EditArea(() => MarkupAreaSeries.ShowDataLabels = AreaValuesCheckBox.IsChecked == true, "Area value labels updated.");

        private void OnAreaMarkersClick(object sender, RoutedEventArgs e)
        {
            EditArea(() =>
            {
                MarkupAreaSeries.ShowDataMarkers = AreaMarkersCheckBox.IsChecked == true;
                MarkupAreaSeries.MarkerShape = MarkerShape.Circle;
                MarkupAreaSeries.DataMarkerBrush = MarkupAreaSeries.ShowDataMarkers ? MarkupAreaSeries.Stroke : null;
            }, "Area point markers updated.");
        }

        private void OnAreaLegendClick(object sender, RoutedEventArgs e) =>
            EditArea(() => AreaMarkupChart.ShowLegend = AreaLegendCheckBox.IsChecked == true, "Area legend updated.");

        private void OnResetAreaClick(object sender, RoutedEventArgs e)
        {
            EditArea(() =>
            {
                MarkupAreaSeries.Fill = new SolidColorBrush(OriginalAreaFill);
                MarkupAreaSeries.Stroke = new SolidColorBrush(OriginalAreaStroke);
                MarkupAreaSeries.IsVisible = true;
                MarkupAreaSeries.ShowDataLabels = false;
                MarkupAreaSeries.ShowDataMarkers = false;
                MarkupAreaSeries.MarkerShape = MarkerShape.Circle;
                MarkupAreaSeries.DataMarkerBrush = null;
                AreaMarkupChart.ShowLegend = true;
            }, "Area example reset. Other charts and the application theme are unchanged.");
        }

        private void OnBarColorChanged(object sender, SelectionChangedEventArgs e)
        {
            EditBar(() =>
            {
                Color fill = SelectedExampleColor(BarColorChoice, OriginalBarFill);
                Color stroke = SelectedExampleColor(BarColorChoice, OriginalBarStroke);
                MarkupBarSeries.Fill = new SolidColorBrush(fill);
                MarkupBarSeries.Stroke = new SolidColorBrush(stroke);
            }, "Bar color updated. The source data is unchanged.");
        }

        private void OnBarVisibilityClick(object sender, RoutedEventArgs e) =>
            EditBar(() => MarkupBarSeries.IsVisible = BarVisibleCheckBox.IsChecked == true, "Bar series visibility updated.");

        private void OnBarValuesClick(object sender, RoutedEventArgs e) =>
            EditBar(() => MarkupBarSeries.ShowDataLabels = BarValuesCheckBox.IsChecked == true, "Bar value labels updated.");

        private void OnBarLegendClick(object sender, RoutedEventArgs e) =>
            EditBar(() => BarMarkupChart.ShowLegend = BarLegendCheckBox.IsChecked == true, "Bar legend updated.");

        private void OnResetBarClick(object sender, RoutedEventArgs e)
        {
            EditBar(() =>
            {
                MarkupBarSeries.Fill = new SolidColorBrush(OriginalBarFill);
                MarkupBarSeries.Stroke = new SolidColorBrush(OriginalBarStroke);
                MarkupBarSeries.IsVisible = true;
                MarkupBarSeries.ShowDataLabels = false;
                BarMarkupChart.ShowLegend = true;
                MarkupBarSeries.Orientation = BarOrientation.Horizontal;
            }, "Bar example reset to horizontal. Other charts and the application theme are unchanged.");
        }

        private LineSeries SelectedPresentationSeries() => PresentationSeriesComboBox.SelectedIndex == 1 ? ExpensesSeries : ProfitSeries;

        private uint? SelectedOverrideIndex()
        {
            double value = OverrideIndexNumberBox.Value;
            if (!double.IsFinite(value) || value < 0 || value >= _months.Count || value != Math.Truncate(value))
            {
                PresentationKnobStatusText.Text = "Point index must be a whole number from 0 to 5.";
                return null;
            }
            return (uint)value;
        }

        private void SyncPresentationKnobs()
        {
            var series = SelectedPresentationSeries();
            ShowDataLabelsCheckBox.IsChecked = series.ShowDataLabels;
            ShowDataMarkersCheckBox.IsChecked = series.ShowDataMarkers;
            DataLabelBrushCheckBox.IsChecked = series.DataLabelBrush != null;
            DataMarkerBrushCheckBox.IsChecked = series.DataMarkerBrush != null;
            SyncPresentationOverrideEditors();
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
            if (!_ready || _syncing) return;
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
            ApplyEdit(edit, sync, PresentationKnobStatusText, message);

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
            _syncing = true;
            try { sync(); }
            finally { _syncing = false; }
        }

        private void ApplyEdit(Action edit, Action sync, TextBlock status, string success,
            string invalid = "That value is not supported. The current setting has been restored.", Chart chart = null)
        {
            if (!_ready || _syncing) return;
            try
            {
                edit();
                // Brush changes alone can leave the rendered plot at its previous appearance.
                chart?.InvalidateArrange();
                status.Text = success;
            }
            catch (ArgumentException)
            {
                status.Text = invalid;
            }
            catch (COMException ex) when (ex.HResult == unchecked((int)0x80070057))
            {
                status.Text = invalid;
            }
            finally
            {
                Synchronize(sync);
            }
        }

        private void SyncAxes()
        {
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

        private void EditAxis(Action edit, Action sync) => ApplyEdit(edit, sync, AxisStatusText, "Profit axes updated.",
            "Invalid axis value. Use finite bounds with minimum below maximum, and positive spacing, or leave blank for Auto. Current settings restored.",
            MarkupChart);
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
                bool incompatible = daily ? axis.IntervalType == DateTimeIntervalType.Year :
                    axis.IntervalType == DateTimeIntervalType.Day || axis.IntervalType == DateTimeIntervalType.Week;
                warning.Text = daily ? "Warning: Year is not meaningful for a 75-day range." :
                    "Warning: Day/Week ticks are too dense for a three-year range.";
                warning.Visibility = incompatible ? Visibility.Visible : Visibility.Collapsed;
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
                if (preview.Length > 0 && status.Text == "Label format applied.")
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
                        StatusText.Text = "Secondary window closed and its UI thread stopped.";
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
                    StatusText.Text = error;
                }
                else
                {
                    StatusText.Text = "Secondary window updates on its own UI thread. Toggle again to close it.";
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
