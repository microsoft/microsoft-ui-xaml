// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.Data
{
    public class Person : INotifyPropertyChanged
    {
        #region Private Fields

        private static int globalId = 0;   // not thread-safe
        private int id;
        private string firstName;
        private string lastName;
        private string middleName;
        private bool likesCake;
        private string cake = String.Empty;
        private Uri homepage = null;
        private DateTime dob;
        private EyeColor eyeColor = EyeColor.Unknown;
        private int _age;

        #endregion Private Fields

        #region Constructors

        public Person()
        {
        }

        public Person(string firstName, string lastName)
            : this(firstName, String.Empty, lastName)
        {
        }        

        public Person(string firstName, string lastName, DateTime dob)
            : this(firstName, String.Empty, lastName, dob)
        {
        }

        public Person(string firstName, string middleName, string lastName)
            : this(firstName, middleName, lastName, new DateTime(2008, 10, 26))
        {
        }

        public Person(string firstName, string middleName, string lastName, DateTime dob)
            : this(firstName, middleName, lastName, true, "Chocolate", dob)
        {
        }

        public Person(string firstName, string middleName, string lastName, bool likeCake, string cake, DateTime dob)
        {
            FirstName = firstName;
            MiddleName = middleName;
            LastName = lastName;
            LikesCake = likeCake;
            Cake = cake;
            DOB = dob;
            Id = globalId++;

            string prefix = firstName.ToLower() + "." + lastName.ToLower();
            Homepage = new Uri("http://" + prefix.Replace(' ', '_') + ".whitehouse.gov/");
        }

        public Person(string firstName, string lastName, DateTime dob, int age)
            : this(firstName, String.Empty, lastName, true, "Chocolate", dob, age)
        {
        }

        public Person(string firstName, string middleName, string lastName, bool likeCake, string cake, DateTime dob, int age = 100)
        {
            FirstName = firstName;
            MiddleName = middleName;
            LastName = lastName;
            LikesCake = likeCake;
            Cake = cake;
            DOB = dob;
            Id = globalId++;
            Age = age;

            string prefix = firstName.ToLower() + "." + lastName.ToLower();
            Homepage = new Uri("http://" + prefix.Replace(' ', '_') + ".whitehouse.gov/");
        }

        #endregion Constructors

        #region Public Properties

        public int Id
        {
            get { return id; }
            set
            {
                id = value;
                OnPropertyChanged("Id");
            }
        }

        public string FirstName
        {
            get { return firstName; }
            set
            {
                firstName = value;
                OnPropertyChanged("FirstName");
            }
        }

        public string MiddleName
        {
            get { return middleName; }
            set
            {
                middleName = value;
                OnPropertyChanged("MiddleName");
            }
        }

        public string LastName
        {
            get { return lastName; }
            set
            {
                lastName = value;
                OnPropertyChanged("LastName");
            }
        }

        public bool LikesCake
        {
            get { return likesCake; }
            set
            {
                likesCake = value;
                OnPropertyChanged("LikesCake");
            }
        }

        public string Cake
        {
            get { return cake; }
            set
            {
                cake = value;
                OnPropertyChanged("Cake");
            }
        }

        public Uri Homepage
        {
            get
            {
                return homepage;
            }
            set
            {
                homepage = value;
                OnPropertyChanged("Homepage");
            }
        }

        public DateTime DOB
        {
            get
            {
                return dob;
            }
            set
            {
                dob = value;
                OnPropertyChanged("DOB");
            }
        }

        public EyeColor MyEyeColor
        {
            get
            {
                return eyeColor;
            }
            set
            {
                eyeColor = value;
                OnPropertyChanged("MyEyeColor");
            }
        }

        public int Age
        {
            get { return _age; }
            set
            {
                _age = value;
                OnPropertyChanged("Age");
            }
        }

        #endregion Public Properties

        #region Public Members

        public enum EyeColor
        {
            Unknown,
            Blue,
            Green,
            Brown,
            Other
        };

        public override string ToString()
        {
            return FirstName + " " + LastName;
        }

        public override bool Equals(object obj)
        {
            if (obj != null && obj is Person)
            {
                return this.FirstName == (obj as Person).FirstName &&
                       this.LastName == (obj as Person).LastName &&
                       this.Id == (obj as Person).Id;
            }
            else
            {
                return false;
            }
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        #endregion Public Members

        #region INotifyPropertyChanged

        public event PropertyChangedEventHandler PropertyChanged;

        private void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        #endregion INotifyPropertyChanged

    }

    public class DataSourceHelper
    {
        private static readonly Random _random = new Random();

        public static ObservableCollection<object> GeneratePersonDataSource(int limit = Int32.MaxValue, int multiplier = 1)
        {
            ObservableCollection<object> ds = new ObservableCollection<object>();

            for (int i = 0; i < multiplier; i++)
            {
                ds.Add(new Person("George", "Washington", GenerateRandomDate()));
                ds.Add(new Person("John", "Adams", GenerateRandomDate()));
                ds.Add(new Person("Thomas", "Jefferson", GenerateRandomDate()));
                ds.Add(new Person("James", "Madison", GenerateRandomDate()));                
                ds.Add(new Person("John", "Quincy", "Adams", GenerateRandomDate()));
                ds.Add(new Person("James", "Monroe", GenerateRandomDate()));
                ds.Add(new Person("Andrew", "Jackson", GenerateRandomDate()));
                ds.Add(new Person("Martin", "Van Buren", GenerateRandomDate()));
                ds.Add(new Person("William", "H.", "Harrison", GenerateRandomDate()));
                ds.Add(new Person("John", "Tyler", GenerateRandomDate()));
                ds.Add(new Person("James", "K.", "Polk", GenerateRandomDate()));
                ds.Add(new Person("Zachary", "Taylor", GenerateRandomDate()));
                ds.Add(new Person("Millard", "Fillmore", GenerateRandomDate()));
                ds.Add(new Person("Franklin", "Pierce", GenerateRandomDate()));
                ds.Add(new Person("James", "Buchanan", GenerateRandomDate()));
                ds.Add(new Person("Abraham", "Lincoln", GenerateRandomDate()));
                ds.Add(new Person("Andrew", "Johnson", GenerateRandomDate()));
                ds.Add(new Person("Ulysses", "S.", "Grant", GenerateRandomDate()));
                ds.Add(new Person("Rutherford", "B.", "Hayes", GenerateRandomDate()));
                ds.Add(new Person("James", "Garfield", GenerateRandomDate()));
                ds.Add(new Person("Chester", "A.", "Arthur", GenerateRandomDate()));
                ds.Add(new Person("Grover", "Cleveland", GenerateRandomDate()));
                ds.Add(new Person("Benjamin", "Harrison", GenerateRandomDate()));
                ds.Add(new Person("William", "McKinley", GenerateRandomDate()));
                ds.Add(new Person("Theodore", "Roosevelt", GenerateRandomDate()));
                ds.Add(new Person("William", "H.", "Taft", GenerateRandomDate()));
                ds.Add(new Person("Woodrow", "Wilson", GenerateRandomDate()));
                ds.Add(new Person("Warren", "G.", "Harding", GenerateRandomDate()));
                ds.Add(new Person("Calvin", "Coolidge", GenerateRandomDate()));
                ds.Add(new Person("Herbert", "Hoover", GenerateRandomDate()));
                ds.Add(new Person("Franklin", "D.", "Roosevelt", GenerateRandomDate()));
                ds.Add(new Person("Harry", "S.", "Truman", GenerateRandomDate()));
                ds.Add(new Person("Dwight", "D.", "Eisenhower", GenerateRandomDate()));
                ds.Add(new Person("John", "F.", "Kennedy", GenerateRandomDate()));
                ds.Add(new Person("Lyndon", "B.", "Johnson", GenerateRandomDate()));
                ds.Add(new Person("Richard", "Nixon", GenerateRandomDate()));
                ds.Add(new Person("Gerald", "Ford", GenerateRandomDate()));
                ds.Add(new Person("Jimmy", "Carter", GenerateRandomDate()));
                ds.Add(new Person("Ronald", "Reagan", GenerateRandomDate()));
                ds.Add(new Person("George", "Bush", GenerateRandomDate()));
                ds.Add(new Person("Bill", "Clinton", GenerateRandomDate()));
                ds.Add(new Person("George", "W.", "Bush", GenerateRandomDate()));
            }

            if (limit != Int32.MaxValue)
            {
                return new ObservableCollection<object>(ds.Take(limit).ToList());
            }
            else
            {
                return ds;
            }
        }

        public static DateTime GenerateRandomDate()
        {
            return new DateTime(_random.Next(1900, 2000), _random.Next(1, 12), _random.Next(1, 28));
        }
    }

    public partial class CustomVectorChangedEventArgs : IVectorChangedEventArgs
    {
        #region IVectorChangedEventArgs Members

        public CollectionChange CollectionChange
        {
            get;
            set;
        }

        public uint Index
        {
            get;
            set;
        }

        #endregion
    }

    public class CustomVectorView : IReadOnlyList<object>, IEnumerable<object>, IEnumerable
    {
        private ObservableCollection<object> _internalCollection;

        public CustomVectorView(ObservableCollection<object> list)
        {
            _internalCollection = list;
        }

        #region IVectorView<object> Members

        public object GetAt(uint index)
        {
            return _internalCollection[(int)index];
        }

        public uint GetMany(uint startIndex, object[] items)
        {
            throw new NotImplementedException();
        }

        public bool IndexOf(object value, out uint index)
        {
            index = (uint)_internalCollection.IndexOf(value);
            return true;
        }

        public int Count
        {
            get
            {
                return _internalCollection.Count;
            }
        }

        public object this[int index]
        {
            get
            {
                return _internalCollection[index];
            }
        }
        #endregion

        #region IEnumerable Members

        public IEnumerator GetEnumerator()
        {
            return _internalCollection.GetEnumerator();
        }

        #endregion

        #region IEnumerable<object> Members

        IEnumerator<object> IEnumerable<object>.GetEnumerator()
        {
            return _internalCollection.GetEnumerator();
        }

        #endregion
    }

    public partial class CustomCollection : IList<object>, IEnumerable<object>, IEnumerable, ISupportIncrementalLoading, IObservableVector<object>
    {
        private ObservableCollection<object> _interalCollection;
        private CustomVectorView _customVectorView;

        public CustomCollection(ObservableCollection<object> list)
        {
            MaxItemCount = Int32.MaxValue;

            _interalCollection = list;
            _customVectorView = new CustomVectorView(list);
        }

        #region IList<object> Members

        public int IndexOf(object item)
        {
            return _interalCollection.IndexOf(item);
        }

        public void Insert(int index, object item)
        {
            _interalCollection.Insert(index, item);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemInserted, Index = (uint)index });
            }
        }

        public void RemoveAt(int index)
        {
            _interalCollection.RemoveAt(index);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemRemoved, Index = (uint)index });
            }
        }

        public object this[int index]
        {
            get
            {
                return _interalCollection[index];
            }
            set
            {
                _interalCollection[index] = value;

                if (VectorChanged != null)
                {
                    VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemChanged, Index = (uint)index });
                }
            }
        }

        #endregion

        #region ICollection<object> Members

        public void Add(object item)
        {
            _interalCollection.Add(item);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemInserted, Index = (uint)(_interalCollection.Count - 1) });
            }
        }

        public bool Contains(object item)
        {
            return _interalCollection.Contains(item);
        }

        public void CopyTo(object[] array, int arrayIndex)
        {
            _interalCollection.CopyTo(array, arrayIndex);
        }

        public int Count
        {
            get
            {
                return _interalCollection.Count;
            }
        }

        public bool IsReadOnly
        {
            get
            {
                return false;
            }
        }

        public bool Remove(object item)
        {
            int index = _interalCollection.IndexOf(item);
            bool retVal = _interalCollection.Remove(item);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemRemoved, Index = (uint)index });
            }

            return retVal;
        }

        #endregion

        #region IEnumerable Members

        IEnumerator IEnumerable.GetEnumerator()
        {
            return _interalCollection.GetEnumerator();
        }

        #endregion

        #region IEnumerable<object> Members

        public IEnumerator<object> GetEnumerator()
        {
            return _interalCollection.GetEnumerator();
        }

        #endregion

        #region IObservableVector<object> Members

        public event VectorChangedEventHandler<object> VectorChanged;

        #endregion

        #region IVector<object> Members

        public void Append(object value)
        {
            _interalCollection.Add(value);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemInserted, Index = (uint)(_interalCollection.Count - 1) });
            }
        }

        public void Clear()
        {
            _interalCollection.Clear();

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.Reset, Index = 0 });
            }
        }

        public object GetAt(uint index)
        {
            return _interalCollection[(int)index];
        }

        public uint GetMany(uint startIndex, object[] items)
        {
            throw new NotImplementedException();
        }

        public IReadOnlyList<object> GetView()
        {
            return _customVectorView;
        }

        public bool IndexOf(object value, out uint index)
        {
            index = (uint)_interalCollection.IndexOf(value);
            return true;
        }

        public void InsertAt(uint index, object value)
        {
            _interalCollection.Insert((int)index, value);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemInserted, Index = index });
            }
        }

        public void RemoveAt(uint index)
        {
            _interalCollection.RemoveAt((int)index);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemRemoved, Index = index });
            }
        }

        public void RemoveAtEnd()
        {
            _interalCollection.RemoveAt(_interalCollection.Count - 1);

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemRemoved, Index = (uint)(_interalCollection.Count - 1) });
            }
        }

        public void ReplaceAll(object[] items)
        {
            throw new NotImplementedException();
        }

        public void SetAt(uint index, object value)
        {
            _interalCollection[(int)index] = value;

            if (VectorChanged != null)
            {
                VectorChanged(this, new CustomVectorChangedEventArgs { CollectionChange = CollectionChange.ItemChanged, Index = index });
            }
        }

        public uint Size
        {
            get
            {
                return (uint)_interalCollection.Count;
            }
        }

        #endregion

        #region ISupportIncrementalLoading Members

        public bool HasMoreItems
        {
            get;
            set;
        }

        public global::Windows.Foundation.IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
        {
            LastNumItemsToLoad = count;
            if (LoadMoreItemsAsyncFired != null)
            {
                LoadMoreItemsAsyncFired(this, null);
            }

            return new LoadMoreItemsAsync(this, count);
        }

        public int MaxItemCount { get; set; }

        #endregion

        public event EventHandler<object> LoadMoreItemsAsyncFired;
        public uint LastNumItemsToLoad;
    }

    public partial class LoadMoreItemsAsync : global::Windows.Foundation.IAsyncOperation<LoadMoreItemsResult>
    {
        private CustomCollection _internalCollection;
        private uint _numItemsToLoad;

        public int CloseMethodCallCount { get; set; }

        public LoadMoreItemsAsync(CustomCollection collection, uint count)
        {
            _internalCollection = collection;
            _numItemsToLoad = count;
            CloseMethodCallCount = 0;

            Start();
        }

        #region IAsyncOperation<LoadMoreItemsResult> Members

        public global::Windows.Foundation.AsyncOperationCompletedHandler<LoadMoreItemsResult> Completed
        {
            get
            {
                return _completed;
            }
            set
            {
                _completed = value;
            }
        }

        private global::Windows.Foundation.AsyncOperationCompletedHandler<LoadMoreItemsResult> _completed;

        public LoadMoreItemsResult GetResults()
        {
            return new LoadMoreItemsResult { Count = _numItemsToLoad };
        }

        #endregion

        #region IAsyncInfo Members

        public void Cancel()
        {
        }

        public void Close()
        {
            CloseMethodCallCount++;
        }

        public Exception ErrorCode
        {
            get { throw new NotImplementedException(); }
        }

        public uint Id
        {
            get { throw new NotImplementedException(); }
        }

        public void Start()
        {
            if (_internalCollection.Count + _numItemsToLoad < _internalCollection.MaxItemCount)
            {
                // add more items to the collection
                for (int i = 0; i < _numItemsToLoad; i++)
                {
                    _internalCollection.Add(new Person("George", "Washington", DataSourceHelper.GenerateRandomDate()));
                }
            }

            if (Completed != null)
            {
                Completed(this, Status);
            }
        }

        public global::Windows.Foundation.AsyncStatus Status
        {
            get 
            {
                return global::Windows.Foundation.AsyncStatus.Completed;
            }
        }

        #endregion
    }
}
