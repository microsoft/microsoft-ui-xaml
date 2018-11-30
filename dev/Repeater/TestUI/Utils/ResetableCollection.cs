// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

namespace MUXControlsTestApp.Utils
{
    public class ResetableCollection<T> : ObservableCollection<T>
    {
        private Random _rnd = new Random(123);

        public ResetableCollection(IEnumerable<T> collection)
            : base(collection)
        { }

        public void ShuffleAndReset()
        {
            for (int i = 0; i < 5; i++)
            {
                int from = _rnd.Next(0, Items.Count - 1);
                var value = Items[from];
                Items.RemoveAt(from);
                int to = _rnd.Next(0, Items.Count - 1);
                Items.Insert(to, value);
            }

            OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
        }
    }
}
