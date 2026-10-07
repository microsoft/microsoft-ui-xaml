// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using Windows.Foundation.Collections;
using Windows.UI;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ChartSeriesTests : ChartTestBase
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

        private sealed class MapChangeRecord
        {
            public MapChangeRecord(CollectionChange change, uint key)
            {
                Change = change;
                Key = key;
            }

            public CollectionChange Change { get; }
            public uint Key { get; }
        }

        [TestMethod]
        public void LineSeriesDefaultsArePublicProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();

                VerifyDefaultCartesianSeriesProperties(series);
                Verify.IsNull(series.Stroke, "Default Stroke should be null.");
            });
        }

        [TestMethod]
        public void AreaSeriesDefaultsArePublicProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new AreaSeries();

                VerifyDefaultCartesianSeriesProperties(series);
                Verify.IsNull(series.Stroke, "Default Stroke should be null.");
                Verify.IsNull(series.Fill, "Default Fill should be null.");
            });
        }

        [TestMethod]
        public void BarSeriesDefaultsArePublicProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new BarSeries();

                VerifyDefaultCartesianSeriesProperties(series);
                Verify.IsNull(series.Stroke, "Default Stroke should be null.");
                Verify.IsNull(series.Fill, "Default Fill should be null.");
                Verify.AreEqual(BarOrientation.Horizontal, series.Orientation, "Default Orientation should be Horizontal.");
            });
        }

        [TestMethod]
        public void CartesianSeriesPropertiesRoundTripThroughClrProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in CreateAllSeries())
                {
                    var xValues = CreateSamples(0.0, 1.0, 2.0);
                    var yValues = CreateSamples(2.0, 3.0, 4.0);
                    var stroke = CreateBrush(Colors.Red);
                    var labelBrush = CreateBrush(Colors.Blue);
                    var markerBrush = CreateBrush(Colors.Green);
                    // Every series type plots categories along XAxis and values along YAxis.
                    var xAxis = new CategoryAxis();
                    var yAxis = new LinearAxis();

                    series.Title = "Revenue";
                    series.IsVisible = false;
                    series.XValues = xValues;
                    series.YValues = yValues;
                    series.Stroke = stroke;
                    series.StrokeThickness = 2.5;
                    series.StrokeDashStyle = StrokeDashStyle.DashDot;
                    series.ShowDataLabels = true;
                    series.ShowDataMarkers = true;
                    series.MarkerShape = MarkerShape.Diamond;
                    series.DataLabelBrush = labelBrush;
                    series.DataMarkerBrush = markerBrush;
                    series.XAxis = xAxis;
                    series.YAxis = yAxis;

                    Verify.AreEqual("Revenue", series.Title, "Title should round-trip.");
                    Verify.IsFalse(series.IsVisible, "IsVisible should round-trip.");
                    Verify.AreSame(xValues, series.XValues, "XValues should round-trip.");
                    Verify.AreSame(yValues, series.YValues, "YValues should round-trip.");
                    Verify.AreSame(stroke, series.Stroke, "Stroke should round-trip.");
                    Verify.AreEqual(2.5, series.StrokeThickness, "StrokeThickness should round-trip.");
                    Verify.AreEqual(StrokeDashStyle.DashDot, series.StrokeDashStyle, "StrokeDashStyle should round-trip.");
                    Verify.IsTrue(series.ShowDataLabels, "ShowDataLabels should round-trip.");
                    Verify.IsTrue(series.ShowDataMarkers, "ShowDataMarkers should round-trip.");
                    Verify.AreEqual(MarkerShape.Diamond, series.MarkerShape, "MarkerShape should round-trip.");
                    Verify.AreSame(labelBrush, series.DataLabelBrush, "DataLabelBrush should round-trip.");
                    Verify.AreSame(markerBrush, series.DataMarkerBrush, "DataMarkerBrush should round-trip.");
                    Verify.AreSame(xAxis, series.XAxis, "XAxis should round-trip.");
                    Verify.AreSame(yAxis, series.YAxis, "YAxis should round-trip.");
                }
            });
        }

        [TestMethod]
        public void CartesianSeriesPropertiesRoundTripThroughDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in CreateAllSeries())
                {
                    var xValues = CreateSamples(1.0, 2.0);
                    var yValues = CreateSamples(3.0, 4.0);
                    var stroke = CreateBrush(Colors.Orange);
                    var labelBrush = CreateBrush(Colors.Purple);
                    var markerBrush = CreateBrush(Colors.Brown);

                    series.SetValue(CartesianSeries.TitleProperty, "Expenses");
                    series.SetValue(CartesianSeries.IsVisibleProperty, false);
                    series.SetValue(CartesianSeries.XValuesProperty, xValues);
                    series.SetValue(CartesianSeries.YValuesProperty, yValues);
                    series.SetValue(CartesianSeries.StrokeProperty, stroke);
                    series.SetValue(CartesianSeries.StrokeThicknessProperty, 3.25);
                    series.SetValue(CartesianSeries.StrokeDashStyleProperty, StrokeDashStyle.Dot);
                    series.SetValue(CartesianSeries.ShowDataLabelsProperty, true);
                    series.SetValue(CartesianSeries.ShowDataMarkersProperty, true);
                    series.SetValue(CartesianSeries.MarkerShapeProperty, MarkerShape.Plus);
                    series.SetValue(CartesianSeries.DataLabelBrushProperty, labelBrush);
                    series.SetValue(CartesianSeries.DataMarkerBrushProperty, markerBrush);

                    Verify.AreEqual("Expenses", series.Title, "Title should round-trip through its property identifier.");
                    Verify.IsFalse(series.IsVisible, "IsVisible should round-trip through its property identifier.");
                    Verify.AreSame(xValues, series.XValues, "XValues should round-trip through its property identifier.");
                    Verify.AreSame(yValues, series.YValues, "YValues should round-trip through its property identifier.");
                    Verify.AreSame(stroke, series.Stroke, "Stroke should round-trip through its property identifier.");
                    Verify.AreEqual(3.25, series.StrokeThickness, "StrokeThickness should round-trip through its property identifier.");
                    Verify.AreEqual(StrokeDashStyle.Dot, series.StrokeDashStyle, "StrokeDashStyle should round-trip through its property identifier.");
                    Verify.IsTrue(series.ShowDataLabels, "ShowDataLabels should round-trip through its property identifier.");
                    Verify.IsTrue(series.ShowDataMarkers, "ShowDataMarkers should round-trip through its property identifier.");
                    Verify.AreEqual(MarkerShape.Plus, series.MarkerShape, "MarkerShape should round-trip through its property identifier.");
                    Verify.AreSame(labelBrush, series.DataLabelBrush, "DataLabelBrush should round-trip through its property identifier.");
                    Verify.AreSame(markerBrush, series.DataMarkerBrush, "DataMarkerBrush should round-trip through its property identifier.");
                }
            });
        }

        [TestMethod]
        public void ClearValueRestoresCartesianSeriesDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in CreateAllSeries())
                {
                    series.Title = "Temporary";
                    series.IsVisible = false;
                    series.XValues = CreateSamples(1.0);
                    series.YValues = CreateSamples(2.0);
                    series.Stroke = CreateBrush(Colors.Red);
                    series.StrokeThickness = 5.0;
                    series.StrokeDashStyle = StrokeDashStyle.DashDotDot;
                    series.ShowDataLabels = true;
                    series.ShowDataMarkers = true;
                    series.MarkerShape = MarkerShape.Square;
                    series.DataLabelBrush = CreateBrush(Colors.Blue);
                    series.DataMarkerBrush = CreateBrush(Colors.Green);

                    series.ClearValue(CartesianSeries.TitleProperty);
                    series.ClearValue(CartesianSeries.IsVisibleProperty);
                    series.ClearValue(CartesianSeries.XValuesProperty);
                    series.ClearValue(CartesianSeries.YValuesProperty);
                    series.ClearValue(CartesianSeries.StrokeProperty);
                    series.ClearValue(CartesianSeries.StrokeThicknessProperty);
                    series.ClearValue(CartesianSeries.StrokeDashStyleProperty);
                    series.ClearValue(CartesianSeries.ShowDataLabelsProperty);
                    series.ClearValue(CartesianSeries.ShowDataMarkersProperty);
                    series.ClearValue(CartesianSeries.MarkerShapeProperty);
                    series.ClearValue(CartesianSeries.DataLabelBrushProperty);
                    series.ClearValue(CartesianSeries.DataMarkerBrushProperty);

                    VerifyDefaultCartesianSeriesProperties(series);
                    Verify.IsNull(series.Stroke, "Cleared Stroke should be null.");
                }
            });
        }

        [TestMethod]
        public void AreaSeriesFillRoundTripsAndClearValueRestoresDefault()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new AreaSeries();
                var brush = CreateBrush(Colors.Red);

                series.Fill = brush;
                Verify.AreSame(brush, series.Fill, "Fill should round-trip.");

                var replacement = CreateBrush(Colors.Blue);
                series.SetValue(AreaSeries.FillProperty, replacement);
                Verify.AreSame(replacement, series.Fill, "Fill should round-trip through its property identifier.");

                series.ClearValue(AreaSeries.FillProperty);
                Verify.IsNull(series.Fill, "Cleared Fill should be null.");
            });
        }

        [TestMethod]
        public void BarSeriesFillAndOrientationRoundTripAndClearValueRestoresDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new BarSeries();
                var brush = CreateBrush(Colors.Red);

                series.Fill = brush;
                series.Orientation = BarOrientation.Vertical;
                Verify.AreSame(brush, series.Fill, "Fill should round-trip.");
                Verify.AreEqual(BarOrientation.Vertical, series.Orientation, "Orientation should round-trip.");

                var replacement = CreateBrush(Colors.Blue);
                series.SetValue(BarSeries.FillProperty, replacement);
                series.SetValue(BarSeries.OrientationProperty, BarOrientation.Horizontal);
                Verify.AreSame(replacement, series.Fill, "Fill should round-trip through its property identifier.");
                Verify.AreEqual(BarOrientation.Horizontal, series.Orientation, "Orientation should round-trip through its property identifier.");

                series.ClearValue(BarSeries.FillProperty);
                series.ClearValue(BarSeries.OrientationProperty);
                Verify.IsNull(series.Fill, "Cleared Fill should be null.");
                Verify.AreEqual(BarOrientation.Horizontal, series.Orientation, "Cleared Orientation should be Horizontal.");
            });
        }

        [TestMethod]
        public void StrokeThicknessValuesAreRetainedByPublicProperty()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in CreateAllSeries())
                {
                    foreach (double value in new[] { 0.0, 0.25, 2.5, (double)float.MaxValue, -1.0, double.PositiveInfinity, double.MaxValue, double.NaN })
                    {
                        series.StrokeThickness = 2.0;
                        series.StrokeThickness = value;

                        if (double.IsNaN(value))
                        {
                            Verify.IsTrue(double.IsNaN(series.StrokeThickness), "StrokeThickness should retain NaN.");
                        }
                        else
                        {
                            Verify.AreEqual(value, series.StrokeThickness, "StrokeThickness should retain the assigned value.");
                        }
                    }
                }
            });
        }

        [TestMethod]
        public void StrokeThicknessInvalidValuesDoNotThrowWhileLoaded()
        {
            Chart chart = null;
            List<CartesianSeries> seriesList = null;

            RunOnUIThread.Execute(() =>
            {
                seriesList = new List<CartesianSeries>(CreateAllSeries());
                chart = CreateChartWithSeries(seriesList);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            foreach (double value in new[] { -1.0, double.PositiveInfinity, double.MaxValue, double.NaN })
            {
                RunOnUIThread.Execute(() =>
                {
                    foreach (var series in seriesList)
                    {
                        series.StrokeThickness = value;
                    }
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    foreach (var series in seriesList)
                    {
                        if (double.IsNaN(value))
                        {
                            Verify.IsTrue(double.IsNaN(series.StrokeThickness), "StrokeThickness should retain NaN while loaded.");
                        }
                        else
                        {
                            Verify.AreEqual(value, series.StrokeThickness, "StrokeThickness should retain the assigned value while loaded.");
                        }
                    }
                    VerifyChartSurvived(chart);
                });
            }
        }

        [TestMethod]
        public void StrokeDashStyleAcceptsEveryValue()
        {
            Chart chart = null;
            List<CartesianSeries> seriesList = null;

            RunOnUIThread.Execute(() =>
            {
                seriesList = new List<CartesianSeries>(CreateAllSeries());
                chart = CreateChartWithSeries(seriesList);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            foreach (var value in StrokeDashStyles())
            {
                RunOnUIThread.Execute(() =>
                {
                    foreach (var series in seriesList)
                    {
                        series.StrokeDashStyle = value;
                    }
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    foreach (var series in seriesList)
                    {
                        Verify.AreEqual(value, series.StrokeDashStyle, "StrokeDashStyle should retain the assigned enum value while loaded.");
                    }
                    VerifyChartSurvived(chart);
                });
            }
        }

        [TestMethod]
        public void UnsupportedStrokeBrushIsAcceptedAndRetained()
        {
            Chart chart = null;
            List<CartesianSeries> seriesList = null;
            var brushes = new List<Brush>();

            RunOnUIThread.Execute(() =>
            {
                seriesList = new List<CartesianSeries>(CreateAllSeries());
                chart = CreateChartWithSeries(seriesList);
                for (int index = 0; index < seriesList.Count; index++)
                {
                    var brush = new LinearGradientBrush();
                    brushes.Add(brush);
                    seriesList[index].Stroke = brush;
                }
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                for (int index = 0; index < seriesList.Count; index++)
                {
                    Verify.AreSame(brushes[index], seriesList[index].Stroke, "Unsupported Stroke brush should remain assigned.");
                }
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void AutomaticStrokeIsNotWrittenBackWhileLoaded()
        {
            Chart chart = null;
            List<CartesianSeries> seriesList = null;

            RunOnUIThread.Execute(() =>
            {
                seriesList = new List<CartesianSeries>(CreateAllSeries());
                chart = CreateChartWithSeries(seriesList);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in seriesList)
                {
                    Verify.IsNull(series.Stroke, "Automatic Stroke should not be written back to the public property.");
                }
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void StrokeCanChangeAndClearWhileLoaded()
        {
            Chart chart = null;
            List<CartesianSeries> seriesList = null;
            var brushes = new List<Brush>();

            RunOnUIThread.Execute(() =>
            {
                seriesList = new List<CartesianSeries>(CreateAllSeries());
                chart = CreateChartWithSeries(seriesList);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in seriesList)
                {
                    var brush = CreateBrush(Colors.Red);
                    brushes.Add(brush);
                    series.Stroke = brush;
                }
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                for (int index = 0; index < seriesList.Count; index++)
                {
                    Verify.AreSame(brushes[index], seriesList[index].Stroke, "Stroke should change while loaded.");
                }
                VerifyChartSurvived(chart);

                foreach (var series in seriesList)
                {
                    series.ClearValue(CartesianSeries.StrokeProperty);
                }
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                foreach (var series in seriesList)
                {
                    Verify.IsNull(series.Stroke, "Cleared Stroke should be null while loaded.");
                }
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void TitleAndVisibilityCanChangeBeforeAndAfterSeriesIsAddedToLoadedChart()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                chart = CreateEmptyChart();
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                series = new LineSeries
                {
                    Title = "Before",
                    IsVisible = false,
                    XValues = CreateSamples(0.0, 1.0),
                    YValues = CreateSamples(1.0, 2.0)
                };

                chart.Series.Add(series);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Before", series.Title, "Title should be retained after adding to a loaded chart.");
                Verify.IsFalse(series.IsVisible, "IsVisible should be retained after adding to a loaded chart.");
                VerifyChartSurvived(chart);

                series.Title = "After";
                series.IsVisible = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("After", series.Title, "Title should change while loaded.");
                Verify.IsTrue(series.IsVisible, "IsVisible should change while loaded.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void SeriesCanBeRemovedAndAddedAgain()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyLineSeries();
                series.Title = "Reusable";
                series.StrokeThickness = 3.5;
                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {

                chart.Series.Remove(series);
                chart.Series.Add(series);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, chart.Series.Count, "Series should be added again.");
                Verify.AreSame(series, chart.Series[0], "The same series instance should be in the collection.");
                Verify.AreEqual("Reusable", series.Title, "Title should survive removal and add.");
                Verify.AreEqual(3.5, series.StrokeThickness, "StrokeThickness should survive removal and add.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void AreaSeriesCanShareExplicitAxesWithLineSeries()
        {
            Chart chart = null;
            AreaSeries area = null;
            CategoryAxis xAxis = null;
            LinearAxis yAxis = null;
            RunOnUIThread.Execute(() =>
            {
                xAxis = new CategoryAxis();
                yAxis = new LinearAxis();
                var line = CreateReadyLineSeries();
                area = CreateReadyAreaSeries();
                line.XAxis = xAxis;
                line.YAxis = yAxis;
                area.XAxis = xAxis;
                area.YAxis = yAxis;

                chart = CreateEmptyChart();
                chart.Axes.Add(xAxis);
                chart.Axes.Add(yAxis);
                chart.Series.Add(line);
                chart.Series.Add(area);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, chart.Series.Count, "LineSeries and AreaSeries should load together.");
                Verify.AreSame(xAxis, area.XAxis, "AreaSeries XAxis should remain assigned.");
                Verify.AreSame(yAxis, area.YAxis, "AreaSeries YAxis should remain assigned.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void BarOrientationChangesWhileLoadedWithAutomaticAxes()
        {
            Chart chart = null;
            BarSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyBarSeries();
                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                series.Orientation = BarOrientation.Vertical;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(BarOrientation.Vertical, series.Orientation, "Orientation should change to Vertical while loaded.");
                VerifyChartSurvived(chart);

                series.Orientation = BarOrientation.Horizontal;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(BarOrientation.Horizontal, series.Orientation, "Orientation should change to Horizontal while loaded.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void BarOrientationIncompatibleWithExplicitAxesIsRetainedWithoutThrowing()
        {
            Chart chart = null;
            BarSeries bar = null;
            CategoryAxis xAxis = null;
            LinearAxis yAxis = null;

            RunOnUIThread.Execute(() =>
            {
                xAxis = new CategoryAxis();
                yAxis = new LinearAxis();
                var line = CreateReadyLineSeries();
                bar = CreateReadyBarSeries();
                line.XAxis = xAxis;
                line.YAxis = yAxis;
                bar.Orientation = BarOrientation.Vertical;
                bar.XAxis = xAxis;
                bar.YAxis = yAxis;

                chart = CreateEmptyChart();
                chart.Axes.Add(xAxis);
                chart.Axes.Add(yAxis);
                chart.Series.Add(line);
                chart.Series.Add(bar);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                bar.Orientation = BarOrientation.Horizontal;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(BarOrientation.Horizontal, bar.Orientation, "Public Orientation should retain the assigned value.");
                Verify.AreSame(xAxis, bar.XAxis, "XAxis should remain assigned.");
                Verify.AreSame(yAxis, bar.YAxis, "YAxis should remain assigned.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void MarkerAndLabelPropertiesCanChangeWhileLoaded()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyLineSeries();
                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                series.ShowDataLabels = true;
                series.ShowDataMarkers = true;
                series.MarkerShape = MarkerShape.Plus;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {

                Verify.IsTrue(series.ShowDataLabels, "ShowDataLabels should change while loaded.");
                Verify.IsTrue(series.ShowDataMarkers, "ShowDataMarkers should change while loaded.");
                Verify.AreEqual(MarkerShape.Plus, series.MarkerShape, "MarkerShape should change while loaded.");
                VerifyChartSurvived(chart);

                series.ShowDataLabels = false;
                series.ShowDataMarkers = false;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(series.ShowDataLabels, "ShowDataLabels should turn off while loaded.");
                Verify.IsFalse(series.ShowDataMarkers, "ShowDataMarkers should turn off while loaded.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void MarkerShapeAcceptsEveryValue()
        {
            Chart chart = null;
            LineSeries series = null;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyLineSeries();
                series.ShowDataMarkers = true;
                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            foreach (var value in MarkerShapes())
            {
                RunOnUIThread.Execute(() =>
                {
                    series.MarkerShape = value;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(value, series.MarkerShape, "MarkerShape should retain the assigned enum value while loaded.");
                    VerifyChartSurvived(chart);
                });
            }
        }

        [TestMethod]
        public void InvalidMarkerShapeIsRetainedByPublicProperty()
        {
            Chart chart = null;
            LineSeries series = null;
            var invalid = (MarkerShape)0xffff;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyLineSeries();
                series.ShowDataMarkers = true;
                series.MarkerShape = invalid;
                Verify.AreEqual(invalid, series.MarkerShape, "Invalid MarkerShape should be retained before load.");

                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                series.MarkerShape = MarkerShape.Circle;
                series.MarkerShape = invalid;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(invalid, series.MarkerShape, "Invalid MarkerShape should be retained while loaded.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void DataBrushesSetAndClearWhileLoaded()
        {
            Chart chart = null;
            LineSeries series = null;
            SolidColorBrush labelBrush = null;
            SolidColorBrush markerBrush = null;

            RunOnUIThread.Execute(() =>
            {
                series = CreateReadyLineSeries();
                series.ShowDataLabels = true;
                series.ShowDataMarkers = true;
                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                labelBrush = CreateBrush(Colors.Blue);
                markerBrush = CreateBrush(Colors.Green);
                series.DataLabelBrush = labelBrush;
                series.DataMarkerBrush = markerBrush;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(labelBrush, series.DataLabelBrush, "DataLabelBrush should change while loaded.");
                Verify.AreSame(markerBrush, series.DataMarkerBrush, "DataMarkerBrush should change while loaded.");
                VerifyChartSurvived(chart);

                series.ClearValue(CartesianSeries.DataLabelBrushProperty);
                series.ClearValue(CartesianSeries.DataMarkerBrushProperty);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(series.DataLabelBrush, "Cleared DataLabelBrush should be null while loaded.");
                Verify.IsNull(series.DataMarkerBrush, "Cleared DataMarkerBrush should be null while loaded.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void FillSetAndClearWhileLoaded()
        {
            Chart areaChart = null;
            Chart barChart = null;
            AreaSeries area = null;
            BarSeries bar = null;
            SolidColorBrush areaFill = null;
            SolidColorBrush barFill = null;

            RunOnUIThread.Execute(() =>
            {
                area = CreateReadyAreaSeries();
                areaChart = CreateChartWithSeries(area);
                LoadChart(areaChart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                areaFill = CreateBrush(Colors.Red);
                area.Fill = areaFill;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(areaFill, area.Fill, "AreaSeries Fill should change while loaded.");
                VerifyChartSurvived(areaChart);

                area.ClearValue(AreaSeries.FillProperty);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(area.Fill, "Cleared AreaSeries Fill should be null while loaded.");
                VerifyChartSurvived(areaChart);

                bar = CreateReadyBarSeries();
                barChart = CreateChartWithSeries(bar);
                LoadChart(barChart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                barFill = CreateBrush(Colors.Blue);
                bar.Fill = barFill;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(barFill, bar.Fill, "BarSeries Fill should change while loaded.");
                VerifyChartSurvived(barChart);

                bar.ClearValue(BarSeries.FillProperty);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(bar.Fill, "Cleared BarSeries Fill should be null while loaded.");
                VerifyChartSurvived(barChart);
            });
        }

        [TestMethod]
        public void DataLabelOverridesStartEmptyAndShareContents()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var overrides = series.DataLabelOverrides;
                var sameOverrides = series.DataLabelOverrides;

                Verify.IsNotNull(overrides, "DataLabelOverrides should exist.");
                Verify.AreEqual(0, overrides.Count, "DataLabelOverrides should start empty.");

                var label = new DataLabelOverride("Shared", null);
                overrides.Add(0, label);
                Verify.AreSame(label, sameOverrides[0], "DataLabelOverrides should share contents across property reads.");
            });
        }

        [TestMethod]
        public void DataMarkerOverridesStartEmptyAndShareContents()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var overrides = series.DataMarkerOverrides;
                var sameOverrides = series.DataMarkerOverrides;

                Verify.IsNotNull(overrides, "DataMarkerOverrides should exist.");
                Verify.AreEqual(0, overrides.Count, "DataMarkerOverrides should start empty.");

                var marker = new DataMarkerOverride(MarkerShape.Circle, null);
                overrides.Add(0, marker);
                Verify.AreSame(marker, sameOverrides[0], "DataMarkerOverrides should share contents across property reads.");
            });
        }

        [TestMethod]
        public void DataLabelOverrideMapSupportsAddUpdateRemoveAndClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var map = series.DataLabelOverrides;
                var first = new DataLabelOverride("First", CreateBrush(Colors.Red));
                var second = new DataLabelOverride("Second", CreateBrush(Colors.Blue));

                map.Add(1, first);
                Verify.AreEqual(1, map.Count, "DataLabelOverrides should contain the added item.");
                Verify.AreSame(first, map[1], "DataLabelOverrides should store the added item.");

                map[1] = second;
                Verify.AreSame(second, map[1], "DataLabelOverrides should store the updated item.");

                Verify.IsTrue(map.Remove(1), "DataLabelOverrides should remove an existing item.");
                Verify.AreEqual(0, map.Count, "DataLabelOverrides should be empty after remove.");

                map.Add(2, first);
                map.Add(3, second);
                map.Clear();
                Verify.AreEqual(0, map.Count, "DataLabelOverrides should be empty after clear.");
            });
        }

        [TestMethod]
        public void DataMarkerOverrideMapSupportsAddUpdateRemoveAndClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var map = series.DataMarkerOverrides;
                var first = new DataMarkerOverride(MarkerShape.Circle, CreateBrush(Colors.Red));
                var second = new DataMarkerOverride(MarkerShape.Diamond, CreateBrush(Colors.Blue));

                map.Add(1, first);
                Verify.AreEqual(1, map.Count, "DataMarkerOverrides should contain the added item.");
                Verify.AreSame(first, map[1], "DataMarkerOverrides should store the added item.");

                map[1] = second;
                Verify.AreSame(second, map[1], "DataMarkerOverrides should store the updated item.");

                Verify.IsTrue(map.Remove(1), "DataMarkerOverrides should remove an existing item.");
                Verify.AreEqual(0, map.Count, "DataMarkerOverrides should be empty after remove.");

                map.Add(2, first);
                map.Add(3, second);
                map.Clear();
                Verify.AreEqual(0, map.Count, "DataMarkerOverrides should be empty after clear.");
            });
        }

        [TestMethod]
        public void DataLabelOverrideMapRaisesEventsForAddUpdateRemoveAndClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var map = series.DataLabelOverrides;
                var observableMap = (IObservableMap<uint, DataLabelOverride>)map;
                var records = new List<MapChangeRecord>();
                observableMap.MapChanged += (sender, args) =>
                {
                    records.Add(new MapChangeRecord(args.CollectionChange, args.Key));
                };

                map.Add(4, new DataLabelOverride("First", null));
                map[4] = new DataLabelOverride("Second", null);
                Verify.AreEqual("Second", map[4].Text, "Update should replace the override.");
                map.Remove(4);
                map.Add(5, new DataLabelOverride("Third", null));
                map.Clear();

                // The event kind raised for a replacement comes from the platform map implementation, so
                // only the key is checked for that step.
                Verify.AreEqual(5, records.Count, "Each mutation should raise one event.");
                Verify.AreEqual(CollectionChange.ItemInserted, records[0].Change, "Add should raise ItemInserted.");
                Verify.AreEqual(4u, records[0].Key, "Add should report the changed key.");
                Log.Comment("Replacement raised " + records[1].Change);
                Verify.AreEqual(4u, records[1].Key, "Update should report the changed key.");
                Verify.AreEqual(CollectionChange.ItemRemoved, records[2].Change, "Remove should raise ItemRemoved.");
                Verify.AreEqual(4u, records[2].Key, "Remove should report the changed key.");
                Verify.AreEqual(CollectionChange.Reset, records[4].Change, "Clear should raise Reset.");
            });
        }

        [TestMethod]
        public void DataMarkerOverrideMapRaisesEventsForAddUpdateRemoveAndClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = new LineSeries();
                var map = series.DataMarkerOverrides;
                var observableMap = (IObservableMap<uint, DataMarkerOverride>)map;
                var records = new List<MapChangeRecord>();
                observableMap.MapChanged += (sender, args) =>
                {
                    records.Add(new MapChangeRecord(args.CollectionChange, args.Key));
                };

                map.Add(4, new DataMarkerOverride(MarkerShape.Circle, null));
                map[4] = new DataMarkerOverride(MarkerShape.Diamond, null);
                Verify.AreEqual(MarkerShape.Diamond, map[4].Shape, "Update should replace the override.");
                map.Remove(4);
                map.Add(5, new DataMarkerOverride(MarkerShape.Plus, null));
                map.Clear();

                // The event kind raised for a replacement comes from the platform map implementation, so
                // only the key is checked for that step.
                Verify.AreEqual(5, records.Count, "Each mutation should raise one event.");
                Verify.AreEqual(CollectionChange.ItemInserted, records[0].Change, "Add should raise ItemInserted.");
                Verify.AreEqual(4u, records[0].Key, "Add should report the changed key.");
                Log.Comment("Replacement raised " + records[1].Change);
                Verify.AreEqual(4u, records[1].Key, "Update should report the changed key.");
                Verify.AreEqual(CollectionChange.ItemRemoved, records[2].Change, "Remove should raise ItemRemoved.");
                Verify.AreEqual(4u, records[2].Key, "Remove should report the changed key.");
                Verify.AreEqual(CollectionChange.Reset, records[4].Change, "Clear should raise Reset.");
            });
        }

        [TestMethod]
        public void DataLabelOverrideDescriptorsRetainTextAndBrush()
        {
            RunOnUIThread.Execute(() =>
            {
                var brush = CreateBrush(Colors.Red);
                var label = new DataLabelOverride("Peak", brush);
                var empty = new DataLabelOverride(string.Empty, null);

                Verify.AreEqual("Peak", label.Text, "DataLabelOverride should retain text.");
                Verify.AreSame(brush, label.Brush, "DataLabelOverride should retain brush.");
                Verify.AreEqual(string.Empty, empty.Text, "DataLabelOverride should allow empty text.");
                Verify.IsNull(empty.Brush, "DataLabelOverride should allow null brush.");
            });
        }

        [TestMethod]
        public void DataMarkerOverrideDescriptorsRetainShapeAndBrush()
        {
            RunOnUIThread.Execute(() =>
            {
                var brush = CreateBrush(Colors.Blue);
                var marker = new DataMarkerOverride(MarkerShape.Diamond, brush);
                var noBrush = new DataMarkerOverride(MarkerShape.Plus, null);

                Verify.AreEqual(MarkerShape.Diamond, marker.Shape, "DataMarkerOverride should retain shape.");
                Verify.AreSame(brush, marker.Brush, "DataMarkerOverride should retain brush.");
                Verify.AreEqual(MarkerShape.Plus, noBrush.Shape, "DataMarkerOverride should retain shape with null brush.");
                Verify.IsNull(noBrush.Brush, "DataMarkerOverride should allow null brush.");
            });
        }

        [TestMethod]
        public void OverridesBeyondCurrentSamplesAreAcceptedAndSurviveGrowth()
        {
            Chart chart = null;
            ObservableCollection<double> values = null;
            LineSeries series = null;
            DataLabelOverride label = null;
            DataMarkerOverride marker = null;

            RunOnUIThread.Execute(() =>
            {
                values = new ObservableCollection<double> { 1.0, 2.0 };
                series = new LineSeries
                {
                    XValues = CreateSamples(0.0, 1.0),
                    YValues = new Samples { ItemsSource = values },
                    ShowDataLabels = true,
                    ShowDataMarkers = true
                };
                label = new DataLabelOverride("Future", null);
                marker = new DataMarkerOverride(MarkerShape.Plus, null);
                series.DataLabelOverrides.Add(3, label);
                series.DataMarkerOverrides.Add(3, marker);

                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(label, series.DataLabelOverrides[3], "Out-of-range label override should be stored.");
                Verify.AreSame(marker, series.DataMarkerOverrides[3], "Out-of-range marker override should be stored.");
                VerifyChartSurvived(chart);

                values.Add(3.0);
                values.Add(4.0);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(label, series.DataLabelOverrides[3], "Label override should survive sample growth.");
                Verify.AreSame(marker, series.DataMarkerOverrides[3], "Marker override should survive sample growth.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void OverridesSurviveSampleChangesAndSeriesRemoval()
        {
            Chart chart = null;
            ObservableCollection<double> values = null;
            Samples samples = null;
            LineSeries series = null;
            DataLabelOverride label = null;
            DataMarkerOverride marker = null;

            RunOnUIThread.Execute(() =>
            {
                values = new ObservableCollection<double> { 1.0, 2.0, 3.0 };
                samples = new Samples { ItemsSource = values };
                series = new LineSeries
                {
                    XValues = CreateSamples(0.0, 1.0, 2.0),
                    YValues = samples,
                    ShowDataLabels = true,
                    ShowDataMarkers = true
                };
                label = new DataLabelOverride("Index one", null);
                marker = new DataMarkerOverride(MarkerShape.Triangle, null);
                series.DataLabelOverrides.Add(1, label);
                series.DataMarkerOverrides.Add(1, marker);

                chart = CreateChartWithSeries(series);
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                values.Insert(0, 0.0);
                values.RemoveAt(0);
                samples.ItemsSource = new ObservableCollection<double> { 9.0, 8.0, 7.0 };
                chart.Series.Remove(series);
                chart.Series.Add(series);
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(label, series.DataLabelOverrides[1], "Label override should survive sample changes and removal.");
                Verify.AreSame(marker, series.DataMarkerOverrides[1], "Marker override should survive sample changes and removal.");
                VerifyChartSurvived(chart);
            });
        }

        [TestMethod]
        public void SameBrushInstanceCanBeSharedBySeriesAndOverrides()
        {
            Chart chart = null;
            SolidColorBrush sharedBrush = null;
            LineSeries first = null;
            LineSeries second = null;

            RunOnUIThread.Execute(() =>
            {
                sharedBrush = CreateBrush(Colors.Red);
                first = CreateReadyLineSeries();
                second = CreateReadyLineSeries();

                first.ShowDataLabels = true;
                first.ShowDataMarkers = true;
                first.Stroke = sharedBrush;
                first.DataLabelBrush = sharedBrush;
                first.DataMarkerBrush = sharedBrush;
                first.DataLabelOverrides.Add(0, new DataLabelOverride("Shared", sharedBrush));
                first.DataMarkerOverrides.Add(0, new DataMarkerOverride(MarkerShape.Diamond, sharedBrush));
                second.ShowDataLabels = true;
                second.ShowDataMarkers = true;
                second.Stroke = sharedBrush;
                second.DataLabelBrush = sharedBrush;
                second.DataMarkerBrush = sharedBrush;
                second.DataLabelOverrides.Add(0, new DataLabelOverride("Shared", sharedBrush));
                second.DataMarkerOverrides.Add(0, new DataMarkerOverride(MarkerShape.Circle, sharedBrush));

                chart = CreateChartWithSeries(new CartesianSeries[] { first, second });
                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(sharedBrush, first.Stroke, "First Stroke should use the shared brush.");
                Verify.AreSame(sharedBrush, first.DataLabelBrush, "First DataLabelBrush should use the shared brush.");
                Verify.AreSame(sharedBrush, first.DataMarkerBrush, "First DataMarkerBrush should use the shared brush.");
                Verify.AreSame(sharedBrush, first.DataLabelOverrides[0].Brush, "First label override should use the shared brush.");
                Verify.AreSame(sharedBrush, first.DataMarkerOverrides[0].Brush, "First marker override should use the shared brush.");
                Verify.AreSame(sharedBrush, second.Stroke, "Second Stroke should use the shared brush.");
                Verify.AreSame(sharedBrush, second.DataLabelBrush, "Second DataLabelBrush should use the shared brush.");
                Verify.AreSame(sharedBrush, second.DataMarkerBrush, "Second DataMarkerBrush should use the shared brush.");
                Verify.AreSame(sharedBrush, second.DataLabelOverrides[0].Brush, "Second label override should use the shared brush.");
                Verify.AreSame(sharedBrush, second.DataMarkerOverrides[0].Brush, "Second marker override should use the shared brush.");
                VerifyChartSurvived(chart);
            });
        }

        private static IList<CartesianSeries> CreateAllSeries()
        {
            return new List<CartesianSeries>
            {
                CreateReadyLineSeries(),
                CreateReadyAreaSeries(),
                CreateReadyBarSeries()
            };
        }

        private static IEnumerable<StrokeDashStyle> StrokeDashStyles()
        {
            yield return StrokeDashStyle.Solid;
            yield return StrokeDashStyle.Dash;
            yield return StrokeDashStyle.Dot;
            yield return StrokeDashStyle.DashDot;
            yield return StrokeDashStyle.DashDotDot;
        }

        private static IEnumerable<MarkerShape> MarkerShapes()
        {
            yield return MarkerShape.None;
            yield return MarkerShape.Square;
            yield return MarkerShape.Diamond;
            yield return MarkerShape.Triangle;
            yield return MarkerShape.X;
            yield return MarkerShape.Asterisk;
            yield return MarkerShape.ShortDash;
            yield return MarkerShape.LongDash;
            yield return MarkerShape.Circle;
            yield return MarkerShape.Plus;
        }

        private static Samples CreateSamples(params double[] values)
        {
            return new Samples
            {
                ItemsSource = new ObservableCollection<double>(values)
            };
        }

        private static SolidColorBrush CreateBrush(Color color)
        {
            return new SolidColorBrush(color);
        }

        private static LineSeries CreateReadyLineSeries()
        {
            return new LineSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0),
                YValues = CreateSamples(1.0, 3.0, 2.0)
            };
        }

        private static AreaSeries CreateReadyAreaSeries()
        {
            return new AreaSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0),
                YValues = CreateSamples(1.0, 3.0, 2.0)
            };
        }

        private static BarSeries CreateReadyBarSeries()
        {
            return new BarSeries
            {
                XValues = CreateSamples(0.0, 1.0, 2.0),
                YValues = CreateSamples(1.0, 3.0, 2.0)
            };
        }

        private static Chart CreateEmptyChart()
        {
            return new Chart
            {
                Width = 400,
                Height = 300
            };
        }

        private static Chart CreateChartWithSeries(CartesianSeries series)
        {
            var chart = CreateEmptyChart();
            chart.Series.Add(series);
            return chart;
        }

        private static Chart CreateChartWithSeries(IEnumerable<CartesianSeries> seriesList)
        {
            var chart = CreateEmptyChart();
            foreach (var series in seriesList)
            {
                chart.Series.Add(series);
            }
            return chart;
        }

        private static void VerifyDefaultCartesianSeriesProperties(CartesianSeries series)
        {
            Verify.AreEqual(string.Empty, series.Title, "Default Title should be empty.");
            Verify.IsTrue(series.IsVisible, "Default IsVisible should be true.");
            Verify.IsNull(series.XValues, "Default XValues should be null.");
            Verify.IsNull(series.YValues, "Default YValues should be null.");
            Verify.AreEqual(1.0, series.StrokeThickness, "Default StrokeThickness should be 1.");
            Verify.AreEqual(StrokeDashStyle.Solid, series.StrokeDashStyle, "Default StrokeDashStyle should be Solid.");
            Verify.IsFalse(series.ShowDataLabels, "Default ShowDataLabels should be false.");
            Verify.IsFalse(series.ShowDataMarkers, "Default ShowDataMarkers should be false.");
            Verify.AreEqual(MarkerShape.Circle, series.MarkerShape, "Default MarkerShape should be Circle.");
            Verify.IsNull(series.DataLabelBrush, "Default DataLabelBrush should be null.");
            Verify.IsNull(series.DataMarkerBrush, "Default DataMarkerBrush should be null.");
            Verify.IsNull(series.XAxis, "Default XAxis should be null.");
            Verify.IsNull(series.YAxis, "Default YAxis should be null.");
            Verify.IsNotNull(series.DataLabelOverrides, "DataLabelOverrides should exist.");
            Verify.IsNotNull(series.DataMarkerOverrides, "DataMarkerOverrides should exist.");
            Verify.AreEqual(0, series.DataLabelOverrides.Count, "DataLabelOverrides should start empty.");
            Verify.AreEqual(0, series.DataMarkerOverrides.Count, "DataMarkerOverrides should start empty.");
        }
    }
}
