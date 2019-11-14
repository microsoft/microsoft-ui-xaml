using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    // BUG: IList shouldn't be necessary here.  IReadOnlyList<T> (IVectorView<T> in winrt) should be enough.
    // https://github.com/Microsoft/microsoft-ui-xaml/issues/151
    // Implementing IList in the meantime
    public class ObservableDataSource<T> : /*IReadOnlyList<T>*/IList, INotifyCollectionChanged, IKeyIndexMapping
    {
        public ObservableDataSource() { }

        public ObservableDataSource(IEnumerable<T> data)
        {
            Data = new ObservableCollection<T>(data);
        }

        #region IReadOnlyList<T>

        public int Count => Data.Count;

        public T this[int index] => Data[index];

        public IEnumerator<T> GetEnumerator()
        {
            return this.Data.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        #endregion

        #region INotifyCollectionChanged

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        protected virtual void OnCollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            this.CollectionChanged?.Invoke(this, e);
        }

        #endregion

        public string KeyFromIndex(int index)
        {
            return this[index].GetHashCode().ToString();
        }

        public int IndexFromKey(string key)
        {
            throw new System.NotImplementedException();
        }

        public ObservableCollection<T> Data
        {
            get
            {
                if (_data == null)
                {
                    _data = new ObservableCollection<T>();
                }
                return _data;
            }

            set
            {
                if (_data != value)
                {
                    // Listen for future changes
                    if (_data != null)
                        _data.CollectionChanged -= this.OnCollectionChanged;

                    _data = value;

                    if (_data != null)
                        _data.CollectionChanged += this.OnCollectionChanged;
                }

                // Raise a reset event
                this.OnCollectionChanged(
                    this,
                    new NotifyCollectionChangedEventArgs(
                        NotifyCollectionChangedAction.Reset));
            }
        }

        private ObservableCollection<T> _data = null;

        // BUG / TODO: Remove the IList junk once issue 151 is fixed
        #region IList implementation

        public int Add(object value)
        {
            return ((IList)Data).Add(value);
        }

        public void Clear()
        {
            ((IList)Data).Clear();
        }

        public bool Contains(object value)
        {
            return ((IList)Data).Contains(value);
        }

        public int IndexOf(object value)
        {
            return ((IList)Data).IndexOf(value);
        }

        public void Insert(int index, object value)
        {
            ((IList)Data).Insert(index, value);
        }

        public void Remove(object value)
        {
            ((IList)Data).Remove(value);
        }

        public void RemoveAt(int index)
        {
            ((IList)Data).RemoveAt(index);
        }

        public void CopyTo(Array array, int index)
        {
            ((IList)Data).CopyTo(array, index);
        }

        public bool IsFixedSize => ((IList)Data).IsFixedSize;

        public bool IsReadOnly => ((IList)Data).IsReadOnly;

        public bool IsSynchronized => ((IList)Data).IsSynchronized;

        public object SyncRoot => ((IList)Data).SyncRoot;

        object IList.this[int index] { get => ((IList)Data)[index]; set => ((IList)Data)[index] = value; }

        #endregion
    }

    public class GroupedObservableCollection<T> : ObservableCollection<T>
    {
        public string Key
        {
            get;
            set;
        }

        public GroupedObservableCollection(string key, IEnumerable<T> collection) : base(collection)
        {
            this.Key = key;
        }
    }
}
