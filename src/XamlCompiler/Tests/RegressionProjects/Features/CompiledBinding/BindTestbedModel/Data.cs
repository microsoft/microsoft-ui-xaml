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
using System.ComponentModel.DataAnnotations;

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

    public sealed class DataModel : DependencyObject, INotifyPropertyChanged
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
                SetValue(ref _StringPropWithINPC, value);
            }
        }

        private int _IntPropWithINPC;
        public int IntPropWithINPC
        {
            get { return _IntPropWithINPC; }
            set
            {
                SetValue(ref _IntPropWithINPC, value);
            }
        }

        // Regression: github #4787 / #7986. A nullable value type (projects as
        // Windows.Foundation.IReference<Int32>) so it can be two-way bound to an Object/IInspectable
        // target and exercise the nullable box/unbox codegen path.
        private int? _NullableIntPropWithINPC = 42;
        public int? NullableIntPropWithINPC
        {
            get { return _NullableIntPropWithINPC; }
            set
            {
                SetValue(ref _NullableIntPropWithINPC, value);
            }
        }

        private bool _BoolPropWithINPC;
        public bool BoolPropWithINPC
        {
            get { return _BoolPropWithINPC; }
            set
            {
                SetValue(ref _BoolPropWithINPC, value);
            }
        }

        private bool _BoolPropWithINPCDefaultTrue;
        public bool BoolPropWithINPCDefaultTrue
        {
            get { return _BoolPropWithINPCDefaultTrue; }
            set
            {
                SetValue(ref _BoolPropWithINPCDefaultTrue, value);
            }
        }

        public Boolean? NullableBoolProperty { get; set; } = true;
        public Boolean? AlwaysNullNullableBoolProperty { get; set; } = null;

        private Color _ColorPropWithINPC;
        public Color ColorPropWithINPC
        {
            get { return _ColorPropWithINPC; }
            set
            {
                SetValue(ref _ColorPropWithINPC, value);
            }
        }

        private IEmployee _EmployeePropWithINPC;
        public IEmployee EmployeePropWithINPC
        {
            get { return _EmployeePropWithINPC; }
            set
            {
                SetValue(ref _EmployeePropWithINPC, value);
            }
        }

        private EmployeeCollection _Employees;
        public EmployeeCollection Employees
        {
            get { return _Employees; }
            set
            {
                SetValue(ref _Employees, value);
            }
        }

        public IObservableMap<string, IEmployee> ThatOneGuy { get; private set; } = new ObservableMap<string, IEmployee>();

        private IManager _ManagerProp;
        public IManager ManagerProp
        {
            get { return _ManagerProp; }
            set
            {
                SetValue(ref _ManagerProp, value);
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

        public double? NullableDoublePropertyDP
        {
            get { return (double?)GetValue(NullableDoublePropertyDPProperty); }
            set { SetValue(NullableDoublePropertyDPProperty, value); }
        }
        #region NullableDoublePropertyDP DP
        private const string NullableDoublePropertyDPName = "NullableDoublePropertyDP";
        private static readonly DependencyProperty _NullableDoublePropertyDPProperty =
            DependencyProperty.Register(NullableDoublePropertyDPName, typeof(double?), typeof(DataModel), new PropertyMetadata(0));
        public static DependencyProperty NullableDoublePropertyDPProperty { get { return _NullableDoublePropertyDPProperty; } }
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
            return String.Format("FunctionOnModelOneStringArg({0})", arg) ;
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

        public double? FunctionReturningNullableDouble(double? a)
        {
            if (!a.HasValue)
            {
                return null;
            }

            return new Nullable<double>((double)(a.Value + 10));
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
            ColorPropWithINPC = Microsoft.UI.Colors.Red;
            IntPropertyForFunctionINPC = 5;
            IntPropNoINPC = 42;
            IntPropWithINPC = 2;
            IntPropertyDP = 42;
            NullableDoublePropertyDP = 1;
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
            this.ReentrancyString = "";
        }

        public void UpdateValues()
        {
            this.IntPropertyForFunctionINPC = IntPropertyForFunctionINPC + 10 % 100;
            NotifyPropertyChanged("FunctionReturningIntProperty");

            this.IntPropNoINPC = IntPropNoINPC + 10 % 100;
            this.IntPropWithINPC = IntPropWithINPC + 1 % 10;
            this.IntPropertyDP = IntPropertyDP + 10 % 100;

            // Cycle from 0 -> 1 -> 2 -> null -> 0...
            if (this.NullableDoublePropertyDP.HasValue)
            {
                this.NullableDoublePropertyDP++;
                if (this.NullableDoublePropertyDP >= 3)
                {
                    this.NullableDoublePropertyDP = null;
                }
            }
            else
            {
                this.NullableDoublePropertyDP = 0;
            }

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

            // Cycle from null -> true -> false -> null...
            if (NullableBoolProperty.HasValue)
            {
                if (NullableBoolProperty.Value)
                {
                    NullableBoolProperty = false;
                }
                else
                {
                    NullableBoolProperty = null;
                }
            }
            else
            {
                NullableBoolProperty = true;
            }

            SelectEmployeeOfTheMonth();
            this.ReentrancyString = "";
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

        public event PropertyChangedEventHandler PropertyChanged;
        
        private void SetValue<T>(ref T currentValue, T newValue, [CallerMemberName] string propertyName = "")
        {
            if (!EqualityComparer<T>.Default.Equals(currentValue, newValue))
            {
                currentValue = newValue;
                NotifyPropertyChanged(propertyName);
            }
        }
        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        #endregion

        #region Re-entrancy testing - properties and methods
        private string _ReentrancyString;
        public string ReentrancyString
        {
            get { return _ReentrancyString; }
            set
            {
                SetValue(ref _ReentrancyString, value);
            }
        }

        private int _reentrantCount = 0;

        // Updates the ReentrancyString on the given model with the input string.  Designed to recurse a certain number of times
        // when model.ReentrancyString is passed in as inputString in a function binding.
        public static string UpdateReentrancyString(DataModel model, string inputString)
        {
            const int maxCount = 3;
            model._reentrantCount++;
            if (model._reentrantCount <= maxCount)
            {
                model.ReentrancyString = inputString + model._reentrantCount;
            }
            else
            {
                model._reentrantCount = 0;
            }

            return model.ReentrancyString;
        }
        #endregion
    }

    public sealed class DataErrorModel : INotifyPropertyChanged
#if INDEI
        , Microsoft.UI.Xaml.Data.INotifyDataErrorInfo
#endif
    {
        private int _intWithRange;
        [Range(0, 100, ErrorMessage = "IntWithRange is not within expected range")]
        public int IntWithRange
        {
            get { return _intWithRange; }
            set { SetValue(ref _intWithRange, value); }
        }

        private string _requiredString;
        [DefaultValue("")]
        [Required(ErrorMessage = "RequiredString is required, please enter a field")]
        public string RequiredString
        {
            get { return _requiredString ?? ""; }
            set
            {
                SetValue(ref _requiredString, value);
            }
        }

        private string _requiredFunctionString;
        [DefaultValue("")]
        [Required(ErrorMessage = "RequiredFunctionString is required, please enter a field")]
        public string RequiredFunctionString
        {
            get { return _requiredFunctionString ?? ""; }
            set
            {
                SetValue(ref _requiredFunctionString, value);
            }
        }
        public string FunctionOnModelToString(object toString)
        {
            return toString != null ? toString.ToString() : "";
        }
        public void FunctionOnModelToStringBindBack(string value)
        {
            this.RequiredFunctionString = value;
        }

#region INPC/INDEI
        private void SetValue<T>(ref T currentValue, T newValue, [CallerMemberName] string propertyName = "")
        {
            if (!EqualityComparer<T>.Default.Equals(currentValue, newValue))
            {
                currentValue = newValue;
                NotifyPropertyChanged(propertyName);
                OnPropertyChanged(newValue, propertyName);
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        Dictionary<string, List<System.ComponentModel.DataAnnotations.ValidationResult>> _errors = new Dictionary<string, List<System.ComponentModel.DataAnnotations.ValidationResult>>();
        public bool HasErrors
        {
            get
            {
                return _errors.Any();
            }
        }
        public IEnumerable<object> GetErrors(string propertyName)
        {
            return _errors[propertyName];
        }

#if INDEI
        public event EventHandler<Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs> ErrorsChanged;
#endif

        private void OnPropertyChanged(object value, string propertyName)
        {
            ClearErrors(propertyName);
            var results = new List<System.ComponentModel.DataAnnotations.ValidationResult>();
            var result = Validator.TryValidateProperty(
                value,
                new ValidationContext(this, null, null)
                {
                    MemberName = propertyName
                },
                results
                );

            if (!result)
            {
                AddErrors(propertyName, results);
            }
        }

        private void AddErrors(string propertyName, IEnumerable<ValidationResult> results)
        {
            List<ValidationResult> errors = null;
            if (!_errors.TryGetValue(propertyName, out errors))
            {
                errors = new List<ValidationResult>();
                _errors.Add(propertyName, errors);
            }

            errors.AddRange(results);
#if INDEI
            ErrorsChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs(propertyName));
#endif
        }

        private void ClearErrors(string propertyName)
        {
            if (_errors.TryGetValue(propertyName, out var errors))
            {
                errors.Clear();
#if INDEI
                ErrorsChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs(propertyName));
#endif
            }
        }
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
