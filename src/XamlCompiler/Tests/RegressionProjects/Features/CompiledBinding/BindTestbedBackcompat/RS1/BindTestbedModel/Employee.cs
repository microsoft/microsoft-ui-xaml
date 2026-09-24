// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml.Data;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Collections.ObjectModel;

using Windows.Foundation.Collections;
using System.Collections;
using System.Collections.Specialized;
using Microsoft.UI.Xaml;
using Windows.UI.Popups;
using Windows.UI;

namespace BindTestbedModel
{
    public interface IEmployee : Microsoft.UI.Xaml.Data.INotifyPropertyChanged
    {
        string FirstName { get; set; }
        string LastName { get; set; }
        IManager DirectManager { get; set; }
        bool IsManager { get; }
        string Title { get; }
        string Name { get; }
        string GetName();
        bool Visibility { get; set; }
        Visibility GetVisibility();
        string NullStringProperty { get; }
        Microsoft.UI.Xaml.Media.ImageSource NullImageSource { get; }
        void Click(object sender, RoutedEventArgs e);
        TShirtSize TShirt { get; set; }
        bool IsEmployeeOfTheMonth { get; set; }
    }

    internal class Employee : IEmployee
    {
        private string _FirstName;
        private string _LastName;
        private IManager _Manager;
        private bool _Visibility;
        private TShirtSize _TShirt;
        private bool _IsEmployeeOfTheMonth;

        public event Microsoft.UI.Xaml.Data.PropertyChangedEventHandler PropertyChanged;

        public Employee()
        {
            _Visibility = true;
            _TShirt = TShirtSize.Medium;
        }

        public void Click(object sender, RoutedEventArgs e)
        {
            MessageDialog dlg = new MessageDialog(string.Format("Click on {0}", Title));
            var t = dlg.ShowAsync();
        }

        public bool Visibility
        {
            get { return _Visibility; }
            set { if (value != _Visibility) { _Visibility = value; NotifyPropertyChanged(); } }
        }

        public Visibility GetVisibility() { return Visibility ? Microsoft.UI.Xaml.Visibility.Visible : Microsoft.UI.Xaml.Visibility.Collapsed; }

        public string NullStringProperty
        {
            get { return null; }
        }
        public Microsoft.UI.Xaml.Media.ImageSource NullImageSource
        {
            get { return null; }
        }

        public string FirstName
        {
            get { return _FirstName; }
            set { if (value != _FirstName) { _FirstName = value; NotifyPropertyChanged(""); } }
        }

        public string LastName
        {
            get { return _LastName; }
            set { if (value != _LastName) { _LastName = value; NotifyPropertyChanged(""); } }
        }

        public IManager DirectManager
        {
            get { return _Manager; }
            set { if (value != _Manager) { _Manager = value; NotifyPropertyChanged(); } }
        }

        public bool IsManager
        {
            get { return GetIsManager(); }
        }

        protected virtual bool GetIsManager()
        {
            return false;
        }

        public string Title
        {
            get { return GetTitle(); }
        }

        protected virtual string GetTitle()
        {
            return "Developer";
        }

        public string Name
        {
            get { return FirstName + " " + LastName; }
        }

        public string GetName()
        {
            return Name;
        }

        public TShirtSize TShirt
        {
            get { return _TShirt; }
            set { if (value != _TShirt) { _TShirt = value; NotifyPropertyChanged(); } }
        }

        public bool IsEmployeeOfTheMonth
        {
            get { return _IsEmployeeOfTheMonth; }
            set { if (value != _IsEmployeeOfTheMonth) { _IsEmployeeOfTheMonth = value; NotifyPropertyChanged(); } }
        }

