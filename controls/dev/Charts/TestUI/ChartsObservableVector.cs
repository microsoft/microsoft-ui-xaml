// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using Windows.Foundation.Collections;

namespace MUXControlsTestApp
{
    // An ObservableCollection<T> that also raises WinRT VectorChanged, so a chart bound to it sees live
    // changes. Same pattern as the ChartApp sample's ObservableVector<T>. Partial so C#/WinRT can generate
    // the marshalling code (CsWinRT1028).
    internal sealed partial class ChartsObservableVector<T> : ObservableCollection<T>, IObservableVector<T>
    {
        public ChartsObservableVector() { }

        public ChartsObservableVector(IEnumerable<T> values) : base(values) { }

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
            public VectorChange(CollectionChange change, uint index)
            {
                CollectionChange = change;
                Index = index;
            }

            public CollectionChange CollectionChange { get; }

            public uint Index { get; }
        }
    }
}
