// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Interop;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;

using Windows.Foundation.Collections;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableViewSource2 (the shaping redesign), Milestones 1 and 2: a flat source bound end to end, then followed incrementally.
    // As for TableViewSource, the projection is observed only through a bound TableView:
    // GetRowsRepeater(table).ItemsSourceView is the projection itself, and VerifyProjection reads
    // the realized rows' ShapedPerson names in order.
    [TestClass]
    public partial class TableViewSource2Tests : TableViewApiTestBase
    {
        private static readonly string[] s_people = { "Asha", "Diego", "Mei", "Rafa", "Ines", "Owen" };

        // Tall enough that every row of MakeShapedPeople realizes.
        private static TableView CreateTable(object itemsSource)
            => CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 500, height: 700);

        private static int ProjectedCount(TableView tableView)
            => GetRowsRepeater(tableView).ItemsSourceView?.Count ?? 0;

        private static void VerifyArgumentException(Action action, string expectedMessage, string context)
        {
            try
            {
                action();
            }
            catch (ArgumentException e)
            {
                Verify.IsTrue(e.Message.Contains(expectedMessage),
                    $"{context}: the message should contain '{expectedMessage}', was '{e.Message}'.");
                return;
            }

            Verify.Fail($"{context}: expected an ArgumentException.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 over a flat list projects every item in source order.")]
        public void VerifyFlatBindProjectsSourceOrder()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(TableViewSource2.From(MakeShapedPeople()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "Every source item must be one row.");
                VerifyProjection(tableView, s_people, "Flat bind");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 over an empty collection projects no rows.")]
        public void VerifyEmptySourceProjectsNoRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(TableViewSource2.From(new List<ShapedPerson>()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(GetRowsRepeater(tableView).ItemsSourceView, "An empty source still binds a view.");
                Verify.AreEqual(0, ProjectedCount(tableView), "An empty source projects no rows.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a null item in the source fails From with E_INVALIDARG.")]
        public void VerifyNullItemThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var people = MakeShapedPeople();
                people.Insert(2, null);

                VerifyArgumentException(() => TableViewSource2.From(people),
                    "A row could not be given a stable identity: null item", "Null item");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the same item object twice in the source fails From with E_INVALIDARG.")]
        public void VerifyDuplicateItemThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                var people = MakeShapedPeople();
                people.Add(people[1]);

                VerifyArgumentException(() => TableViewSource2.From(people),
                    "The same item object appears in the source more than once", "Duplicate item");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies From rejects null and objects that are not collections.")]
        public void VerifyUnsupportedSourceThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.Throws<ArgumentException>(() => TableViewSource2.From(null), "From(null) must throw.");
                VerifyArgumentException(() => TableViewSource2.From(42),
                    "items must implement a supported collection interface", "From(42)");
                VerifyArgumentException(() => TableViewSource2.From(new ShapedPerson("Asha", "Designer", "Studio")),
                    "items must implement a supported collection interface", "From(single item)");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a plain List still binds through the v1 TableViewSource path.")]
        public void VerifyPlainListStillBindsThroughV1()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTable(MakeShapedPeople());
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "A plain list projects every item.");
                VerifyProjection(tableView, s_people, "Plain list");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Refresh re-reads a non-observable source and shows the new rows.")]
        public void VerifyRefreshRereadsSource()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                tableView = CreateTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                people.RemoveAt(0);
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                source.Refresh();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new[] { "Diego", "Mei", "Rafa", "Ines", "Owen", "Zoe" }, "After Refresh");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Refresh that finds a duplicate throws and leaves the projection as it was.")]
        public void VerifyRefreshWithDuplicateKeepsProjection()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                tableView = CreateTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                people.Add(people[0]);
                VerifyArgumentException(() => source.Refresh(),
                    "The same item object appears in the source more than once", "Refresh with duplicate");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people, "A failed Refresh must not change the rows");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies unbinding, unloading and reloading a table keeps or reclaims its TableViewSource2.")]
        public void VerifyUnbindUnloadReloadReclaims()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                tableView = CreateTable(source);
                host = new StackPanel();
                host.Children.Add(tableView);
                LoadContent(host);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.ItemsSource = null);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual(0, ProjectedCount(tableView), "Unbound: no rows."));

            RunOnUIThread.Execute(() => tableView.ItemsSource = source);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyProjection(tableView, s_people, "Rebound"));

            RunOnUIThread.Execute(() => host.Children.Remove(tableView));
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => host.Children.Add(tableView));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "Reloaded: the table reclaims the source.");
                VerifyProjection(tableView, s_people, "Reloaded");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a second TableView bound to an owned TableViewSource2 renders nothing, then takes over when the owner unloads.")]
        public void VerifySecondTableWaitsThenTakesOver()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The owner keeps its rows.");
                Verify.AreEqual(0, ProjectedCount(second), "The refused table must render no rows.");
                host.Children.Remove(first);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second), "The waiting table takes over when the owner unloads.");
                GC.KeepAlive(first);
            });
        }

        // Review Focus: a LINQ result reaches native code as an iterable whose iterator may fail
        // with E_NOINTERFACE; the accessor must retry through IBindableIterable.
        [TestMethod]
        [TestProperty("Description", "Verifies a CLR IEnumerable that is not a list binds through TableViewSource2.")]
        public void VerifyIterableOnlySourceBinds()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                IEnumerable<ShapedPerson> query = MakeShapedPeople().Select(p => p);
                tableView = CreateTable(TableViewSource2.From(query));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyProjection(tableView, s_people, "Iterable-only source"));
        }

        // Review Focus: a managed item crosses into native code through different interface
        // pointers; lookups must resolve it through its canonical IUnknown.
        [TestMethod]
        [TestProperty("Description", "Verifies ItemsSourceView.IndexOf finds a managed item by object identity.")]
        public void VerifyIndexOfResolvesThroughCanonicalIdentity()
        {
            TableView tableView = null;
            List<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                tableView = CreateTable(TableViewSource2.From(people));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var view = GetRowsRepeater(tableView).ItemsSourceView;
                for (int i = 0; i < people.Count; i++)
                {
                    Verify.AreEqual(i, view.IndexOf(people[i]), $"IndexOf({people[i].Name})");
                }

                Verify.AreEqual(-1, view.IndexOf(new ShapedPerson("Asha", "Designer", "Studio")),
                    "An equal-looking but distinct object is not in the source.");
                Verify.AreSame(people[2], view.GetAt(2), "GetAt returns the app's own object.");
            });
        }

        // Review Focus: the same source set again on the same table must be reclaimed, and the
        // release after it must still let another table take over.
        [TestMethod]
        [TestProperty("Description", "Verifies rebinding the same TableViewSource2 to the same TableView reclaims it and later releases it cleanly.")]
        public void VerifyRebindSameSourceToSameTable()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                first.ItemsSource = source;
                first.ItemsSource = null;
                first.ItemsSource = source;
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The table must reclaim its own source.");
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, ProjectedCount(second), "The rebound table still owns the source.");
                first.ItemsSource = null;
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second), "Unbinding the owner hands the source to the waiting table.");
                Verify.AreEqual(0, ProjectedCount(first), "The unbound table renders no rows.");
            });
        }

        // Review Focus: the source holds its owner as a raw pointer; a table that goes away must
        // not leave it behind.
        [TestMethod]
        [TestProperty("Description", "Verifies a TableViewSource2 that outlives its TableView can be refreshed and bound again.")]
        public void VerifySourceOutlivesTableView()
        {
            TableViewSource2 source = null;
            List<ShapedPerson> people = null;
            WeakReference firstTable = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeShapedPeople();
                source = TableViewSource2.From(people);
                firstTable = LoadTableAndForget(source);
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Content = null);
            IdleSynchronizer.Wait();

            for (int attempt = 0; attempt < 5 && firstTable.IsAlive; attempt++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(firstTable.IsAlive, "The first TableView must be collected.");

            TableView second = null;
            RunOnUIThread.Execute(() =>
            {
                // No owner now: a Refresh must not call back into the destroyed table.
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                source.Refresh();

                second = CreateTable(source);
                LoadContent(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length + 1, ProjectedCount(second), "A new table owns the surviving source.");
            });
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private WeakReference LoadTableAndForget(TableViewSource2 source)
        {
            var tableView = CreateTable(source);
            LoadContent(tableView);
            return new WeakReference(tableView);
        }

        // Review Focus: each swap releases the previous source of the other generation.
        [TestMethod]
        [TestProperty("Description", "Verifies swapping ItemsSource from TableViewSource2 to a v1 source and back releases and reclaims ownership.")]
        public void VerifySwapV2ToV1ToV2()
        {
            TableView first = null;
            TableView second = null;
            TableViewSource2 source = null;
            StackPanel host = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource2.From(MakeShapedPeople());
                first = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host = new StackPanel();
                host.Children.Add(first);
                LoadContent(host);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() => first.ItemsSource = TableViewSource.From(MakeShapedPeople()));
            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "The v1 source projects its rows.");
                second = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 300);
                host.Children.Add(second);
            });

            SettleLayout(second);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(second),
                    "Swapping to v1 must have released the v2 source, so another table can own it.");
                first.ItemsSource = source;
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, ProjectedCount(first), "The v2 source is owned by the second table now.");
                host.Children.Remove(second);
            });

            SettleLayout(first);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(first), "Swapped back to v2, the table takes the source over.");
                VerifyProjection(first, s_people, "v2 -> v1 -> v2");
                GC.KeepAlive(second);
            });
        }

        private static ObservableCollection<ShapedPerson> MakeObservablePeople()
            => new ObservableCollection<ShapedPerson>(MakeShapedPeople());

        private static string[] Names(IEnumerable<object> items)
            => items.Select(item => ((ShapedPerson)item).Name).ToArray();

        // IObservableVector<object> whose args throw from Index on Reset, as some implementations do
        // (Review Focus 5). Modeled on Repeater's WinRTCollection.
        private sealed partial class ObservableVector : IObservableVector<object>
        {
            private List<object> m_data;

            public ObservableVector(IEnumerable<object> items) { m_data = new List<object>(items); }

            public event VectorChangedEventHandler<object> VectorChanged;

            public object this[int index]
            {
                get => m_data[index];
                set { m_data[index] = value; Raise(CollectionChange.ItemChanged, (uint)index); }
            }

            public int Count => m_data.Count;
            public bool IsReadOnly => false;
            public void Add(object item) { m_data.Add(item); Raise(CollectionChange.ItemInserted, (uint)m_data.Count - 1); }
            public void Insert(int index, object item) { m_data.Insert(index, item); Raise(CollectionChange.ItemInserted, (uint)index); }
            public void RemoveAt(int index) { m_data.RemoveAt(index); Raise(CollectionChange.ItemRemoved, (uint)index); }
            public bool Remove(object item) { int index = m_data.IndexOf(item); if (index < 0) return false; RemoveAt(index); return true; }
            public void Clear() { m_data.Clear(); Raise(CollectionChange.Reset, 0); }
            public void ResetWith(IEnumerable<object> items) { m_data = new List<object>(items); Raise(CollectionChange.Reset, 0); }
            public bool Contains(object item) => m_data.Contains(item);
            public int IndexOf(object item) => m_data.IndexOf(item);
            public void CopyTo(object[] array, int arrayIndex) => m_data.CopyTo(array, arrayIndex);
            public IEnumerator<object> GetEnumerator() => m_data.GetEnumerator();
            IEnumerator IEnumerable.GetEnumerator() => m_data.GetEnumerator();

            private void Raise(CollectionChange change, uint index) => VectorChanged?.Invoke(this, new Args(change, index));

            private sealed partial class Args : IVectorChangedEventArgs
            {
                private readonly uint m_index;

                public Args(CollectionChange change, uint index) { CollectionChange = change; m_index = index; }

                public CollectionChange CollectionChange { get; }

                public uint Index => CollectionChange == CollectionChange.Reset
                    ? throw new InvalidOperationException("Index is undefined for Reset.")
                    : m_index;
            }
        }

        // IBindableObservableVector (projected as IList plus VectorChanged), with no
        // INotifyCollectionChanged, so the accessor picks the bindable observe kind.
        private sealed partial class BindableObservableList : IBindableObservableVector
        {
            private readonly List<object> m_data;

            public BindableObservableList(IEnumerable<object> items) { m_data = new List<object>(items); }

            public event BindableVectorChangedEventHandler VectorChanged;

            public object this[int index] { get => m_data[index]; set { m_data[index] = value; Raise(); } }
            public bool IsFixedSize => false;
            public bool IsReadOnly => false;
            public int Count => m_data.Count;
            public bool IsSynchronized => false;
            public object SyncRoot => this;
            public int Add(object value) { m_data.Add(value); Raise(); return m_data.Count - 1; }
            public void Clear() { m_data.Clear(); Raise(); }
            public bool Contains(object value) => m_data.Contains(value);
            public int IndexOf(object value) => m_data.IndexOf(value);
            public void Insert(int index, object value) { m_data.Insert(index, value); Raise(); }
            public void Remove(object value) { if (m_data.Remove(value)) Raise(); }
            public void RemoveAt(int index) { m_data.RemoveAt(index); Raise(); }
            public void CopyTo(Array array, int index) => ((ICollection)m_data).CopyTo(array, index);
            public IEnumerator GetEnumerator() => m_data.GetEnumerator();

            private void Raise() => VectorChanged?.Invoke(this, null);
        }

        private TableView BindObservable(object itemsSource)
        {
            var tableView = CreateTable(TableViewSource2.From(itemsSource));
            LoadContent(tableView);
            return tableView;
        }

        private void VerifyObservableEdit(
            Action<ObservableCollection<ShapedPerson>> mutate, string[] expected, string context)
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                tableView = BindObservable(people);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => mutate(people));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyProjection(tableView, expected, context));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an Add on an observable source inserts one row at its position.")]
        public void VerifyObservableAddInsertsRow()
        {
            VerifyObservableEdit(
                people => people.Insert(2, new ShapedPerson("Zoe", "Manager", "Studio")),
                new[] { "Asha", "Diego", "Zoe", "Mei", "Rafa", "Ines", "Owen" }, "After Insert");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Remove on an observable source removes one row.")]
        public void VerifyObservableRemoveRemovesRow()
        {
            VerifyObservableEdit(
                people => people.RemoveAt(1),
                new[] { "Asha", "Mei", "Rafa", "Ines", "Owen" }, "After RemoveAt");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Replace on an observable source updates one row.")]
        public void VerifyObservableReplaceUpdatesRow()
        {
            VerifyObservableEdit(
                people => people[3] = new ShapedPerson("Zoe", "Manager", "Studio"),
                new[] { "Asha", "Diego", "Mei", "Zoe", "Ines", "Owen" }, "After Replace");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Move on an observable source reorders one row.")]
        public void VerifyObservableMoveReordersRow()
        {
            VerifyObservableEdit(
                people => people.Move(0, 5),
                new[] { "Diego", "Mei", "Rafa", "Ines", "Owen", "Asha" }, "After Move");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Reset on an observable source re-reads the rows.")]
        public void VerifyObservableResetRereads()
        {
            VerifyObservableEdit(
                people =>
                {
                    people.Clear();
                    people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                },
                new[] { "Zoe" }, "After Clear and Add");
        }

        // Review Focus 2.
        [TestMethod]
        [TestProperty("Description", "Verifies adding an existing item throws, keeps the rows, and the next change recovers.")]
        public void VerifyDuplicateAddedLaterDivergesAndRecovers()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                tableView = BindObservable(people);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyArgumentException(() => people.Add(people[0]),
                    "The same item object appears in the source more than once", "Add duplicate");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people, "A rejected Add must not change the rows");

                // The source is back to valid; this notification re-reads the diverged table.
                people.RemoveAt(people.Count - 1);
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people.Append("Zoe").ToArray(), "Recovered after the next change");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding null throws and keeps the rows.")]
        public void VerifyNullAddedLaterThrowsAndKeepsProjection()
        {
            TableView tableView = null;
            ObservableCollection<object> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<object>(MakeShapedPeople());
                tableView = BindObservable(items);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyArgumentException(() => items.Add(null),
                    "A row could not be given a stable identity: null item", "Add null");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people, "A rejected null Add must not change the rows");
            });
        }

        // Review Focus 1.
        [TestMethod]
        [TestProperty("Description", "Verifies a source change made inside a view notification is deferred and re-read once.")]
        public void VerifyChangeDuringFreezeIsDeferredToReread()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> people = null;
            int viewNotifications = 0;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                tableView = BindObservable(people);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var view = GetRowsRepeater(tableView).ItemsSourceView;
                bool added = false;
                view.CollectionChanged += (s, e) =>
                {
                    viewNotifications++;
                    if (!added)
                    {
                        // Inside the commit scope: skipped now, re-read when the dispatch ends.
                        added = true;
                        people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                    }
                };

                people.RemoveAt(0);

                // The re-read ran synchronously at the end of RemoveAt's notification.
                Verify.AreEqual(s_people.Length, view.Count, "Removed one, re-read added one.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new[] { "Diego", "Mei", "Rafa", "Ines", "Owen", "Zoe" }, "After the deferred re-read");
                // Each commit raises one view notification (a Reset until Milestone 5).
                Verify.AreEqual(2, viewNotifications, "One notification for RemoveAt, one for the re-read.");
            });
        }

        // Review Focus 4.
        [TestMethod]
        [TestProperty("Description", "Verifies an app that changes the source on every view notification does not loop.")]
        public void VerifyMutateOnEveryNotificationDoesNotLoop()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> people = null;
            int handlerCalls = 0;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                tableView = BindObservable(people);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var view = GetRowsRepeater(tableView).ItemsSourceView;
                view.CollectionChanged += (s, e) =>
                {
                    handlerCalls++;
                    if (people.Count < 20)
                    {
                        people.Add(new ShapedPerson($"P{people.Count}", "Engineer", "Platform"));
                    }
                };

                people.RemoveAt(0);

                Verify.AreEqual(2, handlerCalls, "One notification for RemoveAt, one for the single synchronous re-read.");
            });

            // The remaining re-reads are posted, one per dispatcher turn.
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(20, people.Count, "The app stopped adding at 20.");
                Verify.AreEqual(20, ProjectedCount(tableView), "The projection caught up with the source.");
            });
        }

        // Review Focus 5.
        [TestMethod]
        [TestProperty("Description", "Verifies an IObservableVector source follows insert, remove, replace and reset.")]
        public void VerifyIObservableVectorSourceTracksChanges()
        {
            TableView tableView = null;
            ObservableVector items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableVector(MakeShapedPeople());
                tableView = BindObservable(items);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                items.Insert(1, new ShapedPerson("Zoe", "Manager", "Studio"));
                items.RemoveAt(0);
                items[2] = new ShapedPerson("Kai", "Engineer", "Platform");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new[] { "Zoe", "Diego", "Kai", "Rafa", "Ines", "Owen" }, "Vector edits");

                // Its args throw from Index on Reset: the table must re-read without reading it.
                items.ResetWith(new object[] { new ShapedPerson("Lea", "Designer", "Studio") });
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => VerifyProjection(tableView, new[] { "Lea" }, "Vector reset"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an IBindableObservableVector source re-reads on every change.")]
        public void VerifyBindableObservableVectorSourceTracksChanges()
        {
            TableView tableView = null;
            BindableObservableList items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new BindableObservableList(MakeShapedPeople());
                tableView = BindObservable(items);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => items.Insert(1, new ShapedPerson("Zoe", "Manager", "Studio")));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, new[] { "Asha", "Zoe", "Diego", "Mei", "Rafa", "Ines", "Owen" }, "Bindable insert"));

            RunOnUIThread.Execute(() => items.RemoveAt(0));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, new[] { "Zoe", "Diego", "Mei", "Rafa", "Ines", "Owen" }, "Bindable remove"));

            RunOnUIThread.Execute(() => items[2] = new ShapedPerson("Kai", "Engineer", "Platform"));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
                VerifyProjection(tableView, new[] { "Zoe", "Diego", "Kai", "Rafa", "Ines", "Owen" }, "Bindable set"));

            RunOnUIThread.Execute(() =>
            {
                items.Clear();
                items.Add(new ShapedPerson("Lea", "Designer", "Studio"));
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyProjection(tableView, new[] { "Lea" }, "Bindable clear and add"));
        }

        // Review Focus 3.
        [TestMethod]
        [TestProperty("Description", "Verifies a collected TableViewSource2 unsubscribes from its observable collection and leaves it usable.")]
        public void VerifyCollectionOutlivesTableViewSource2()
        {
            CountingObservableList people = null;
            WeakReference weakSource = null;

            RunOnUIThread.Execute(() =>
            {
                people = new CountingObservableList(MakeShapedPeople());
                weakSource = BindObservableAndForget(people);
            });

            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, people.SubscriberCount, "The bound table holds one CollectionChanged handler.");
                Content = null;
            });
            IdleSynchronizer.Wait();

            for (int attempt = 0; attempt < 5 && (weakSource.IsAlive || people.SubscriberCount != 0); attempt++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();
                IdleSynchronizer.Wait();
            }

            Verify.IsFalse(weakSource.IsAlive, "The TableViewSource2 must be collected.");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, people.SubscriberCount, "Teardown must revoke the CollectionChanged handler.");

                // No handler may run against the torn-down table.
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                people.Move(0, 1);
                people.RemoveAt(0);
                people.Clear();
            });

            IdleSynchronizer.Wait();
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        private WeakReference BindObservableAndForget(object people)
        {
            var source = TableViewSource2.From(people);
            var tableView = CreateTable(source);
            LoadContent(tableView);
            return new WeakReference(source);
        }

        // INotifyCollectionChanged list that counts its current CollectionChanged subscribers.
        private sealed partial class CountingObservableList : IList<object>, INotifyCollectionChanged
        {
            private readonly List<object> m_data;
            private NotifyCollectionChangedEventHandler m_handlers;
            private int m_subscriberCount;

            public CountingObservableList(IEnumerable<object> items) { m_data = new List<object>(items); }

            public int SubscriberCount => global::System.Threading.Volatile.Read(ref m_subscriberCount);

            public event NotifyCollectionChangedEventHandler CollectionChanged
            {
                add { if (value != null) { lock (m_data) { m_handlers += value; m_subscriberCount++; } } }
                remove
                {
                    if (value == null) return;
                    lock (m_data)
                    {
                        var before = m_handlers;
                        m_handlers -= value;
                        if (!ReferenceEquals(before, m_handlers)) m_subscriberCount--;
                    }
                }
            }

            public object this[int index]
            {
                get => m_data[index];
                set
                {
                    var old = m_data[index];
                    m_data[index] = value;
                    Raise(new NotifyCollectionChangedEventArgs(
                        NotifyCollectionChangedAction.Replace, value, old, index));
                }
            }

            public int Count => m_data.Count;
            public bool IsReadOnly => false;
            public void Add(object item) => Insert(m_data.Count, item);
            public void Insert(int index, object item)
            {
                m_data.Insert(index, item);
                Raise(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Add, item, index));
            }
            public void RemoveAt(int index)
            {
                var item = m_data[index];
                m_data.RemoveAt(index);
                Raise(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Remove, item, index));
            }
            public bool Remove(object item) { int index = m_data.IndexOf(item); if (index < 0) return false; RemoveAt(index); return true; }
            public void Move(int oldIndex, int newIndex)
            {
                var item = m_data[oldIndex];
                m_data.RemoveAt(oldIndex);
                m_data.Insert(newIndex, item);
                Raise(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Move, item, newIndex, oldIndex));
            }
            public void Clear()
            {
                m_data.Clear();
                Raise(new NotifyCollectionChangedEventArgs(
                    NotifyCollectionChangedAction.Reset));
            }
            public bool Contains(object item) => m_data.Contains(item);
            public int IndexOf(object item) => m_data.IndexOf(item);
            public void CopyTo(object[] array, int arrayIndex) => m_data.CopyTo(array, arrayIndex);
            public IEnumerator<object> GetEnumerator() => m_data.GetEnumerator();
            IEnumerator IEnumerable.GetEnumerator() => m_data.GetEnumerator();

            private void Raise(NotifyCollectionChangedEventArgs args) => m_handlers?.Invoke(this, args);
        }

        // Threading: a bound table rejects a root collection change raised off its thread.
        [TestMethod]
        [TestProperty("Description", "Verifies a source change raised off the bound thread throws into the mutation and leaves the rows unchanged.")]
        public void VerifyOffThreadChangeThrows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                tableView = BindObservable(people);
            });

            SettleLayout(tableView);

            Exception caught = null;
            Task.Run(() =>
            {
                try
                {
                    people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));
                }
                catch (Exception e)
                {
                    caught = e;
                }
            }).Wait();

            Verify.IsNotNull(caught, "An off-thread Add must throw into the app's mutation.");
            Log.Comment($"Off-thread Add threw {caught.GetType().FullName}, HResult 0x{caught.HResult:X8}: {caught.Message}");

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(s_people.Length, ProjectedCount(tableView), "The rejected off-thread Add must not change the rows.");

                // The source now holds Zoe; this out-of-range Add re-reads on the bound thread.
                people.Add(new ShapedPerson("Kai", "Engineer", "Platform"));
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, s_people.Concat(new[] { "Zoe", "Kai" }).ToArray(), "Recovered on the bound thread");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies toggling IsLiveShaping subscribes and unsubscribes items without changing rows.")]
        public void VerifyLiveShapingToggleKeepsRows()
        {
            TableView tableView = null;
            TableViewSource2 source = null;
            ObservableCollection<ShapedPerson> people = null;

            RunOnUIThread.Execute(() =>
            {
                people = MakeObservablePeople();
                source = TableViewSource2.From(people);
                tableView = CreateTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                source.IsLiveShaping = true;
                people[0].Role = "Lead";

                // Subscribed at creation.
                people.Add(new ShapedPerson("Zoe", "Manager", "Studio"));

                // Revoked by ~SourceRecord.
                people.RemoveAt(1);

                source.IsLiveShaping = false;
                people[0].Role = "Designer";
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyProjection(tableView, new[] { "Asha", "Mei", "Rafa", "Ines", "Owen", "Zoe" }, "After the live shaping toggle");
            });
        }
    }
}
