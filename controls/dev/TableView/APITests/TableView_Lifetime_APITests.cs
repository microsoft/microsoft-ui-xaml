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
    // Each test keeps the would-be rooter (source, items, peers) alive; dropping both would prove nothing.
    [TestClass]
    public class TableViewLifetimeTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a TableView is collected after it is unloaded, while the items source it was bound to stays alive.")]
        public void VerifyTableViewIsCollectedWhileItsItemsSourceOutlivesIt()
        {
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadTable(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            Verify.AreEqual(40, items.Count, "The items source must still be alive after the table was collected.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableView bound through a shaped source with live sort-key tracking is still collected after unload.")]
        public void VerifyTableViewIsCollectedAfterShapedSourceWithLiveTracking()
        {
            var items = new ObservableCollection<NotifyingPerson>(
                Enumerable.Range(0, 40).Select(i => new NotifyingPerson { Name = $"Person {i}", Role = $"Role {i % 5}" }));

            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadShapedTable(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            // Raised on the UI thread so a surviving handler shows up as a leak, not a cross-thread error.
            RunOnUIThread.Execute(() => items[0].Role = "Changed");

            IdleSynchronizer.Wait();
            Verify.AreEqual(40, items.Count, "The items must still be alive after the table was collected.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies realizing automation peers for a TableView does not keep the control alive after unload.")]
        public void VerifyAutomationPeersDoNotKeepTheTableAlive()
        {
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateLoadAndWalkPeers(items, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            Verify.AreEqual(40, items.Count, "The items source must still be alive after the table was collected.");
        }

        // NoInlining so the JIT cannot keep the control alive in a stack slot; on the test class because LoadContent is protected.
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

        // Retried because a rooted object never becomes collectable, so retrying cannot hide a leak.
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
