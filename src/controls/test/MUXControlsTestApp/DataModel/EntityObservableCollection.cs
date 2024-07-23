// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;

namespace MUXControlsTestApp.Samples.Model
{
    public class EntityObservableCollection : IList, INotifyCollectionChanged
    {
        private ObservableCollection<Entity> _colEntities = null;

        public EntityObservableCollection()
        {
            _colEntities = new ObservableCollection<Entity>();

            _colEntities.CollectionChanged += Entities_CollectionChanged;
        }

        public EntityObservableCollection(IEnumerable<Entity> entities)
        {
            _colEntities = new ObservableCollection<Entity>(entities);

            _colEntities.CollectionChanged += Entities_CollectionChanged;
        }

        #region IList

        public int Count
        {
            get
            {
                return _colEntities.Count;
            }
        }

        public object this[int index]
        {
            get { return _colEntities[index]; }
            set { _colEntities[index] = value as Entity; }
        }

        public bool IsFixedSize
        {
            get { return false; }
        }

        public bool IsReadOnly
        {
            get { return false; }
        }

        public bool IsSynchronized
        {
            get { return false; }
        }

        public object SyncRoot
        {
            get { return null; }
        }

        public int Add(object value)
        {
            _colEntities.Add(value as Entity);
            return _colEntities.Count;
        }

        public void Clear()
        {
            _colEntities.Clear();
        }

        public bool Contains(object value)
        {
            return _colEntities.Contains(value as Entity);
        }

        public void CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }

        public IEnumerator GetEnumerator()
        {
            return _colEntities.GetEnumerator();
        }

        public int IndexOf(object value)
        {
            return _colEntities.IndexOf(value as Entity);
        }

        public void Insert(int index, object value)
        {
            _colEntities.Insert(index, value as Entity);
        }

        public void Remove(object value)
        {
            _colEntities.Remove(value as Entity);
        }

        public void RemoveAt(int index)
        {
            _colEntities.RemoveAt(index);
        }

        #endregion

        #region INotifyCollectionChanged

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        #endregion

        public ObservableCollection<Entity> Collection
        {
            get { return _colEntities; }
        }

        public void RaiseResetNotification()
        {
            OnCollectionChanged(new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
        }

        private void Entities_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnCollectionChanged(e);
        }

        private void OnCollectionChanged(NotifyCollectionChangedEventArgs e)
        {
            if (CollectionChanged != null)
            {
                CollectionChanged(this, e);
            }
        }
    }
}
