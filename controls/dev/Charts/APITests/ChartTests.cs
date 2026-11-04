// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.XamlTypeInfo;
using MUXControlsTestApp.Utilities;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Xaml.Interop;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ChartTests : ChartTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void ClassInitialize(TestContext context)
        {
            RunOnUIThread.Execute(ChartTestHelpers.EnsureChartsResources);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            RunOnUIThread.Execute(ChartTestHelpers.RemoveChartsResources);
        }

        private const string SamplesTypeName = "Microsoft.UI.Xaml.Controls.Charts.Samples";
        private const string ChartTypeName = "Microsoft.UI.Xaml.Controls.Charts.Chart";
        private const string AxisTypeName = "Microsoft.UI.Xaml.Controls.Charts.Axis";
        private const string CartesianAxisTypeName = "Microsoft.UI.Xaml.Controls.Charts.CartesianAxis";
        private const string LinearAxisTypeName = "Microsoft.UI.Xaml.Controls.Charts.LinearAxis";
        private const string CategoryAxisTypeName = "Microsoft.UI.Xaml.Controls.Charts.CategoryAxis";
        private const string DateTimeAxisTypeName = "Microsoft.UI.Xaml.Controls.Charts.DateTimeAxis";
        private const string CartesianSeriesTypeName = "Microsoft.UI.Xaml.Controls.Charts.CartesianSeries";
        private const string LineSeriesTypeName = "Microsoft.UI.Xaml.Controls.Charts.LineSeries";
        private const string AreaSeriesTypeName = "Microsoft.UI.Xaml.Controls.Charts.AreaSeries";
        private const string BarSeriesTypeName = "Microsoft.UI.Xaml.Controls.Charts.BarSeries";
        private const string BarOrientationTypeName = "Microsoft.UI.Xaml.Controls.Charts.BarOrientation";
        private const string MarkerShapeTypeName = "Microsoft.UI.Xaml.Controls.Charts.MarkerShape";
        private const string StrokeDashStyleTypeName = "Microsoft.UI.Xaml.Controls.Charts.StrokeDashStyle";
        private const string GridLinesTypeName = "Microsoft.UI.Xaml.Controls.Charts.GridLines";
        private const string SortOrderTypeName = "Microsoft.UI.Xaml.Controls.Charts.SortOrder";
        private const string DateTimeIntervalTypeName = "Microsoft.UI.Xaml.Controls.Charts.DateTimeIntervalType";
        private const string ResourcesTypeName = "Microsoft.UI.Xaml.Controls.Charts.XamlChartsResources";

        private struct VectorEvent
        {
            public CollectionChange Change;
            public uint Index;
        }

        private static Samples SamplesFrom<T>(IEnumerable<T> values)
        {
            return new Samples { ItemsSource = values };
        }

        private static LineSeries CreateLineSeries()
        {
            return new LineSeries
            {
                XValues = SamplesFrom(new[] { "Jan", "Feb", "Mar" }),
                YValues = SamplesFrom(new[] { 1.0, 2.0, 3.0 })
            };
        }

        private static AreaSeries CreateAreaSeries()
        {
            return new AreaSeries
            {
                XValues = SamplesFrom(new[] { "Jan", "Feb", "Mar" }),
                YValues = SamplesFrom(new[] { 3.0, 2.0, 1.0 })
            };
        }

        private static BarSeries CreateBarSeries()
        {
            return new BarSeries
            {
                XValues = SamplesFrom(new[] { "Jan", "Feb", "Mar" }),
                YValues = SamplesFrom(new[] { 4.0, 5.0, 6.0 })
            };
        }

        private static Chart CreatePopulatedChart()
        {
            var chart = new Chart
            {
                Width = 400,
                Height = 300
            };
            chart.Series.Add(CreateLineSeries());
            return chart;
        }

        private static XamlControlsChartsXamlMetaDataProvider CreateProvider()
        {
            return new XamlControlsChartsXamlMetaDataProvider();
        }

        private static SolidColorBrush Brush(Color color)
        {
            return new SolidColorBrush(color);
        }

        private static void VerifyVectorEvent(VectorEvent actual, CollectionChange change, uint index)
        {
            Verify.AreEqual(change, actual.Change, "Vector change should match.");
            Verify.AreEqual(index, actual.Index, "Vector change index should match.");
        }

        private static void VerifyNextVectorEvent(IList<VectorEvent> events, int before, CollectionChange change, uint index)
        {
            Verify.AreEqual(before + 1, events.Count, "Mutation should raise exactly one vector event.");
            VerifyVectorEvent(events[before], change, index);
        }

        [TestMethod]
        public void ChartDefaultsExposeEmptyCollectionsAndNoLegend()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();

                Verify.IsFalse(chart.ShowLegend, "Default ShowLegend should be false.");
                Verify.AreEqual(string.Empty, chart.LegendTitle, "Default LegendTitle should be empty.");
                Verify.AreEqual(0, chart.Series.Count, "Default Series should be empty.");
                Verify.AreEqual(0, chart.Axes.Count, "Default Axes should be empty.");
                Verify.AreEqual(0, chart.Data.Count, "Default Data should be empty.");
                Verify.AreSame(chart.Series, chart.Series, "Series should return the same collection.");
                Verify.AreSame(chart.Axes, chart.Axes, "Axes should return the same collection.");
                Verify.AreSame(chart.Data, chart.Data, "Data should return the same collection.");
            });
        }

        [TestMethod]
        public void ChartMeasureUsesAvailableMinimumsAndFallback()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();

                chart.Measure(new Size(80, 48));
                Verify.AreEqual(80.0, chart.DesiredSize.Width, "Finite available width should be desired width.");
                Verify.AreEqual(48.0, chart.DesiredSize.Height, "Finite available height should be desired height.");

                chart.Measure(new Size(0, 0));
                Verify.AreEqual(0.0, chart.DesiredSize.Width, "Zero available width should stay zero.");
                Verify.AreEqual(0.0, chart.DesiredSize.Height, "Zero available height should stay zero.");

                chart.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
                Verify.AreEqual(160.0, chart.DesiredSize.Width, "Infinite available width should use fallback width.");
                Verify.AreEqual(96.0, chart.DesiredSize.Height, "Infinite available height should use fallback height.");

                chart.MinWidth = 200.0;
                chart.MinHeight = 120.0;
                chart.Measure(new Size(double.PositiveInfinity, double.PositiveInfinity));
                Verify.AreEqual(200.0, chart.DesiredSize.Width, "MinWidth should raise the fallback width.");
                Verify.AreEqual(120.0, chart.DesiredSize.Height, "MinHeight should raise the fallback height.");
            });
        }

        [TestMethod]
        public void ChartCanLoadUnloadAndReload()
        {
            Chart chart = null;
            int loadedCount = 0;
            int unloadedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();
                chart.Loaded += (sender, args) => loadedCount++;
                chart.Unloaded += (sender, args) => unloadedCount++;

                Content = chart;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = new Grid();
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = chart;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, loadedCount, "Chart should load each time it enters content.");
                Verify.AreEqual(1, unloadedCount, "Chart should unload after it leaves content.");
                Verify.AreEqual(1, chart.Series.Count, "Series should remain after reload.");
                Verify.IsTrue(chart.IsLoaded, "Chart should be loaded after reload.");
            });
        }

        [TestMethod]
        public void ChartResizeWhileLoadedUpdatesActualSize()
        {
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();

                Content = chart;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                chart.Width = 640;
                chart.Height = 360;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after resize.");
                Verify.AreEqual(640.0, chart.ActualWidth, "ActualWidth should update after resize.");
                Verify.AreEqual(360.0, chart.ActualHeight, "ActualHeight should update after resize.");
            });
        }

        [TestMethod]
        public void ParentRequestedThemeCanChangeWhileChartIsLoaded()
        {
            Chart chart = null;
            Grid host = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();
                host = new Grid();
                host.Children.Add(chart);

                Content = host;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                host.RequestedTheme = ElementTheme.Dark;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after parent Dark theme.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after parent Dark theme.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after parent Dark theme.");
                Verify.AreEqual(ElementTheme.Dark, chart.ActualTheme, "Parent Dark theme should apply.");
                host.RequestedTheme = ElementTheme.Light;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after parent Light theme.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after parent Light theme.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after parent Light theme.");
                Verify.AreEqual(ElementTheme.Light, chart.ActualTheme, "Parent Light theme should apply.");
            });
        }

        [TestMethod]
        public void LegendPropertiesRoundTripBeforeAndAfterLoad()
        {
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();
                chart.ShowLegend = true;
                chart.LegendTitle = "Quarterly revenue";

                Content = chart;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should be loaded.");
                Verify.IsTrue(chart.ShowLegend, "ShowLegend should be true after load.");
                Verify.AreEqual("Quarterly revenue", chart.LegendTitle, "LegendTitle should be set after load.");

                chart.LegendTitle = "Annual revenue";
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after LegendTitle update.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after LegendTitle update.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after LegendTitle update.");
                Verify.AreEqual("Annual revenue", chart.LegendTitle, "LegendTitle should update while loaded.");

                chart.LegendTitle = string.Empty;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after clearing LegendTitle.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after clearing LegendTitle.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after clearing LegendTitle.");
                Verify.AreEqual(string.Empty, chart.LegendTitle, "Empty LegendTitle should round-trip.");

                chart.ShowLegend = false;
                chart.LegendTitle = "Hidden title";
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after hiding legend.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after hiding legend.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after hiding legend.");
                Verify.IsFalse(chart.ShowLegend, "LegendTitle should not force legend visibility.");
                Verify.AreEqual("Hidden title", chart.LegendTitle, "LegendTitle should remain set while legend is hidden.");
            });
        }

        [TestMethod]
        public void ChartLegendDependencyPropertiesUpdateAndClearValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();

                chart.SetValue(Chart.ShowLegendProperty, true);
                chart.SetValue(Chart.LegendTitleProperty, "Revenue");
                Verify.IsTrue(chart.ShowLegend, "ShowLegendProperty should update ShowLegend.");
                Verify.AreEqual("Revenue", chart.LegendTitle, "LegendTitleProperty should update LegendTitle.");
                Verify.IsTrue((bool)chart.GetValue(Chart.ShowLegendProperty), "ShowLegendProperty should read the local value.");
                Verify.AreEqual("Revenue", (string)chart.GetValue(Chart.LegendTitleProperty), "LegendTitleProperty should read the local value.");

                chart.ClearValue(Chart.ShowLegendProperty);
                chart.ClearValue(Chart.LegendTitleProperty);
                Verify.IsFalse(chart.ShowLegend, "ClearValue should restore ShowLegend default.");
                Verify.AreEqual(string.Empty, chart.LegendTitle, "ClearValue should restore LegendTitle default.");
            });
        }

        [TestMethod]
        public void ChartFormattingPropertiesRoundTripBeforeAndAfterLoad()
        {
            Chart chart = null;
            SolidColorBrush background = null;
            SolidColorBrush foreground = null;
            SolidColorBrush replacementBackground = null;
            SolidColorBrush replacementForeground = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();
                background = Brush(Colors.Red);
                foreground = Brush(Colors.Blue);
                chart.Background = background;
                chart.Foreground = foreground;
                chart.FontFamily = new FontFamily("Consolas");
                chart.FontSize = 18.0;

                Content = chart;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should be loaded.");
                Verify.AreSame(background, chart.Background, "Background should round-trip after load.");
                Verify.AreSame(foreground, chart.Foreground, "Foreground should round-trip after load.");
                Verify.AreEqual("Consolas", chart.FontFamily.Source, "FontFamily should round-trip after load.");
                Verify.AreEqual(18.0, chart.FontSize, "FontSize should round-trip after load.");

                replacementBackground = Brush(Colors.Green);
                replacementForeground = Brush(Colors.Yellow);
                chart.Background = replacementBackground;
                chart.Foreground = replacementForeground;
                chart.FontFamily = new FontFamily("Segoe UI");
                chart.FontSize = 20.0;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded after formatting changes.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain stable after formatting changes.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain stable after formatting changes.");
                Verify.AreSame(replacementBackground, chart.Background, "Background should update while loaded.");
                Verify.AreSame(replacementForeground, chart.Foreground, "Foreground should update while loaded.");
                Verify.AreEqual("Segoe UI", chart.FontFamily.Source, "FontFamily should update while loaded.");
                Verify.AreEqual(20.0, chart.FontSize, "FontSize should update while loaded.");
            });
        }

        [TestMethod]
        public void LineSeriesWithoutExplicitAxesKeepsAxisPropertiesNullAfterLoad()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreatePopulatedChart();
                series = (LineSeries)chart.Series[0];

                Content = chart;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Chart should be loaded before axis assertions.");
                Verify.IsNull(series.XAxis, "Default XAxis should remain null.");
                Verify.IsNull(series.YAxis, "Default YAxis should remain null.");
                Verify.AreEqual(0, chart.Axes.Count, "Automatic axes should not be added to Chart.Axes.");
            });
        }

        [TestMethod]
        public void SeriesCollectionSupportsIndexingCopyAndEnumeration()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var first = CreateLineSeries();
                var second = CreateAreaSeries();

                chart.Series.Add(first);
                chart.Series.Add(second);

                Verify.AreEqual(2, chart.Series.Count, "Series count should update.");
                Verify.AreSame(first, chart.Series[0], "Indexer should return the first series.");
                Verify.AreSame(second, chart.Series[1], "Indexer should return the second series.");
                Verify.IsTrue(chart.Series.Contains(first), "Contains should find an added series.");

                var copied = new CartesianSeries[2];
                chart.Series.CopyTo(copied, 0);
                Verify.AreSame(first, copied[0], "CopyTo should copy the first series.");
                Verify.AreSame(second, copied[1], "CopyTo should copy the second series.");

                var iterated = new List<CartesianSeries>();
                foreach (var series in chart.Series)
                {
                    iterated.Add(series);
                }

                Verify.AreEqual(2, iterated.Count, "Enumeration should visit all series.");
                Verify.AreSame(first, iterated[0], "Enumeration should preserve order.");
                Verify.AreSame(second, iterated[1], "Enumeration should preserve order.");
            });
        }

        [TestMethod]
        public void SeriesCollectionMutationDuringIterationThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                chart.Series.Add(CreateLineSeries());

                using (var iterator = chart.Series.GetEnumerator())
                {
                    Verify.IsTrue(iterator.MoveNext(), "Iterator should start on the first item.");
                    chart.Series.Add(CreateAreaSeries());
                    Verify.Throws<InvalidOperationException>(() => iterator.MoveNext(), "Mutation should invalidate iteration.");
                }
            });
        }

        [TestMethod]
        public void SeriesCollectionRaisesExactVectorEvents()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var events = new List<VectorEvent>();
                var observable = (IObservableVector<CartesianSeries>)chart.Series;
                observable.VectorChanged += (sender, args) =>
                {
                    events.Add(new VectorEvent { Change = args.CollectionChange, Index = args.Index });
                };

                var first = CreateLineSeries();
                var second = CreateAreaSeries();
                var third = CreateBarSeries();

                int eventCount = events.Count;
                chart.Series.Add(first);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                eventCount = events.Count;
                chart.Series.Insert(0, second);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                eventCount = events.Count;
                chart.Series[1] = third;
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemChanged, 1);

                eventCount = events.Count;
                chart.Series[1] = third;
                Verify.AreEqual(eventCount, events.Count, "Setting the same series should not raise an event.");

                eventCount = events.Count;
                chart.Series.RemoveAt(0);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemRemoved, 0);

                eventCount = events.Count;
                chart.Series.Clear();
                VerifyNextVectorEvent(events, eventCount, CollectionChange.Reset, 0);

                eventCount = events.Count;
                chart.Series.Clear();
                Verify.AreEqual(eventCount, events.Count, "Clearing an empty collection should not raise an event.");
            });
        }

        [TestMethod]
        public void SeriesCollectionRejectsOutOfRangeAndNull()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();

                Verify.Throws<ArgumentOutOfRangeException>(() => { var item = chart.Series[0]; }, "Indexer should reject an empty collection.");
                Verify.Throws<ArgumentOutOfRangeException>(() => chart.Series[0] = CreateLineSeries(), "Setter should reject an empty collection.");
                Verify.Throws<ArgumentOutOfRangeException>(() => chart.Series.Insert(1, CreateLineSeries()), "Insert should reject index past Count.");
                Verify.Throws<ArgumentOutOfRangeException>(() => chart.Series.RemoveAt(0), "RemoveAt should reject an empty collection.");
                Verify.Throws<ArgumentException>(() => chart.Series.Add(null), "Series should reject null.");
                Verify.AreEqual(0, chart.Series.Count, "Rejected mutations should leave the collection empty.");
            });
        }

        [TestMethod]
        public void SeriesCanBeAddedAgainAfterRemoval()
        {
            RunOnUIThread.Execute(() =>
            {
                var firstChart = new Chart();
                var secondChart = new Chart();
                var series = CreateLineSeries();

                firstChart.Series.Add(series);
                firstChart.Series.RemoveAt(0);
                secondChart.Series.Add(series);

                Verify.AreEqual(0, firstChart.Series.Count, "First chart should be empty after removal.");
                Verify.AreEqual(1, secondChart.Series.Count, "Removed series should be reusable.");
                Verify.AreSame(series, secondChart.Series[0], "Reused series should be retained.");
            });
        }

        [TestMethod]
        public void FailedSeriesMutationsDoNotChangeCollectionOrRaiseEvents()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var secondChart = new Chart();
                var original = CreateLineSeries();
                chart.Series.Add(original);

                int eventCount = 0;
                var observable = (IObservableVector<CartesianSeries>)chart.Series;
                observable.VectorChanged += (sender, args) => eventCount++;

                Verify.Throws<ArgumentException>(() => chart.Series.Add(original), "Duplicate Add should throw.");
                Verify.Throws<ArgumentException>(() => chart.Series.Insert(0, original), "Duplicate Insert should throw.");
                Verify.Throws<ArgumentException>(() => secondChart.Series.Add(original), "Series already in a chart should be rejected.");
                chart.Series[0] = original;
                Verify.AreEqual(0, eventCount, "Rejected or unchanged mutations should not raise events.");
                Verify.AreEqual(1, chart.Series.Count, "Collection should retain the original item.");
                Verify.AreEqual(0, secondChart.Series.Count, "Second chart should stay empty.");
                Verify.AreSame(original, chart.Series[0], "Original series should remain.");
            });
        }

        [TestMethod]
        public void AxisCollectionSupportsAddRemoveAndVectorEvents()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var events = new List<VectorEvent>();
                var observable = (IObservableVector<Axis>)chart.Axes;
                observable.VectorChanged += (sender, args) =>
                {
                    events.Add(new VectorEvent { Change = args.CollectionChange, Index = args.Index });
                };

                var first = new LinearAxis();
                var second = new CategoryAxis();
                int eventCount = events.Count;
                chart.Axes.Add(first);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                eventCount = events.Count;
                chart.Axes.Insert(0, second);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                Verify.AreEqual(2, chart.Axes.Count, "Axes count should update.");
                Verify.AreSame(second, chart.Axes[0], "Insert should place the category axis first.");
                Verify.AreSame(first, chart.Axes[1], "Linear axis should move to index 1.");

                eventCount = events.Count;
                chart.Axes.RemoveAt(1);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemRemoved, 1);

                eventCount = events.Count;
                chart.Axes.Clear();
                VerifyNextVectorEvent(events, eventCount, CollectionChange.Reset, 0);
                Verify.AreEqual(0, chart.Axes.Count, "Axes should be empty after Clear.");
            });
        }

        [TestMethod]
        public void AxisCollectionRejectsDuplicateCrossChartNullAndOutOfRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var firstChart = new Chart();
                var secondChart = new Chart();
                var axis = new LinearAxis();

                firstChart.Axes.Add(axis);

                Verify.Throws<ArgumentException>(() => firstChart.Axes.Add(axis), "Duplicate axis should be rejected.");
                Verify.Throws<ArgumentException>(() => secondChart.Axes.Add(axis), "Axis already in a chart should be rejected.");
                Verify.Throws<ArgumentException>(() => firstChart.Axes.Add(null), "Axes should reject null.");
                Verify.Throws<ArgumentOutOfRangeException>(() => firstChart.Axes[1] = new LinearAxis(), "Setter should reject index past Count.");
                Verify.Throws<ArgumentOutOfRangeException>(() => firstChart.Axes.Insert(3, new LinearAxis()), "Insert should reject index past Count.");
                Verify.Throws<ArgumentOutOfRangeException>(() => secondChart.Axes.RemoveAt(0), "RemoveAt should reject an empty collection.");
                Verify.AreEqual(1, firstChart.Axes.Count, "First chart should keep its axis.");
                Verify.AreEqual(0, secondChart.Axes.Count, "Second chart should stay empty.");
            });
        }

        [TestMethod]
        public void ExplicitAxesCanBeAssignedToSeriesInSameChart()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var xAxis = new CategoryAxis();
                var yAxis = new LinearAxis();
                var series = CreateLineSeries();

                chart.Axes.Add(xAxis);
                chart.Axes.Add(yAxis);
                series.XAxis = xAxis;
                series.YAxis = yAxis;
                chart.Series.Add(series);

                Content = chart;
                Content.UpdateLayout();

                Verify.AreSame(xAxis, series.XAxis, "Series XAxis should keep the explicit axis.");
                Verify.AreSame(yAxis, series.YAxis, "Series YAxis should keep the explicit axis.");
                Verify.AreEqual(2, chart.Axes.Count, "Explicit axes should remain in Chart.Axes.");
            });
        }

        [TestMethod]
        public void SeriesRejectsAxisNotPresentInSameChart()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var series = CreateLineSeries();
                var missingAxis = new LinearAxis();

                series.YAxis = missingAxis;

                Verify.Throws<ArgumentException>(() => chart.Series.Add(series), "Series should reject an axis outside Chart.Axes.");
                Verify.AreEqual(0, chart.Series.Count, "Rejected series should not be added.");
            });
        }

        [TestMethod]
        public void SeriesRejectsAxisFromAnotherChart()
        {
            RunOnUIThread.Execute(() =>
            {
                var firstChart = new Chart();
                var secondChart = new Chart();
                var axis = new LinearAxis();
                var series = CreateLineSeries();

                firstChart.Axes.Add(axis);
                secondChart.Series.Add(series);

                Verify.Throws<ArgumentException>(() => series.YAxis = axis, "Series should reject an axis from another chart.");
                Verify.IsNull(series.YAxis, "Rejected axis assignment should not stick.");
            });
        }

        [TestMethod]
        public void DataCollectionSupportsBasicListAndVectorSemantics()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var events = new List<VectorEvent>();
                var observable = (IObservableVector<Samples>)chart.Data;
                observable.VectorChanged += (sender, args) =>
                {
                    events.Add(new VectorEvent { Change = args.CollectionChange, Index = args.Index });
                };

                var first = SamplesFrom(new[] { 1.0, 2.0 });
                var second = SamplesFrom(new[] { 3.0, 4.0 });
                var replacement = SamplesFrom(new[] { 5.0, 6.0 });

                int eventCount = events.Count;
                chart.Data.Add(first);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                eventCount = events.Count;
                chart.Data.Insert(0, second);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemInserted, 0);

                Verify.AreEqual(2, chart.Data.Count, "Data count should update.");
                Verify.AreSame(second, chart.Data[0], "Data indexing should preserve order.");
                Verify.AreSame(first, chart.Data[1], "Data indexing should preserve order.");

                eventCount = events.Count;
                chart.Data[1] = replacement;
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemChanged, 1);
                Verify.AreSame(replacement, chart.Data[1], "Data replacement should use the new samples.");

                eventCount = events.Count;
                chart.Data.RemoveAt(0);
                VerifyNextVectorEvent(events, eventCount, CollectionChange.ItemRemoved, 0);

                eventCount = events.Count;
                chart.Data.Clear();
                VerifyNextVectorEvent(events, eventCount, CollectionChange.Reset, 0);
            });
        }

        [TestMethod]
        public void XamlMetadataUnknownTypesReturnNull()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();

                Verify.IsNull(provider.GetXamlType("Microsoft.UI.Xaml.Controls.Charts.NoSuchType"), "Unknown chart type should be null.");
                Verify.IsNull(provider.GetXamlType("System.SomeOtherNamespace.Foo"), "Unknown namespace should be null.");
                Verify.IsNull(provider.GetXamlType(LineSeriesTypeName).GetMember("NoSuchMember"), "Unknown member should be null.");
            });
        }

        [TestMethod]
        public void XamlMetadataSamplesActivatesAndItemsSourceRoundTrips()
        {
            RunOnUIThread.Execute(() =>
            {
                var type = CreateProvider().GetXamlType(SamplesTypeName);
                Verify.IsNotNull(type, "Samples type should resolve.");
                Verify.IsTrue(type.IsConstructible, "Samples should be constructible.");
                Verify.AreEqual(SamplesTypeName, type.FullName, "Samples full name should match.");

                var instance = type.ActivateInstance();
                var member = type.GetMember("ItemsSource");
                var values = new List<double> { 1.0, 2.0 };

                Verify.IsNotNull(member, "ItemsSource member should resolve.");
                Verify.IsTrue(member.IsDependencyProperty, "ItemsSource should be a dependency property.");
                Verify.IsFalse(member.IsReadOnly, "ItemsSource should be writable.");

                member.SetValue(instance, values);
                Verify.AreSame(values, member.GetValue(instance), "ItemsSource should round-trip.");
            });
        }

        [TestMethod]
        public void XamlMetadataChartActivatesAndAddsDataAxesAndSeries()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();
                var chartType = provider.GetXamlType(ChartTypeName);
                var chart = chartType.ActivateInstance();

                Verify.IsTrue(chartType.IsConstructible, "Chart should be constructible.");
                Verify.IsFalse(chartType.IsCollection, "Chart should not be a collection.");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.Control", chartType.BaseType.FullName, "Chart base type should be Control.");

                var dataMember = chartType.GetMember("Data");
                var axesMember = chartType.GetMember("Axes");
                var seriesMember = chartType.GetMember("Series");
                Verify.IsTrue(dataMember.IsReadOnly, "Data should be read-only.");
                Verify.IsTrue(axesMember.IsReadOnly, "Axes should be read-only.");
                Verify.IsTrue(seriesMember.IsReadOnly, "Series should be read-only.");

                var samples = provider.GetXamlType(SamplesTypeName).ActivateInstance();
                var dataVectorType = provider.GetXamlType("Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.Charts.Samples>");
                Verify.IsTrue(dataVectorType.IsCollection, "Data vector should be a collection.");
                Verify.AreEqual(SamplesTypeName, dataVectorType.ItemType.FullName, "Data vector item type should be Samples.");
                dataVectorType.AddToVector(dataMember.GetValue(chart), samples);

                var axis = provider.GetXamlType(LinearAxisTypeName).ActivateInstance();
                var axesVectorType = provider.GetXamlType("Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.Charts.Axis>");
                Verify.IsTrue(axesVectorType.IsCollection, "Axes vector should be a collection.");
                Verify.AreEqual(AxisTypeName, axesVectorType.ItemType.FullName, "Axes vector item type should be Axis.");
                axesVectorType.AddToVector(axesMember.GetValue(chart), axis);

                var line = provider.GetXamlType(LineSeriesTypeName).ActivateInstance();
                var seriesVectorType = provider.GetXamlType("Windows.Foundation.Collections.IObservableVector`1<Microsoft.UI.Xaml.Controls.Charts.CartesianSeries>");
                Verify.IsTrue(seriesVectorType.IsCollection, "Series vector should be a collection.");
                Verify.AreEqual(CartesianSeriesTypeName, seriesVectorType.ItemType.FullName, "Series vector item type should be CartesianSeries.");
                seriesVectorType.AddToVector(seriesMember.GetValue(chart), line);

                var typedChart = (Chart)chart;
                Verify.AreEqual(1, typedChart.Data.Count, "Data item should be added.");
                Verify.AreEqual(1, typedChart.Axes.Count, "Axis item should be added.");
                Verify.AreEqual(1, typedChart.Series.Count, "Series item should be added.");
            });
        }

        [TestMethod]
        public void XamlMetadataChartLegendDependencyPropertiesRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var type = CreateProvider().GetXamlType(ChartTypeName);
                var instance = type.ActivateInstance();
                var showLegend = type.GetMember("ShowLegend");
                var legendTitle = type.GetMember("LegendTitle");

                Verify.IsTrue(showLegend.IsDependencyProperty, "ShowLegend should be a dependency property.");
                Verify.IsTrue(legendTitle.IsDependencyProperty, "LegendTitle should be a dependency property.");
                Verify.IsFalse((bool)showLegend.GetValue(instance), "Default ShowLegend should be false.");
                Verify.AreEqual(string.Empty, (string)legendTitle.GetValue(instance), "Default LegendTitle should be empty.");

                showLegend.SetValue(instance, true);
                legendTitle.SetValue(instance, "Revenue");
                Verify.IsTrue((bool)showLegend.GetValue(instance), "ShowLegend should round-trip.");
                Verify.AreEqual("Revenue", (string)legendTitle.GetValue(instance), "LegendTitle should round-trip.");
            });
        }

        [TestMethod]
        public void XamlMetadataCartesianSeriesMembersAreOnBaseType()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();
                var type = provider.GetXamlType(CartesianSeriesTypeName);

                Verify.IsNotNull(type, "CartesianSeries type should resolve.");
                Verify.IsFalse(type.IsConstructible, "CartesianSeries should not be constructible.");
                Verify.IsNull(type.ActivateInstance(), "CartesianSeries activation should return null.");

                foreach (var memberName in new[]
                {
                    "Title", "IsVisible", "XValues", "YValues", "Stroke", "StrokeThickness",
                    "StrokeDashStyle", "ShowDataLabels", "ShowDataMarkers", "MarkerShape",
                    "DataLabelBrush", "DataMarkerBrush"
                })
                {
                    var member = type.GetMember(memberName);
                    Verify.IsNotNull(member, memberName + " should resolve.");
                    Verify.IsTrue(member.IsDependencyProperty, memberName + " should be a dependency property.");
                }

                // The axis references are plain properties: the public API has no XAxisProperty/YAxisProperty.
                foreach (var memberName in new[] { "XAxis", "YAxis" })
                {
                    var member = type.GetMember(memberName);
                    Verify.IsNotNull(member, memberName + " should resolve.");
                    Verify.IsFalse(member.IsDependencyProperty, memberName + " should not be a dependency property.");
                }

                Verify.IsNull(type.GetMember("DataLabelOverrides"), "Override maps should not be XAML members.");
                Verify.IsNull(type.GetMember("DataMarkerOverrides"), "Override maps should not be XAML members.");
            });
        }

        [TestMethod]
        public void XamlMetadataLineSeriesInheritedMembersRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();
                var lineType = provider.GetXamlType(LineSeriesTypeName);
                var instance = lineType.ActivateInstance();
                var baseType = lineType.BaseType;

                Verify.IsTrue(lineType.IsConstructible, "LineSeries should be constructible.");
                Verify.AreEqual(CartesianSeriesTypeName, baseType.FullName, "LineSeries base type should be CartesianSeries.");

                baseType.GetMember("Title").SetValue(instance, "Revenue");
                baseType.GetMember("IsVisible").SetValue(instance, false);
                var samples = new Samples();
                baseType.GetMember("XValues").SetValue(instance, samples);

                Verify.AreEqual("Revenue", (string)baseType.GetMember("Title").GetValue(instance), "Title should round-trip.");
                Verify.IsFalse((bool)baseType.GetMember("IsVisible").GetValue(instance), "IsVisible should round-trip.");
                Verify.AreSame(samples, baseType.GetMember("XValues").GetValue(instance), "XValues should round-trip.");

                var markerShape = baseType.GetMember("MarkerShape");
                var strokeDashStyle = baseType.GetMember("StrokeDashStyle");
                var strokeThickness = baseType.GetMember("StrokeThickness");
                var stroke = baseType.GetMember("Stroke");
                var labelBrush = baseType.GetMember("DataLabelBrush");
                var markerBrush = baseType.GetMember("DataMarkerBrush");

                Verify.AreEqual(MarkerShape.Circle, (MarkerShape)markerShape.GetValue(instance), "Default MarkerShape should be Circle.");
                Verify.AreEqual(StrokeDashStyle.Solid, (StrokeDashStyle)strokeDashStyle.GetValue(instance), "Default StrokeDashStyle should be Solid.");
                Verify.AreEqual(1.0, (double)strokeThickness.GetValue(instance), "Default StrokeThickness should be 1.");
                Verify.IsNull(stroke.GetValue(instance), "Default Stroke should be null.");
                Verify.IsNull(labelBrush.GetValue(instance), "Default DataLabelBrush should be null.");
                Verify.IsNull(markerBrush.GetValue(instance), "Default DataMarkerBrush should be null.");

                var brush = Brush(Colors.Red);
                markerShape.SetValue(instance, MarkerShape.Diamond);
                strokeDashStyle.SetValue(instance, StrokeDashStyle.DashDot);
                strokeThickness.SetValue(instance, 2.5);
                stroke.SetValue(instance, brush);
                labelBrush.SetValue(instance, brush);
                markerBrush.SetValue(instance, brush);

                Verify.AreEqual(MarkerShape.Diamond, (MarkerShape)markerShape.GetValue(instance), "MarkerShape should round-trip.");
                Verify.AreEqual(StrokeDashStyle.DashDot, (StrokeDashStyle)strokeDashStyle.GetValue(instance), "StrokeDashStyle should round-trip.");
                Verify.AreEqual(2.5, (double)strokeThickness.GetValue(instance), "StrokeThickness should round-trip.");
                Verify.AreSame(brush, stroke.GetValue(instance), "Stroke should round-trip.");
                Verify.AreSame(brush, labelBrush.GetValue(instance), "DataLabelBrush should round-trip.");
                Verify.AreSame(brush, markerBrush.GetValue(instance), "DataMarkerBrush should round-trip.");
            });
        }

        [TestMethod]
        public void XamlMetadataAreaAndBarSeriesActivateAndExposeFill()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();
                foreach (var typeName in new[] { AreaSeriesTypeName, BarSeriesTypeName })
                {
                    var type = provider.GetXamlType(typeName);
                    var instance = type.ActivateInstance();
                    var fill = type.GetMember("Fill");

                    Verify.IsTrue(type.IsConstructible, typeName + " should be constructible.");
                    Verify.AreEqual(CartesianSeriesTypeName, type.BaseType.FullName, typeName + " base type should be CartesianSeries.");
                    Verify.IsNotNull(fill, "Fill should resolve.");
                    Verify.IsTrue(fill.IsDependencyProperty, "Fill should be a dependency property.");
                    Verify.AreEqual("Microsoft.UI.Xaml.Media.Brush", fill.Type.FullName, "Fill type should be Brush.");
                    Verify.IsNull(fill.GetValue(instance), "Default Fill should be null.");

                    var brush = Brush(Colors.Green);
                    fill.SetValue(instance, brush);
                    Verify.AreSame(brush, fill.GetValue(instance), "Fill should round-trip.");
                }
            });
        }

        [TestMethod]
        public void XamlMetadataEnumTypesCreateFromString()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();

                Verify.AreEqual(BarOrientation.Vertical, (BarOrientation)provider.GetXamlType(BarOrientationTypeName).CreateFromString("Vertical"), "BarOrientation should parse.");
                Verify.AreEqual(MarkerShape.Circle, (MarkerShape)provider.GetXamlType(MarkerShapeTypeName).CreateFromString("Circle"), "MarkerShape should parse.");
                Verify.AreEqual(StrokeDashStyle.Dash, (StrokeDashStyle)provider.GetXamlType(StrokeDashStyleTypeName).CreateFromString("Dash"), "StrokeDashStyle should parse.");
                Verify.AreEqual(GridLines.Major, (GridLines)provider.GetXamlType(GridLinesTypeName).CreateFromString("Major"), "GridLines should parse.");
                Verify.AreEqual(SortOrder.Descending, (SortOrder)provider.GetXamlType(SortOrderTypeName).CreateFromString("Descending"), "SortOrder should parse.");
                Verify.AreEqual(DateTimeIntervalType.Month, (DateTimeIntervalType)provider.GetXamlType(DateTimeIntervalTypeName).CreateFromString("Month"), "DateTimeIntervalType should parse.");
                Verify.Throws<ArgumentException>(() => provider.GetXamlType(BarOrientationTypeName).CreateFromString("vertical"), "Enum parsing should be case-sensitive.");
            });
        }

        [TestMethod]
        public void XamlMetadataReportsUnderlyingTypes()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();

                var expected = new Dictionary<string, Type>
                {
                    { SamplesTypeName, typeof(Samples) },
                    { ChartTypeName, typeof(Chart) },
                    { LinearAxisTypeName, typeof(LinearAxis) },
                    { CategoryAxisTypeName, typeof(CategoryAxis) },
                    { DateTimeAxisTypeName, typeof(DateTimeAxis) },
                    { LineSeriesTypeName, typeof(LineSeries) },
                    { AreaSeriesTypeName, typeof(AreaSeries) },
                    { BarSeriesTypeName, typeof(BarSeries) },
                    { BarOrientationTypeName, typeof(BarOrientation) },
                    { ResourcesTypeName, typeof(XamlChartsResources) }
                };

                foreach (var pair in expected)
                {
                    Verify.AreEqual(pair.Value, provider.GetXamlType(pair.Key).UnderlyingType, pair.Key + " should map to its projected type.");
                }
            });
        }

        [TestMethod]
        public void XamlMetadataBaseTypeChainAndContentPropertiesAreReported()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();

                foreach (var typeName in new[] { LineSeriesTypeName, AreaSeriesTypeName, BarSeriesTypeName })
                {
                    var seriesType = provider.GetXamlType(typeName);
                    Verify.AreEqual(CartesianSeriesTypeName, seriesType.BaseType.FullName, typeName + " base type should be CartesianSeries.");
                    Verify.AreEqual("Microsoft.UI.Xaml.DependencyObject", seriesType.BaseType.BaseType.FullName, "Series base chain should end at DependencyObject.");
                }

                Verify.AreEqual("Microsoft.UI.Xaml.Controls.Control", provider.GetXamlType(ChartTypeName).BaseType.FullName, "Chart base type should be Control.");
                Verify.AreEqual("Microsoft.UI.Xaml.ResourceDictionary", provider.GetXamlType(ResourcesTypeName).BaseType.FullName, "Resources base type should be ResourceDictionary.");
                Verify.AreEqual("ItemsSource", provider.GetXamlType(SamplesTypeName).ContentProperty.Name, "Samples content property should be ItemsSource.");
                Verify.AreEqual("Series", provider.GetXamlType(ChartTypeName).ContentProperty.Name, "Chart content property should be Series.");
                Verify.IsNull(provider.GetXamlType(CartesianSeriesTypeName).ContentProperty, "CartesianSeries should not have a content property.");
            });
        }

        [TestMethod]
        public void XamlMetadataTypeOverloadAndXmlnsDefinitionsWork()
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = CreateProvider();
                var type = provider.GetXamlType(typeof(Samples));
                Verify.IsNotNull(type, "Type overload should resolve Samples.");
                Verify.AreEqual(SamplesTypeName, type.FullName, "Type overload should match the string overload.");

                var definitions = provider.GetXmlnsDefinitions();
                Verify.IsTrue(definitions == null || definitions.Length == 0, "Charts should not declare XML namespace mappings.");
            });
        }

        [TestMethod]
        public void XamlChartsResourcesMetadataResolvesAndConstructorSetsSource()
        {
            RunOnUIThread.Execute(() =>
            {
                var type = CreateProvider().GetXamlType(ResourcesTypeName);
                var resources = new XamlChartsResources();

                Verify.IsNotNull(type, "XamlChartsResources type should resolve.");
                Verify.IsTrue(type.IsConstructible, "XamlChartsResources should be constructible.");
                Verify.AreEqual(ResourcesTypeName, type.FullName, "XamlChartsResources full name should match.");
                Verify.IsNotNull(resources.Source, "XamlChartsResources should set Source.");
            });
        }

        [TestMethod]
        public void XamlReaderLoadsChartMarkupWithSeriesAxesSamplesAndEnums()
        {
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                chart = (Chart)XamlReader.Load(
                    @"<charts:Chart
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:charts='using:Microsoft.UI.Xaml.Controls.Charts'
                        Width='400'
                        Height='300'
                        ShowLegend='True'
                        LegendTitle='Revenue'>
                        <charts:Chart.Data>
                            <charts:Samples ItemsSource='chart-level' />
                        </charts:Chart.Data>
                        <charts:Chart.Axes>
                            <charts:LinearAxis Label='Value' />
                            <charts:CategoryAxis Label='Category' SortOrder='Descending' />
                            <charts:DateTimeAxis Label='Date' IntervalType='Month' />
                        </charts:Chart.Axes>
                        <charts:LineSeries Title='Line' StrokeDashStyle='Dash' MarkerShape='Circle'>
                            <charts:LineSeries.XValues>
                                <charts:Samples ItemsSource='Jan,Feb,Mar' />
                            </charts:LineSeries.XValues>
                            <charts:LineSeries.YValues>
                                <charts:Samples ItemsSource='1,2,3' />
                            </charts:LineSeries.YValues>
                        </charts:LineSeries>
                        <charts:AreaSeries Title='Area'>
                            <charts:AreaSeries.XValues>
                                <charts:Samples ItemsSource='Jan,Feb,Mar' />
                            </charts:AreaSeries.XValues>
                            <charts:AreaSeries.YValues>
                                <charts:Samples ItemsSource='3,2,1' />
                            </charts:AreaSeries.YValues>
                        </charts:AreaSeries>
                        <charts:BarSeries Title='Bar' Orientation='Vertical'>
                            <charts:BarSeries.XValues>
                                <charts:Samples ItemsSource='Jan,Feb,Mar' />
                            </charts:BarSeries.XValues>
                            <charts:BarSeries.YValues>
                                <charts:Samples ItemsSource='4,5,6' />
                            </charts:BarSeries.YValues>
                        </charts:BarSeries>
                    </charts:Chart>");

                Verify.IsTrue(chart.ShowLegend, "ShowLegend should load from markup.");
                Verify.AreEqual("Revenue", chart.LegendTitle, "LegendTitle should load from markup.");
                Verify.AreEqual(1, chart.Data.Count, "Chart.Data should load Samples.");
                Verify.AreEqual(3, chart.Axes.Count, "Chart.Axes should load axes.");
                Verify.AreEqual(3, chart.Series.Count, "Chart.Series should load series.");

                Verify.AreEqual(SortOrder.Descending, ((CategoryAxis)chart.Axes[1]).SortOrder, "SortOrder should load.");
                Verify.AreEqual(DateTimeIntervalType.Month, ((DateTimeAxis)chart.Axes[2]).IntervalType, "IntervalType should load.");
                Verify.AreEqual(StrokeDashStyle.Dash, chart.Series[0].StrokeDashStyle, "StrokeDashStyle should load.");
                Verify.AreEqual(MarkerShape.Circle, chart.Series[0].MarkerShape, "MarkerShape should load.");
                Verify.AreEqual(BarOrientation.Vertical, ((BarSeries)chart.Series[2]).Orientation, "Orientation should load.");
                Verify.IsNotNull(chart.Series[0].XValues, "LineSeries XValues should load.");
                Verify.IsNotNull(chart.Series[0].YValues, "LineSeries YValues should load.");

                Content = chart;
                Content.UpdateLayout();
                Verify.AreEqual(400.0, chart.ActualWidth, "Markup chart width should match Width.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Markup chart height should match Height.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(chart.IsLoaded, "Markup chart should be loaded.");
                Verify.AreEqual(400.0, chart.ActualWidth, "Loaded markup chart width should remain 400.");
                Verify.AreEqual(300.0, chart.ActualHeight, "Loaded markup chart height should remain 300.");
            });
        }

        [TestMethod]
        public void XamlReaderLoadsXamlChartsResourcesInMergedDictionaries()
        {
            RunOnUIThread.Execute(() =>
            {
                var dictionary = (ResourceDictionary)XamlReader.Load(
                    @"<ResourceDictionary
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:charts='using:Microsoft.UI.Xaml.Controls.Charts'>
                        <ResourceDictionary.MergedDictionaries>
                            <charts:XamlChartsResources />
                        </ResourceDictionary.MergedDictionaries>
                    </ResourceDictionary>");

                Verify.AreEqual(1, dictionary.MergedDictionaries.Count, "One merged dictionary should load.");
                Verify.IsTrue(dictionary.MergedDictionaries[0] is XamlChartsResources, "Merged dictionary should be XamlChartsResources.");
            });
        }
    }
}
