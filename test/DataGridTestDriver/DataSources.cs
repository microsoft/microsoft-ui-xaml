using Microsoft.Toolkit.Uwp.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Interop;

namespace DataGridTestDriver
{
    public class MockData
    {
        public interface INotifyPropertyChangedImplementer
        {
            void RaisePropertyChanged(string propertyName);
        }

        public enum State
        {
            Alabama,
            Alaska,
            Arizona,
            Arkansas,
            California,
            Colorado,
            Connecticut,
            Delaware,
            Florida,
            Georgia,
            Hawaii,
            Idaho,
            Illinois,
            Indiana,
            Iowa,
            Kansas,
            Kentucky,
            Louisiana,
            Maine,
            Maryland,
            Massachusetts,
            Michigan,
            Minnesota,
            Mississippi,
            Missouri,
            Montana,
            Nebraska,
            Nevada,
            NewHampshire,
            NewJersey,
            NewMexico,
            NewYork,
            NorthCarolina,
            NorthDakota,
            Ohio,
            Oklahoma,
            Oregon,
            Pennsylvania,
            RhodeIsland,
            SouthCarolina,
            SouthDakota,
            Tennessee,
            Texas,
            Utah,
            Vermont,
            Virginia,
            Washington,
            WestVirginia,
            Wisconsin,
            Wyoming
        };

        public class Address
        {
            public Address(int number, string street, string city, int zipCode, State state)
            {
                this.Number = number;
                this.Street = street;
                this.City = city;
                this.ZipCode = zipCode;
                this.State = state;
            }

            public int Number
            {
                get;
                set;
            }

            public string Street
            {
                get;
                set;
            }

            public string City
            {
                get;
                set;
            }

            public int ZipCode
            {
                get;
                set;
            }

            public State State
            {
                get;
                set;
            }
        }

        public List<string> FirstNames = new List<string>()
        {
            "Aaron",
            "Abbey",
            "Abbie",

            "Bailey",
            "Bambi",
            "Bao",

            "Amélie",

            "Caleb",
            "Calista",

            "Barabara",

            "Daisey",

            "Chloé",

            "Eda",

            "Daisy",

            "Kim",

            "Edda",

            "Owen",

            "Fabrice",
            "Frida",
        };

        public List<string> LastNames = new List<string>()
        {
            "Armstrong",
            "Bond",
            "De Frutos",
            "Grasser",
            "Klein",
            "Smith",
            "Turlututu"
        };

        public List<string> Cities = new List<string>()
        {
            "Austin",
            "Boston",
            "Chicago",
            "Detroit",
            "Enumclaw",
            "Fort Lauderdale",
            "Georgetown",
            "Houston",
            "Istanbul",
            "Jerusalem",
            "Kensington",
            "Lima",
            "Misoula",
            "New York",
            "Orlando",
            "Paris",
            "Quebec",
            "Rome",
            "San Diego",
            "Toulon",
            "Umatilla",
            "Vancouver",
            "Walla Walla",
            "Xenia",
            "Zillah"
        };

        //Both List<int> and List<object> are supported by the DataGridComboBoxColumn type.        
        //public List<object> ZipCodes = new List<object>()
        public List<int> ZipCodes = new List<int>()
        {
            67800,
            67850,
            98110
        };

        public List<Address> Addresses;

        public CollectionViewSource PersonCollectionViewSource;
        public CollectionViewSource EmployeeCollectionViewSource;
        public CollectionViewSource PersonGroupedByLastNameCollectionViewSource;
        public CollectionViewSource EmployeeGroupedByLastNameCollectionViewSource;
        public CollectionViewSource EmployeeGroupedByLastNameAndResidencyCollectionViewSource;
        public List<Person> PersonList;
        public List<Employee> EmployeeList;
        public PersonBindableVectorView PersonBindableVectorView;
        public EmployeeBindableVectorView EmployeeBindableVectorView;
        public AdvancedCollectionView EmployeeAdvancedCollectionView;
        public ObservableCollection<Person> PersonObservableCollection;
        public ObservableCollection<Employee> EmployeeObservableCollection;
        public ObservableCollection<Person> EmptyPersonObservableCollection;
        public ObservableCollection<Employee> EmptyEmployeeObservableCollection;
        public ProgressiveObservableCollection<Employee> EmployeeProgressiveObservableCollection;