        protected void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new Microsoft.UI.Xaml.Data.PropertyChangedEventArgs(propertyName));
            }
        }
    }

    public sealed class VectorChangedEventAgrs : IVectorChangedEventArgs
    {
        public CollectionChange CollectionChange { get; set; }
        public uint Index { get; set; }
    }


    public sealed class EmployeeCollection : INotifyCollectionChanged, IList<IEmployee>, Windows.Foundation.Collections.IObservableVector<IEmployee>, IList
    {
        private ObservableCollection<IEmployee> data;
        #region Boring implementation delegating to data member

        public EmployeeCollection()
        {
            data = new ObservableCollection<IEmployee>();
            data.CollectionChanged += Data_CollectionChanged;
        }

        private void Data_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
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
            get { return data[index]; }
            set { data[index] = value; }
        }

        public int Count { get { return data.Count; } }

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

        bool IList.IsReadOnly { get { return false; } }

        int ICollection.Count { get { return data.Count; } }
        bool ICollection.IsSynchronized { get { return false; } }

        object ICollection.SyncRoot
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        object IList.this[int index]
        {
            get
            {
                return data[index];
            }

            set
            {
                data[index] = (Employee)value;
            }
        }

        public event VectorChangedEventHandler<IEmployee> VectorChanged;

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        public void Add(IEmployee item)
        {
            data.Add(item);
        }

        public void Clear()
        {
            data.Clear();
        }

        public bool Contains(IEmployee item)
        {
            return data.Contains(item);
        }

        public void CopyTo(IEmployee[] array, int arrayIndex)
        {
            data.CopyTo(array, arrayIndex);
        }

        public IEnumerator<IEmployee> GetEnumerator()
        {
            return data.GetEnumerator();
        }

        public int IndexOf(IEmployee item)
        {
            return data.IndexOf(item);
        }

        public void Insert(int index, IEmployee item)
        {
            data.Insert(index, item);
        }

        public bool Remove(IEmployee item)
        {
            return data.Remove(item);
        }

        public void RemoveAt(int index)
        {
            data.RemoveAt(index);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return data.GetEnumerator();
        }

        int IList.Add(object item)
        {
            data.Add((IEmployee)item);
            return data.Count;
        }

        bool IList.Contains(object item) { return data.Contains(item); }

        int IList.IndexOf(object item) { return data.IndexOf((IEmployee)item); }


        void IList.Insert(int index, object item)
        {
            data.Insert(index, (IEmployee)item);
        }

        void IList.Remove(object item)
        {
            data.Remove((IEmployee)item);
        }
        void IList.Clear()
        {
            data.Clear();
        }

        void IList.RemoveAt(int index)
        {
            data.RemoveAt(index);
        }

        void ICollection.CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }
        #endregion

    }

    public sealed class ObservableEmployees : IObservableVector<IEmployee>, INotifyCollectionChanged
    {
        private EmployeeCollection _collection = new EmployeeCollection();

        public event VectorChangedEventHandler<IEmployee> VectorChanged;

        public ObservableEmployees()
        {
            _collection.VectorChanged += _collection_VectorChanged;
        }

        public event NotifyCollectionChangedEventHandler CollectionChanged
        {
            add
            {
                ((INotifyCollectionChanged)_collection).CollectionChanged += value;
            }

            remove
            {
                ((INotifyCollectionChanged)_collection).CollectionChanged -= value;
            }
        }

        private void _collection_VectorChanged(IObservableVector<IEmployee> sender, IVectorChangedEventArgs @event)
        {
            this.VectorChanged?.Invoke(this, @event);
        }

        public int IndexOf(IEmployee item)
        {
            return _collection.IndexOf(item);
        }

        public void Insert(int index, IEmployee item)
        {
            _collection.Insert(index, item);
        }

        public void RemoveAt(int index)
        {
            _collection.RemoveAt(index);
        }

        public IEmployee this[int index] { get => _collection[index]; set => _collection[index] = value; }

        public void Add(IEmployee item)
        {
            _collection.Add(item);
        }

        public void Clear()
        {
            _collection.Clear();
        }

        public bool Contains(IEmployee item)
        {
            return _collection.Contains(item);
        }

        public void CopyTo(IEmployee[] array, int arrayIndex)
        {
            _collection.CopyTo(array, arrayIndex);
        }

        public bool Remove(IEmployee item)
        {
            return _collection.Remove(item);
        }

        public int Count => _collection.Count();

        public bool IsReadOnly => _collection.IsReadOnly;

        public IEnumerator<IEmployee> GetEnumerator()
        {
            return _collection.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return _collection.GetEnumerator();
        }
    }
}
