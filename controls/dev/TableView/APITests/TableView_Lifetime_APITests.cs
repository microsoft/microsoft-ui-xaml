// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewLifetimeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 18 of the TableView test plan: lifetime.
    //
    // WHAT IS ALREADY COVERED, AND WHY IT IS NOT THIS.
    // TableViewTests.cs has two teardown tests. VerifyUnloadWhileItemsSourcePendingDoesNotCrash
    // asserts the control does not CRASH when torn down mid-layout, and the load/unload cycle test
    // asserts the subscription count on the source does not GROW. Neither asserts that the control is
    // ever COLLECTED. A TableView that is rooted forever passes both: it does not crash, and it holds
    // exactly one subscription while doing so.
    //
    // WHY TABLEVIEW SPECIFICALLY. The control holds more cross-object state than its peers, and most
    // of it points at objects that outlive it - the app's items source, the app's data items, and the
    // automation tree:
    //   * a collection-changed subscription on the items source (TableView.h:479-489)
    //   * per-row property-changed subscriptions for sort-key tracking, added by #12190
    //   * a cached group index, cached automation peers, and focus tracking
    // Every one of those is a handle held by something the control does not own, which is the shape of
    // a leak: not the control keeping a child alive, but a survivor keeping the CONTROL alive.
    //
    // THE ASSERTION DIRECTION MATTERS. Each test deliberately lets the thing that would do the rooting
    // outlive the table - the source, or the items - and then asserts the TABLE was collected. A test
    // that drops both at once proves nothing, because the whole graph becomes unreachable together
    // regardless of who referenced whom.
    //
    // FLAKINESS. LeakTests.cs carries two [Ignore]d tests, so the known failure mode here is a false
    // RED, not a false green. Three mitigations, all from that file's working tests: objects are
    // created inside a [MethodImpl(NoInlining)] helper so the JIT cannot extend a local's lifetime
    // past its last use; collection is retried rather than attempted once; and every UI object is
    // created and released on the UI thread. The retry only ever converts a slow collection into a
    // pass - it cannot mask a real leak, because a rooted object never becomes collectable.
    [TestClass]
    public class TableViewLifetimeTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a TableView is collected after it is unloaded, while the items source it was bound to stays alive.")]
        public void VerifyTableViewIsCollectedWhileItsItemsSourceOutlivesIt()
        {
            // The source is held by the TEST for the whole test, so it is still rooted when collection
            // runs. If the control's collection-changed subscription is not released on unload, the
            // source roots the control and this fails - which is the real-world shape, since an app
            // typically outlives any one page.
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadTable(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            // Proves the source was genuinely still reachable throughout, so the assertion above was a
            // real test of the subscription rather than of a graph that had already gone away.
            Verify.AreEqual(40, items.Count, "The items source must still be alive after the table was collected.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableView bound through a shaped source with live sort-key tracking is still collected after unload.")]
        public void VerifyTableViewIsCollectedAfterShapedSourceWithLiveTracking()
        {
            // #12190 added per-row INotifyPropertyChanged subscriptions so a row whose sort key changes
            // re-sorts itself. Each subscription is a handle the ITEM holds on the control's handler,
            // and the items outlive the table here. Nothing else in the suite proves those are released.
            var items = new ObservableCollection<NotifyingPerson>(
                Enumerable.Range(0, 40).Select(i => new NotifyingPerson { Name = $"Person {i}", Role = $"Role {i % 5}" }));

            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadShapedTable(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            // A property change after teardown must reach nothing. If a stale handler survived, this is
            // where it would run against a collected control. Raised on the UI thread so that a
            // surviving handler fails as a leak rather than as a cross-thread error.
            RunOnUIThread.Execute(() => items[0].Role = "Changed");

            IdleSynchronizer.Wait();
            Verify.AreEqual(40, items.Count, "The items must still be alive after the table was collected.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies realizing automation peers for a TableView does not keep the control alive after unload.")]
        public void VerifyAutomationPeersDoNotKeepTheTableAlive()
        {
            // #11820 cached peers that outlived their containers. A peer holds its owner, so a cache
            // that is not cleared on unload roots the whole control - and an AT having visited the
            // table once is enough to populate it.
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateLoadAndWalkPeers(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            Verify.AreEqual(40, items.Count, "The items source must still be alive after the table was collected.");
        }

        // Each creation helper is NoInlining so the control never becomes a local of the calling test
        // method. An inlined local can stay reachable from a stack slot the JIT has not yet reused,
        // which is the classic source of a false leak report. They live on the test class rather than
        // in the helper class because LoadContent is protected on TableViewApiTestBase.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private void CreateAndLoadTable(object itemsSource, Dictionary<string, WeakReference> tracked)
        {
            var tableView = CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 400, height: 300);
            LoadContent(tableView);

            tracked["TableView"] = new WeakReference(tableView);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private void CreateAndLoadShapedTable(object itemsSource, Dictionary<string, WeakReference> tracked)
        {
            var source = TableViewSource.From(itemsSource);
            source.Sort("Name", SortDirection.Ascending);

            var tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 400, height: 300);
            LoadContent(tableView);

            tracked["TableView"] = new WeakReference(tableView);
            tracked["TableViewSource"] = new WeakReference(source);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private void CreateLoadAndWalkPeers(object itemsSource, Dictionary<string, WeakReference> tracked)
        {
            var tableView = CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 400, height: 300);
            LoadContent(tableView);

            // Realize the peer tree the way a UIA client would, so the control's peer caches are
            // populated before teardown. Deliberately not retained: the test is whether the CONTROL
            // survives, not whether a peer the test is still holding does.
            foreach (var row in GetRealizedRows(tableView))
            {
                var rowPeer = GetRowPeer(row);
                rowPeer?.GetName();
                GetCellPeer(rowPeer, 0)?.GetName();
            }

            tracked["TableView"] = new WeakReference(tableView);
        }
    }

    internal sealed class NotifyingPerson : INotifyPropertyChanged
    {
        private string _name;
        private string _role;

        public event PropertyChangedEventHandler PropertyChanged;

        public string Name
        {
            get => _name;
            set { _name = value; Raise(nameof(Name)); }
        }

        public string Role
        {
            get => _role;
            set { _role = value; Raise(nameof(Role)); }
        }

        private void Raise(string property) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(property));
    }

    internal static class TableViewLifetimeTestHelpers
    {
        internal static void SettleAfterLoad()
        {
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => { });

            IdleSynchronizer.Wait();
        }

        // Retried rather than attempted once. A finalizer queue that has not drained yet, or a
        // dispatcher callback still holding the last reference, resolves on a later pass; a genuinely
        // rooted object never does, so retrying cannot hide a leak.
        private const int CollectionAttempts = 5;

        internal static void AssertCollected(Dictionary<string, WeakReference> tracked)
        {
            Verify.IsGreaterThan(tracked.Count, 0, "The fixture must have tracked at least one object.");

            var alive = new List<string>();

            for (var attempt = 1; attempt <= CollectionAttempts; attempt++)
            {
                IdleSynchronizer.Wait();

                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                IdleSynchronizer.Wait();

                alive = tracked.Where(pair => pair.Value.IsAlive).Select(pair => pair.Key).ToList();

                if (alive.Count == 0)
                {
                    Log.Comment($"All tracked objects collected after {attempt} attempt(s).");
                    return;
                }

                Log.Comment($"Attempt {attempt}: still alive - {string.Join(", ", alive)}.");
            }

            Verify.Fail(
                $"These objects were still alive after {CollectionAttempts} collection attempts: {string.Join(", ", alive)}. " +
                "Something that outlived the control is still holding it - most likely a subscription or peer cache that is not released on unload.");
        }
    }
}
