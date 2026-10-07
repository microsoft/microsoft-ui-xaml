// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Charts;
using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ChartAutomationTests : ChartTestBase
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
        public void LabeledBySuppliesAutomationName()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                var label = new TextBlock { Text = "CPU history" };
                AutomationProperties.SetLabeledBy(chart, label);

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(chart);

                Verify.IsNotNull(peer, "Chart should create an automation peer.");
                Verify.AreEqual("CPU history", peer.GetName(), "LabeledBy should supply the name.");
            });
        }

        [TestMethod]
        public void ExplicitAutomationNameWinsOverLabeledBy()
        {
            RunOnUIThread.Execute(() =>
            {
                // Apps can set the two properties in either order.
                foreach (bool nameFirst in new[] { true, false })
                {
                    string order = nameFirst ? "Name set before LabeledBy" : "Name set after LabeledBy";
                    var chart = new Chart();
                    var label = new TextBlock { Text = "CPU history" };
                    if (nameFirst)
                    {
                        AutomationProperties.SetName(chart, "Processor utilization");
                        AutomationProperties.SetLabeledBy(chart, label);
                    }
                    else
                    {
                        AutomationProperties.SetLabeledBy(chart, label);
                        AutomationProperties.SetName(chart, "Processor utilization");
                    }

                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(chart);

                    Verify.IsNotNull(peer, order + ": Chart should create an automation peer.");
                    Verify.AreEqual("Processor utilization", peer.GetName(), order + ": explicit name should win over LabeledBy.");
                }
            });
        }

        [TestMethod]
        public void AppFullDescriptionWins()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart();
                AutomationProperties.SetName(chart, "CPU history");
                AutomationProperties.SetFullDescription(
                    chart,
                    "Processor utilization over the last minute.");

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(chart);

                Verify.IsNotNull(peer, "Chart should create an automation peer.");
                Verify.AreEqual(
                    "Processor utilization over the last minute.",
                    peer.GetFullDescription(),
                    "Explicit FullDescription should win.");
            });
        }

        // Assistive technology queries a chart's name and role without it being on screen. Those queries
        // must answer from the properties alone and must not lay the chart out.
        [TestMethod]
        public void PeerQueriesDoNotLayOutTheChart()
        {
            RunOnUIThread.Execute(() =>
            {
                var chart = new Chart
                {
                    Width = 400,
                    Height = 300
                };
                chart.Series.Add(new LineSeries
                {
                    XValues = new Samples { ItemsSource = new List<string> { "Q1", "Q2" } },
                    YValues = new Samples { ItemsSource = new List<double> { 18.0, 27.0 } },
                    Title = "Revenue"
                });
                AutomationProperties.SetName(chart, "Quarterly revenue");

                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(chart);

                Verify.IsNotNull(peer, "Chart should create an automation peer before load.");
                Verify.AreEqual("Quarterly revenue", peer.GetName(), "Name should be available before load.");
                Log.Comment("ControlType: " + peer.GetAutomationControlType());
                Log.Comment("LocalizedControlType: " + peer.GetLocalizedControlType());
                Verify.AreEqual(0.0, chart.ActualWidth, "Peer queries should not lay out the chart.");
                Verify.AreEqual(0.0, chart.DesiredSize.Width, "Peer queries should not measure the chart.");
            });
        }
    }
}
