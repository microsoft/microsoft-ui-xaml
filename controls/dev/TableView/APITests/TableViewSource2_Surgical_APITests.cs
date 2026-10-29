// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 5: under an applied Filter, Sort or GroupBy a single source change
    // links or unlinks only the rows that enter or leave, creating and pruning groups as members
    // arrive and leave. MakeLiveFlat: A10 X, B20 Y, C30 X, D40 Y.
    public partial class TableViewSource2Tests
    {
        private static LiveRow NewLive(int id, string name, int score, string team)
            => new LiveRow { Id = id, Name = name, Score = score, Team = team, Addr = new LiveAddress(name) };

        [TestMethod]
        [TestProperty("Description", "Verifies an Add under a Sort inserts the row at its sorted position with one targeted Add, and equal keys keep source order.")]
        public void VerifyAddUnderSortInsertsAtSortedPosition()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Score ascending"));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows.Insert(0, NewLive(5, "E", 25, "X")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B E C D", "E between B and C"));
            VerifyEvents(events, "Add@2", "sorted insert");

            // Equal keys: F is after B in the source, G before it.
            OnUI(tableView, () => rows.Add(NewLive(6, "F", 20, "Y")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B F E C D", "F after B (source order among equal keys)"));
            VerifyEvents(events, "Add@2", "tie after");

            OnUI(tableView, () => rows.Insert(0, NewLive(7, "G", 20, "Y")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A G B F E C D", "G before B (source order among equal keys)"));
            VerifyEvents(events, "Add@1", "tie before");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Add whose group key is new creates the group header at its sorted place, and an Add to an existing group joins it.")]
        public void VerifyAddCreatesNewGroup()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "GroupBy Team ascending"));
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => rows.Add(NewLive(5, "E", 50, "Z")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D [H] E", "Group Z created last"));
            VerifyEvents(events, "Add@6+2", "a new group and its member arrive as one insert");

            OnUI(tableView, () => rows.Add(NewLive(6, "F", 60, "W")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] F [H] A C [H] B D [H] E", "Group W created first"));
            VerifyEvents(events, "Add@0+2", "a new group sorted before the others");

            OnUI(tableView, () => rows.Add(NewLive(7, "G", 70, "X")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] F [H] A C G [H] B D [H] E", "G joins group X"));
            VerifyEvents(events, "Add@5+1", "joining an existing group");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a group's last member removes the header with it, and a later arrival with that key creates the group again.")]
        public void VerifyRemoveLastGroupMemberRemovesHeader()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "GroupBy Team ascending"));
            var events = RecordViewEventsWithCounts(tableView);

            OnUI(tableView, () => rows.RemoveAt(1));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] D", "B left group Y"));
            VerifyEvents(events, "Remove@4+1", "a member leaves");

            OnUI(tableView, () => rows.RemoveAt(2));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C", "D was Y's last member: the header went with it"));
            VerifyEvents(events, "Remove@3+2", "the header and its last member leave as one remove");

            OnUI(tableView, () => rows.Add(NewLive(5, "E", 50, "Y")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] E", "Group Y created again"));
            VerifyEvents(events, "Add@3+2", "the group comes back");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source Move a Sort hides raises nothing, and a Move that changes the order among equal keys moves just that row.")]
        public void VerifyMoveUnderSortIsIgnoredForOrder()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Ascending));
            var events = RecordViewEvents(tableView);

            // Source B C D A: A's score still sorts it first.
            OnUI(tableView, () => rows.Move(0, 3));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "The sort hides the move"));
            VerifyEvents(events, "", "a hidden move raises nothing");

            OnUI(tableView, () => source.ClearSort().Sort(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C A B D", "Team ascending, source order B C D A"));
            events.Clear();

            // Source A B C D: A now precedes C among the X rows.
            OnUI(tableView, () => rows.Move(3, 0));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A C B D", "A moved before C"));
            VerifyEvents(events, "Remove@1,Add@0", "a move among equal keys");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Add, Remove and Replace under a Filter and Sort each raise one targeted notification (no Reset), and changes to hidden rows raise nothing.")]
        public void VerifyIncrementalChangeRaisesSingleNotification()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.Filter(MinScore(15)).Sort(LiveByScore, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B", "Score >= 15, descending"));
            var events = RecordViewEvents(tableView);

            OnUI(tableView, () => rows.Add(NewLive(5, "E", 35, "X")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D E C B", "E added"));
            VerifyEvents(events, "Add@1", "add");

            OnUI(tableView, () => rows.RemoveAt(2));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D E B", "C removed"));
            VerifyEvents(events, "Remove@2", "remove");

            OnUI(tableView, () => rows.Add(NewLive(6, "F", 5, "Y")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D E B", "F is filtered out"));
            VerifyEvents(events, "", "a hidden arrival raises nothing");

            // Hidden A is replaced by a shown row, then that row by a hidden one.
            OnUI(tableView, () => rows[0] = NewLive(7, "G", 50, "X"));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "G D E B", "G replaced hidden A"));
            VerifyEvents(events, "Add@0", "replace a hidden row by a shown one");

            OnUI(tableView, () => rows[0] = NewLive(8, "H", 1, "X"));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D E B", "H replaced G and is filtered out"));
            VerifyEvents(events, "Remove@0", "replace a shown row by a hidden one");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an app view handler that throws during a surgical Add under GroupBy leaves a consistent view and later changes still apply surgically. The engine's own listener-fault Reset is not reachable through the public API (XAML absorbs the handler's exception), so it is not covered here.")]
        public void VerifyThrowingViewHandlerDuringSurgicalAddRecovers()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Ascending));

            var thrown = 0;
            var events = RecordViewEventsWithCounts(tableView, e =>
            {
                if (thrown++ == 0)
                {
                    throw new InvalidOperationException("listener failed");
                }
            });

            OnUI(tableView, () => rows.Add(NewLive(5, "E", 50, "Z")));
            RunOnUIThread.Execute(() => Verify.IsTrue(thrown >= 1, "The throwing handler must have run."));
            VerifyEvents(events, "Add@6+2", "add with a throwing handler");
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D [H] E", "Consistent after the throw"));

            OnUI(tableView, () => rows.Add(NewLive(6, "F", 60, "X")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C F [H] B D [H] E", "A later Add joins group X"));
            VerifyEvents(events, "Add@3+1", "a later surgical add");

            OnUI(tableView, () => source.Refresh());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C F [H] B D [H] E", "Refresh re-reads the same view"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Add under a Sort whose key selector throws propagates the exception, leaves the shown rows unchanged and hides the arrival; once the selector recovers, the next change re-reads everything (drift #33).")]
        public void VerifyAddUnderThrowingSortKeyKeepsViewConsistent()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            bool armed = false;
            var byScore = LiveKey(r =>
            {
                if (armed && r.Name == "E")
                {
                    throw new InvalidOperationException("key selector failed");
                }
                return r.Score;
            });
            OnUI(tableView, () => source.Sort(byScore, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Score ascending"));

            RunOnUIThread.Execute(() =>
            {
                armed = true;
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => rows.Insert(0, NewLive(5, "E", 25, "X"))), "The key selector's exception must surface from the Add.");
                VerifyShaped(tableView, "A B C D", "The arrival the key selector threw on is hidden; the rest kept in order");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Still consistent after idle"));

            RunOnUIThread.Execute(() => armed = false);
            var events = RecordViewEvents(tableView);
            OnUI(tableView, () => rows.Add(NewLive(6, "F", 5, "Y")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "F A B E C D", "The next change re-reads everything, E included"));
            VerifyEvents(events, "Reset", "a stale engine rebuilds");

            OnUI(tableView, () => rows.Add(NewLive(7, "G", 35, "X")));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "F A B E C G D", "Surgical again after the rebuild"));
            VerifyEvents(events, "Add@5", "surgical add");
        }
    }
}
