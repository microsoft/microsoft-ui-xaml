// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Popups;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Data;

namespace BindTestbedModel
{
    public struct Coordinates
    {
        public double Longitude;
        public double Latitude;
    }

    public struct Location
    {
        public Coordinates Coords;
        public int Altitude;
    }

    public enum TShirtSize
    {
        XtraSmall,
        Small,
        Medium,
        Large,
        XtraLarge
    };

    public sealed class DataModel : DependencyObject, Microsoft.UI.Xaml.Data.INotifyPropertyChanged // TODO: add Microsoft.UI.Xaml.Data.INotifyDataErrorInfo support
    {
        public DataModel()
        {
            ThatOneGuy.Add("You Know", null); // key must always exist
            _managers.Add(new Manager());
            _managers.Add(new Manager());
            this.NestedLoad = new NestedLoadModel();
            InitializeValues();
        }

        static Random rnd = new Random(8638665);
        public IObservableVector<IManager> ManagerCollection { get; set; }

        public string StringPropNoINPC { get; set; }
        public string StringPropAsInteger { get; set; }
        public int IntPropNoINPC { get; set; }
        public int IntPropertyForFunctionINPC { get; set; }
        public IEmployee EmployeePropNoINPC { get; set; }
        private Location _ExtractionPoint;
        public Location ExtractionPoint
        {
            get { return _ExtractionPoint; }
        }
        private string _StringPropWithINPC;
        public string StringPropWithINPC
        {
            get { return _StringPropWithINPC; }
            set
            {
                if (value != _StringPropWithINPC)
                {
                    _StringPropWithINPC = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private int _IntPropWithINPC;
        public int IntPropWithINPC
        {
            get { return _IntPropWithINPC; }
            set
            {
                if (value != _IntPropWithINPC)
                {
                    _IntPropWithINPC = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool _BoolPropWithINPC;
        public bool BoolPropWithINPC
        {
            get { return _BoolPropWithINPC; }
            set
            {
                if (value != _BoolPropWithINPC)
                {
                    _BoolPropWithINPC = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private bool _BoolPropWithINPCDefaultTrue;
        public bool BoolPropWithINPCDefaultTrue
        {
            get { return _BoolPropWithINPCDefaultTrue; }
            set
            {
                if (value != _BoolPropWithINPCDefaultTrue)
                {
                    _BoolPropWithINPCDefaultTrue = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public Boolean? NullableBoolProperty { get; } = true;

        private Color _ColorPropWithINPC;
        public Color ColorPropWithINPC
        {
            get { return _ColorPropWithINPC; }
            set
            {
                if (value != _ColorPropWithINPC)
                {
                    _ColorPropWithINPC = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private IEmployee _EmployeePropWithINPC;
        public IEmployee EmployeePropWithINPC
        {
            get { return _EmployeePropWithINPC; }
            set
            {
                if (value != _EmployeePropWithINPC)
                {
                    _EmployeePropWithINPC = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private EmployeeCollection _Employees;
        public EmployeeCollection Employees
        {
            get { return _Employees; }
            set
            {
                if (value != _Employees)
                {
                    _Employees = value;
                    NotifyPropertyChanged();
                }
            }
        }

        public IObservableMap<string, IEmployee> ThatOneGuy { get; private set; } = new ObservableMap<string, IEmployee>();

        private IManager _ManagerProp;
        public IManager ManagerProp
        {
            get { return _ManagerProp; }
            set
            {
                if (value != _ManagerProp)
                {
                    _ManagerProp = value;
                    NotifyPropertyChanged();
                }
            }
        }

        private List<IManager> _managers = new List<IManager>();
        public IList<IManager> Managers
        {
            get { return _managers; }
        }

        public object ManagerObjectForCasting
        {
            get { return Managers[0]; }
        }

        public int IntPropertyDP
        {
            get { return (int)GetValue(IntPropertyDPProperty); }
            set { SetValue(IntPropertyDPProperty, value); }
        }
        #region IntPropertyDP DP
        private const string IntPropertyDPName = "IntPropertyDP";
        private static readonly DependencyProperty _IntPropertyDPProperty =
            DependencyProperty.Register(IntPropertyDPName, typeof(int), typeof(DataModel), new PropertyMetadata(0));
        public static DependencyProperty IntPropertyDPProperty { get { return _IntPropertyDPProperty; } }
        #endregion

        public string StringPropertyDP
        {
            get { return (string)GetValue(StringPropertyDPProperty); }
            set { SetValue(StringPropertyDPProperty, value); }
        }
        #region StringPropertyDP DP
        private const string StringPropertyDPName = "StringPropertyDP";
        private static readonly DependencyProperty _StringPropertyDPProperty =
            DependencyProperty.Register(StringPropertyDPName, typeof(string), typeof(DataModel), new PropertyMetadata(""));
        public static DependencyProperty StringPropertyDPProperty { get { return _StringPropertyDPProperty; } }
        #endregion

        public IEmployee EmployeePropertyDP
        {
            get { return (IEmployee)GetValue(EmployeePropertyDPProperty); }
            set { SetValue(EmployeePropertyDPProperty, value); }
        }
        #region EmployeePropertyDP DP
        private const string EmployeePropertyDPName = "EmployeePropertyDP";
        private static readonly DependencyProperty _EmployeePropertyDPProperty =
            DependencyProperty.Register(EmployeePropertyDPName, typeof(IEmployee), typeof(DataModel), new PropertyMetadata(0));
        public static DependencyProperty EmployeePropertyDPProperty { get { return _EmployeePropertyDPProperty; } }

        #endregion

        #region "Function Binding"

        public float FloatProperty
        {
            get
            {
                return 8.1F;
            }
        }

        public short ShortProperty
        {
            get
            {
                return 8;
            }
        }

        public static string StaticStringProperty
        {
            get
            {
                return "StaticStringProperty";
            }
        }

        public string FunctionOnModelNoArgs()
        {
            return "FunctionOnModelNoArgs_" + IntPropWithINPC.ToString();
        }

        public string FunctionOnModelOneStringArg(string arg)
        {
            return String.Format("FunctionOnModelOneStringArg({0})", arg);
        }

        public string FunctionOnModelThreeArgs(string arg, double doubleArg, bool booleanArg)
        {
            string argString = arg ?? "null_arg";
            return String.Format("FunctionOnModelThreeArgs({0}, {1}, {2})", argString, doubleArg, booleanArg);
        }

        public string FunctionOnModelTwoArgs(int intArg, double doubleArg)
        {
            return String.Format("FunctionOnModelTwoArgs({0}, {1})", intArg, doubleArg);
        }

        public string FunctionOnModelInfrequentTypes(short shortArg, float floatArg)
        {
            return String.Format("FunctionOnModelInfrequentTypes({0}, {1})", shortArg, floatArg);
        }

        public string FunctionOnModelNullArg(string strArg)
        {
            return String.Format("FunctionOnModelNullArg({0})", strArg);
        }

        public static string StaticFunctionOnModelNoArgs()
        {
            return "StaticFunctionOnModelNoArgs";
        }

        public IEmployee NullEmployee
        {
            get { return null; }
        }

        public string FunctionReturningNull()
        {
            return null;
        }

        public int FunctionReturningInt()
        {
            return 16;
        }

        public int FunctionReturningIntProperty()
        {
            return IntPropertyForFunctionINPC;
        }

        public string ArityTest() { return "ArityTest()"; }
        public string ArityTest(int a) { return "ArityTest(int)"; }
        public string ArityTest(int a, int b) { return "ArityTest(int, int)"; }
        public string ArityTest(int a, int b, int c) { return "ArityTest(int, int, int)"; }

        #endregion

        public NestedLoadModel NestedLoad { get; set; }

        public void ShowMessageDialog(string message)
        {
            MessageDialog dlg = new MessageDialog(message);
            var t = dlg.ShowAsync();
        }

        public void InitializeValues()
        {
            ManagerProp = CreateManagerEmployeeTree();
            Employees = ManagerProp.ReportsOC;

            StringPropNoINPC = "String Property without INPC";
            StringPropWithINPC = "Private string with INPC";
            ColorPropWithINPC = Colors.Red;
            IntPropertyForFunctionINPC = 5;
            IntPropNoINPC = 42;
            IntPropWithINPC = 2;
            IntPropertyDP = 42;
            BoolPropWithINPCDefaultTrue = true;
            StringPropAsInteger = "65";
            EmployeePropNoINPC = Employees[0];
            EmployeePropWithINPC = Employees[1];
            ThatOneGuy["You Know"] = EmployeePropWithINPC;
            _ExtractionPoint.Coords.Latitude = 47;
            _ExtractionPoint.Coords.Longitude = 123;
            _ExtractionPoint.Altitude = 420;
            _managers[0].FirstName = "Rob";
            _managers[0].LastName = "S";
            _managers[1].FirstName = "Bill";
            _managers[1].LastName = "G";
            SelectEmployeeOfTheMonth();
        }

        public void UpdateValues()
        {
            this.IntPropertyForFunctionINPC = IntPropertyForFunctionINPC + 10 % 100;
            NotifyPropertyChanged("FunctionReturningIntProperty");

            this.IntPropNoINPC = IntPropNoINPC + 10 % 100;
            this.IntPropWithINPC = IntPropWithINPC + 1 % 10;
            this.IntPropertyDP = IntPropertyDP + 10 % 100;

            this.BoolPropWithINPC = !this.BoolPropWithINPC;

            string ms = DateTime.Now.Millisecond.ToString();
            this.StringPropNoINPC = ms;
            this.StringPropWithINPC = ms;
            this.StringPropertyDP = ms;

            EmployeePropWithINPC = Employees[IntPropWithINPC % Employees.Count];
            ThatOneGuy["You Know"] = EmployeePropWithINPC;
            EmployeePropertyDP = Employees[(IntPropWithINPC + 1) % Employees.Count];
            // Swap Manager Reports [0], [2]
            var temp = ManagerProp.ReportsOC[0];
            ManagerProp.ReportsOC[0] = ManagerProp.ReportsOC[2];
            ManagerProp.ReportsOC[2] = temp;
            Managers[0].FirstName = "Rob" + IntPropertyDP.ToString();
            Managers[0].LastName = "S" + IntPropertyDP.ToString();

            Employees[0].Visibility = !Employees[0].Visibility;

            SelectEmployeeOfTheMonth();
        }

        void SelectEmployeeOfTheMonth()
        {
            foreach (IEmployee e in Employees.Where(e => e.IsEmployeeOfTheMonth))
            {
                e.IsEmployeeOfTheMonth = false;
            }
            Employees[rnd.Next() % (Employees.Count - 1)].IsEmployeeOfTheMonth = true;
        }

        #region Employee Tree Creation
        private static string[] fnames = { "Alice", "Brian", "Chris", "David", "Emily", "Freda", "Garry", "Harriet", "Isobel", "John", "Keith", "Lisa", "Mark", "Nathan", "Olivia", "Penelope", "Quentin", "Robert", "Simon", "Tom", "Veronica", "Will", "Xavier", "Yvette", "Zach" };
        private static string[] lnames = { "Smith", "Johnson", "Williams", "Jones", "Brown", "Davis", "Miller", "Wilson", "Moore", "Taylor", "Anderson", "Thomas", "Jackson", "White", "Harris", "Martin", "Thompson", "Garcia", "Martinez", "Robinson", "Clark", "Rodriguez", "Lewis", "Lee", "Walker" };

        public static IManager CreateManagerEmployeeTree()
        {
            Manager Alice = new Manager() { FirstName = "Alice", LastName = "Smith" };
            CreateReports(5, Alice, rnd);
            return Alice;
        }

        private static void CreateReports(int depth, Manager m, Random rnd)
        {
            int count = rnd.Next(10) + 3;
            for (int i = 0; i < count; i++)
            {
                int j = rnd.Next(10);
                IEmployee e = (j > 2 && depth > 1) ? (IEmployee)new Manager() : (IEmployee)new Employee();
                e.LastName = lnames[rnd.Next(lnames.Length)];
                e.FirstName = fnames[rnd.Next(fnames.Length)];
                e.DirectManager = m;
                m.ReportsList.Add(e);
                if (e.IsManager) CreateReports(depth - 1, (Manager)e, rnd);
            }
        }

        #endregion
        #region INPC

        public event Microsoft.UI.Xaml.Data.PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new Microsoft.UI.Xaml.Data.PropertyChangedEventArgs(propertyName));
            }
        }
        // Allow these to throw, compiler should not be calling these methods or generating code since InputValidation
        // is not available pre-RS5.
        public IEnumerable<object> GetErrors(string propertyName)
        {
            throw new NotImplementedException();
        }

        public bool HasErrors => throw new NotImplementedException();
        /* TODO: INotifyDataErrorInfo support
        public event EventHandler<Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs> ErrorsChanged
        {
            add { throw new NotImplementedException(); }
            remove { throw new NotImplementedException(); }
        }
        */
        #endregion

    }

    internal sealed class ObservableMap<K, V> : IObservableMap<K, V>
    {
        private Dictionary<K, V> _map = new Dictionary<K, V>();
#pragma warning disable 649
        private K _emptyKey;
#pragma warning restore 649

        private sealed class MapChangedEventArgs : IMapChangedEventArgs<K>
        {
            public MapChangedEventArgs(CollectionChange change, K key)
            {
                CollectionChange = change;
                Key = key;
            }

            public CollectionChange CollectionChange { get; private set; }
            public K Key { get; private set; }
        }
        private void RaiseMapChanged(CollectionChange change, K key)
        {
            if (null != MapChanged)
            {
                MapChanged(this, new MapChangedEventArgs(change, key));
            }
        }

        public V this[K key]
        {
            get { return _map[key]; }
            set
            {
                _map[key] = value;
                RaiseMapChanged(CollectionChange.ItemChanged, key);
            }
        }

        public int Count { get { return _map.Count; } }

        public bool IsReadOnly { get { return false; } }

        public ICollection<K> Keys { get { return _map.Keys; } }

        public ICollection<V> Values { get { return _map.Values; } }

        public event MapChangedEventHandler<K, V> MapChanged;

        public void Add(KeyValuePair<K, V> item) { Add(item.Key, item.Value); }

        public void Add(K key, V value)
        {
            _map.Add(key, value);
            RaiseMapChanged(CollectionChange.ItemInserted, key);
        }

        public void Clear()
        {
            _map.Clear();
            RaiseMapChanged(CollectionChange.Reset, _emptyKey);
        }

        public bool Contains(KeyValuePair<K, V> item) { return ContainsKey(item.Key); }

        public bool ContainsKey(K key) { return _map.ContainsKey(key); }

        public void CopyTo(KeyValuePair<K, V>[] array, int arrayIndex)
        {
            throw new NotImplementedException();
        }

        public IEnumerator<KeyValuePair<K, V>> GetEnumerator() { return _map.GetEnumerator(); }

        public bool Remove(KeyValuePair<K, V> item) { return Remove(item.Key); }

        public bool Remove(K key)
        {
            if (_map.Remove(key))
            {
                RaiseMapChanged(CollectionChange.ItemRemoved, key);
                return true;
            }
            return false;
        }

        public bool TryGetValue(K key, out V value) { return _map.TryGetValue(key, out value); }

        IEnumerator IEnumerable.GetEnumerator() { return _map.GetEnumerator(); }
    }

}
