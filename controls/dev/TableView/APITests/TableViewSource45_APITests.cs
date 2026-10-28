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
    // Smoke tests for the ChildrenWith benchmark variants: v4 (structure cache) and v5 (eager index,
    // coalescing, incremental filter and adapter). Each is reached through the registry, as the
    // benchmark does. The tree is MakeTree's: Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus, with
    // the roots as the source and each node's reports returned by ChildrenWith.
    [TestClass]
    public class TableViewSource45Tests : TableViewApiTestBase
    {
        private const string Roots = "Ada1+ Eve1+ Gus1";
        private const string FullTree = "Ada1- Ben2- Dan3 Cy2 Eve1- Fay2 Gus1";
        private static readonly TableViewIdentitySelector PersonKey = o => Person(o).Name;

        [TestMethod]
        [TestProperty("Description", "Verifies the registry creates and lists v4 and v5.")]
        public void VerifyRegistryCreatesV4AndV5()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(TableViewSourceRegistry.Variants.Contains("v4"), "Variants must list v4.");
                Verify.IsTrue(TableViewSourceRegistry.Variants.Contains("v5"), "Variants must list v5.");
                Verify.IsTrue(TableViewSourceRegistry.Create("v4", MakeShapedPeople()) is TableViewSource4, "v4 is a TableViewSource4.");
                Verify.IsTrue(TableViewSourceRegistry.Create("v5", MakeShapedPeople()) is TableViewSource5, "v5 is a TableViewSource5.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row key and ParentBy throw E_NOTIMPL on v4 and v5.")]
        public void VerifyUnsupportedVerbsThrowNotImplemented()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var variant in new[] { "v4", "v5" })
                {
                    Verify.Throws<NotImplementedException>(() => TableViewSourceRegistry.Create(variant, MakeShapedPeople(), PersonKey),
                        variant + " does not support a row key.");
                    var source = TableViewSourceRegistry.Create(variant, MakeTree());
                    Verify.Throws<NotImplementedException>(() => source.ParentBy(ById, ByManager), variant + " does not support ParentBy.");
                    Verify.Throws<ArgumentException>(() => source.ChildrenWith(null), variant + " rejects a null children selector.");
                }
            });
        }

        [TestMethod]
        public void VerifyFilterAndSortV4() => VerifyFilterAndSort("v4");

        [TestMethod]
        public void VerifyFilterAndSortV5() => VerifyFilterAndSort("v5");

        [TestMethod]
        public void VerifyChildrenWithExpandCollapseV4() => VerifyChildrenWithExpandCollapse("v4");

        [TestMethod]
        public void VerifyChildrenWithExpandCollapseV5() => VerifyChildrenWithExpandCollapse("v5");

        [TestMethod]
        public void VerifyChildrenWithFilterKeepsAncestorsV4() => VerifyChildrenWithFilterKeepsAncestors("v4");

        [TestMethod]
        public void VerifyChildrenWithFilterKeepsAncestorsV5() => VerifyChildrenWithFilterKeepsAncestors("v5");

        private void VerifyFilterAndSort(string variant)
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSourceRegistry.Create(variant, MakeShapedPeople())
                    .Filter(o => Person(o).Role == "Engineer")
                    .Sort("Name", SortDirection.Descending);
                tableView = CreateShapedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Diego" }, variant + " filter + sort descending");
                source.ClearFilter();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new List<string> { "Rafa", "Owen", "Mei", "Ines", "Diego", "Asha" }, variant + " filter cleared");
                source.ClearSort();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, MakeShapedPeople().Select(p => p.Name).ToList(), variant + " sort cleared"));
        }

        private void VerifyChildrenWithExpandCollapse(string variant)
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTreeTable(ChildrenTree(variant));
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, Roots, variant + " first projection");
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, FullTree, variant + " expanded");
                tableView.CollapseAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, Roots, variant + " collapsed again"));
        }

        private void VerifyChildrenWithFilterKeepsAncestors(string variant)
        {
            TableView tableView = null;
            ITableViewShapingSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = ChildrenTree(variant);
                tableView = CreateTreeTable(source);
                LoadContent(tableView);
                tableView.ExpandAllRows();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => source.Filter(o => ((TreeEmployee)o).Name == "Dan"));

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyTree(tableView, "Ada1- Ben2- Dan3", variant + " filtered to Dan");
                source.ClearFilter();
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyTree(tableView, FullTree, variant + " filter cleared"));
        }

        // The roots as the source; each node's reports as its children.
        private static ITableViewShapingSource ChildrenTree(string variant)
        {
            var all = MakeTree();
            var roots = new ObservableCollection<TreeEmployee>(all.Where(e => !all.Any(m => m.Id == e.ManagerId)));
            var reports = all.ToDictionary(
                e => e.Id,
                e => new ObservableCollection<TreeEmployee>(all.Where(c => c.ManagerId == e.Id)));
            return TableViewSourceRegistry.Create(variant, roots)
                .ChildrenWith(o => reports[((TreeEmployee)o).Id], o => reports[((TreeEmployee)o).Id].Count > 0);
        }
    }
}
