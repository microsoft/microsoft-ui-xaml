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

        [TestMethod]
        [TestProperty("Description", "Verifies releasing a TableView whose rows were recycled into the template pools does not fail-fast, for both an unloaded table and one in a closed secondary window.")]
        public void VerifyReleasingTableViewWithRecycledRowsDoesNotCrash()
        {
            foreach (var inSecondaryWindow in new[] { false, true })
            {
                var tracked = new Dictionary<string, WeakReference>();
                TableView tableView = null;
                Window window = null;

                try
                {
                    RunOnUIThread.Execute(() =>
                    {
                        EnsureTabularControlsResources();
                        tableView = CreateTableView(MakeManyItems(40), headers: new[] { "Name", "Role" }, width: 400, height: 300);
                        tracked["TableView"] = new WeakReference(tableView);
                        if (inSecondaryWindow)
                        {
                            window = new Window() { Content = tableView };
                            window.Activate();
                        }
                        else
                        {
                            LoadContent(tableView);
                        }
                    });
                    SettleAfterLoad();

                    // Swapping the source recycles the realized rows into the template's pool.
                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsGreaterThan(GetRealizedRows(tableView).Count, 0, "Rows must be realized before the source swap.");
                        tableView.ItemsSource = MakeManyItems(40);
                        tableView.UpdateLayout();
                    });
                    SettleAfterLoad();

                    RunOnUIThread.Execute(() =>
                    {
                        Verify.IsTrue(RowTemplateIsPooled(tableView), "The row template must carry a RecyclePool so Detach has a pool edge to break.");

                        tableView = null;
                        if (inSecondaryWindow)
                        {
                            window.Close();
                            window = null;
                        }
                        else
                        {
                            Content = null;
                        }
                    });

                    AssertCollected(tracked);
                }
                finally
                {
                    RunOnUIThread.Execute(() =>
                    {
                        window?.Close();
                        MUXControlsTestApp.App.CurrentWindow.Activate();
                    });
                }

                Log.Comment($"Released a TableView with pooled row templates (secondary window: {inSecondaryWindow}) without crashing.");
            }
        }

        // NoInlining so the selector and template do not linger in a stack slot past the call.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool RowTemplateIsPooled(TableView tableView)
        {
            var repeater = (Microsoft.UI.Xaml.Controls.ItemsRepeater)tableView.FindVisualChildByName("PART_RowsRepeater");
            var selector = (Microsoft.UI.Xaml.Controls.DataTemplateSelector)repeater.ItemTemplate;
            var rowTemplate = selector.SelectTemplate(((IList<Person>)tableView.ItemsSource)[0]);
            return rowTemplate != null && Microsoft.UI.Xaml.Controls.RecyclePool.GetPoolInstance(rowTemplate) != null;
        }
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

    // Bug 64723090: ~TableView -> TableViewRowTemplateSelector::Detach -> RecyclePool.SetPoolInstance throws when
    // SetValue fails because XAML on the thread is shutting down, and the throw out of the destructor fail-fasts.
    // Not derived from ApiTestBase and isolated: shutting XAML down on a second thread leaves the host's
    // IdleSynchronizer unable to report idle, so the shared Setup/Cleanup would hang.
    [TestClass]
    public class TableViewShutdownTests
    {
        // App-lifetime strong reference, like an app view model holding the control past ShutdownAllPeers.
        private static TableView s_keepAlive;

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Description", "Verifies releasing a TableView whose row templates are pooled, after XAML shut down on its thread, does not fail-fast.")]
        public void VerifyTableViewReleasedAfterXamlShutdownOnItsThreadDoesNotCrash()
        {
            Exception failure = null;
            bool templatesPooled = false;

            var thread = new global::System.Threading.Thread(() =>
            {
                try
                {
                    var controller = Microsoft.UI.Dispatching.DispatcherQueueController.CreateOnCurrentThread();
                    var xaml = Microsoft.UI.Xaml.Hosting.WindowsXamlManager.InitializeForCurrentThread();

                    templatesPooled = ShowTableViewAndRunUntilExit(controller.DispatcherQueue);

                    // Leave only XAML's own references and s_keepAlive.
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();

                    // Window stays open: FrameworkShutdownStarting closes it, then ShutdownAllPeers runs
                    // and the thread's DXamlCore goes away while s_keepAlive still holds the TableView.
                    controller.ShutdownQueue();
                    xaml.Dispose();

                    // Final release on the shut-down thread runs ~TableView -> Detach -> SetValue, which fails.
                    var tableView = s_keepAlive;
                    s_keepAlive = null;
                    ((WinRT.IWinRTObject)tableView).NativeObject.Dispose();
                }
                catch (Exception ex)
                {
                    failure = ex;
                }
            });
            thread.SetApartmentState(global::System.Threading.ApartmentState.STA);
            thread.Start();

            Verify.IsTrue(thread.Join(TimeSpan.FromSeconds(60)), "The secondary UI thread should finish shutting down XAML.");
            Verify.IsNull(failure, $"Secondary UI thread failed: {failure}");
            Verify.IsTrue(templatesPooled, "The row template must carry a RecyclePool so Detach has a pool edge to break.");

            Log.Comment("Releasing the TableView after XAML shutdown completed without crashing.");
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static bool ShowTableViewAndRunUntilExit(Microsoft.UI.Dispatching.DispatcherQueue dispatcher)
        {
            bool templatesPooled = false;
            var items = MakeManyItems(30);
            var tableView = CreateTableView(items, headers: new[] { "Name", "Role" });
            var window = new Window() { Content = tableView };
            s_keepAlive = tableView;

            tableView.Loaded += (sender, args) =>
            {
                tableView.UpdateLayout();

                // Swap the source so realized rows are recycled into the template's pool.
                var replacement = MakeManyItems(30);
                tableView.ItemsSource = replacement;
                tableView.UpdateLayout();

                dispatcher.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
                {
                    try
                    {
                        var repeater = (Microsoft.UI.Xaml.Controls.ItemsRepeater)tableView.FindVisualChildByName("PART_RowsRepeater");
                        var selector = (Microsoft.UI.Xaml.Controls.DataTemplateSelector)repeater.ItemTemplate;
                        var rowTemplate = selector.SelectTemplate(replacement[0]);
                        templatesPooled = rowTemplate != null && Microsoft.UI.Xaml.Controls.RecyclePool.GetPoolInstance(rowTemplate) != null;
                        Log.Comment($"Realized rows: {GetRealizedRows(tableView).Count}; row template pooled: {templatesPooled}.");
                    }
                    finally
                    {
                        dispatcher.EnqueueEventLoopExit();
                    }
                });
            };

            window.Activate();
            dispatcher.RunEventLoop();
            return templatesPooled;
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
