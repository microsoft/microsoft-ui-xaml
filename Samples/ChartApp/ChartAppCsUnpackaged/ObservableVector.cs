using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using Windows.Foundation.Collections;

namespace ChartsSample
{
    // Expose VectorChanged as well as .NET collection notifications to WinRT consumers.
    internal sealed class ObservableVector<T> : ObservableCollection<T>, IObservableVector<T>
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

        private sealed class VectorChange : IVectorChangedEventArgs
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
