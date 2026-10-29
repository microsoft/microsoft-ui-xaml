// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2, Milestone 4: Filter, Sort and GroupBy, each applied by a rebuild. Rows are read
    // from the repeater's ItemsSourceView (the shaped list itself): a data row is its LiveRow, a group
    // header is an opaque GroupedEntry, shown as [H]. MakeLiveFlat: A10 X, B20 Y, C30 X, D40 Y.
    public partial class TableViewSource2Tests
    {
        // Rows of the shaped list in order: data rows by Name, group headers as [H].
        private static string ShapedLabels(TableView tableView)
        {
            var view = GetRowsRepeater(tableView).ItemsSourceView;
            var parts = new List<string>();
            for (int i = 0; i < (view?.Count ?? 0); i++)
            {
                var item = view.GetAt(i);
                parts.Add(item is LiveRow row ? row.Name : "[H]");
            }

            return string.Join(" ", parts);
        }

        private static void VerifyShaped(TableView tableView, string expected, string context)
            => Verify.AreEqual(expected, ShapedLabels(tableView), $"Shaped rows ({context}).");

        private TableView PlaceFlat(out TableViewSource2 source, out ObservableCollection<LiveRow> rows)
        {
            rows = MakeLiveFlat();
            source = FromOnUI(rows);
            return PlaceTreeSource(source, null);
        }

        private static readonly TableViewKeySelector LiveByName = LiveKey(r => r.Name);

        [TestMethod]
        [TestProperty("Description", "Verifies Filter hides the rows its predicate rejects and a new Filter replaces the old one.")]
        public void VerifyFilterHidesRows()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source.Filter(MinScore(25)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C D", "Score >= 25"));

            OnUI(tableView, () => source.Filter(MinScore(15)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "A second Filter replaces the first"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an exception from a filter predicate propagates out of Filter and leaves the shown rows unchanged; ClearFilter recovers.")]
        public void VerifyFilterPredicateThrowsPropagatesAndKeepsProjection()
        {
            var tableView = PlaceFlat(out var source, out _);
            OnUI(tableView, () => source.Filter(MinScore(25)));

            RunOnUIThread.Execute(() =>
            {
                var throwing = new TableViewPredicate(o =>
                {
                    if (((LiveRow)o).Name == "B")
                    {
                        throw new InvalidOperationException("predicate failed");
                    }
                    return true;
                });

                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => source.Filter(throwing)), "The predicate's exception must propagate.");
                VerifyShaped(tableView, "C D", "A failed Filter keeps the applied filter's rows");
            });

            OnUI(tableView, () => source.ClearFilter());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "ClearFilter after the failure"));
        }

        // Score >= 15, but throws for B while `armed` is set (drift #33).
        private static TableViewPredicate ThrowingForB(Func<bool> armed) => new TableViewPredicate(o =>
        {
            var row = (LiveRow)o;
            if (armed() && row.Name == "B")
            {
                throw new InvalidOperationException("predicate failed");
            }
            return row.Score >= 15;
        });

        [TestMethod]
        [TestProperty("Description", "Verifies a source Remove under a throwing filter drops the removed row without calling the filter (a surgical Remove runs no app code) and keeps the rest of the view.")]
        public void VerifyFilterThrowsDuringRemoveDropsDepartedRow()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            bool armed = false;
            OnUI(tableView, () => source.Filter(ThrowingForB(() => armed)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "Score >= 15"));

            RunOnUIThread.Execute(() =>
            {
                armed = true;
                var d = rows[3];
                Verify.AreEqual("no exception", ThrownTypeName(() => rows.Remove(d)), "A Remove does not call the predicate.");
                Verify.AreEqual(2, GetRowsRepeater(tableView).ItemsSourceView.Count, "The removed row left the view.");
                VerifyShaped(tableView, "B C", "Departed row removed, the rest kept");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C", "Still consistent after idle"));

            RunOnUIThread.Execute(() => armed = false);
            OnUI(tableView, () => source.ClearFilter());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C", "ClearFilter after the failure re-reads everything"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source Clear under a throwing filter empties the view, and an arrival the filter throws on stays hidden until the next apply.")]
        public void VerifyFilterThrowsAroundClearKeepsViewConsistent()
        {
            var tableView = PlaceFlat(out var source, out var rows);
            bool armed = false;
            OnUI(tableView, () => source.Filter(ThrowingForB(() => armed)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "Score >= 15"));

            RunOnUIThread.Execute(() =>
            {
                armed = true;
                Verify.AreEqual("no exception", ThrownTypeName(() => rows.Clear()), "Clear reads no predicate.");
                Verify.AreEqual(0, GetRowsRepeater(tableView).ItemsSourceView.Count, "Clear empties the view.");

                var b = new LiveRow { Id = 5, Name = "B", Score = 20, Team = "Y", Addr = new LiveAddress("Bern") };
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => rows.Add(b)), "The predicate's exception must surface from the Add.");
                Verify.AreEqual(0, GetRowsRepeater(tableView).ItemsSourceView.Count, "The arrival the filter threw on is hidden.");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "", "Still consistent after idle"));

            RunOnUIThread.Execute(() => armed = false);
            OnUI(tableView, () => source.ClearFilter());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B", "ClearFilter shows the arrival"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a failed Filter followed by a Reset that inserts at the front keeps the rows in source order, and a later single Remove drops exactly that row.")]
        public void VerifyFailedFilterThenResetThenRemoveKeepsSourceOrder()
        {
            var rows = new ResettableCollection<LiveRow>(MakeLiveFlat());
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, null);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "No shaping"));

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => source.Filter(ThrowingForB(() => true))), "The Filter fails.");
                VerifyShaped(tableView, "A B C D", "A failed Filter keeps the empty spec's rows");

                var e = new LiveRow { Id = 5, Name = "E", Score = 50, Team = "X", Addr = new LiveAddress("Essen") };
                var next = new List<LiveRow> { e };
                next.AddRange(rows);
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => rows.ReplaceAll(next)), "The desired filter's exception surfaces from the Reset.");
                VerifyShaped(tableView, "A B C D", "Source order kept; the arrival is hidden");

                var c = rows[3];
                Verify.AreEqual("no exception", ThrownTypeName(() => rows.Remove(c)), "The Remove rebuilds with the applied (empty) spec.");
                VerifyShaped(tableView, "E A B D", "Exactly C left; the engine re-read everything");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "E A B D", "Still consistent after idle"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sort by path orders rows by the property path; calling it again on the same path replaces the level.")]
        public void VerifySortByPathOrdersRows()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source.Sort("Score", SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B A", "Score descending"));

            OnUI(tableView, () => source.Sort("Score", SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "Same path replaced, ascending"));

            OnUI(tableView, () => source.Sort("Missing.Path", SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "An unevaluable path gives null keys, keeping the higher level's order"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sort by key selector orders rows, keeps equal keys in source order, and appends lower-priority levels.")]
        public void VerifySortByKeySelectorOrdersRows()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source.Sort(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A C B D", "Team ascending, stable"));

            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C A D B", "Team, then Score descending"));

            OnUI(tableView, () => source.Sort(LiveByTeam, SortDirection.None));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B A", "Direction None removes the Team level"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sort with a comparer orders rows by the app comparer.")]
        public void VerifySortWithComparerOrdersRows()
        {
            var tableView = PlaceFlat(out var source, out _);
            var byScoreDescending = new TableViewComparer2((x, y) => ((LiveRow)y).Score.CompareTo(((LiveRow)x).Score));

            OnUI(tableView, () => source.Sort(byScoreDescending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B A", "Comparer: score descending"));

            OnUI(tableView, () => source.ClearSort());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "ClearSort"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies null sort keys come first ascending and last descending, equal (null) keys in source order.")]
        public void VerifySortNullKeysFirstAscending()
        {
            var tableView = PlaceFlat(out var source, out _);
            var scoreOrNull = LiveKey(r => r.Team == "Y" ? null : (object)r.Score);

            OnUI(tableView, () => source.Sort(scoreOrNull, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B D A C", "Nulls first ascending"));

            OnUI(tableView, () => source.Sort(scoreOrNull, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "C A B D", "Nulls last descending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupBy adds one header row per group (first-appearance order for None) and the header resolves through IndexOf.")]
        public void VerifyGroupByAddsHeaders()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.None));
            RunOnUIThread.Execute(() =>
            {
                VerifyShaped(tableView, "[H] A C [H] B D", "GroupBy Team");

                var view = GetRowsRepeater(tableView).ItemsSourceView;
                var header = view.GetAt(3);
                Verify.AreSame(header, view.GetAt(3), "A header's row value is stable across reads.");
                Verify.AreEqual(3, view.IndexOf(header), "A header resolves to its row through IndexOf.");
            });

            OnUI(tableView, () => source.Sort(LiveByScore, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C A [H] D B", "Sort orders rows inside each group, never the groups"));

            OnUI(tableView, () => source.Filter(MinScore(25)));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C [H] D", "Filter applies before grouping"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the GroupBy direction orders the groups by key.")]
        public void VerifyGroupDirectionOrdersGroups()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Descending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] B D [H] A C", "Groups descending"));

            OnUI(tableView, () => source.GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] A C [H] B D", "Groups ascending"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearGroupBy, ClearSort and ClearFilter restore the source order.")]
        public void VerifyClearVerbsRestoreSourceOrder()
        {
            var tableView = PlaceFlat(out var source, out _);

            OnUI(tableView, () => source
                .Filter(MinScore(15))
                .Sort(LiveByName, SortDirection.Descending)
                .GroupBy(LiveByTeam, SortDirection.Ascending));
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "[H] C [H] D B", "Filter, Sort and GroupBy"));

            OnUI(tableView, () => source.ClearGroupBy());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B", "ClearGroupBy"));

            OnUI(tableView, () => source.ClearSort());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "B C D", "ClearSort"));

            OnUI(tableView, () => source.ClearFilter());
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "A B C D", "ClearFilter"));
        }

        // AppliedSpec is native-only (no projection), so this checks what it promises through the
        // public surface: verbs chain on the same object, a verb that does not change the spec does
        // not reshape, and a failed apply leaves the shown (applied) shape in place.
        [TestMethod]
        [TestProperty("Description", "Verifies verbs chain on the same source, a no-op verb keeps the rows, and a failed apply keeps the applied shape.")]
        public void VerifyAppliedSpecIsReadOnlySnapshot()
        {
            var tableView = PlaceFlat(out var source, out _);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(source, source.Sort(LiveByScore, SortDirection.Descending), "Sort returns the same source.");
                Verify.AreSame(source, source.Filter(MinScore(15)), "Filter returns the same source.");
                Verify.AreSame(source, source.Sort("Name", SortDirection.None), "Removing a missing level returns the same source.");
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyShaped(tableView, "D C B", "Applied: Score descending, Score >= 15"));

            RunOnUIThread.Execute(() =>
            {
                var throwingKey = LiveKey(r => throw new InvalidOperationException("key failed"));
                Verify.AreEqual("InvalidOperationException", ThrownTypeName(() => source.Sort(throwingKey, SortDirection.Ascending)), "A sort key's exception must propagate.");
                VerifyShaped(tableView, "D C B", "A failed Sort keeps the applied shape");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Filter on a hierarchy keeps the ancestors of matching rows as expanded context rows.")]
        public void VerifyFilterInHierarchyKeepsAncestorsOfMatches()
        {
            var rows = MakeLiveTree();
            var source = FromOnUI(rows);
            var tableView = PlaceTreeSource(source, s => s.ParentBy(LiveById, LiveByParent));
            VerifyTreeOnUI(tableView, "Ann1+ Dee1+", "Before Filter");

            OnUI(tableView, () => source.Filter(new TableViewPredicate(o => ((LiveRow)o).Name == "Cat" || ((LiveRow)o).Name == "Eli")));
            VerifyTreeOnUI(tableView, "Ann1- Cat2 Dee1- Eli2", "Ancestors of matches shown expanded");

            OnUI(tableView, () => source.Filter(new TableViewPredicate(o => ((LiveRow)o).Name == "Bob")));
            VerifyTreeOnUI(tableView, "Ann1- Bob2", "Rows with no match below are hidden");

            OnUI(tableView, () => source.ClearFilter());
            VerifyTreeOnUI(tableView, "Ann1+ Dee1+", "ClearFilter restores the collapsed tree");
        }

        // The view model holds the source and its delegates close over the view model: a cycle
        // through app code that only tracker references let the GC collect.
        private sealed class ShapingViewModel
        {
            public TableViewSource2 Source;
            public int MinScore = 15;
            private readonly Dictionary<string, TeamKey> m_teams = new Dictionary<string, TeamKey>();
            public bool Keep(object o) => ((LiveRow)o).Score >= MinScore;
            public object Team(object o)
            {
                var team = ((LiveRow)o).Team;
                if (!m_teams.TryGetValue(team, out var key))
                {
                    key = new TeamKey { Owner = this, Team = team };
                    m_teams.Add(team, key);
                }
                return key;
            }
            public object Score(object o) => ((LiveRow)o).Score;
        }

        // A group key object that refers back to the view model: the group header must not hold it.
        private sealed class TeamKey
        {
            public ShapingViewModel Owner;
            public string Team;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the filter, sort and group delegates do not keep the source or their target alive once the table is gone.")]
        public void VerifyDelegatesDoNotLeakAfterSourceCollected()
        {
            WeakReference weakSource = null;
            WeakReference weakViewModel = null;

            RunOnUIThread.Execute(() => BindShapedViewModelAndForget(out weakSource, out weakViewModel));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Content = null);
            IdleSynchronizer.Wait();

            for (int attempt = 0; attempt < 5 && (weakSource.IsAlive || weakViewModel.IsAlive); attempt++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(weakSource.IsAlive, "The TableViewSource2 must be collected.");
            Verify.IsFalse(weakViewModel.IsAlive, "The view model its delegates close over must be collected.");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private void BindShapedViewModelAndForget(out WeakReference weakSource, out WeakReference weakViewModel)
        {
            var vm = new ShapingViewModel();
            vm.Source = TableViewSource2.From(MakeLiveFlat())
                .Filter(new TableViewPredicate(vm.Keep))
                .Sort(new TableViewKeySelector(vm.Score), SortDirection.Descending)
                .GroupBy(new TableViewKeySelector(vm.Team), SortDirection.None);
            var tableView = CreateTreeTable(vm.Source);
            LoadContent(tableView);
            var labels = ShapedLabels(tableView);
            Verify.AreEqual(2, labels.Split(' ').Count(p => p == "[H]"), $"Both group headers (object keys) are read: {labels}.");
            weakSource = new WeakReference(vm.Source);
            weakViewModel = new WeakReference(vm);
        }
    }
}
