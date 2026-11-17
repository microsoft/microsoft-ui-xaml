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
        [TestProperty("Description", "Verifies a TableViewSource whose shaping delegates capture the source is collected after its TableView unloads (GitHub #12474).")]
        [TestProperty("data:Shaping", "{GroupBy, GroupByWithIdentity, Sort, Filter, ParentBy}")]
        public void VerifyShapedSourceIsCollectedWhenItsDelegatesCaptureIt()
        {
            var shaping = ((string)TestContext.DataRow["Shaping"]).Trim();
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadSelfCapturingShapedTable(items, shaping, tracked));

            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);

            AssertCollected(tracked);

            Verify.AreEqual(40, items.Count, "The items must still be alive after the source was collected.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cleared or replaced shaping delegate is released while its TableViewSource stays alive.")]
        [TestProperty("data:Change", "{ClearGroupBy, ReplaceGroupBy, ClearFilter, ReplaceFilter, ClearSort, RemoveSort}")]
        public void VerifyClearedOrReplacedDelegateIsReleasedWhileTheSourceLives()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();
            TableViewSource source = null;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(items);
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 400, height: 300);
                LoadContent(tableView);
                tracked["Delegate"] = DeclareThenClearOrReplace(source, change);
                tableView.UpdateLayout();
            });

            SettleAfterLoad();

            AssertCollected(tracked);

            // The replacement delegates must still shape a new item after the GC.
            RunOnUIThread.Execute(() =>
            {
                items.Add(new Person { Name = "Person 40", Role = "Excluded" });
                items.Add(new Person { Name = "Person 41", Role = "Role 41" });
            });
            SettleAfterLoad();

            RunOnUIThread.Execute(() =>
            {
                var projected = GetProjectedItems(tableView);
                switch (change)
                {
                    case "ReplaceGroupBy":
                        Verify.AreEqual(42 * 2, projected.Count, "Grouped by Name, every item must head its own group.");
                        Verify.AreEqual(42, projected.OfType<Person>().Count(), "Every item, including the new one, must project once.");
                        break;
                    case "ReplaceFilter":
                        Verify.AreEqual(41, projected.Count, "The replacement predicate must exclude the new Excluded item.");
                        Verify.IsFalse(projected.OfType<Person>().Any(p => p.Role == "Excluded"), "The Excluded item must be filtered out.");
                        break;
                    default:
                        Verify.AreEqual(42, projected.Count, "With the shaping cleared, every item must project as a plain row.");
                        break;
                }
            });
            GC.KeepAlive(source);
            GC.KeepAlive(tableView);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource whose shaping delegates capture it keeps shaping across GCs, and after an unload and a reload into a new TableView.")]
        [TestProperty("data:Shaping", "{GroupBy, GroupByWithIdentity, Sort, Filter, ParentBy}")]
        public void VerifyShapedSourceKeepsShapingAcrossUnloadReloadAndCollection()
        {
            var shaping = ((string)TestContext.DataRow["Shaping"]).Trim();
            var grouped = shaping.StartsWith("GroupBy");
            var items = new ObservableCollection<Person>(
                new[] { "A", "B", "A", "B", "C", "A" }.Select((role, i) => new Person { Name = $"Person {i}", Role = role }));
            // GroupBy: 3 headers and 6 rows. Filter drops C; ParentBy nests C, collapsed, under Person 0.
            var shapedCount = grouped ? 6 + 3 : shaping == "Sort" ? 6 : 5;
            var firstTable = new Dictionary<string, WeakReference>();
            TableViewSource source = null;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                source = CreateSelfCapturingShapedSource(items, shaping);
                firstTable["TableView"] = LoadInNewTable(source);
            });
            SettleAfterLoad();

            RunOnUIThread.Execute(() => Content = null);
            AssertCollected(firstTable);

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 400, height: 600);
                LoadContent(tableView);
            });
            SettleAfterLoad();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(shapedCount, GetProjectedCount(tableView), "A reloaded source must still be shaped.");
                // From here only the loaded table keeps the source and its delegates alive.
                source = null;
            });

            CollectGarbage();

            RunOnUIThread.Execute(() =>
            {
                items.Add(new Person { Name = "Person 6", Role = "D" });
                // Filtered out, or a collapsed child of Person 0 under ParentBy.
                items.Add(new Person { Name = "Person 7", Role = "C" });
            });
            SettleAfterLoad();

            RunOnUIThread.Execute(() =>
            {
                switch (shaping)
                {
                    case "Sort":
                        Verify.AreEqual(shapedCount + 2, GetProjectedCount(tableView), "Both new items must project.");
                        Verify.AreEqual("D", ((Person)GetProjectedItem(tableView, 0)).Role, "The key selector must still sort the new D item first, descending.");
                        break;
                    case "Filter":
                    case "ParentBy":
                        Verify.AreEqual(shapedCount + 1, GetProjectedCount(tableView), "Only the new D item may project as a new row.");
                        Verify.IsFalse(GetProjectedItems(tableView).OfType<Person>().Any(p => p.Name == "Person 7"), "The delegates must still keep Person 7 out of the projection.");
                        break;
                    default:
                        Verify.AreEqual(shapedCount + 2 + 1, GetProjectedCount(tableView), "The key selector must still bucket the new items: one into C, one into a new group D.");
                        var keys = GetProjectedElements(tableView).OfType<TableViewGroupHeader>().Select(h => (h.Content as TableViewGroupInfo)?.Key as string).ToList();
                        Verify.IsTrue(keys.Contains("D"), $"The new item must start group D; groups: {string.Join(", ", keys)}.");
                        break;
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collection change that reaches a shaped source after the GC collected its delegates, but before the source's posted delete runs, does not throw into the app's collection call.")]
        [TestProperty("data:Shaping", "{GroupBy, GroupByWithIdentity, ParentBy}")]
        public void VerifyCollectionChangeOnCollectedSourceDoesNotThrow()
        {
            var shaping = ((string)TestContext.DataRow["Shaping"]).Trim();
            var items = new ObservableCollection<Person>(MakeManyItems(40));
            var tracked = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() => CreateAndLoadSelfCapturingShapedTable(items, shaping, tracked));
            SettleAfterLoad();
            RunOnUIThread.Execute(() => Content = null);
            SettleAfterLoad();

            var collected = false;
            Exception thrown = null;
            RunOnUIThread.Execute(() =>
            {
                // Collected on the UI thread, so the delete the finalizer posts here cannot run before the Add.
                for (var attempt = 0; attempt < 5 && !collected; attempt++)
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                    collected = !tracked["KeySelector"].IsAlive;
                }

                if (collected)
                {
                    try
                    {
                        items.Add(new Person { Name = "Person 40", Role = "Role 40" });
                    }
                    catch (Exception e)
                    {
                        thrown = e;
                    }
                }
            });

            Verify.IsTrue(collected, "Precondition: the key selector was collected while the source was still subscribed.");
            Verify.IsNull(thrown, $"A collection change must not throw into the app once the source is obsolete: {thrown}");
            Verify.AreEqual(41, items.Count, "The app's Add must complete.");
            AssertCollected(tracked);
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

        // Each delegate captures the source, as an app closure that also uses the source does.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private void CreateAndLoadSelfCapturingShapedTable(object itemsSource, string shaping, Dictionary<string, WeakReference> tracked)
        {
            TableViewSource source = null;
            source = TableViewSource.From(itemsSource);
            var key = new TableViewKeySelector(item => source == null ? null : ((Person)item).Role);
            switch (shaping)
            {
                case "GroupBy": source.GroupBy(key); break;
                case "GroupByWithIdentity":
                    var identity = new TableViewIdentitySelector(k => source == null ? null : (string)k);
                    tracked["IdentitySelector"] = new WeakReference(identity);
                    source.GroupBy(key, identity);
                    break;
                case "Sort": source.Sort(key, SortDirection.Descending); break;
                case "Filter":
                    var predicate = new TableViewPredicate(item => source != null);
                    tracked["Predicate"] = new WeakReference(predicate);
                    source.Filter(predicate);
                    break;
                case "ParentBy":
                    var parentKey = new TableViewKeySelector(item => source == null ? "" : null);
                    tracked["ParentKeySelector"] = new WeakReference(parentKey);
                    source.ParentBy(key, parentKey);
                    break;
            }

            var tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 400, height: 300);
            LoadContent(tableView);
            tableView.UpdateLayout();
            Verify.IsGreaterThan(GetRealizedRows(tableView).Count, 0, "The shaped source must project rows.");

            tracked["TableView"] = new WeakReference(tableView);
            tracked["TableViewSource"] = new WeakReference(source);
            if (shaping != "Filter")
            {
                tracked["KeySelector"] = new WeakReference(key);
            }
        }

        // Captures 'change' so the compiler does not cache the delegates in static fields.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static WeakReference DeclareThenClearOrReplace(TableViewSource source, string change)
        {
            var key = new TableViewKeySelector(item => change == null ? null : ((Person)item).Role);
            var predicate = new TableViewPredicate(item => change != null);
            switch (change)
            {
                case "ClearGroupBy": source.GroupBy(key).ClearGroupBy(); break;
                case "ReplaceGroupBy": source.GroupBy(key).GroupBy(item => change == null ? null : ((Person)item).Name); break;
                case "ClearFilter": source.Filter(predicate).ClearFilter(); return new WeakReference(predicate);
                case "ReplaceFilter": source.Filter(predicate).Filter(item => change != null && ((Person)item).Role != "Excluded"); return new WeakReference(predicate);
                case "ClearSort": source.Sort(key, SortDirection.Ascending).ClearSort(); break;
                case "RemoveSort": source.Sort(key, SortDirection.Ascending).Sort(key, SortDirection.None); break;
            }

            return new WeakReference(key);
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private static TableViewSource CreateSelfCapturingShapedSource(object itemsSource, string shaping)
        {
            TableViewSource source = null;
            source = TableViewSource.From(itemsSource);
            TableViewKeySelector role = item => source == null ? null : ((Person)item).Role;
            switch (shaping)
            {
                case "GroupBy": source.GroupBy(role); break;
                case "GroupByWithIdentity": source.GroupBy(role, k => source == null ? null : (string)k); break;
                case "Sort": source.Sort(role, SortDirection.Descending); break;
                case "Filter": source.Filter(item => source != null && ((Person)item).Role != "C"); break;
                case "ParentBy":
                    source.ParentBy(
                        item => source == null ? null : ((Person)item).Name,
                        item => source == null || ((Person)item).Role != "C" ? null : "Person 0");
                    break;
            }

            return source;
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private WeakReference LoadInNewTable(TableViewSource source)
        {
            var tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 400, height: 600);
            LoadContent(tableView);
            return new WeakReference(tableView);
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

    // Isolated, not ApiTestBase: after XAML shuts down on a second thread the shared Setup/Cleanup would hang.
    [TestClass]
    public class TableViewShutdownTests
    {
        // Holds the control past ShutdownAllPeers, like an app view model would.
        private static TableView s_keepAlive;
        private static DataTemplate s_rowTemplate;

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Description", "Verifies releasing a TableView whose row templates are pooled, after XAML shut down on its thread, does not fail-fast.")]
        public void VerifyTableViewReleasedAfterXamlShutdownOnItsThreadDoesNotCrash()
        {
            Exception failure = null;
            Exception clearPoolFailure = null;
            bool templatesPooled = false;

            var thread = new global::System.Threading.Thread(() =>
            {
                try
                {
                    var controller = Microsoft.UI.Dispatching.DispatcherQueueController.CreateOnCurrentThread();
                    var xaml = Microsoft.UI.Xaml.Hosting.WindowsXamlManager.InitializeForCurrentThread();

                    templatesPooled = ShowTableViewAndRunUntilExit(controller.DispatcherQueue);

                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();

                    // Window stays open, so shutdown runs while s_keepAlive still holds the TableView.
                    controller.ShutdownQueue();
                    xaml.Dispose();

                    // The state from the crash stack: clearing the template's pool, as main's Detach did, now throws.
                    var rowTemplate = s_rowTemplate;
                    s_rowTemplate = null;
                    try
                    {
                        Microsoft.UI.Xaml.Controls.RecyclePool.SetPoolInstance(rowTemplate, null);
                    }
                    catch (Exception ex)
                    {
                        clearPoolFailure = ex;
                    }

                    // Drop the test's references after shutdown. In-process this does not reach Detach's throwing
                    // call (it passes without the fix too), so it only checks that releasing the control does not crash.
                    var tableView = s_keepAlive;
                    s_keepAlive = null;
                    ((WinRT.IWinRTObject)tableView).NativeObject.Dispose();
                    ((WinRT.IWinRTObject)rowTemplate).NativeObject.Dispose();
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
            Verify.IsNotNull(clearPoolFailure, "Precondition: after XAML shutdown, clearing the row template's pool must throw; that is the state Detach has to tolerate.");
            Log.Comment($"Clearing the pool after shutdown threw {clearPoolFailure?.GetType().Name} 0x{clearPoolFailure?.HResult:X8}: {clearPoolFailure?.Message}");
            Verify.AreEqual(unchecked((int)0x8000FFFF), clearPoolFailure?.HResult ?? 0, "Precondition: clearing the pool must fail with E_UNEXPECTED, the shutdown failure.");

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
                        s_rowTemplate = rowTemplate;
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

        internal static void CollectGarbage(int passes = CollectionAttempts)
        {
            for (var pass = 0; pass < passes; pass++)
            {
                IdleSynchronizer.Wait();
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
            }

            IdleSynchronizer.Wait();
        }

        internal static void AssertCollected(Dictionary<string, WeakReference> tracked)
        {
            Verify.IsGreaterThan(tracked.Count, 0, "The fixture must have tracked at least one object.");

            var alive = new List<string>();

            for (var attempt = 1; attempt <= CollectionAttempts; attempt++)
            {
                CollectGarbage(passes: 1);

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
