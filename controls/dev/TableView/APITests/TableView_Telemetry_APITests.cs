// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using Windows.Foundation;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    [TestProperty("Category", "TableViewRealTiP")]
    public class TableViewTelemetryTests : TableViewApiTestBase
    {
        [TestMethod]
        public void VerifyTemplateSetupAndSelectionNoOp()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTableView();
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                table.Select(1);
                table.Select(1);
                Verify.AreEqual(1, table.SelectedIndex);
            });
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            VerifySingleSelection(events);
            var summary = Summary(events);
            Verify.AreEqual(1u, summary.Number("SelectionCount"));
            Verify.AreEqual(1, events.Count(e => e.Provider == TableViewTelemetryCapture.SdkProvider),
                "Selection must not add per-action SDK spans or feature-use events.");
        }

        [TestMethod]
        public void VerifyTemplateSetupDoesNotWaitForRectangleClipReveal()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            RectangleGeometry clip = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTableView();
                clip = new RectangleGeometry { Rect = new Rect(0, 0, 0, 0) };
                table.Clip = clip;
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            long reveal = 0;
            RunOnUIThread.Execute(() =>
            {
                reveal = Stopwatch.GetTimestamp();
                clip.Rect = new Rect(0, 0, 500, 300);
            });
            IdleSynchronizer.Wait();
            var events = UnloadAndRead(capture);
            Verify.IsTrue(events.Single(e => e.IsTip(64477764)).TimestampQpc < reveal,
                "Template setup must finish before the application's later clip reveal.");
            VerifyInitialization(events);
        }

        [TestMethod]
        public void VerifyBeginningEditVetoIsIgnored()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            var items = MakeEditableItems();
            RunOnUIThread.Execute(() =>
            {
                table = CreateEditableTable(items);
                table.IsReadOnly = false;
                table.BeginningEdit += (_, args) => args.Cancel = true;
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(CaptureSetValue(GetValueProvider(table, items[0], 0), "Rejected"));
                Verify.AreEqual("Asha", items[0].Name);
                Verify.IsFalse(table.IsEditing);
            });
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            VerifyTip(events, operation: 9, kind: 4, reason: 101, count: 1);
            Verify.AreEqual(0u, Summary(events).Number("BeginEditCount"));
            VerifyNoProductFailures(events);
        }

        [TestMethod]
        public void VerifyValidationRejectionIsIgnored()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            var items = MakeEditableItems();
            items[0].RejectPrefix = "Invalid:";
            RunOnUIThread.Execute(() =>
            {
                table = CreateEditableTable(items);
                table.IsReadOnly = false;
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(CaptureSetValue(GetValueProvider(table, items[0], 0), "Invalid:value"));
                Verify.IsTrue(table.IsEditing, "Rejected validation must retain the edit.");
                Verify.AreEqual("Asha", items[0].Name, "A rejected value must be rolled back off the item.");
                Verify.IsTrue(table.CancelEdit());
                Verify.AreEqual("Asha", items[0].Name);
                Verify.IsFalse(table.IsEditing);
            });
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            VerifyTip(events, operation: 10, kind: 4, reason: 102, count: 1);
            var summary = Summary(events);
            Verify.AreEqual(1u, summary.Number("BeginEditCount"));
            Verify.AreEqual(0u, summary.Number("CommitEditCount"));
            Verify.AreEqual(1u, summary.Number("CancelEditCount"));
            VerifyNoProductFailures(events);
        }

        [TestMethod]
        public void VerifyReentrantShapingDoesNotCountSupersededSort()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            TableViewSource source = null;
            var items = MakeItems();
            bool notified = false;
            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(items);
                table = CreateTableView(source);
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                table.Sorted += (_, _) =>
                {
                    notified = true;
                    source.Filter(item => !ReferenceEquals(item, items[1]));
                };
                Verify.IsTrue(table.SortByColumn(table.Columns[0], SortDirection.Descending));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(notified);
                var projected = GetProjectedItems(table);
                Verify.AreEqual(2, projected.Count);
                Verify.AreSame(items[2], projected[0]);
                Verify.AreSame(items[0], projected[1]);
            });
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            VerifyTip(events, operation: 2, kind: 4, reason: 103, count: 1);
            Verify.AreEqual(0u, Summary(events).Number("SortCount"));
            VerifyNoProductFailures(events);
        }

        [TestMethod]
        public void VerifyRecoveredBindingFaultIsNotProductFailure()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            var items = MakeItems();
            RunOnUIThread.Execute(() =>
            {
                table = CreateTableView(items);
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                table.Columns[0].SortMemberPath = "(";
                Verify.IsTrue(table.SortByColumn(table.Columns[0], SortDirection.Ascending));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetProjectedItems(table).SequenceEqual(items));
                table.Columns[0].SortMemberPath = "Missing";
                Verify.IsTrue(table.SortByColumn(table.Columns[0], SortDirection.Descending));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(GetProjectedItems(table).SequenceEqual(items)));
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            // The resolver already recovers from a bad path (null key), so the sort completes normally.
            VerifyTip(events, operation: 2, kind: 1, reason: 1, count: 2);
            Verify.AreEqual(2, events.Count(e => e.IsTip(64477774) && e.Number("metricsBucket") == 2),
                "A recovered binding fault must not add an ignored or failed result.");
            Verify.IsFalse(events.Any(e => e.Provider == TableViewTelemetryCapture.DebugProvider),
                "A recovered binding fault is application data; nothing is recorded for it.");
            Verify.AreEqual(2u, Summary(events).Number("SortCount"));
            VerifyNoProductFailures(events);
        }

        [TestMethod]
        public void VerifySelectionHandlerFaultIsApplicationFault()
        {
            using var capture = new TableViewTelemetryCapture();
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTableView();
                LoadContent(table);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                table.SelectionChanged += (_, _) => throw new InvalidOperationException("Application handler fault.");
                bool propagated = false;
                try { table.Select(1); }
                catch (Exception) { propagated = true; }
                Verify.IsTrue(propagated, "The handler's exception must still reach the caller.");
            });
            var events = UnloadAndRead(capture);
            VerifyInitialization(events);
            VerifyTip(events, operation: 8, kind: 4, reason: 108, count: 1);
            Verify.IsTrue(events.Any(e => e.Provider == TableViewTelemetryCapture.DebugProvider &&
                e.Number("Attribution") == 2 && e.Number("AppFaultKind") == 1),
                "The app fault is attributed to the application callback.");
            Verify.AreEqual(0u, Summary(events).Number("SelectionCount"));
            VerifyNoProductFailures(events);
        }

        private IReadOnlyList<TableViewTelemetryCapture.Record> UnloadAndRead(TableViewTelemetryCapture capture)
        {
            RunOnUIThread.Execute(() => Content = null);
            IdleSynchronizer.Wait();
            return capture.Stop();
        }

        private static TableViewTelemetryCapture.Record Summary(IReadOnlyList<TableViewTelemetryCapture.Record> events)
        {
            var summary = events.Single(e => e.Name == "TableView_SessionSummary");
            Verify.AreEqual(5u, summary.Number("SchemaVersion"));
            Verify.AreEqual(4u, summary.Number("SessionSummaryVersion"));
            Verify.IsFalse(summary.Has("AppFaultCount"), "Application faults are not counted in the summary.");
            Verify.IsTrue(summary.Flag("UTCReplace_AppSessionGuid"),
                "Use the existing provider collection-context directive, not a control identity lookup.");
            return summary;
        }

        private static void VerifyInitialization(IReadOnlyList<TableViewTelemetryCapture.Record> events)
        {
            Verify.AreEqual(0, events.Count(e => e.Name.StartsWith("TableView_Initialization", StringComparison.Ordinal)),
                "TiP supplies initialization timing; SDK must not duplicate that activity.");
            Verify.AreEqual(1, events.Count(e => e.IsTip(64477764)));
            VerifyTip(events, operation: 0, kind: 1, reason: 1, count: 1);
            Verify.AreEqual(1u, Summary(events).Number("InitialOutcome"));
        }

        private static void VerifyTip(IReadOnlyList<TableViewTelemetryCapture.Record> events,
            uint operation, uint kind, uint reason, int count)
        {
            uint measure = operation == 0 ? 64477764u : 64477774u;
            Verify.AreEqual(count, events.Count(e => e.IsTip(measure) &&
                e.Number("metricsBucket") == operation &&
                e.Number("completionKind") == kind && e.Number("reason") == reason));
        }

        private static void VerifySingleSelection(IReadOnlyList<TableViewTelemetryCapture.Record> events)
        {
            VerifyTip(events, operation: 8, kind: 1, reason: 1, count: 1);
            Verify.AreEqual(1, events.Count(e => e.IsTip(64477774) && e.Number("metricsBucket") == 8),
                "A selection no-op must not add an ignored or failed native result.");
            VerifyNoProductFailures(events);
        }

        private static void VerifyNoProductFailures(IReadOnlyList<TableViewTelemetryCapture.Record> events)
        {
            Verify.IsFalse(events.Where(e => e.IsTip(64477764) || e.IsTip(64477774))
                .Any(e => e.Number("completionKind") == 3));
        }
    }
}