        public MockData()
        {
            this.Addresses = new List<Address>();
            this.Addresses.Add(new Address(
                51, "Zorn Street", "Sirlisheim", 67850, State.Alaska));
            this.Addresses.Add(new Address(
                2, "National Avenue", "Dearheim", 67800, State.Arkansas));
            this.Addresses.Add(new Address(
                14818, "102 Av NE", "Bothlui", 98110, State.Arizona));

            this.PersonList = new List<Person>();
            this.EmployeeList = new List<Employee>();

            PopulatePersonList();
            PopulateEmployeeList();

            this.PersonBindableVectorView = new PersonBindableVectorView(this.PersonList);
            this.EmployeeBindableVectorView = new EmployeeBindableVectorView(this.EmployeeList);

            this.PersonObservableCollection = new ObservableCollection<Person>(this.PersonList);
            this.EmployeeObservableCollection = new ObservableCollection<Employee>(this.EmployeeList);
            this.EmployeeProgressiveObservableCollection = new ProgressiveObservableCollection<Employee>(this.EmployeeList);

            this.EmployeeAdvancedCollectionView = new AdvancedCollectionView();
            this.EmployeeAdvancedCollectionView.Source = this.EmployeeObservableCollection;

            this.PersonCollectionViewSource = new CollectionViewSource();
            this.PersonCollectionViewSource.Source = this.PersonList;

            this.EmployeeCollectionViewSource = new CollectionViewSource();
            this.EmployeeCollectionViewSource.Source = this.EmployeeList;

            this.PersonGroupedByLastNameCollectionViewSource = new CollectionViewSource();
            this.PersonGroupedByLastNameCollectionViewSource.IsSourceGrouped = true;
            this.PersonGroupedByLastNameCollectionViewSource.Source = GetPersonGroupsByLastName();

            this.EmployeeGroupedByLastNameCollectionViewSource = new CollectionViewSource();
            this.EmployeeGroupedByLastNameCollectionViewSource.IsSourceGrouped = true;
            this.EmployeeGroupedByLastNameCollectionViewSource.Source = GetEmployeeGroupsByLastName();

            this.EmployeeGroupedByLastNameAndResidencyCollectionViewSource = new CollectionViewSource();
            this.EmployeeGroupedByLastNameAndResidencyCollectionViewSource.IsSourceGrouped = true;
            this.EmployeeGroupedByLastNameAndResidencyCollectionViewSource.Source = GetEmployeeGroupsByLastNameAndResidency();

            this.EmptyPersonObservableCollection = new ObservableCollection<Person>();
            this.EmptyEmployeeObservableCollection = new ObservableCollection<Employee>();
        }

        public void PopulatePersonList()
        {
            Random random = new Random((int)DateTime.Now.ToBinary());
            int cities = this.Cities.Count;
            int addresses = this.Addresses.Count;

            this.PersonList.Clear();
            foreach (string firstName in FirstNames)
            {
                foreach (string lastName in LastNames)
                {
                    Person newPerson = new Person(
                        firstName,
                        lastName,
                        firstName[0] != lastName[0],
                        new DateTime(DateTime.Today.Year - 20 - random.Next(50), 1 + random.Next(11), 1 + random.Next(27)),
                        this.Cities[random.Next(cities)],
                        this.Addresses[random.Next(addresses)].ZipCode);
                    for (int c = 0; c < random.Next(10); c++)
                    {
                        newPerson.AddCityOfResidence(this.Cities[random.Next(cities)]);
                    }
                    this.PersonList.Add(newPerson);
                }
            }
        }

        public void PopulateEmployeeList()
        {
            Random random = new Random((int)DateTime.Now.ToBinary());
            int cities = this.Cities.Count;
            int addresses = this.Addresses.Count;

            this.EmployeeList.Clear();
            foreach (string firstName in FirstNames)
            {
                foreach (string lastName in LastNames)
                {
                    this.EmployeeList.Add(
                        new Employee(
                            firstName,
                            lastName,
                            firstName[0] != lastName[0],
                            new DateTime(DateTime.Today.Year - 20 - random.Next(50), 1 + random.Next(11), 1 + random.Next(27)),
                            this.Cities[random.Next(cities)],
                            this.Addresses[random.Next(addresses)].ZipCode,
                            new DateTime(DateTime.Today.Year - 1 - random.Next(19), 1 + random.Next(11), 1 + random.Next(27)),
                            firstName[1] != lastName[1],
                            1 + random.Next(50000)));
                }
            }
        }

