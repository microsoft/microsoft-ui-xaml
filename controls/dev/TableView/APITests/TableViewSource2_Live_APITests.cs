// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 6: with IsLiveShaping on, an item property change re-filters,
    // re-sorts and re-groups just the changed rows on the next dispatcher turn. Changes made in one
    // turn coalesce into one drain. OnUI settles the dispatcher, which runs the posted drain.
    // MakeLiveFlat: A10 X, B20 Y, C30 X, D40 Y.
    public partial class TableViewSource2Tests
    {
        private TableView PlaceLiveFlat(out TableViewSource2 source, out ObservableCollection<LiveRow> rows)
        {
            var tableView = PlaceFlat(out source, out rows);
            var placed = source;
            OnUI(tableView, () => placed.IsLiveShaping = true);
            return tableView;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live change that fails the filter hides the row, and one that passes it shows the row, each with one targeted notification.")]
        public void VerifyLiveFilterHidesChangedRow()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.Filter(MinScore(15)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "Score >= 15"));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows[1].Score = 5);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C D", "B fails the filter"));
            VerifyEvents(events, "Remove@0", "B leaves");

            OnUI(tableView, () => rows[0].Score = 50);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A C D", "A passes the filter"));
            VerifyEvents(events, "Add@0", "A enters in source order");

            // A change the filter does not see raises nothing.
            OnUI(tableView, () => rows[2].Name = "C");
            VerifyEvents(events, "", "no shaping change");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live change to a sort key moves only that row to its new sorted place.")]
        public void VerifyLiveSortMovesChangedRow()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Score ascending"));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows[0].Score = 35);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "A moves between C and D"));
            VerifyEvents(events, "Remove@0,Add@2", "one row moves");

            // A new key that sorts to the same place raises nothing.
            OnUI(tableView, () => rows[0].Score = 36);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "A keeps its place"));
            VerifyEvents(events, "", "same place");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live change to a group key moves the row to its new group, and creates and prunes groups as needed.")]
        public void VerifyLiveGroupMovesChangedRow()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "GroupBy Team ascending"));
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => rows[0].Team = "Y");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C [H] A B D", "A joins group Y in source order"));
            VerifyEvents(events, "Remove@1+1,Add@3+1", "A leaves X and joins Y");

            OnUI(tableView, () => rows[2].Team = "Z");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A B D [H] C", "X pruned, Z created"));
            VerifyEvents(events, "Remove@0+2,Add@4+2", "the last member takes its header; the new group arrives with it");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies live changes made in one dispatcher turn are applied together on the next turn, not inside the app's setter.")]
        public void VerifyLiveChangesCoalescePerTurn()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () =>
            {
                rows[0].Score = 35;
                rows[1].Score = 45;
                rows[0].Score = 50;
                VerifyShaped(tableView, "A B C D", "nothing moves inside the setter");
                Verify.AreEqual(0, events.Count, "No event is raised inside the setter.");
            });
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C D B A", "both changes landed on the next turn"));
            VerifyEvents(events, "Remove@0,Add@3,Remove@0,Add@2", "one move per changed row, A's last value only");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies more live changes in one turn than the bulk threshold apply as one Reset.")]
        public void VerifyLiveBulkChangeRaisesOneReset()
        {
            var rows = new ObservableCollection<LiveRow>(
                Enumerable.Range(0, 20).Select(i => NewLive(i, $"R{i}", i, "X")));
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending).IsLiveShaping = true);
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () =>
            {
                foreach (var row in rows)
                {
                    row.Score = -row.Score;
                }
            });

            string expected = string.Join(" ", Enumerable.Range(0, 20).Reverse().Select(i => $"R{i}"));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, expected, "every score negated"));
            VerifyEvents(events, "Reset", "one rebuild");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a bulk live drain with one throwing key falls back to per-row placement: every other change applies and only the failing row stays.")]
        public void VerifyLiveBulkDrainExceptionDropsOnlyFailingRow()
        {
            var rows = new ObservableCollection<LiveRow>(
                Enumerable.Range(0, 20).Select(i => NewLive(i, $"R{i}", i + 1, "X")));
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);
            var throwingAt99 = LiveKey(r => r.Score == 99 ? throw new InvalidOperationException("key") : r.Score);
            OnUI(tableView, () => source.Sort(throwingAt99, SortDirection.Ascending).IsLiveShaping = true);
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () =>
            {
                foreach (var row in rows)
                {
                    row.Score = row.Id == 5 ? 99 : -row.Score;
                }
            });

            // R5's key throws, so it keeps its old key (6) and sorts after every negated row.
            var negated = Enumerable.Range(0, 20).Reverse().Where(i => i != 5).Select(i => $"R{i}");
            string expected = string.Join(" ", negated.Concat(new[] { "R5" }));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, expected, "every change but R5's applied"));
            Verify.IsFalse(events.Contains("Reset"), "The failed rebuild fell back to per-row edits.");

            OnUI(tableView, () => rows[5].Score = -50);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "R5 " + string.Join(" ", negated), "R5's next change applies"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a live change raised by app code while a verb applies, to a key the rebuild already read, is drained after it, not lost.")]
        public void VerifyLiveChangeDuringApplyIsDrainedAfter()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            // The rebuild reads sort keys in source order, so A's key (10) is already consumed when
            // D's is read; changing A there leaves the rebuilt order stale until the drain.
            bool changed = false;
            var a = rows[0];
            var d = rows[3];
            var touching = LiveKey(r =>
            {
                if (!changed && ReferenceEquals(r, d))
                {
                    changed = true;
                    a.Score = 35;
                }
                return r.Score;
            });

            OnUI(tableView, () => source.Sort(touching, SortDirection.Ascending));
            Verify.IsTrue(changed, "The key selector ran for D.");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "A's change made during the apply was drained"));
            VerifyEvents(events, "Reset,Remove@0,Add@2", "the verb's Reset, then the drain moves A");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a managed item's off-thread PropertyChanged fails in the platform before it reaches the source, and leaves the source consistent.")]
        public void VerifyOffThreadManagedPropertyChangeLeavesSourceConsistent()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            // C# raises PropertyChanged through a Microsoft.UI.Xaml.Data.PropertyChangedEventArgs, which
            // can only be created on a XAML thread, so an off-thread raise throws RPC_E_WRONG_THREAD in
            // the setter before any subscriber runs. The source's off-thread marshalling is reachable
            // only from items that raise agile args (Header-Changes drift #71).
            var a = rows[0];
            Exception caught = null;
            Task.Run(() =>
            {
                try
                {
                    a.Score = 35;
                }
                catch (Exception e)
                {
                    caught = e;
                }
            }).Wait();

            Verify.IsNotNull(caught, "The platform rejects creating the event args off the XAML thread.");
            Verify.AreEqual(unchecked((int)0x8001010E), caught.HResult, "RPC_E_WRONG_THREAD.");
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "the failed raise reached nothing"));
            VerifyEvents(events, "", "no notification");

            // Score already holds 35 (the setter stores before it raises), so raise a new value on the UI thread.
            OnUI(tableView, () => a.Score = 36);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "a later UI-thread change applies"));
            VerifyEvents(events, "Remove@0,Add@2", "one row moves");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the IsLiveShaping setter throws while an apply runs, and works after it.")]
        public void VerifyIsLiveShapingSetterThrowsDuringApply()
        {
            var tableView = PlaceFlat(out var source, out _);
            string thrown = null;
            var placed = source;
            var probe = new TableViewPredicate(o =>
            {
                if (thrown == null)
                {
                    thrown = ThrownTypeName(() => placed.IsLiveShaping = true);
                }
                return true;
            });

            OnUI(tableView, () => source.Filter(probe));
            Verify.AreEqual("InvalidOperationException", thrown, "IsLiveShaping cannot be set from inside a predicate.");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(source.IsLiveShaping, "The rejected set changed nothing.");
                source.IsLiveShaping = true;
                Verify.IsTrue(source.IsLiveShaping, "The setter works after the apply.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a key selector that throws during a live drain leaves the row where it was, and a later change applies.")]
        public void VerifyLiveDrainExceptionLeavesTreeConsistent()
        {
            var tableView = PlaceLiveFlat(out var source, out var rows);
            var throwingAt99 = LiveKey(r => r.Score == 99 ? throw new InvalidOperationException("key") : r.Score);
            OnUI(tableView, () => source.Sort(throwingAt99, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows[0].Score = 99);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "the failed change left A in place"));
            VerifyEvents(events, "", "nothing committed");

            OnUI(tableView, () => rows[0].Score = 35);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "the next change applies"));
            VerifyEvents(events, "Remove@0,Add@2", "one move");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changes are ignored while IsLiveShaping is off, and Refresh picks them up.")]
        public void VerifyLiveShapingOffIgnoresChanges()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows[0].Score = 35);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "not live: A stays"));
            VerifyEvents(events, "", "no live change");

            OnUI(tableView, () => source.Refresh());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C A D", "Refresh re-reads A"));
        }
    }
}
