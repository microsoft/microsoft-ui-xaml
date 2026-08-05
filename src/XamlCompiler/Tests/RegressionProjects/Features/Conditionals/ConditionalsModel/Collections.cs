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
    public interface IObservableCollection : INotifyCollectionChanged, IList, Windows.Foundation.Collections.IObservableVector<IEmployee>
    {
        int ItemCount { get; }
    }

    public sealed class VectorChangedEventAgrs : IVectorChangedEventArgs
    {
        public CollectionChange CollectionChange { get; set; }
        public uint Index { get; set; }
    }


    internal class EmployeeCollection<Version> : IObservableCollection, IList<IEmployee>
    {
        private ObservableCollection<IEmployee> data;

        public EmployeeCollection()
        {
            Test.EnsureVersion<Version>();
            data = new ObservableCollection<IEmployee>();
            data.CollectionChanged += Data_CollectionChanged;
        }

        private void Data_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            Test.EnsureVersion<Version>();

            if (VectorChanged != null)
            {
                VectorChangedEventAgrs args = new VectorChangedEventAgrs();
                args.CollectionChange = CollectionChange.Reset;
                VectorChanged(this, args);
            }

            if (CollectionChanged != null)
            {
                CollectionChanged(this, new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
                CollectionChanged(this, e);
            }
        }

        public IEmployee this[int index]
        {
            get { Test.EnsureVersion<Version>(); return data[index]; }
            set { Test.EnsureVersion<Version>(); data[index] = value; }
        }

        public int Count { get { Test.EnsureVersion<Version>(); return data.Count; } }

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

        bool IList.IsReadOnly { get { Test.EnsureVersion<Version>(); return false; } }

        int ICollection.Count { get { Test.EnsureVersion<Version>(); return data.Count; } }
        bool ICollection.IsSynchronized { get { Test.EnsureVersion<Version>(); return false; } }

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
                Test.EnsureVersion<Version>();
                return Count;
            }
        }

        object IList.this[int index]
        {
            get
            {
                Test.EnsureVersion<Version>();
                return data[index];
            }

            set
            {
                Test.EnsureVersion<Version>();
                data[index] = (Employee)value;
            }
        }

        public event VectorChangedEventHandler<IEmployee> VectorChanged;

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        public void Add(IEmployee item)
        {
            Test.EnsureVersion<Version>();
            data.Add(item);
        }

        public void Clear()
        {
            Test.EnsureVersion<Version>();
            data.Clear();
        }

        public bool Contains(IEmployee item)
        {
            Test.EnsureVersion<Version>();
            return data.Contains(item);
        }

        public void CopyTo(IEmployee[] array, int arrayIndex)
        {
            Test.EnsureVersion<Version>();
            data.CopyTo(array, arrayIndex);
        }

        public IEnumerator<IEmployee> GetEnumerator()
        {
            Test.EnsureVersion<Version>();
            return data.GetEnumerator();
        }

        public int IndexOf(IEmployee item)
        {
            Test.EnsureVersion<Version>();
            return data.IndexOf(item);
        }

        public void Insert(int index, IEmployee item)
        {
            Test.EnsureVersion<Version>();
            data.Insert(index, item);
        }

        public bool Remove(IEmployee item)
        {
            Test.EnsureVersion<Version>();
            return data.Remove(item);
        }

        public void RemoveAt(int index)
        {
            Test.EnsureVersion<Version>();
            data.RemoveAt(index);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            Test.EnsureVersion<Version>();
            return data.GetEnumerator();
        }

        int IList.Add(object item)
        {
            Test.EnsureVersion<Version>();
            data.Add((IEmployee)item);
            return data.Count;
        }

        bool IList.Contains(object item) { Test.EnsureVersion<Version>(); return data.Contains(item); }

        int IList.IndexOf(object item) { Test.EnsureVersion<Version>(); return data.IndexOf((IEmployee)item); }


        void IList.Insert(int index, object item)
        {
            Test.EnsureVersion<Version>();
            data.Insert(index, (IEmployee)item);
        }

        void IList.Remove(object item)
        {
            Test.EnsureVersion<Version>();
            data.Remove((IEmployee)item);
        }
        void IList.Clear()
        {
            Test.EnsureVersion<Version>();
            data.Clear();
        }

        void IList.RemoveAt(int index)
        {
            Test.EnsureVersion<Version>();
            data.RemoveAt(index);
        }

        void ICollection.CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }
    }
}
