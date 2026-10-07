// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls.Charts;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ChartSamplesTests : ChartTestBase
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

        [TestMethod]
        public void SamplesItemsSourceRoundTripsNull()
        {
            RunOnUIThread.Execute(() =>
            {
                var samples = new Samples();

                Verify.IsNull(samples.ItemsSource, "Default ItemsSource should be null.");

                samples.ItemsSource = new List<double> { 1.0 };
                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should accept a source.");

                samples.ItemsSource = null;
                Verify.IsNull(samples.ItemsSource, "ItemsSource should clear to null.");
            });
        }

        [TestMethod]
        public void SamplesItemsSourceDependencyPropertyUpdatesAndClearsValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var samples = new Samples();
                var values = new List<double> { 1.0, 2.0 };

                samples.SetValue(Samples.ItemsSourceProperty, values);
                Verify.AreSame(values, samples.ItemsSource, "ItemsSourceProperty should update ItemsSource.");
                Verify.AreSame(values, samples.GetValue(Samples.ItemsSourceProperty), "ItemsSourceProperty should read the local value.");

                samples.ClearValue(Samples.ItemsSourceProperty);
                Verify.IsNull(samples.ItemsSource, "ClearValue should restore the null ItemsSource.");
            });
        }

        [TestMethod]
        public void NumericListItemsSourceCanLoad()
        {
            Chart chart = null;
            Samples samples = null;
            List<double> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<double> { 1.0, 2.5, -3.0 };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain the list instance.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void NumericArrayItemsSourceCanLoad()
        {
            Chart chart = null;
            Samples samples = null;

            RunOnUIThread.Execute(() =>
            {
                samples = new Samples { ItemsSource = new[] { 7.0, 8.0, 9.0 } };
                chart = CreateChartWithYValues(samples);

                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should retain an array source.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void StringListItemsSourceCanLoadAsXValues()
        {
            Chart chart = null;
            Samples samples = null;
            List<string> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<string> { "Jan", "Feb", "Mar" };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithXValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain the string list.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void DateTimeOffsetListItemsSourceCanLoadAsXValues()
        {
            Chart chart = null;
            Samples samples = null;
            List<DateTimeOffset> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<DateTimeOffset>
                {
                    new DateTimeOffset(2026, 1, 1, 0, 0, 0, TimeSpan.Zero),
                    new DateTimeOffset(2026, 1, 2, 0, 0, 0, TimeSpan.Zero)
                };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithXValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain the date list.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void BoxedNumericItemsSourceCanLoad()
        {
            Chart chart = null;
            Samples samples = null;
            List<object> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<object> { 4.0, 5.0 };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain boxed numeric values.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void BoxedMixedNumericItemsSourceCanLoad()
        {
            Chart chart = null;
            Samples samples = null;
            List<object> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<object>
                {
                    1.0,
                    null,
                    double.NaN,
                    new DateTimeOffset(2026, 1, 3, 0, 0, 0, TimeSpan.Zero),
                    "missing"
                };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain mixed boxed numeric values.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void BoxedStringItemsSourceCanLoadAsXValues()
        {
            Chart chart = null;
            Samples samples = null;
            List<object> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<object> { "x", "y" };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithXValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain boxed string values.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void EmptyBoxedItemsSourceCanLoadAsYValues()
        {
            Chart chart = null;
            Samples samples = null;
            List<object> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<object>();
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain the empty source.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void UnsupportedScalarItemsSourceIsRetainedAndDoesNotPreventLayout()
        {
            Chart chart = null;
            Samples samples = null;

            RunOnUIThread.Execute(() =>
            {
                samples = new Samples { ItemsSource = 42 };
                chart = CreateChartWithYValues(samples);

                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should retain unsupported scalar values.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void UnsupportedBoxedElementIsRetainedAndDoesNotPreventLayout()
        {
            Chart chart = null;
            Samples samples = null;
            List<object> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new List<object> { true };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should retain unsupported boxed elements.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void IntegerAndFloatSourcesAreRetainedWithoutSetValidation()
        {
            Chart chart = null;
            Samples integerSamples = null;
            Samples floatSamples = null;
            List<int> integers = null;
            List<float> floats = null;

            RunOnUIThread.Execute(() =>
            {
                integers = new List<int> { 1, 2, 3 };
                floats = new List<float> { 1.0f, 2.0f };
                integerSamples = new Samples { ItemsSource = integers };
                floatSamples = new Samples { ItemsSource = floats };
                chart = CreateChartWithTwoSeries(integerSamples, floatSamples);

                Verify.AreSame(integers, integerSamples.ItemsSource, "ItemsSource should retain integer lists.");
                Verify.AreSame(floats, floatSamples.ItemsSource, "ItemsSource should retain float lists.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(integers, integerSamples.ItemsSource, "Integer source should remain set after layout.");
                Verify.AreSame(floats, floatSamples.ItemsSource, "Float source should remain set after layout.");
            });
        }

        [TestMethod]
        public void IteratorSourceIsRetainedWithoutSetValidation()
        {
            Chart chart = null;
            Samples samples = null;

            RunOnUIThread.Execute(() =>
            {
                samples = new Samples { ItemsSource = YieldDoubles() };
                chart = CreateChartWithYValues(samples);

                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should retain iterator sources.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.IsNotNull(samples.ItemsSource, "ItemsSource should remain set after layout.");
            });
        }

        [TestMethod]
        public void XValuesAcceptStringsAndNumbers()
        {
            Chart chart = null;
            LineSeries series = null;
            Samples numericSamples = null;
            Samples stringSamples = null;

            RunOnUIThread.Execute(() =>
            {
                stringSamples = new Samples { ItemsSource = new List<string> { "A", "B" } };
                numericSamples = CreateSamples(1.0, 2.0);
                series = new LineSeries { XValues = stringSamples, YValues = CreateSamples(1.5, 2.5) };
                chart = CreateChartWithSeries(series);

                Verify.AreSame(stringSamples, series.XValues, "XValues should retain string samples.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                series.XValues = numericSamples;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(numericSamples, series.XValues, "XValues should retain numeric samples.");
            });
        }

        [TestMethod]
        public void StringYValuesAreRetainedButDoNotPreventLayout()
        {
            Chart chart = null;
            Samples samples = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                samples = new Samples { ItemsSource = new List<string> { "nope" } };
                series = new LineSeries { XValues = CreateSamples(0.0), YValues = samples };
                chart = CreateChartWithSeries(series);

                Verify.AreSame(samples, series.YValues, "YValues should retain string samples.");
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(samples, series.YValues, "YValues should remain set after layout.");
            });
        }

        [TestMethod]
        public void NullSamplesAndNullItemsSourceDoNotPreventLayout()
        {
            Chart chart = null;
            LineSeries series = null;
            Samples nullSourceSamples = null;

            RunOnUIThread.Execute(() =>
            {
                nullSourceSamples = new Samples { ItemsSource = null };
                series = new LineSeries();
                chart = CreateChartWithSeries(series);

                series.XValues = null;
                series.YValues = null;
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                series.XValues = nullSourceSamples;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(nullSourceSamples, series.XValues, "XValues should retain samples with null ItemsSource.");
                Verify.IsNull(series.YValues, "YValues should remain null.");
            });
        }

        [TestMethod]
        public void ClearingXValuesKeepsChartUsable()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                series = new LineSeries { XValues = CreateSamples(1.0, 2.0), YValues = CreateSamples(3.0, 4.0) };
                chart = CreateChartWithSeries(series);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                series.XValues = null;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.IsNull(series.XValues, "XValues should clear to null.");
                Verify.IsNotNull(series.YValues, "YValues should remain set.");
            });
        }

        [TestMethod]
        public void XValuesCanFlipBetweenNumericAndStringSources()
        {
            Chart chart = null;
            LineSeries series = null;
            Samples stringSamples = null;

            RunOnUIThread.Execute(() =>
            {
                var numericSamples = CreateSamples(1.0, 2.0);
                stringSamples = new Samples { ItemsSource = new List<string> { "a" } };
                series = new LineSeries { XValues = numericSamples, YValues = CreateSamples(3.0, 4.0) };
                chart = CreateChartWithSeries(series);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                series.XValues = stringSamples;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(stringSamples, series.XValues, "XValues should retain string samples after numeric samples.");
            });
        }

        [TestMethod]
        public void ReplacingSamplesLeavesOldSourceHarmless()
        {
            Chart chart = null;
            LineSeries series = null;
            ObservableCollection<double> oldSource = null;
            Samples oldSamples = null;
            ObservableCollection<double> newSource = null;
            Samples newSamples = null;

            RunOnUIThread.Execute(() =>
            {
                oldSource = new ObservableCollection<double> { 1.0 };
                oldSamples = new Samples { ItemsSource = oldSource };
                newSource = new ObservableCollection<double> { 2.0 };
                newSamples = new Samples { ItemsSource = newSource };
                series = new LineSeries { XValues = CreateSamples(0.0), YValues = oldSamples };
                chart = CreateChartWithSeries(series);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                series.YValues = newSamples;
                oldSource.Add(3.0);
                oldSamples.ItemsSource = new ObservableCollection<double> { 4.0 };
                newSource.Add(5.0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(newSamples, series.YValues, "YValues should keep the replacement samples.");
                Verify.AreSame(newSource, newSamples.ItemsSource, "Replacement samples should keep their source.");
            });
        }

        [TestMethod]
        public void SwappingItemsSourceLeavesOldSourceHarmless()
        {
            Chart chart = null;
            Samples samples = null;
            ObservableCollection<double> oldSource = null;
            ObservableCollection<double> newSource = null;

            RunOnUIThread.Execute(() =>
            {
                oldSource = new ObservableCollection<double> { 1.0 };
                newSource = new ObservableCollection<double> { 2.0 };
                samples = new Samples { ItemsSource = oldSource };
                chart = CreateChartWithYValues(samples);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                samples.ItemsSource = newSource;
                oldSource.Add(3.0);
                newSource.Add(4.0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(samples, chart.Series[0].YValues, "YValues should keep the same samples object.");
                Verify.AreSame(newSource, samples.ItemsSource, "Samples should keep the replacement source.");
            });
        }

        [TestMethod]
        public void NumericCollectionChangesWhileLoadedKeepChartUsable()
        {
            Chart chart = null;
            Samples samples = null;
            ObservableCollection<double> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new ObservableCollection<double> { 1.0, 2.0, 3.0 };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithYValues(samples);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Add(4.0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Insert(1, 1.5);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.RemoveAt(2);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source[0] = 99.0;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Clear();
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should keep the live numeric source.");
            });
        }

        [TestMethod]
        public void StringCategoryCollectionChangesWhileLoadedKeepChartUsable()
        {
            Chart chart = null;
            Samples samples = null;
            ObservableCollection<string> source = null;

            RunOnUIThread.Execute(() =>
            {
                source = new ObservableCollection<string> { "A", "B", "C" };
                samples = new Samples { ItemsSource = source };
                chart = CreateChartWithXValues(samples);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Add("D");
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Insert(1, "AA");
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.RemoveAt(2);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source[0] = "Z";
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                source.Clear();
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(source, samples.ItemsSource, "ItemsSource should keep the live string source.");
            });
        }

        [TestMethod]
        public void OneSamplesInstanceCanBeSharedByTwoSeries()
        {
            Chart chart = null;
            Samples sharedSamples = null;
            ObservableCollection<double> sharedSource = null;
            LineSeries first = null;
            LineSeries second = null;

            RunOnUIThread.Execute(() =>
            {
                sharedSource = new ObservableCollection<double> { 1.0, 2.0 };
                sharedSamples = new Samples { ItemsSource = sharedSource };
                first = new LineSeries { XValues = CreateSamples(0.0, 1.0), YValues = sharedSamples };
                second = new LineSeries { XValues = CreateSamples(0.0, 1.0), YValues = sharedSamples };
                chart = CreateEmptyChart();

                chart.Series.Add(first);
                chart.Series.Add(second);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                sharedSource.Add(3.0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(sharedSamples, first.YValues, "First series should keep the shared samples.");
                Verify.AreSame(sharedSamples, second.YValues, "Second series should keep the shared samples.");
                Verify.AreSame(sharedSource, sharedSamples.ItemsSource, "Shared samples should keep the shared source.");
            });
        }

        [TestMethod]
        public void SamplesCanBeUsedBySeriesAndChartSamplesCollection()
        {
            Chart chart = null;
            LineSeries series = null;
            Samples sharedSamples = null;

            RunOnUIThread.Execute(() =>
            {
                sharedSamples = new Samples
                {
                    ItemsSource = new ObservableCollection<double> { 0.0, 1.0 }
                };
                series = new LineSeries { XValues = sharedSamples, YValues = CreateSamples(1.0, 2.0) };
                chart = CreateChartWithSeries(series);

                chart.Data.Add(sharedSamples);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyChartSurvived(chart);
                Verify.AreSame(sharedSamples, series.XValues, "Series should keep the shared samples.");
                Verify.AreSame(sharedSamples, chart.Data[0], "Chart samples collection should keep the shared samples.");
            });
        }

        private static Samples CreateSamples(params double[] values)
        {
            return new Samples
            {
                ItemsSource = new ObservableCollection<double>(values)
            };
        }

        private static IEnumerable<double> YieldDoubles()
        {
            yield return 1.1;
            yield return 2.2;
            yield return 3.3;
        }

        private static Chart CreateEmptyChart()
        {
            return new Chart
            {
                Width = 400.0,
                Height = 300.0
            };
        }

        private static Chart CreateChartWithSeries(CartesianSeries series)
        {
            var chart = CreateEmptyChart();
            chart.Series.Add(series);
            return chart;
        }

        private static Chart CreateChartWithYValues(Samples yValues)
        {
            return CreateChartWithSeries(new LineSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0, 3.0),
                YValues = yValues
            });
        }

        private static Chart CreateChartWithXValues(Samples xValues)
        {
            return CreateChartWithSeries(new LineSeries
            {
                XValues = xValues,
                YValues = CreateSamples(1.0, 2.0, 3.0, 4.0)
            });
        }

        private static Chart CreateChartWithTwoSeries(Samples firstYValues, Samples secondYValues)
        {
            var chart = CreateEmptyChart();
            chart.Series.Add(new LineSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0),
                YValues = firstYValues
            });
            chart.Series.Add(new LineSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0),
                YValues = secondYValues
            });
            return chart;
        }

    }
}