        public List<GroupInfoList<object>> GetFirstNameGroupsByInitial()
        {
            List<GroupInfoList<object>> groups = new List<GroupInfoList<object>>();

            var query = from item in FirstNames
                        orderby item
                        group item by item[0] into g
                        select new { GroupName = g.Key.ToString().ToUpper(), Items = g };
            foreach (var g in query)
            {
                GroupInfoList<object> info = new GroupInfoList<object>();
                info.Key = g.GroupName;
                foreach (var item in g.Items)
                {
                    info.Add(item);
                }
                groups.Add(info);
            }

            return groups;
        }

        public List<GroupInfoList<Person>> GetPersonGroupsByLastName()
        {
            List<GroupInfoList<Person>> groups = new List<GroupInfoList<Person>>();

            var query = from item in PersonList
                        orderby item
                        group item by item.LastName into g
                        select new { GroupName = g.Key, Items = g };
            foreach (var g in query)
            {
                GroupInfoList<Person> info = new GroupInfoList<Person>();
                info.Key = g.GroupName;
                foreach (var item in g.Items)
                {
                    info.Add(item);
                }
                groups.Add(info);
            }

            return groups;
        }

        public List<GroupInfoList<Employee>> GetEmployeeGroupsByLastName()
        {
            List<GroupInfoList<Employee>> groups = new List<GroupInfoList<Employee>>();

            var query = from item in this.EmployeeList
                        orderby item
                        group item by item.LastName into g
                        select new { GroupName = g.Key, Items = g };
            foreach (var g in query)
            {
                GroupInfoList<Employee> info = new GroupInfoList<Employee>();
                info.Key = g.GroupName;
                foreach (var item in g.Items)
                {
                    info.Add(item);
                }
                groups.Add(info);
            }

            return groups;
        }

        public List<GroupInfoList<GroupInfoList<Employee>>> GetEmployeeGroupsByLastNameAndResidency()
        {
            List<GroupInfoList<GroupInfoList<Employee>>> outerGroups = new List<GroupInfoList<GroupInfoList<Employee>>>();

            var query = from item in this.EmployeeList
                        orderby item
                        group item by item.LastName into g
                        select new { GroupName = g.Key, Items = g };
            foreach (var g in query)
            {
                GroupInfoList<GroupInfoList<Employee>> outerInfo = new GroupInfoList<GroupInfoList<Employee>>();
                outerInfo.Key = g.GroupName;

                List<Employee> outerList = new List<Employee>();
                foreach (var item in g.Items)
                {
                    outerList.Add(item);
                }

                List<GroupInfoList<Employee>> groupsByResidency = GetEmployeeGroupsByResidency(outerList);
                foreach (GroupInfoList<Employee> group in groupsByResidency)
                {
                    outerInfo.Add(group);
                }

                outerGroups.Add(outerInfo);
            }

            return outerGroups;
        }

        private List<GroupInfoList<Employee>> GetEmployeeGroupsByResidency(List<Employee> employees)
        {
            List<GroupInfoList<Employee>> groups = new List<GroupInfoList<Employee>>();

            var query = from item in employees
                        orderby item
                        group item by item.IsResidentAlien into g
                        select new { GroupName = g.Key, Items = g };
            foreach (var g in query)
            {
                GroupInfoList<Employee> info = new GroupInfoList<Employee>();
                info.Key = g.GroupName;
                foreach (var item in g.Items)
                {
                    info.Add(item);
                }
                groups.Add(info);
            }

            return groups;
        }
    }

    public class GroupInfoList<T> : List<T>
    {
        public object Key { get; set; }

        public new IEnumerator<T> GetEnumerator()
        {
            return (IEnumerator<T>)base.GetEnumerator();
        }
    }

    public class Person : IComparable
    {
        private string _firstName;
        private string _lastName;
        private bool _isResidentAlien;
        private int _age;
        private DateTime _dateOfBirth;
        private string _cityOfResidence;
        private List<string> _citiesOfResidence;
        private int _zipCode;

