using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Text;

namespace MUXControlsTestApp.Utilities
{
    class ExtendedObservableCollection<T> : ObservableCollection<T>
    {
        public void ReplaceAll(ICollection<T> items)
        {
            this.Items.Clear();
            foreach(T item in items)
            {
                this.Items.Add(item);
            }
            this.OnCollectionChanged(
                new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset)
            );
        }

    }
}
