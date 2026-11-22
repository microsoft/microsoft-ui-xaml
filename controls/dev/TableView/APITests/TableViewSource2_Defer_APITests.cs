// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 8: DeferRefresh. Verbs inside an open handle edit the desired spec
    // only; the outermost Close applies once (one Reset). Incremental source changes still show while
    // open (WPF model); a source Reset, Refresh() and hierarchy verbs wait for Close.
    // MakeLiveFlat: A10 X, B20 Y, C30 X, D40 Y.
    public partial class TableViewSource2Tests
    {
        [TestMethod]
        [TestProperty("Description", "Verifies Filter, Sort and GroupBy inside one DeferRefresh change nothing until Close, which applies them with one Reset.")]
        public void VerifyDeferRefreshBatchesVerbs()
        {
            var tableView = PlaceFlat(out var source, out _);
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Filter(MinScore(15)).Sort("Score", SortDirection.Descending).GroupBy(LiveByTeam, SortDirection.Ascending);
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "Verbs inside the scope raise nothing");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "The applied spec is unchanged while open"));

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "Close applies every verb once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C [H] D B", "Filter, Sort and GroupBy applied together"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an inner Close neither applies nor ends the deferral; only the outermost Close applies, once.")]
        public void VerifyNestedDeferRefreshAppliesOnOutermostClose()
        {
            var tableView = PlaceFlat(out var source, out _);
            var events = RecordViewEvents(tableView);

            IDisposable outer = null;
            IDisposable inner = null;
            RunOnUIThread.Execute(() =>
            {
                outer = source.DeferRefresh();
                inner = source.DeferRefresh();
                source.Filter(MinScore(15));
                inner.Dispose();
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "An inner Close applies nothing");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Still deferred after the inner Close"));

            OnUI(tableView, () => source.Sort("Score", SortDirection.Descending));
            VerifyEvents(events, "", "A verb after the inner Close is still deferred");

            OnUI(tableView, () => outer.Dispose());
            VerifyEvents(events, "Reset", "The outermost Close applies once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B", "Both verbs applied"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a second Close of a handle is a no-op: it neither applies again nor ends an enclosing deferral, and verbs apply at once after the outermost Close.")]
        public void VerifyDeferRefreshCloseIsIdempotent()
        {
            var tableView = PlaceFlat(out var source, out _);
            var events = RecordViewEvents(tableView);

            IDisposable outer = null;
            RunOnUIThread.Execute(() =>
            {
                outer = source.DeferRefresh();
                var inner = source.DeferRefresh();
                source.Filter(MinScore(15));
                inner.Dispose();
                inner.Dispose();
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "Closing the inner handle twice does not end the outer deferral");

            OnUI(tableView, () => outer.Dispose());
            VerifyEvents(events, "Reset", "The outer Close applies once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "Filter applied"));

            OnUI(tableView, () => outer.Dispose());
            VerifyEvents(events, "", "A second outer Close is a no-op");

            OnUI(tableView, () => source.Filter(MinScore(25)));
            VerifyEvents(events, "Reset", "A verb after the deferral applies at once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C D", "Filter applied immediately"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Refresh inside DeferRefresh is deferred to Close without stopping incremental changes from showing.")]
        public void VerifyRefreshInsideDeferIsDeferred()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Refresh();
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "Refresh inside the scope re-reads nothing yet");

            OnUI(tableView, () => rows.Add(new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" }));
            VerifyEvents(events, "Add@4", "An incremental change still shows while the Refresh is owed");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D E", "The added row is shown"));

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "Close runs the owed re-read once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D E", "The re-read matches the source"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source Reset inside DeferRefresh is held until Close, which re-reads and applies the deferred Filter with one Reset.")]
        public void VerifyDeferRefreshHoldsSourceResetUntilClose()
        {
            var rows = new ResettableCollection<LiveRow>(MakeLiveFlat());
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Filter(MinScore(15));
                var next = new List<LiveRow>(rows);
                next.Insert(0, new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" });
                next.RemoveAt(2);
                rows.ReplaceAll(next);
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "The source Reset is held");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "The old rows stay while open"));

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "Close re-reads and reshapes once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "E C D", "New source contents under the deferred Filter"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ParentBy and Sort in one DeferRefresh build the tree and sort it with one Reset at Close.")]
        public void VerifyDeferRefreshAppliesHierarchyAndSortOnce()
        {
            var source = FromOnUI(MakeLiveTree());
            var tableView = PlaceTreeSource(source, null);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "Ann Bob Cat Dee Eli", "Flat"));
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.ParentBy(LiveById, LiveByParent).Sort(LiveByScore, SortDirection.Ascending);
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "A parked ParentBy and a deferred Sort raise nothing");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "Ann Bob Cat Dee Eli", "Still flat while open"));

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "One Reset for the tree and the sort");
            VerifyTreeOnUI(tableView, "Dee1+ Ann1+", "Roots sorted by Score");
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Dee1- Eli2 Ann1- Cat2 Bob2", "Children sorted by Score");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a parked ParentBy whose key selector throws at Close is dropped: the re-read still shows source edits made in the scope, the exception surfaces from Close and the deferral ends (drift #22).")]
        public void VerifyDeferRefreshParkedParentByThrowShowsSourceEdits()
        {
            var rows = new ResettableCollection<LiveRow>(MakeLiveFlat());
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);
            var events = RecordViewEvents(tableView);
            var throwing = new TableViewKeySelector(o => throw new InvalidOperationException("key failed"));

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.ParentBy(throwing, LiveByParent);
                rows.Add(new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" });
            });
            SettleLayout(tableView);
            VerifyEvents(events, "Add@4", "The incremental Add shows under the current (flat) tree");

            RunOnUIThread.Execute(() =>
            {
                var next = new List<LiveRow>(rows);
                next.RemoveAt(0);
                next.Add(new LiveRow { Id = 6, Name = "F", Score = 60, Team = "Y" });
                rows.ReplaceAll(next);
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "The source Reset is held");

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => handle.Dispose()), "The selector's exception surfaces from Close."));
            SettleLayout(tableView);
            VerifyEvents(events, "Reset", "The re-read finishes with the current strategy");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D E F", "Edits made in the scope show, flat"));

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("no exception", ThrownTypeName(() => handle.Dispose()), "A second Close is a no-op."));
            OnUI(tableView, () => source.Filter(MinScore(35)));
            VerifyEvents(events, "Reset", "The deferral ended: a verb applies at once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D E F", "Filter applied"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a filter that throws when Close applies it propagates from Close, keeps the applied rows (including an incremental change made in the scope) and ends the deferral.")]
        public void VerifyDeferRefreshThrowingFilterAtClose()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.Filter(MinScore(15)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "Score >= 15"));
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Filter(ThrowingForB(() => true));
                rows.Add(new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" });
            });
            SettleLayout(tableView);
            VerifyEvents(events, "Add@3", "The Add is shaped with the applied filter");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => handle.Dispose()), "The predicate's exception surfaces from Close.");
                VerifyShaped(tableView, "B C D E", "The applied filter's rows stay");
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "A failed apply raises nothing");

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("no exception", ThrownTypeName(() => handle.Dispose()), "A second Close is a no-op."));
            OnUI(tableView, () => source.ClearFilter());
            VerifyEvents(events, "Reset", "The deferral ended: ClearFilter applies at once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D E", "ClearFilter applied"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ParentBy adoption Reset inside DeferRefresh (raised by the hierarchy, not held) keeps the applied spec: a deferred Filter waits for Close, which applies it with one Reset.")]
        public void VerifyDeferRefreshParentByAdoptionResetKeepsAppliedSpec()
        {
            var people = MakeTree();
            Named(people, "Fay").ManagerId = 99;
            var source = FromOnUI(people);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(ById, ByManager));
            ExpandAll(tableView);
            VerifyTreeOnUI(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1 Fay1 Gus1", "Fay and Gus are orphans");
            var events = RecordViewEvents(tableView);

            IDisposable handle = null;
            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Filter(new TableViewPredicate(o => ((TreeEmployee)o).Name != "Fay"));
            });
            SettleLayout(tableView);
            VerifyEvents(events, "", "The Filter is deferred");

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("no exception", ThrownTypeName(() => people.Insert(0, new TreeEmployee { Id = 99, Name = "Zed" })), "Adopting two orphans inside the scope does not throw."));
            SettleLayout(tableView);
            VerifyEvents(events, "Reset", "Adopting two orphans raises the hierarchy's Reset");
            OnUI(tableView, () => tableView.ExpandAllRows());
            VerifyTreeOnUI(tableView, "Zed1- Fay2 Gus2 Ada1- Ben2- Dan3 Cy2 Eve1", "The Reset kept the applied spec: Fay is not filtered yet");
            RunOnUIThread.Execute(() => events.Clear());

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "Close applies the deferred Filter once");
            OnUI(tableView, () => tableView.ExpandAllRows());
            VerifyTreeOnUI(tableView, "Zed1- Gus2 Ada1- Ben2- Dan3 Cy2 Eve1", "Fay filtered out after Close");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a predicate that closes the handle or opens a DeferRefresh while a commit applies gets InvalidOperationException for both, the handle stays open, and a later Close applies once.")]
        public void VerifyDeferRefreshReentrantCloseAndOpenThrow()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            IDisposable handle = null;
            bool armed = false;
            string closeResult = null;
            string openResult = null;
            var reentrant = new TableViewPredicate(o =>
            {
                if (armed && ((LiveRow)o).Name == "E")
                {
                    armed = false;
                    closeResult = ThrownTypeName(() => handle.Dispose());
                    openResult = ThrownTypeName(() => source.DeferRefresh());
                }
                return true;
            });
            OnUI(tableView, () => source.Filter(reentrant));
            var events = RecordViewEvents(tableView);

            RunOnUIThread.Execute(() =>
            {
                handle = source.DeferRefresh();
                source.Sort("Score", SortDirection.Descending);
                armed = true;
                rows.Add(new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X" });
            });
            SettleLayout(tableView);
            VerifyEvents(events, "Add@4", "The Add is shaped with the applied spec");
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("InvalidOperationException", closeResult, "Close during an apply throws.");
                Verify.AreEqual("InvalidOperationException", openResult, "DeferRefresh during an apply throws.");
                VerifyShaped(tableView, "A B C D E", "The Sort is still deferred");
            });

            OnUI(tableView, () => handle.Dispose());
            VerifyEvents(events, "Reset", "The handle stayed open; a later Close applies once");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "E D C B A", "Sort applied"));
        }
    }
}
