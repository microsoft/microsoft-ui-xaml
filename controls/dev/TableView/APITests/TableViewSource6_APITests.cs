// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Smoke tests for the v6 benchmark variant: object identity, no flattened row array, GetAt resolved
    // by walking the wrapper tree with per-sibling-list visible counts. Reached through the registry,
    // as the benchmark does. The tree is MakeTree's: Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus.
    [TestClass]
    public class TableViewSource6Tests : TableViewApiTestBase
    {
        private const string V6 = "v6";
        private const string Roots = "Ada1+ Eve1+ Gus1";
        private const string FullTree = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";
        private static readonly TableViewIdentitySelector PersonKey = o => Person(o).Name;

        [TestMethod]
        [TestProperty("Description", "Verifies the registry creates and lists v6.")]
        public void VerifyRegistryCreatesV6()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(TableViewSourceRegistry.Variants.Contains(V6), "Variants must list v6.");
                Verify.IsTrue(TableViewSourceRegistry.Create(V6, MakeShapedPeople()) is TableViewSource6, "v6 is a TableViewSource6.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row key and ParentBy throw E_NOTIMPL on v6.")]
        public void VerifyUnsupportedVerbsThrowNotImplemented()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<NotImplementedException>(() => TableViewSourceRegistry.Create(V6, MakeShapedPeople(), PersonKey),
                    "v6 does not support a row key.");
                var source = TableViewSourceRegistry.Create(V6, MakeTree());
                Verify.Throws<NotImplementedException>(() => source.ParentBy(ById, ByManager), "v6 does not support ParentBy.");
                Verify.Throws<ArgumentException>(() => source.ChildrenWith(null), "v6 rejects a null children selector.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filter and sort, then clearing each, on v6.")]
        public void VerifyFilterAndSort()
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSourceRegistry.Create(V6, MakeShapedPeople())
                    .Filter(o => Person(o).Role == "Engineer")
                    .Sort("Name", SortDirection.Descending);
                tableView = CreateShapedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Diego" }, "filter + sort descending");
                source.ClearFilter();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Mei", "Ines", "Diego", "Asha" }, "filter cleared");
                source.ClearSort();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, MakeShapedPeople().Select(p => p.Name).ToList(), "sort cleared"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies flat inserts and removes project incrementally on v6.")]
        public void VerifyFlatInsertAndRemove()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateShapedTable(TableViewSourceRegistry.Create(V6, items));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                items.Insert(2, new ShapedPerson("Nadia", "Engineer", "Platform"));
                items.RemoveAt(0);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, new List<string> { "Diego", "Nadia", "Mei", "Rafa", "Ines", "Owen" }, "after insert and remove"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupBy headers and that adds and removes create and prune groups on v6.")]
        public void VerifyGroupByAddsAndPrunesGroups()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateShapedTable(TableViewSourceRegistry.Create(V6, items).GroupBy(o => Person(o).Role));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(
                    tableView,
                    new List<string>
                    {
                        "#Designer(2)", "Asha", "Ines",
                        "#Engineer(3)", "Diego", "Rafa", "Owen",
                        "#Architect(1)", "Mei",
                    },
                    "grouped by Role");
                items.Add(new ShapedPerson("Nadia", "Engineer", "Platform"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(
                    tableView,
                    new List<string>
                    {
                        "#Designer(2)", "Asha", "Ines",
                        "#Engineer(4)", "Diego", "Rafa", "Owen", "Nadia",
                        "#Architect(1)", "Mei",
                    },
                    "after adding to an existing group");
                items.Remove(items.First(p => p.Name == "Mei"));
                items.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(
                    tableView,
                    new List<string>
                    {
                        "#Designer(2)", "Asha", "Ines",
                        "#Engineer(4)", "Diego", "Rafa", "Owen", "Nadia",
                        "#Manager(1)", "Zoe",
                    },
                    "after emptying one group and creating another"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ChildrenWith expand-all and collapse-all on v6.")]
        public void VerifyChildrenWithExpandCollapse()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTreeTable(ChildrenTree(out _));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, "first projection");
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "expanded");
                tableView.CollapseAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, "collapsed again"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ChildrenWith filter keeps the ancestors of matches on v6.")]
        public void VerifyChildrenWithFilterKeepsAncestors()
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = ChildrenTree(out _);
                tableView = CreateTreeTable(source);
                LoadContent(tableView);
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => source.Filter(o => ((TreeEmployee)o).Name == "Dan"));

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2- Dan3", "filtered to Dan");
                source.ClearFilter();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, FullTree, "filter cleared"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changes to an expanded node's children collection project incrementally on v6.")]
        public void VerifyChildrenWithChildCollectionChanges()
        {
            TableView tableView = null;
            Dictionary<int, ObservableCollection<TreeEmployee>> reports = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTreeTable(ChildrenTree(out reports));
                LoadContent(tableView);
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "expanded");
                var ada = reports.First(p => p.Value.Any(e => e.Name == "Cy")).Value;
                ada.Remove(ada.First(e => e.Name == "Cy"));
                var eve = reports.First(p => p.Value.Any(e => e.Name == "Fay")).Value;
                eve.Remove(eve.First(e => e.Name == "Fay"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyTree(tableView, "Ada1- Ben2- Dan3 Eve1 Gus1", "after removing Cy and Fay"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Reset of the root collection keeps surviving rows and their expansion on v6.")]
        public void VerifyRootResetKeepsSurvivors()
        {
            TableView tableView = null;
            ResettableCollection<TreeEmployee> roots = null;
            List<TreeEmployee> original = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTreeTable(ChildrenTree(out _, out roots));
                original = roots.ToList();
                LoadContent(tableView);
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => roots.ResetTo(original));

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, "after a Reset to the same roots");
                roots.ResetTo(original.Where(e => e.Name != "Gus"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2", "after a Reset without Gus");
                roots.ResetTo(original.Where(e => e.Name != "Eve"));
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyTree(tableView, "Ada1- Ben2- Dan3 Cy2 Gus1", "after a Reset that swaps Eve for Gus"));
        }

        private sealed class ResettableCollection<T> : ObservableCollection<T>
        {
            public ResettableCollection(IEnumerable<T> items) : base(items) { }

            public void ResetTo(IEnumerable<T> items)
            {
                var list = items.ToList();
                Items.Clear();
                foreach (var item in list)
                {
                    Items.Add(item);
                }
                OnCollectionChanged(new global::System.Collections.Specialized.NotifyCollectionChangedEventArgs(
                    global::System.Collections.Specialized.NotifyCollectionChangedAction.Reset));
            }
        }

        private static ITableViewShapingSource ChildrenTree(out Dictionary<int, ObservableCollection<TreeEmployee>> reports)
            => ChildrenTree(out reports, out _);

        // The roots as the source; each node's reports as its children.
        private static ITableViewShapingSource ChildrenTree(out Dictionary<int, ObservableCollection<TreeEmployee>> reports, out ResettableCollection<TreeEmployee> rootItems)
        {
            var all = MakeTree();
            var roots = new ResettableCollection<TreeEmployee>(all.Where(e => !all.Any(m => m.Id == e.ManagerId)));
            rootItems = roots;
            var map = all.ToDictionary(
                e => e.Id,
                e => new ObservableCollection<TreeEmployee>(all.Where(c => c.ManagerId == e.Id)));
            reports = map;
            return TableViewSourceRegistry.Create(V6, roots)
                .ChildrenWith(o => map[((TreeEmployee)o).Id], o => map[((TreeEmployee)o).Id].Count > 0);
        }
    }
}
