using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using Windows.Foundation.Collections;

namespace ChartsSample
{
    // Expose VectorChanged as well as .NET collection notifications to WinRT consumers.
    // Marked partial so the C#/WinRT source generator can emit the exposed-type marshalling
    // (CsWinRT1028) and keep the helper usable in trimmed or AOT apps.
    internal sealed partial class ObservableVector<T> : ObservableCollection<T>, IObservableVector<T>
    {
        internal ObservableVector() { }
        internal ObservableVector(IEnumerable<T> values) : base(values) { }

        public event VectorChangedEventHandler<T> VectorChanged;

        protected override void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            base.OnCollectionChanged(e);
            var change = e.Action switch
            {
                NotifyCollectionChangedAction.Add => CollectionChange.ItemInserted,
                NotifyCollectionChangedAction.Remove => CollectionChange.ItemRemoved,
                NotifyCollectionChangedAction.Replace => CollectionChange.ItemChanged,
                _ => CollectionChange.Reset
            };
            int index = e.Action == NotifyCollectionChangedAction.Remove ? e.OldStartingIndex : e.NewStartingIndex;
            VectorChanged?.Invoke(this, new VectorChange(change, index < 0 ? 0 : (uint)index));
        }

        private sealed partial class VectorChange : IVectorChangedEventArgs
        {
            internal VectorChange(CollectionChange change, uint index)
            {
                CollectionChange = change;
                Index = index;
            }

            public CollectionChange CollectionChange { get; }
            public uint Index { get; }
        }
    }
}
