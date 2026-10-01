// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.Foundation.Collections;

namespace ConditionalControls
{
    public interface IObservableCollection : 
#if CSWINRT_1642
        INotifyCollectionChanged, IList
#else
        INotifyCollectionChanged, IList, Windows.Foundation.Collections.IObservableVector<IEmployee>
#endif		
    {
        int ItemCount { get; }
    }

#if !CSWINRT_1642
    public sealed class VectorChangedEventArgs : IVectorChangedEventArgs
    {
        public CollectionChange CollectionChange { get; set; }
        public uint Index { get; set; }
    }
#endif

    internal class EmployeeCollection<TVersion> : IObservableCollection, IList<IEmployee>
    {
        private ObservableCollection<IEmployee> data;

        public EmployeeCollection()
        {
            Test.EnsureVersion<TVersion>();
            data = new ObservableCollection<IEmployee>();
            data.CollectionChanged += Data_CollectionChanged;
        }

        private void Data_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            Test.EnsureVersion<TVersion>();

#if !CSWINRT_1642
            if (VectorChanged != null)
            {
                VectorChangedEventArgs args = new VectorChangedEventArgs();
                args.CollectionChange = CollectionChange.Reset;
                VectorChanged(this, args);
            }
#endif

            if (CollectionChanged != null)
            {
                CollectionChanged(this, new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                CollectionChanged(this, e);
            }
        }

        public IEmployee this[int index]
        {
            get { Test.EnsureVersion<TVersion>(); return data[index]; }
            set { Test.EnsureVersion<TVersion>(); data[index] = value; }
        }

        public int Count { get { Test.EnsureVersion<TVersion>(); return data.Count; } }

        public bool IsReadOnly
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        bool IList.IsFixedSize
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        bool IList.IsReadOnly { get { Test.EnsureVersion<TVersion>(); return false; } }

        int ICollection.Count { get { Test.EnsureVersion<TVersion>(); return data.Count; } }
        bool ICollection.IsSynchronized { get { Test.EnsureVersion<TVersion>(); return false; } }

        object ICollection.SyncRoot
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        public int ItemCount
        {
            get
            {
                Test.EnsureVersion<TVersion>();
                return Count;
            }
        }

        object IList.this[int index]
        {
            get
            {
                Test.EnsureVersion<TVersion>();
                return data[index];
            }

            set
            {
                Test.EnsureVersion<TVersion>();
                data[index] = (Employee)value;
            }
        }

#if !CSWINRT_1642
        public event VectorChangedEventHandler<IEmployee> VectorChanged;
#endif

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        public void Add(IEmployee item)
        {
            Test.EnsureVersion<TVersion>();
            data.Add(item);
        }

        public void Clear()
        {
            Test.EnsureVersion<TVersion>();
            data.Clear();
        }

        public bool Contains(IEmployee item)
        {
            Test.EnsureVersion<TVersion>();
            return data.Contains(item);
        }

        public void CopyTo(IEmployee[] array, int arrayIndex)
        {
            Test.EnsureVersion<TVersion>();
            data.CopyTo(array, arrayIndex);
        }

        public IEnumerator<IEmployee> GetEnumerator()
        {
            Test.EnsureVersion<TVersion>();
            return data.GetEnumerator();
        }

        public int IndexOf(IEmployee item)
        {
            Test.EnsureVersion<TVersion>();
            return data.IndexOf(item);
        }

        public void Insert(int index, IEmployee item)
        {
            Test.EnsureVersion<TVersion>();
            data.Insert(index, item);
        }

        public bool Remove(IEmployee item)
        {
            Test.EnsureVersion<TVersion>();
            return data.Remove(item);
        }

        public void RemoveAt(int index)
        {
            Test.EnsureVersion<TVersion>();
            data.RemoveAt(index);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            Test.EnsureVersion<TVersion>();
            return data.GetEnumerator();
        }

        int IList.Add(object item)
        {
            Test.EnsureVersion<TVersion>();
            data.Add((IEmployee)item);
            return data.Count;
        }

        bool IList.Contains(object item) { Test.EnsureVersion<TVersion>(); return data.Contains(item); }

        int IList.IndexOf(object item) { Test.EnsureVersion<TVersion>(); return data.IndexOf((IEmployee)item); }


        void IList.Insert(int index, object item)
        {
            Test.EnsureVersion<TVersion>();
            data.Insert(index, (IEmployee)item);
        }

        void IList.Remove(object item)
        {
            Test.EnsureVersion<TVersion>();
            data.Remove((IEmployee)item);
        }
        void IList.Clear()
        {
            Test.EnsureVersion<TVersion>();
            data.Clear();
        }

        void IList.RemoveAt(int index)
        {
            Test.EnsureVersion<TVersion>();
            data.RemoveAt(index);
        }

        void ICollection.CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }
    }
}
