// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using System;
using System.Collections;
using System.Collections.Specialized;

namespace MUXControlsTestApp.Samples
{
    class DelegatingItemsSource : CustomItemsSourceViewWithUniqueIdMapping
    {
        private IList _data;

        public Func<DelegatingItemsSource, int, string> KeyFromIndexFunc { get; set; }

        protected override int GetSizeCore() { return _data.Count; }

        public DelegatingItemsSource(IList data)
        {
            _data = data;
            var incc = data as INotifyCollectionChanged;
            if (incc != null)
            {
                incc.CollectionChanged += OnCollectionChanged;
            }
        }

        public object GetAt(int index)
        {
            return _data[index];
        }

        protected override object GetAtCore(int index)
        {
            return _data[index];
        }

        protected override string KeyFromIndexCore(int index)
        {
            return KeyFromIndexFunc(this, index);
        }

        private void OnCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            var args = CollectionChangeEventArgsConverters.CreateNotifyArgs(
                e.Action,
                e.OldStartingIndex,
                e.OldItems == null ? -1 : e.OldItems.Count,
                e.NewStartingIndex,
                e.NewItems == null ? -1 : e.NewItems.Count);

            OnItemsSourceChanged(args);
        }
    }
}
