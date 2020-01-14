using Microsoft.UI.Xaml.Controls;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using System.Linq;
using System.Collections.Generic;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class SortingAndFilteringPage : Page
    {
        MyCollection SourceForRepeater = new MyCollection(null);

        public SortingAndFilteringPage()
        {
            this.InitializeComponent();

            SourceForRepeater.InitializeCollection(typeof(SortingAndFilteringPage).GetTypeInfo().Assembly.GetTypes());
            repeater.ItemsSource = SourceForRepeater;
        }

        private void OnSortAscClick(object sender, RoutedEventArgs e)
        {
            SourceForRepeater.InitializeCollection(typeof(SortingAndFilteringPage).GetTypeInfo().Assembly.GetTypes().OrderBy(i => i.ToString()));
        }

        private void OnSortDesClick(object sender, RoutedEventArgs e)
        {
            SourceForRepeater.InitializeCollection(typeof(SortingAndFilteringPage).GetTypeInfo().Assembly.GetTypes().OrderByDescending(i => i.ToString()));
        }

        private void filterText_TextChanged(object sender, TextChangedEventArgs e)
        {
            SourceForRepeater.InitializeCollection(typeof(SortingAndFilteringPage).GetTypeInfo().Assembly.GetTypes().Where(i => i.ToString().Contains(filterText.Text)));
        }
    }

    public class MyCollection : IList, INotifyCollectionChanged, IKeyIndexMapping
    {
        private List<object> inner = new List<object>();

        public int Count => inner.Count;

        public object this[int index]
        {
            get
            {
                return inner[index];
            }

            set
            {
                inner[index] = value;
            }
        }

        public MyCollection(IEnumerable<object> collection)
        {
            InitializeCollection(collection);
        }

        public void InitializeCollection(IEnumerable<object> collection)
        {
            inner.Clear();
            if (collection != null)
            {
                inner.AddRange(collection);
            }

            if (CollectionChanged != null)
            {
                CollectionChanged(this, new NotifyCollectionChangedEventArgs(NotifyCollectionChangedAction.Reset));
            }
        }

        public event NotifyCollectionChangedEventHandler CollectionChanged;

        public string KeyFromIndex(int index)
        {
            return inner[index].ToString();
        }

        public int IndexFromKey(string key)
        {
            return inner.IndexOf(int.Parse(key));
        }

        #region Not used by ItemsRepeater

        public bool IsSynchronized => false;


        public bool IsFixedSize => false;

        public bool IsReadOnly => false;


        public object SyncRoot => throw new NotImplementedException();

        public int Add(object value)
        {
            throw new NotImplementedException();
        }

        public void Clear()
        {
            throw new NotImplementedException();
        }

        public bool Contains(object value)
        {
            throw new NotImplementedException();
        }

        public int IndexOf(object value)
        {
            throw new NotImplementedException();
        }

        public void Insert(int index, object value)
        {
            throw new NotImplementedException();
        }

        public void Remove(object value)
        {
            throw new NotImplementedException();
        }

        public void RemoveAt(int index)
        {
            throw new NotImplementedException();
        }

        public void CopyTo(Array array, int index)
        {
            throw new NotImplementedException();
        }

        public IEnumerator GetEnumerator()
        {
            throw new NotImplementedException();
        }
        #endregion
    }


}