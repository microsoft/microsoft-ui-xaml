// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls.Charts;
using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    internal static class ChartTestHelpers
    {
        private static XamlChartsResources s_chartsResources;

        // Apps merge XamlChartsResources next to XamlControlsResources, because the default Chart style
        // uses its theme resources. Call on the UI thread.
        public static void EnsureChartsResources()
        {
            s_chartsResources ??= new XamlChartsResources();
            App.AppendResourceDictionaryToMergedDictionaries(s_chartsResources);
        }

        public static void RemoveChartsResources()
        {
            App.RemoveResourceDictionaryFromMergedDictionaries(s_chartsResources);
        }
    }

    public class ChartTestBase : ApiTestBase
    {
        protected void LoadChart(Chart chart)
        {
            Content = chart;
            Content.UpdateLayout();
        }

        protected static void VerifyChartSurvived(Chart chart)
        {
            Verify.IsTrue(chart.IsLoaded, "Chart should remain loaded.");
            Verify.AreEqual(400.0, chart.ActualWidth, "Chart width should remain 400.");
            Verify.AreEqual(300.0, chart.ActualHeight, "Chart height should remain 300.");
        }

        protected static void VerifyChartRemainsLoaded(Chart chart)
        {
            VerifyChartSurvived(chart);
        }
    }
}
