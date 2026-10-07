// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Charts;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Charts")]
    public sealed partial class ChartsPage : TestPage
    {
        private static XamlChartsResources s_chartsResources;

        private readonly ObservableCollection<string> _months = new ObservableCollection<string> { "Jan", "Feb", "Mar", "Apr" };
        private readonly ObservableCollection<double> _sales = new ObservableCollection<double> { 12, 15, 9, 18 };
        private readonly ObservableCollection<double> _costs = new ObservableCollection<double> { 7, 8, 6, 10 };
        private readonly ObservableCollection<double> _units = new ObservableCollection<double> { 4, 6, 3, 8 };

        public ChartsPage()
        {
            // The default Chart style uses theme resources from XamlChartsResources, which apps merge at
            // application level next to XamlControlsResources.
            s_chartsResources ??= new XamlChartsResources();
            App.AppendResourceDictionaryToMergedDictionaries(s_chartsResources);

            this.InitializeComponent();
            this.Unloaded += ChartsPage_Unloaded;

            var months = new Samples { ItemsSource = _months };
            TestChart.Series.Add(new LineSeries { Title = "Sales", XValues = months, YValues = new Samples { ItemsSource = _sales }, ShowDataMarkers = true });
            TestChart.Series.Add(new AreaSeries { Title = "Costs", XValues = months, YValues = new Samples { ItemsSource = _costs } });
            TestChart.Series.Add(new BarSeries { Title = "Units", Orientation = BarOrientation.Vertical, XValues = months, YValues = new Samples { ItemsSource = _units } });

            UpdateStatus();
        }

        private void ChartsPage_Unloaded(object sender, RoutedEventArgs e)
        {
            App.RemoveResourceDictionaryFromMergedDictionaries(s_chartsResources);
        }

        private void AddPointButton_Click(object sender, RoutedEventArgs e)
        {
            int next = _months.Count + 1;
            _months.Add("M" + next);
            _sales.Add(10 + next);
            _costs.Add(5 + next);
            _units.Add(next);
            UpdateStatus();
        }

        private void RemoveSeriesButton_Click(object sender, RoutedEventArgs e)
        {
            if (TestChart.Series.Count > 0)
            {
                TestChart.Series.RemoveAt(TestChart.Series.Count - 1);
            }
            UpdateStatus();
        }

        private void ToggleLegendButton_Click(object sender, RoutedEventArgs e)
        {
            TestChart.ShowLegend = !TestChart.ShowLegend;
            UpdateStatus();
        }

        private void ToggleThemeButton_Click(object sender, RoutedEventArgs e)
        {
            TestChart.RequestedTheme = TestChart.RequestedTheme == ElementTheme.Dark ? ElementTheme.Light : ElementTheme.Dark;
            UpdateStatus();
        }

        private void UpdateStatus()
        {
            StatusText.Text = $"Points={_months.Count} Series={TestChart.Series.Count} Legend={(TestChart.ShowLegend ? "On" : "Off")} Theme={TestChart.RequestedTheme}";
        }
    }
}
