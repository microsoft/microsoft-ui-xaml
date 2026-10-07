// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Charts;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ChartAxisTests : ChartTestBase
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
        public void LinearAxisDefaultsAreAutomatic()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                Verify.IsNull(axis.Minimum, "Default Minimum should be null.");
                Verify.IsNull(axis.Maximum, "Default Maximum should be null.");
                Verify.IsNull(axis.Spacing, "Default Spacing should be null.");
            });
        }

        [TestMethod]
        public void LinearAxisRejectsInvalidBoundsAndPreservesPreviousValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis
                {
                    Minimum = 0.0,
                    Maximum = 10.0
                };

                Verify.Throws<ArgumentException>(() => axis.Minimum = 11.0, "Minimum above Maximum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Minimum = 10.0, "Minimum equal to Maximum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Maximum = -1.0, "Maximum below Minimum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Maximum = 0.0, "Maximum equal to Minimum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Minimum = double.NaN, "Minimum NaN should throw.");
                Verify.Throws<ArgumentException>(() => axis.Minimum = double.PositiveInfinity, "Minimum infinity should throw.");
                Verify.Throws<ArgumentException>(() => axis.Maximum = double.NegativeInfinity, "Maximum infinity should throw.");

                Verify.AreEqual(0.0, axis.Minimum.Value, "Minimum should preserve its previous value.");
                Verify.AreEqual(10.0, axis.Maximum.Value, "Maximum should preserve its previous value.");
            });
        }

        [TestMethod]
        public void LinearAxisRejectsInvalidSpacingAndPreservesPreviousValue()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis
                {
                    Minimum = 0.0,
                    Maximum = 10.0,
                    Spacing = 5.0
                };

                Verify.Throws<ArgumentException>(() => axis.Spacing = -1.0, "Negative Spacing should throw.");
                Verify.Throws<ArgumentException>(() => axis.Spacing = 0.0, "Zero Spacing should throw.");
                Verify.Throws<ArgumentException>(() => axis.Spacing = double.NaN, "Spacing NaN should throw.");
                Verify.Throws<ArgumentException>(() => axis.Spacing = double.PositiveInfinity, "Spacing infinity should throw.");
                Verify.Throws<ArgumentException>(() => axis.Spacing = 11.0, "Spacing greater than the range should throw.");

                Verify.AreEqual(5.0, axis.Spacing.Value, "Spacing should preserve its previous value.");
            });
        }

        [TestMethod]
        public void LinearAxisNullClearsToAutomatic()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis
                {
                    Minimum = 1.0,
                    Maximum = 9.0,
                    Spacing = 2.0
                };

                axis.Minimum = null;
                axis.Maximum = null;
                axis.Spacing = null;

                Verify.IsNull(axis.Minimum, "Minimum null should select automatic bounds.");
                Verify.IsNull(axis.Maximum, "Maximum null should select automatic bounds.");
                Verify.IsNull(axis.Spacing, "Spacing null should select automatic tick spacing.");
            });
        }

        [TestMethod]
        public void LinearAxisPropertiesCanChangeWhileLoaded()
        {
            LinearAxis yAxis = null;
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                yAxis = new LinearAxis();
                chart = CreateChart(CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 }), null, yAxis);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                yAxis.Minimum = 0.0;
                yAxis.Maximum = 50.0;
                yAxis.Spacing = 10.0;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0.0, yAxis.Minimum.Value, "Minimum should update while loaded.");
                Verify.AreEqual(50.0, yAxis.Maximum.Value, "Maximum should update while loaded.");
                Verify.AreEqual(10.0, yAxis.Spacing.Value, "Spacing should update while loaded.");
                VerifyChartRemainsLoaded(chart);
            });
        }

        [TestMethod]
        public void CategoryAxisDefaultsAreIndexAscending()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new CategoryAxis();

                Verify.AreEqual(CategorySortKey.Index, axis.SortKey, "Default SortKey should be Index.");
                Verify.AreEqual(SortOrder.Ascending, axis.SortOrder, "Default SortOrder should be Ascending.");
            });
        }

        [TestMethod]
        public void CategoryAxisSortPropertiesRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new CategoryAxis();

                axis.SortKey = CategorySortKey.Value;
                axis.SortOrder = SortOrder.Descending;

                Verify.AreEqual(CategorySortKey.Value, axis.SortKey, "SortKey should round-trip.");
                Verify.AreEqual(SortOrder.Descending, axis.SortOrder, "SortOrder should round-trip.");
            });
        }

        [TestMethod]
        public void CategoryAxisSortPropertiesWorkThroughDependencyProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new CategoryAxis();

                axis.SetValue(CategoryAxis.SortKeyProperty, CategorySortKey.Value);
                axis.SetValue(CategoryAxis.SortOrderProperty, SortOrder.Descending);

                Verify.AreEqual(CategorySortKey.Value, axis.SortKey, "SortKey should read SetValue.");
                Verify.AreEqual(SortOrder.Descending, axis.SortOrder, "SortOrder should read SetValue.");
                Verify.AreEqual(CategorySortKey.Value, (CategorySortKey)axis.GetValue(CategoryAxis.SortKeyProperty), "SortKeyProperty should read the local value.");
                Verify.AreEqual(SortOrder.Descending, (SortOrder)axis.GetValue(CategoryAxis.SortOrderProperty), "SortOrderProperty should read the local value.");

                axis.ClearValue(CategoryAxis.SortKeyProperty);
                axis.ClearValue(CategoryAxis.SortOrderProperty);

                Verify.AreEqual(CategorySortKey.Index, axis.SortKey, "ClearValue should restore SortKey default.");
                Verify.AreEqual(SortOrder.Ascending, axis.SortOrder, "ClearValue should restore SortOrder default.");
            });
        }

        [TestMethod]
        public void CategoryAxisSortDependencyPropertiesUpdateAndClearValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new CategoryAxis();

                axis.SetValue(CategoryAxis.SortKeyProperty, CategorySortKey.Value);
                axis.SetValue(CategoryAxis.SortOrderProperty, SortOrder.Descending);
                Verify.AreEqual(CategorySortKey.Value, axis.SortKey, "SortKeyProperty should update SortKey.");
                Verify.AreEqual(SortOrder.Descending, axis.SortOrder, "SortOrderProperty should update SortOrder.");
                Verify.AreEqual(CategorySortKey.Value, (CategorySortKey)axis.GetValue(CategoryAxis.SortKeyProperty), "SortKeyProperty should read the local value.");
                Verify.AreEqual(SortOrder.Descending, (SortOrder)axis.GetValue(CategoryAxis.SortOrderProperty), "SortOrderProperty should read the local value.");

                axis.ClearValue(CategoryAxis.SortKeyProperty);
                axis.ClearValue(CategoryAxis.SortOrderProperty);
                Verify.AreEqual(CategorySortKey.Index, axis.SortKey, "ClearValue should restore SortKey default.");
                Verify.AreEqual(SortOrder.Ascending, axis.SortOrder, "ClearValue should restore SortOrder default.");
            });
        }

        [TestMethod]
        public void AxisBaseDefaultsAndRoundTrips()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                Verify.IsTrue(axis.IsVisible, "Default IsVisible should be true.");
                Verify.AreEqual(string.Empty, axis.Label, "Default Label should be empty.");

                axis.IsVisible = false;
                axis.Label = "Revenue";

                Verify.IsFalse(axis.IsVisible, "IsVisible should round-trip false.");
                Verify.AreEqual("Revenue", axis.Label, "Label should round-trip.");

                axis.IsVisible = true;
                axis.Label = null;

                Verify.IsTrue(axis.IsVisible, "IsVisible should round-trip true.");
                Verify.AreEqual(string.Empty, axis.Label, "Null Label should read as empty.");
            });
        }

        [TestMethod]
        public void AxisBaseDependencyPropertiesUpdateAndClearValues()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (Axis axis in new Axis[] { new LinearAxis(), new CategoryAxis(), new DateTimeAxis() })
                {
                    string name = axis.GetType().Name;

                    axis.SetValue(Axis.IsVisibleProperty, false);
                    axis.SetValue(Axis.LabelProperty, "Revenue");
                    Verify.IsFalse(axis.IsVisible, name + ": IsVisibleProperty should update IsVisible.");
                    Verify.AreEqual("Revenue", axis.Label, name + ": LabelProperty should update Label.");
                    Verify.IsFalse((bool)axis.GetValue(Axis.IsVisibleProperty), name + ": IsVisibleProperty should read the local value.");
                    Verify.AreEqual("Revenue", (string)axis.GetValue(Axis.LabelProperty), name + ": LabelProperty should read the local value.");

                    axis.ClearValue(Axis.IsVisibleProperty);
                    axis.ClearValue(Axis.LabelProperty);
                    Verify.IsTrue(axis.IsVisible, name + ": ClearValue should restore IsVisible default.");
                    Verify.AreEqual(string.Empty, axis.Label, name + ": ClearValue should restore Label default.");
                }
            });
        }

        [TestMethod]
        public void AxisVisibilityCanChangeWhileLoaded()
        {
            LinearAxis yAxis = null;
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                yAxis = new LinearAxis();
                chart = CreateChart(CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 }), null, yAxis);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                yAxis.IsVisible = false;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(yAxis.IsVisible, "IsVisible should update to false while loaded.");
                VerifyChartRemainsLoaded(chart);
            });

            RunOnUIThread.Execute(() =>
            {
                yAxis.IsVisible = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(yAxis.IsVisible, "IsVisible should update to true while loaded.");
                VerifyChartRemainsLoaded(chart);
            });
        }

        [TestMethod]
        public void CartesianAxisPresentationDefaultsAreExpected()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                Verify.IsTrue(axis.ShowTickLabels, "Default ShowTickLabels should be true.");
                Verify.IsFalse(axis.ShowTickMarks, "Default ShowTickMarks should be false.");
                Verify.AreEqual(GridLines.None, axis.GridLines, "Default GridLines should be None.");
            });
        }

        [TestMethod]
        public void CartesianAxisPresentationPropertiesRoundTrip()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                axis.ShowTickLabels = false;
                axis.ShowTickMarks = true;
                axis.GridLines = GridLines.Major;

                Verify.IsFalse(axis.ShowTickLabels, "ShowTickLabels should round-trip.");
                Verify.IsTrue(axis.ShowTickMarks, "ShowTickMarks should round-trip.");
                Verify.AreEqual(GridLines.Major, axis.GridLines, "GridLines should round-trip Major.");

                axis.GridLines = GridLines.Minor;

                Verify.AreEqual(GridLines.Minor, axis.GridLines, "GridLines should round-trip Minor.");
            });
        }

        [TestMethod]
        public void CartesianAxisPresentationDependencyPropertiesUpdateAndClearValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                axis.SetValue(CartesianAxis.ShowTickLabelsProperty, false);
                axis.SetValue(CartesianAxis.ShowTickMarksProperty, true);
                axis.SetValue(CartesianAxis.GridLinesProperty, GridLines.Major);
                Verify.IsFalse(axis.ShowTickLabels, "ShowTickLabelsProperty should update ShowTickLabels.");
                Verify.IsTrue(axis.ShowTickMarks, "ShowTickMarksProperty should update ShowTickMarks.");
                Verify.AreEqual(GridLines.Major, axis.GridLines, "GridLinesProperty should update GridLines.");
                Verify.IsFalse((bool)axis.GetValue(CartesianAxis.ShowTickLabelsProperty), "ShowTickLabelsProperty should read the local value.");
                Verify.IsTrue((bool)axis.GetValue(CartesianAxis.ShowTickMarksProperty), "ShowTickMarksProperty should read the local value.");
                Verify.AreEqual(GridLines.Major, (GridLines)axis.GetValue(CartesianAxis.GridLinesProperty), "GridLinesProperty should read the local value.");

                axis.ClearValue(CartesianAxis.ShowTickLabelsProperty);
                axis.ClearValue(CartesianAxis.ShowTickMarksProperty);
                axis.ClearValue(CartesianAxis.GridLinesProperty);
                Verify.IsTrue(axis.ShowTickLabels, "ClearValue should restore ShowTickLabels default.");
                Verify.IsFalse(axis.ShowTickMarks, "ClearValue should restore ShowTickMarks default.");
                Verify.AreEqual(GridLines.None, axis.GridLines, "ClearValue should restore GridLines default.");
            });
        }

        [TestMethod]
        public void CartesianAxisBrushDefaultsAreNull()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();

                Verify.IsNull(axis.GridLineMajorBrush, "Default GridLineMajorBrush should be null.");
                Verify.IsNull(axis.GridLineMinorBrush, "Default GridLineMinorBrush should be null.");
                Verify.IsNull(axis.TickBrush, "Default TickBrush should be null.");
                Verify.IsNull(axis.TickLabelBrush, "Default TickLabelBrush should be null.");
                Verify.IsNull(axis.AxisLineBrush, "Default AxisLineBrush should be null.");
            });
        }

        [TestMethod]
        public void CartesianAxisBrushesRoundTripAndClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();
                var major = MakeBrush(0x11, 0x22, 0x33);
                var minor = MakeBrush(0x22, 0x33, 0x44);
                var tick = MakeBrush(0x33, 0x44, 0x55);
                var label = MakeBrush(0x44, 0x55, 0x66);
                var line = MakeBrush(0x55, 0x66, 0x77);

                axis.GridLineMajorBrush = major;
                axis.GridLineMinorBrush = minor;
                axis.TickBrush = tick;
                axis.TickLabelBrush = label;
                axis.AxisLineBrush = line;

                Verify.AreSame(major, axis.GridLineMajorBrush, "GridLineMajorBrush should round-trip.");
                Verify.AreSame(minor, axis.GridLineMinorBrush, "GridLineMinorBrush should round-trip.");
                Verify.AreSame(tick, axis.TickBrush, "TickBrush should round-trip.");
                Verify.AreSame(label, axis.TickLabelBrush, "TickLabelBrush should round-trip.");
                Verify.AreSame(line, axis.AxisLineBrush, "AxisLineBrush should round-trip.");

                axis.GridLineMajorBrush = null;
                axis.GridLineMinorBrush = null;
                axis.TickBrush = null;
                axis.TickLabelBrush = null;
                axis.AxisLineBrush = null;

                Verify.IsNull(axis.GridLineMajorBrush, "GridLineMajorBrush null should clear.");
                Verify.IsNull(axis.GridLineMinorBrush, "GridLineMinorBrush null should clear.");
                Verify.IsNull(axis.TickBrush, "TickBrush null should clear.");
                Verify.IsNull(axis.TickLabelBrush, "TickLabelBrush null should clear.");
                Verify.IsNull(axis.AxisLineBrush, "AxisLineBrush null should clear.");
            });
        }

        [TestMethod]
        public void CartesianAxisBrushDependencyPropertiesUpdateAndClearValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new LinearAxis();
                var major = MakeBrush(0x11, 0x22, 0x33);
                var minor = MakeBrush(0x22, 0x33, 0x44);
                var tick = MakeBrush(0x33, 0x44, 0x55);
                var label = MakeBrush(0x44, 0x55, 0x66);
                var line = MakeBrush(0x55, 0x66, 0x77);

                axis.SetValue(CartesianAxis.GridLineMajorBrushProperty, major);
                axis.SetValue(CartesianAxis.GridLineMinorBrushProperty, minor);
                axis.SetValue(CartesianAxis.TickBrushProperty, tick);
                axis.SetValue(CartesianAxis.TickLabelBrushProperty, label);
                axis.SetValue(CartesianAxis.AxisLineBrushProperty, line);
                Verify.AreSame(major, axis.GridLineMajorBrush, "GridLineMajorBrushProperty should update GridLineMajorBrush.");
                Verify.AreSame(minor, axis.GridLineMinorBrush, "GridLineMinorBrushProperty should update GridLineMinorBrush.");
                Verify.AreSame(tick, axis.TickBrush, "TickBrushProperty should update TickBrush.");
                Verify.AreSame(label, axis.TickLabelBrush, "TickLabelBrushProperty should update TickLabelBrush.");
                Verify.AreSame(line, axis.AxisLineBrush, "AxisLineBrushProperty should update AxisLineBrush.");
                Verify.AreSame(major, axis.GetValue(CartesianAxis.GridLineMajorBrushProperty), "GridLineMajorBrushProperty should read the local value.");
                Verify.AreSame(minor, axis.GetValue(CartesianAxis.GridLineMinorBrushProperty), "GridLineMinorBrushProperty should read the local value.");
                Verify.AreSame(tick, axis.GetValue(CartesianAxis.TickBrushProperty), "TickBrushProperty should read the local value.");
                Verify.AreSame(label, axis.GetValue(CartesianAxis.TickLabelBrushProperty), "TickLabelBrushProperty should read the local value.");
                Verify.AreSame(line, axis.GetValue(CartesianAxis.AxisLineBrushProperty), "AxisLineBrushProperty should read the local value.");

                axis.ClearValue(CartesianAxis.GridLineMajorBrushProperty);
                axis.ClearValue(CartesianAxis.GridLineMinorBrushProperty);
                axis.ClearValue(CartesianAxis.TickBrushProperty);
                axis.ClearValue(CartesianAxis.TickLabelBrushProperty);
                axis.ClearValue(CartesianAxis.AxisLineBrushProperty);
                Verify.IsNull(axis.GridLineMajorBrush, "ClearValue should restore GridLineMajorBrush default.");
                Verify.IsNull(axis.GridLineMinorBrush, "ClearValue should restore GridLineMinorBrush default.");
                Verify.IsNull(axis.TickBrush, "ClearValue should restore TickBrush default.");
                Verify.IsNull(axis.TickLabelBrush, "ClearValue should restore TickLabelBrush default.");
                Verify.IsNull(axis.AxisLineBrush, "ClearValue should restore AxisLineBrush default.");
            });
        }

        [TestMethod]
        public void AxisResourceOverridesAndThemeChangesDoNotChangeLocalBrushValues()
        {
            LinearAxis axis = null;
            Chart chart = null;
            SolidColorBrush explicitBrush = null;

            RunOnUIThread.Execute(() =>
            {
                axis = new LinearAxis
                {
                    ShowTickMarks = true,
                    GridLines = GridLines.Major
                };
                explicitBrush = MakeBrush(0xAA, 0xBB, 0xCC);
                axis.TickLabelBrush = explicitBrush;
                chart = CreateChart(CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 }), null, axis);
                VerifyAxisBrushResourceKeysExist();
                AddAxisBrushResources(chart.Resources);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                chart.RequestedTheme = ElementTheme.Light;
                Content.UpdateLayout();
                chart.RequestedTheme = ElementTheme.Dark;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(explicitBrush, axis.TickLabelBrush, "Explicit tick-label brush should stay set.");
                VerifyChartRemainsLoaded(chart);
            });

            RunOnUIThread.Execute(() =>
            {
                axis.TickLabelBrush = null;
                chart.RequestedTheme = ElementTheme.Default;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(axis.GridLineMajorBrush, "Unset major grid brush should stay null.");
                Verify.IsNull(axis.GridLineMinorBrush, "Unset minor grid brush should stay null.");
                Verify.IsNull(axis.TickBrush, "Unset tick brush should stay null.");
                Verify.IsNull(axis.TickLabelBrush, "Cleared tick-label brush should stay null.");
                Verify.IsNull(axis.AxisLineBrush, "Unset axis-line brush should stay null.");
                VerifyChartRemainsLoaded(chart);
            });
        }

        [TestMethod]
        public void AppLevelAxisResourceOverridesKeepLocalBrushPropertiesUnset()
        {
            ResourceDictionary appResources = null;
            ResourceDictionary overrides = null;

            RunOnUIThread.Execute(() =>
            {
                VerifyAxisBrushResourceKeysExist();
                appResources = Application.Current.Resources;
                overrides = CreateThemedAxisResources();
                appResources.MergedDictionaries.Add(overrides);
            });

            try
            {
                foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark, ElementTheme.Default })
                {
                    LinearAxis axis = null;
                    Chart chart = null;

                    RunOnUIThread.Execute(() =>
                    {
                        axis = new LinearAxis();
                        chart = CreateChart(CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 }), null, axis);
                        chart.RequestedTheme = theme;

                        LoadChart(chart);
                    });
                    IdleSynchronizer.Wait();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsNull(axis.TickLabelBrush, "Resource fallback should not set a local brush.");
                        VerifyChartRemainsLoaded(chart);
                    });
                }
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    appResources.MergedDictionaries.Remove(overrides);
                });
            }
        }

        [TestMethod]
        public void LineSeriesAcceptsCompatibleExplicitAxes()
        {
            CategoryAxis categoryAxis = null;
            DateTimeAxis dateAxis = null;
            LinearAxis linearAxis = null;
            Chart categoryChart = null;
            Chart dateChart = null;

            RunOnUIThread.Execute(() =>
            {
                categoryAxis = new CategoryAxis();
                linearAxis = new LinearAxis();

                categoryChart = CreateChart(CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 }), categoryAxis, linearAxis);
                LoadChart(categoryChart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(categoryAxis, categoryChart.Series[0].XAxis, "CategoryAxis should be accepted for X values.");
                Verify.AreSame(linearAxis, categoryChart.Series[0].YAxis, "LinearAxis should be accepted for Y values.");
                VerifyChartRemainsLoaded(categoryChart);
            });

            RunOnUIThread.Execute(() =>
            {
                dateAxis = new DateTimeAxis();
                dateChart = CreateChart(CreateLineSeries(new[] { Date(2020, 1, 1), Date(2021, 1, 1) }, new[] { 1.0, 2.0 }), dateAxis, new LinearAxis());
                LoadChart(dateChart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(dateAxis, dateChart.Series[0].XAxis, "DateTimeAxis should be accepted for date X values.");
                VerifyChartRemainsLoaded(dateChart);
            });
        }

        [TestMethod]
        public void LineSeriesRejectsIncompatibleExplicitAxesBeforeLoad()
        {
            RunOnUIThread.Execute(() =>
            {
                var series = CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 });

                Verify.Throws<ArgumentException>(() => series.XAxis = new LinearAxis(), "LinearAxis should be rejected for X values.");
                Verify.IsNull(series.XAxis, "Rejected XAxis should leave the previous value intact.");

                Verify.Throws<ArgumentException>(() => series.YAxis = new CategoryAxis(), "CategoryAxis should be rejected for Y values.");
                Verify.IsNull(series.YAxis, "Rejected YAxis should leave the previous value intact.");

                Verify.Throws<ArgumentException>(() => series.YAxis = new DateTimeAxis(), "DateTimeAxis should be rejected for Y values.");
                Verify.IsNull(series.YAxis, "Rejected YAxis should leave the previous value intact.");
            });
        }

        [TestMethod]
        public void LineSeriesPreservesAxisAfterRejectedReplacement()
        {
            RunOnUIThread.Execute(() =>
            {
                var categoryAxis = new CategoryAxis();
                var linearAxis = new LinearAxis();
                var series = CreateLineSeries(new[] { "A", "B" }, new[] { 1.0, 2.0 });

                series.XAxis = categoryAxis;
                series.YAxis = linearAxis;

                Verify.Throws<ArgumentException>(() => series.XAxis = new LinearAxis(), "Incompatible XAxis replacement should throw.");
                Verify.Throws<ArgumentException>(() => series.YAxis = new CategoryAxis(), "Incompatible YAxis replacement should throw.");

                Verify.AreSame(categoryAxis, series.XAxis, "Rejected XAxis replacement should preserve the previous axis.");
                Verify.AreSame(linearAxis, series.YAxis, "Rejected YAxis replacement should preserve the previous axis.");
            });
        }

        [TestMethod]
        public void LineSeriesCategoryAxisHandlesXValueTypeChanges()
        {
            CategoryAxis xAxis = null;
            LinearAxis yAxis = null;
            Samples xSamples = null;
            LineSeries series = null;
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                xAxis = new CategoryAxis();
                yAxis = new LinearAxis();
                xSamples = SamplesOf(new[] { "A", "B" });
                series = new LineSeries
                {
                    XValues = xSamples,
                    YValues = SamplesOf(new[] { 1.0, 2.0 })
                };
                chart = CreateChart(series, xAxis, yAxis);

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                xSamples.ItemsSource = new ObservableCollection<double> { 10.0, 20.0 };
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                xSamples.ItemsSource = new ObservableCollection<DateTimeOffset> { Date(2020, 1, 1), Date(2021, 1, 1) };
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(xAxis, series.XAxis, "XAxis should remain assigned after X value type changes.");
                Verify.AreSame(yAxis, series.YAxis, "YAxis should remain assigned after X value type changes.");
                VerifyChartRemainsLoaded(chart);
            });
        }

        [TestMethod]
        public void DateTimeAxisDefaultsAreAutomatic()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new DateTimeAxis();

                Verify.IsNull(axis.Minimum, "Default Minimum should be null.");
                Verify.IsNull(axis.Maximum, "Default Maximum should be null.");
                Verify.AreEqual(DateTimeIntervalType.Auto, axis.IntervalType, "Default IntervalType should be Auto.");
                Verify.AreEqual(string.Empty, axis.LabelFormat, "Default LabelFormat should be empty.");
            });
        }

        [TestMethod]
        public void DateTimeAxisIntervalTypeRoundTripsThroughPropertyAndDependencyProperty()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new DateTimeAxis();

                foreach (var intervalType in new[]
                {
                    DateTimeIntervalType.Auto,
                    DateTimeIntervalType.Day,
                    DateTimeIntervalType.Week,
                    DateTimeIntervalType.Month,
                    DateTimeIntervalType.Year
                })
                {
                    axis.IntervalType = intervalType;
                    Verify.AreEqual(intervalType, axis.IntervalType, "IntervalType should round-trip.");

                    axis.SetValue(DateTimeAxis.IntervalTypeProperty, intervalType);
                    Verify.AreEqual(intervalType, (DateTimeIntervalType)axis.GetValue(DateTimeAxis.IntervalTypeProperty), "IntervalTypeProperty should round-trip.");
                }

                axis.ClearValue(DateTimeAxis.IntervalTypeProperty);
                Verify.AreEqual(DateTimeIntervalType.Auto, axis.IntervalType, "ClearValue should restore IntervalType default.");
            });
        }

        [TestMethod]
        public void DateTimeAxisLabelFormatRoundTripsThroughPropertyAndDependencyProperty()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new DateTimeAxis();

                axis.LabelFormat = "month year";
                Verify.AreEqual("month year", axis.LabelFormat, "LabelFormat should round-trip.");

                axis.SetValue(DateTimeAxis.LabelFormatProperty, "shortdate");
                Verify.AreEqual("shortdate", axis.LabelFormat, "LabelFormat should read SetValue.");
                Verify.AreEqual("shortdate", (string)axis.GetValue(DateTimeAxis.LabelFormatProperty), "LabelFormatProperty should read the local value.");

                axis.ClearValue(DateTimeAxis.LabelFormatProperty);
                Verify.AreEqual(string.Empty, axis.LabelFormat, "ClearValue should restore LabelFormat default.");
            });
        }

        [TestMethod]
        public void DateTimeAxisBoundsRoundTripAndNullClear()
        {
            RunOnUIThread.Execute(() =>
            {
                var minimum = Date(2020, 1, 1);
                var maximum = Date(2023, 12, 31);
                var axis = new DateTimeAxis
                {
                    Minimum = minimum,
                    Maximum = maximum
                };

                Verify.AreEqual(minimum, axis.Minimum.Value, "Minimum should round-trip.");
                Verify.AreEqual(maximum, axis.Maximum.Value, "Maximum should round-trip.");

                axis.Minimum = null;
                axis.Maximum = null;

                Verify.IsNull(axis.Minimum, "Minimum null should select automatic bounds.");
                Verify.IsNull(axis.Maximum, "Maximum null should select automatic bounds.");
            });
        }

        [TestMethod]
        public void DateTimeAxisRejectsInvalidBoundsAndPreservesPreviousValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var originalMinimum = Date(2020, 1, 1);
                var originalMaximum = Date(2021, 1, 1);
                var axis = new DateTimeAxis
                {
                    Minimum = originalMinimum,
                    Maximum = originalMaximum
                };

                Verify.Throws<ArgumentException>(() => axis.Minimum = Date(2022, 1, 1), "Minimum above Maximum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Minimum = originalMaximum, "Minimum equal to Maximum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Maximum = Date(2019, 1, 1), "Maximum below Minimum should throw.");
                Verify.Throws<ArgumentException>(() => axis.Maximum = originalMinimum, "Maximum equal to Minimum should throw.");

                Verify.AreEqual(originalMinimum, axis.Minimum.Value, "Minimum should preserve its previous value.");
                Verify.AreEqual(originalMaximum, axis.Maximum.Value, "Maximum should preserve its previous value.");
            });
        }

        [TestMethod]
        public void DateTimeAxisAcceptsPreEpochDates()
        {
            RunOnUIThread.Execute(() =>
            {
                var axis = new DateTimeAxis();
                var laterMinimum = Date(1899, 12, 30);
                var earliestMinimum = Date(1601, 1, 1);

                axis.Minimum = laterMinimum;
                Verify.AreEqual(laterMinimum, axis.Minimum.Value, "Pre-1900 Minimum should be accepted.");

                axis.Minimum = earliestMinimum;
                Verify.AreEqual(earliestMinimum, axis.Minimum.Value, "Year 1601 Minimum should be accepted.");
            });
        }

        [TestMethod]
        public void DateTimeAxisLoadsWithDateValues()
        {
            DateTimeAxis axis = null;
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                axis = new DateTimeAxis
                {
                    Minimum = Date(2019, 1, 1),
                    Maximum = Date(2022, 1, 1)
                };
                chart = CreateChart(CreateLineSeries(new[] { Date(2020, 1, 1), Date(2021, 1, 1) }, new[] { 1.0, 2.0 }), axis, new LinearAxis());

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(axis, chart.Series[0].XAxis, "DateTimeAxis should remain assigned after load.");
                Verify.AreEqual(Date(2019, 1, 1), axis.Minimum.Value, "Minimum should remain assigned after load.");
                Verify.AreEqual(Date(2022, 1, 1), axis.Maximum.Value, "Maximum should remain assigned after load.");
                VerifyChartRemainsLoaded(chart);
            });
        }

        [TestMethod]
        public void DateTimeAxisIntervalTypesChangeWhileLoaded()
        {
            DateTimeAxis axis = null;
            Chart chart = null;

            RunOnUIThread.Execute(() =>
            {
                axis = new DateTimeAxis();
                chart = CreateChart(CreateLineSeries(new[] { Date(2020, 1, 1), Date(2021, 1, 1) }, new[] { 1.0, 2.0 }), axis, new LinearAxis());

                LoadChart(chart);
            });
            IdleSynchronizer.Wait();

            foreach (var intervalType in new[]
            {
                DateTimeIntervalType.Day,
                DateTimeIntervalType.Week,
                DateTimeIntervalType.Month,
                DateTimeIntervalType.Year,
                DateTimeIntervalType.Auto
            })
            {
                RunOnUIThread.Execute(() =>
                {
                    axis.IntervalType = intervalType;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(intervalType, axis.IntervalType, "IntervalType should update while loaded.");
                    VerifyChartRemainsLoaded(chart);
                });
            }
        }

        private static Chart CreateChart(LineSeries series, CartesianAxis xAxis, CartesianAxis yAxis)
        {
            var chart = new Chart
            {
                Width = 400,
                Height = 300
            };

            if (xAxis != null)
            {
                chart.Axes.Add(xAxis);
                series.XAxis = xAxis;
            }

            if (yAxis != null)
            {
                chart.Axes.Add(yAxis);
                series.YAxis = yAxis;
            }

            chart.Series.Add(series);
            return chart;
        }

        private static LineSeries CreateLineSeries<TX>(IEnumerable<TX> xValues, IEnumerable<double> yValues)
        {
            return new LineSeries
            {
                XValues = SamplesOf(xValues),
                YValues = SamplesOf(yValues)
            };
        }

        private static Samples SamplesOf<T>(IEnumerable<T> values)
        {
            return new Samples
            {
                ItemsSource = new ObservableCollection<T>(values)
            };
        }

        private static DateTimeOffset Date(int year, int month, int day)
        {
            return new DateTimeOffset(year, month, day, 0, 0, 0, TimeSpan.Zero);
        }

        private static SolidColorBrush MakeBrush(byte red, byte green, byte blue)
        {
            return new SolidColorBrush(ColorHelper.FromArgb(0xFF, red, green, blue));
        }

        private static void AddAxisBrushResources(ResourceDictionary resources)
        {
            resources.Add("ChartsGridLineMajorBrush", MakeBrush(0x10, 0x20, 0x30));
            resources.Add("ChartsGridLineMinorBrush", MakeBrush(0x20, 0x30, 0x40));
            resources.Add("ChartsTickBrush", MakeBrush(0x30, 0x40, 0x50));
            resources.Add("ChartsTickLabelBrush", MakeBrush(0x40, 0x50, 0x60));
            resources.Add("ChartsAxisLineBrush", MakeBrush(0x50, 0x60, 0x70));
        }

        private static ResourceDictionary CreateThemedAxisResources()
        {
            var resources = new ResourceDictionary();
            var light = new ResourceDictionary();
            var dark = new ResourceDictionary();
            var highContrast = new ResourceDictionary();

            AddAxisBrushResources(light);
            AddAxisBrushResources(dark);
            AddAxisBrushResources(highContrast);

            resources.ThemeDictionaries.Add("Light", light);
            resources.ThemeDictionaries.Add("Dark", dark);
            resources.ThemeDictionaries.Add("HighContrast", highContrast);
            return resources;
        }

        private static void VerifyAxisBrushResourceKeysExist()
        {
            var resources = new XamlChartsResources();
            foreach (var key in new[]
            {
                "ChartsGridLineMajorBrush",
                "ChartsGridLineMinorBrush",
                "ChartsTickBrush",
                "ChartsTickLabelBrush",
                "ChartsAxisLineBrush"
            })
            {
                Verify.IsTrue(ContainsResourceKey(resources, key), key + " should exist in XamlChartsResources.");
            }
        }

        private static bool ContainsResourceKey(ResourceDictionary resources, string key)
        {
            if (resources.ContainsKey(key))
            {
                return true;
            }

            foreach (var dictionary in resources.MergedDictionaries)
            {
                if (ContainsResourceKey(dictionary, key))
                {
                    return true;
                }
            }

            foreach (var value in resources.ThemeDictionaries.Values)
            {
                if (value is ResourceDictionary dictionary && ContainsResourceKey(dictionary, key))
                {
                    return true;
                }
            }

            return false;
        }
    }
}