        public Person(string firstName, string lastName, bool isResidentAlien, DateTime dateOfBirth, string cityOfResidence, int zipCode)
        {
            this.FirstName = firstName;
            this.LastName = lastName;
            this.IsResidentAlien = isResidentAlien;
            this.DateOfBirth = dateOfBirth;
            this.Age = (int)((DateTime.Today - dateOfBirth).TotalDays / 365);
            this.CityOfResidence = cityOfResidence;
            this.ZipCode = zipCode;
        }

        [DisplayAttribute(ShortName = "First Name")]
        public string FirstName
        {
            get
            {
                return _firstName;
            }
            set
            {
                if (_firstName != value)
                {
                    _firstName = value;
                    OnPropertyChanged("FirstName");
                }
            }
        }

        [DisplayAttribute(ShortName = "Last Name")]
        public string LastName
        {
            get
            {
                return _lastName;
            }
            set
            {
                if (_lastName != value)
                {
                    _lastName = value;
                    OnPropertyChanged("LastName");
                }
            }
        }

        [DisplayAttribute(ShortName = "Is Resident Alien")]
        public bool IsResidentAlien
        {
            get
            {
                return _isResidentAlien;
            }
            set
            {
                if (_isResidentAlien != value)
                {
                    _isResidentAlien = value;
                    OnPropertyChanged("IsResidentAlien");
                }
            }
        }

        public int Age
        {
            get
            {
                return _age;
            }
            protected set
            {
                if (_age != value)
                {
                    _age = value;
                    OnPropertyChanged("Age");
                }
            }
        }

        [DisplayAttribute(ShortName = "Date Of Birth")]
        public DateTime DateOfBirth
        {
            get
            {
                return _dateOfBirth;
            }
            set
            {
                if (_dateOfBirth != value)
                {
                    _dateOfBirth = value;
                    OnPropertyChanged("DateOfBirth");
                }
            }
        }

        public DateTimeOffset DateOfBirthOffset
        {
            get
            {
                return new DateTimeOffset(this.DateOfBirth);
            }
            set
            {
                if (this.DateOfBirth != value.DateTime)
                {
                    this.DateOfBirth = value.DateTime;
                    OnPropertyChanged("DateOfBirthOffset");
                }
            }
        }

        public string CityOfResidence
        {
            get
            {
                return _cityOfResidence;
            }

            set
            {
                _cityOfResidence = value;
                AddCityOfResidence(value);
                OnPropertyChanged("CityOfResidence");
            }
        }

        [Display(AutoGenerateField = false)]
        public List<string> CitiesOfResidence
        {
            get
            {
                return _citiesOfResidence;
            }

            private set
            {
                _citiesOfResidence = value;
            }
        }

        public int ZipCode
        {
            get
            {
                return _zipCode;
            }
            /*protected*/ set
            {
                if (_zipCode != value)
                {
                    _zipCode = value;
                    OnPropertyChanged("ZipCode");
                }
            }
        }

        public void AddCityOfResidence(string city)
        {
            if (this.CitiesOfResidence == null)
                this.CitiesOfResidence = new List<string>();
            this.CitiesOfResidence.Add(city);
            OnPropertyChanged("CitiesOfResidence");
        }

        int IComparable.CompareTo(object obj)
        {
            int lnCompare = LastName.CompareTo((obj as Person).LastName);

            if (lnCompare == 0)
                return FirstName.CompareTo((obj as Person).FirstName);
            else
                return lnCompare;
        }

        protected virtual void OnPropertyChanged(string propertyName)
        {
        }
    }

    public class Employee : Person, IEditableObject, INotifyPropertyChanged, INotifyDataErrorInfo 
    {
        private bool _isEmployed = false;
        private int _employeeId = 1;
        private DateTime _hireDate = DateTime.Today;
        private Dictionary<string, List<string>> _errors = new Dictionary<string, List<string>>();

        public Employee(string firstName, string lastName, bool isResidentAlien, DateTime dateOfBirth, string cityOfResidence, int zipCode, DateTime hireDate, bool isEmployed, int employeeId) : 
            base(firstName, lastName, isResidentAlien, dateOfBirth, cityOfResidence, zipCode)
        {
            this.HireDate = hireDate;
            this.IsEmployed = isEmployed;
            this.EmployeeId = employeeId;
        }

        private Employee(Employee employee) :
            this(employee.FirstName, employee.LastName, employee.IsResidentAlien, employee.DateOfBirth, employee.CityOfResidence, employee.ZipCode, employee.HireDate, employee.IsEmployed, employee.EmployeeId)
        {
        }

        public event PropertyChangedEventHandler PropertyChanged;
        public event EventHandler<DataErrorsChangedEventArgs> ErrorsChanged;

        [DisplayAttribute(ShortName = "Is Employed")]
        public bool IsEmployed
        {
            get
            {
                return _isEmployed;
            }

            set
            {
                if (_isEmployed != value)
                {
                    _isEmployed = value;
                    OnPropertyChanged("IsEmployed");
                }
            }
        }

        [DisplayAttribute(ShortName = "Employee Id")]
        public int EmployeeId
        {
            get
            {
                return _employeeId;
            }

            set
            {
                if (_employeeId != value)
                {
                    _employeeId = value;
                    OnPropertyChanged("EmployeeId");

                    bool isEmployeeIdValid = !_errors.Keys.Contains("EmployeeId");
                    if (_employeeId < 1 && isEmployeeIdValid)
                    {
                        List<string> errors = new List<string>();
                        errors.Add("EmployeeId must be strictly positive.");
                        _errors.Add("EmployeeId", errors);
                        this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs("EmployeeId"));
                    }
                    else if (_employeeId > 0 && !isEmployeeIdValid)
                    {
                        _errors.Remove("EmployeeId");
                        this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs("EmployeeId"));
                    }
                }
            }
        }

        [DisplayAttribute(ShortName = "Hire Date")]
        public DateTime HireDate
        {
            get
            {
                return _hireDate;
            }

            set
            {
                if (_hireDate != value)
                {
                    _hireDate = value;
                    OnPropertyChanged("HireDate");

                    bool isHireDateValid = !_errors.Keys.Contains("HireDate");
                    if (_hireDate > DateTime.Today && isHireDateValid)
                    {
                        List<string> errors = new List<string>();
                        errors.Add("HireDate must not be in the future.");
                        _errors.Add("HireDate", errors);
                        this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs("HireDate"));
                    }
                    else if (_hireDate <= DateTime.Today && !isHireDateValid)
                    {
                        _errors.Remove("HireDate");
                        this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs("HireDate"));
                    }
                }
            }
        }

        protected override void OnPropertyChanged(string propertyName)
        {
            if (!this.IsBeingEdited)
                this.PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

            bool isEntityValid = !_errors.Keys.Contains(string.Empty);
            if (this.HireDate < this.DateOfBirth && isEntityValid)
            {
                List<string> errors = new List<string>();
                errors.Add("HireDate cannot be prior to DataOfBirth.");
                _errors.Add(string.Empty, errors);
                this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs(string.Empty));
            }
            else if (this.HireDate >= this.DateOfBirth && !isEntityValid)
            {
                _errors.Remove(string.Empty);
                this.ErrorsChanged?.Invoke(this, new DataErrorsChangedEventArgs(string.Empty));
            }
        }

        private bool IsBeingEdited { get; set; }

        private Employee BackupEmployee { get; set; }

        void IEditableObject.BeginEdit()
        {
            if (this.IsBeingEdited) return;
            this.BackupEmployee = new Employee(this);
        }

        void IEditableObject.CancelEdit()
        {
            this.IsBeingEdited = false;
            this.FirstName = this.BackupEmployee.FirstName;
            this.LastName = this.BackupEmployee.LastName;
            this.IsResidentAlien = this.BackupEmployee.IsResidentAlien;
            this.DateOfBirth = this.BackupEmployee.DateOfBirth;
            this.IsEmployed = this.BackupEmployee.IsEmployed;
            this.EmployeeId = this.BackupEmployee.EmployeeId;
        }

        void IEditableObject.EndEdit()
        {
            this.IsBeingEdited = false;

            this.Age = (int)((DateTime.Today - this.DateOfBirth).TotalDays / 365);

            if (this.PropertyChanged != null)
            {
                if (this.FirstName != this.BackupEmployee.FirstName)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("FirstName"));
                }
                if (this.LastName != this.BackupEmployee.LastName)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("LastName"));
                }
                if (this.IsResidentAlien != this.BackupEmployee.IsResidentAlien)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("IsResidentAlien"));
                }
                if (this.DateOfBirth != this.BackupEmployee.DateOfBirth)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("DateOfBirth"));
                }
                if (this.IsEmployed != this.BackupEmployee.IsEmployed)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("IsEmployed"));
                }
                if (this.EmployeeId != this.BackupEmployee.EmployeeId)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("EmployeeId"));
                }
                if (this.Age != this.BackupEmployee.Age)
                {
                    this.PropertyChanged(this, new PropertyChangedEventArgs("Age"));
                }
            }
        }

        bool INotifyDataErrorInfo.HasErrors
        {
            get
            {
                return _errors.Keys.Count > 0;
            }
        }

        IEnumerable INotifyDataErrorInfo.GetErrors(string propertyName)
        {
            if (propertyName == null)
                propertyName = string.Empty;

            if (_errors.Keys.Contains(propertyName))
                return _errors[propertyName];
            else
                return null;
        }
    }

    public class PersonBindableVectorView : IBindableVectorView
    {
        private List<Person> _list;

        public PersonBindableVectorView(List<Person> list)
        {
            if (list == null)
            {
                throw new ArgumentNullException("list");
            }

            _list = list;
        }

        public object GetAt(uint index)
        {
            return _list[(int)index];
        }

        public bool IndexOf(object value, out uint index)
        {
            int i = _list.IndexOf(value as Person);
            index = (uint)i;
            return i != -1;
        }

        public uint Size
        {
            get
            {
                return (uint)_list.Count;
            }
        }

        public IEnumerator GetEnumerator()
        {
            return _list.GetEnumerator();
        }
    }

    public class EmployeeBindableVectorView : IBindableVectorView
    {
        private List<Employee> _list;

        public EmployeeBindableVectorView(List<Employee> list)
        {
            if (list == null)
            {
                throw new ArgumentNullException("list");
            }

            _list = list;
        }

        public object GetAt(uint index)
        {
            return _list[(int)index];
        }

        public bool IndexOf(object value, out uint index)
        {
            int i = _list.IndexOf(value as Employee);
            index = (uint)i;
            return i != -1;
        }

        public uint Size
        {
            get
            {
                return (uint)_list.Count;
            }
        }

        public IEnumerator GetEnumerator()
        {
            return _list.GetEnumerator();
        }
    }

    public class BindableVectorView<T> : IBindableVectorView
    {
        private List<T> _list;

        public BindableVectorView(List<T> list)
        {
            if (list == null)
            {
                throw new ArgumentNullException("list");
            }

            _list = list;
        }

        public object GetAt(uint index)
        {
            return _list[(int)index];
        }

        public bool IndexOf(object value, out uint index)
        {
            int i = _list.IndexOf((T)value);
            index = (uint)i;
            return i != -1;
        }

        public uint Size
        {
            get
            {
                return (uint)_list.Count;
            }
        }

        public IEnumerator GetEnumerator()
        {
            return _list.GetEnumerator();
        }
    }

    public class ProgressiveObservableCollection<T> : ObservableCollection<T>, ISupportIncrementalLoading, MockData.INotifyPropertyChangedImplementer
    {
        private const double c_defaultHiddenRatio = 0.5;
        private List<T> _fullList;
        private uint _hiddenCount = 0;
        private uint _loadMoreItemsCount = 0;
        private bool _insertingFromFullList = false;
        private LoadMoreItemsOperation _loadMoreItemsOperation = null;

        public ProgressiveObservableCollection(List<T> list) : base()
        {
            _fullList = list ?? throw new ArgumentNullException("list");
            _hiddenCount = (uint)(c_defaultHiddenRatio * _fullList.Count);

            _insertingFromFullList = true;
            for (int index = 0; index < _fullList.Count - _hiddenCount; index++)
            {
                Add(_fullList[index]);
            }
            _insertingFromFullList = false;
        }

        public IAsyncOperation<LoadMoreItemsResult> LoadMoreItemsAsync(uint count)
        {
            if (_hiddenCount > 0)
            {
                if (_loadMoreItemsCount == 0)
                {
                    ProgressiveObservableCollection<T>.NextId++;
                    _loadMoreItemsOperation = new LoadMoreItemsOperation(id: ProgressiveObservableCollection<T>.NextId, progressiveObservableCollection: this);
                }
                _loadMoreItemsCount = Math.Max(_loadMoreItemsCount, Math.Min(count, _hiddenCount));
                return _loadMoreItemsOperation;
            }
            return null;
        }

        public bool HasMoreItems
        {
            get
            {
                return _hiddenCount > 0;
            }
        }

        private static uint NextId
        {
            get;
            set;
        }

        public void RaisePropertyChanged(string propertyName)
        {
            OnPropertyChanged(new PropertyChangedEventArgs(propertyName));
        }

        protected override void ClearItems()
        {
            _hiddenCount = 0;
            _loadMoreItemsCount = 0;
            if (_loadMoreItemsOperation != null)
            {
                _loadMoreItemsOperation.Cancel();
                _loadMoreItemsOperation = null;
            }
            _fullList.Clear();

            base.ClearItems();
        }

        protected override void InsertItem(int index, T item)
        {
            if (_insertingFromFullList || index == this.Count)
            {
                if (_insertingFromFullList)
                {
                    base.InsertItem(index, item);
                }
                else
                {
                    _hiddenCount++;
                    _fullList.Add(item);
                }
            }
            else
                throw new NotSupportedException();
        }

        protected override void MoveItem(int oldIndex, int newIndex)
        {
            throw new NotSupportedException();
        }

        protected override void RemoveItem(int index)
        {
            throw new NotSupportedException();
        }

        protected override void SetItem(int index, T item)
        {
            throw new NotSupportedException();
        }

        internal void RevealItem()
        {
            if (_hiddenCount > 0 && _loadMoreItemsCount > 0 && _loadMoreItemsOperation != null)
            {
                _hiddenCount--;
                _loadMoreItemsCount--;
                _insertingFromFullList = true;
                try
                {
                    Add(_fullList[(int)(_fullList.Count - _hiddenCount - 1)]);
                }
                finally
                {
                    _insertingFromFullList = false;
                }

                if (_loadMoreItemsCount == 0)
                {
                    _loadMoreItemsOperation.Complete();
                }
            }
        }

        public class LoadMoreItemsOperation : IAsyncOperation<LoadMoreItemsResult>
        {
            private const double c_defaultLoadIntervalMs = 1500;

            public LoadMoreItemsOperation(uint id, ProgressiveObservableCollection<T> progressiveObservableCollection)
            {
                this.Id = id;
                this.ProgressiveObservableCollection = progressiveObservableCollection;
                this.Timer = new DispatcherTimer();
                this.Timer.Interval = TimeSpan.FromMilliseconds(c_defaultLoadIntervalMs);
                this.Timer.Tick += Timer_Tick;
                this.Timer.Start();
            }

            public LoadMoreItemsResult GetResults()
            {
                if (this.Status == AsyncStatus.Completed)
                    return this.Result;
                throw new InvalidOperationException();
            }

            public uint Id { get; private set; } = 0;

            public AsyncOperationCompletedHandler<LoadMoreItemsResult> Completed { get; set; } = null;

            public Exception ErrorCode { get; private set; } = null;

            public AsyncStatus Status { get; private set; } = AsyncStatus.Started;

            public void Cancel()
            {
                if (this.Status == AsyncStatus.Started)
                {
                    this.Status = AsyncStatus.Canceled;
                    this.Timer.Stop();
                    this.Timer = null;
                    this.Completed?.Invoke(this, this.Status);
                }
                else
                    throw new InvalidOperationException();
            }

            public void Close()
            {
                this.Status = AsyncStatus.Error;
                this.Timer.Stop();
                this.Timer = null;
                this.Completed = null;
            }

            private LoadMoreItemsResult Result { get; set; }

            private ProgressiveObservableCollection<T> ProgressiveObservableCollection { get; set; }

            private DispatcherTimer Timer { get; set; }

            private uint LoadedCount { get; set; } = 0;

            internal void Complete()
            {
                if (this.Status == AsyncStatus.Started)
                {
                    this.Status = AsyncStatus.Completed;
                    this.Result = new LoadMoreItemsResult() { Count = this.LoadedCount };
                    this.Timer.Stop();
                    this.Completed?.Invoke(this, this.Status);
                }
                else
                    throw new InvalidOperationException();
            }

            private void Timer_Tick(object sender, object e)
            {
                this.LoadedCount++;
                this.ProgressiveObservableCollection.RevealItem();
            }
        }
    }
}
