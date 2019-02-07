using Microsoft.Toolkit.Uwp.UI.Controls;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace DataGridTestDriver
{
    public sealed partial class DataSourcesControl : UserControl
    {
        private DataGrid _dataGrid;
        private MockData _mockData;
        private bool _isItemEmployee;
        private IList<Person> _personList;
        private IList<Employee> _employeeList;

        public DataSourcesControl(MockData mockData)
        {
            this.InitializeComponent();
            _mockData = mockData;
            _isItemEmployee = false;
        }

        public DataGrid DataGrid
        {
            get
            {
                return _dataGrid;
            }
            set
            {
                if (_dataGrid != value)
                {
                    _dataGrid = value;
                    OnDataGridChanged();
                }
            }
        }

        private void OnDataGridChanged()
        {
            if (this.DataGrid == null)
            {
                cmbItemsSource.SelectedIndex = 0;
            }
            else
            {
                //  0 - <ComboBoxItem>Null</ComboBoxItem>
                //  1 - <ComboBoxItem>PersonCVS</ComboBoxItem>
                //  2 - <ComboBoxItem>EmployeeCVS</ComboBoxItem>
                //  3 - <ComboBoxItem>PersonGroupedByLastNameCVS</ComboBoxItem>
                //  4 - <ComboBoxItem>EmployeeGroupedByLastNameCVS</ComboBoxItem>
                //  5 - <ComboBoxItem>PersonList</ComboBoxItem>
                //  6 - <ComboBoxItem>EmployeeList</ComboBoxItem>
                //  7 - <ComboBoxItem>PersonBindableVectorView</ComboBoxItem>
                //  8 - <ComboBoxItem>EmployeeBindableVectorView</ComboBoxItem>
                //  9 - <ComboBoxItem>PersonObservableCollection</ComboBoxItem>
                // 10 - <ComboBoxItem>EmployeeObservableCollection</ComboBoxItem>
                // 11 - <ComboBoxItem>EmployeeProgressiveObservableCollection</ComboBoxItem>
                // 12 - <ComboBoxItem>EmployeeAdvancedCollectionView</ComboBoxItem>
                // 13 - <ComboBoxItem>Empty PersonObservableCollection</ComboBoxItem>
                // 14 - <ComboBoxItem>Empty EmployeeObservableCollection</ComboBoxItem>

                if (this.DataGrid.ItemsSource == null)
                {
                    cmbItemsSource.SelectedIndex = 0;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeBindableVectorView)
                {
                    cmbItemsSource.SelectedIndex = 8;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeCollectionViewSource.View)
                {
                    cmbItemsSource.SelectedIndex = 2;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeGroupedByLastNameCollectionViewSource.View)
                {
                    cmbItemsSource.SelectedIndex = 4;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeList)
                {
                    cmbItemsSource.SelectedIndex = 6;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeObservableCollection)
                {
                    cmbItemsSource.SelectedIndex = 10;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeProgressiveObservableCollection)
                {
                    cmbItemsSource.SelectedIndex = 11;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmployeeAdvancedCollectionView)
                {
                    cmbItemsSource.SelectedIndex = 12;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmptyPersonObservableCollection)
                {
                    cmbItemsSource.SelectedIndex = 13;
                }
                else if (this.DataGrid.ItemsSource == _mockData.EmptyEmployeeObservableCollection)
                {
                    cmbItemsSource.SelectedIndex = 14;
                }
                else if (this.DataGrid.ItemsSource == _mockData.PersonBindableVectorView)
                {
                    cmbItemsSource.SelectedIndex = 7;
                }
                else if (this.DataGrid.ItemsSource == _mockData.PersonCollectionViewSource.View)
                {
                    cmbItemsSource.SelectedIndex = 1;
                }
                else if (this.DataGrid.ItemsSource == _mockData.PersonGroupedByLastNameCollectionViewSource.View)
                {
                    cmbItemsSource.SelectedIndex = 3;
                }
                else if (this.DataGrid.ItemsSource == _mockData.PersonList)
                {
                    cmbItemsSource.SelectedIndex = 5;
                }
                else if (this.DataGrid.ItemsSource == _mockData.PersonObservableCollection)
                {
                    cmbItemsSource.SelectedIndex = 9;
                }
            }
        }

        private void cmbItemsSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _isItemEmployee = false;
            _personList = null;
            _employeeList = null;

            switch (cmbItemsSource.SelectedIndex)
            {
                case 0:
                    _dataGrid.ItemsSource = null;
                    break;
                case 1:
                    _dataGrid.ItemsSource = _mockData.PersonCollectionViewSource.View;
                    _personList = _mockData.PersonList;
                    break;
                case 2:
                    _dataGrid.ItemsSource = _mockData.EmployeeCollectionViewSource.View;
                    _employeeList = _mockData.EmployeeList;
                    _isItemEmployee = true;
                    break;
                case 3:
                    _dataGrid.ItemsSource = _mockData.PersonGroupedByLastNameCollectionViewSource.View;
                    break;
                case 4:
                    _dataGrid.ItemsSource = _mockData.EmployeeGroupedByLastNameCollectionViewSource.View;
                    _isItemEmployee = true;
                    break;
                case 5:
                    _dataGrid.ItemsSource = _mockData.PersonList;
                    _personList = _mockData.PersonList;
                    break;
                case 6:
                    _dataGrid.ItemsSource = _mockData.EmployeeList;
                    _employeeList = _mockData.EmployeeList;
                    _isItemEmployee = true;
                    break;
                case 7:
                    _dataGrid.ItemsSource = _mockData.PersonBindableVectorView;
                    _personList = _mockData.PersonList;
                    break;
                case 8:
                    _dataGrid.ItemsSource = _mockData.EmployeeBindableVectorView;
                    _employeeList = _mockData.EmployeeList;
                    _isItemEmployee = true;
                    break;
                case 9:
                    _dataGrid.ItemsSource = _mockData.PersonObservableCollection;
                    _personList = _mockData.PersonObservableCollection;
                    break;
                case 10:
                    _dataGrid.ItemsSource = _mockData.EmployeeObservableCollection;
                    _employeeList = _mockData.EmployeeObservableCollection;
                    _isItemEmployee = true;
                    break;
                case 11:
                    _dataGrid.ItemsSource = _mockData.EmployeeProgressiveObservableCollection;
                    _employeeList = _mockData.EmployeeProgressiveObservableCollection;
                    _isItemEmployee = true;
                    break;
                case 12:
                    _dataGrid.ItemsSource = _mockData.EmployeeAdvancedCollectionView;
                    _employeeList = _mockData.EmployeeObservableCollection;
                    _isItemEmployee = true;
                    break;
                case 13:
                    _dataGrid.ItemsSource = _mockData.EmptyPersonObservableCollection;
                    _personList = _mockData.EmptyPersonObservableCollection;
                    break;
                case 14:
                    _dataGrid.ItemsSource = _mockData.EmptyEmployeeObservableCollection;
                    _employeeList = _mockData.EmptyEmployeeObservableCollection;
                    _isItemEmployee = true;
                    break;
            }

            if (_dataGrid.ItemsSource == null)
            {
                btnAdd.IsEnabled = false;
                btnInsert.IsEnabled = false;
                btnRemove.IsEnabled = false;
                btnClear.IsEnabled = false;
                this.epcItem.Entity = null;
            }
            else
            {
                btnAdd.IsEnabled = true;
                btnInsert.IsEnabled = true;
                if (_isItemEmployee)
                {
                    btnClear.IsEnabled = btnRemove.IsEnabled = _employeeList != null && _employeeList.Count > 0;
                    this.epcItem.Entity = GetNewEmployee();
                    this.epcItem.Level = 2;
                }
                else
                {
                    btnClear.IsEnabled = btnRemove.IsEnabled = _personList != null && _personList.Count > 0;
                    this.epcItem.Entity = GetNewPerson();
                    this.epcItem.Level = 1;
                }
            }
        }

        private void btnClear_Click(object sender, RoutedEventArgs e)
        {
            if (this.DataGrid.ItemsSource == _mockData.EmployeeObservableCollection)
            {
                _mockData.EmployeeObservableCollection.Clear();
            }
            else if (this.DataGrid.ItemsSource == _mockData.EmployeeProgressiveObservableCollection)
            {
                _mockData.EmployeeProgressiveObservableCollection.Clear();
            }
            else if (this.DataGrid.ItemsSource == _mockData.EmptyEmployeeObservableCollection)
            {
                _mockData.EmptyEmployeeObservableCollection.Clear();
            }
            else if (this.DataGrid.ItemsSource == _mockData.EmptyPersonObservableCollection)
            {
                _mockData.EmptyPersonObservableCollection.Clear();
            }
            else if (this.DataGrid.ItemsSource == _mockData.PersonObservableCollection)
            {
                _mockData.PersonObservableCollection.Clear();
            }
            else if (this.DataGrid.ItemsSource == _mockData.EmployeeAdvancedCollectionView)
            {
                _mockData.EmployeeAdvancedCollectionView.Clear();
            }
            
            btnClear.IsEnabled = btnRemove.IsEnabled = false;
        }

        private void btnReset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_isItemEmployee)
                {
                    this.epcItem.Entity = GetNewEmployee();
                }
                else
                {
                    this.epcItem.Entity = GetNewPerson();
                }
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_isItemEmployee)
                {
                    _employeeList.Add(this.epcItem.Entity as Employee);

                    this.epcItem.Entity = GetNewEmployee();
                }
                else
                {
                    _personList.Add(this.epcItem.Entity as Person);

                    this.epcItem.Entity = GetNewPerson();
                }

                btnClear.IsEnabled = btnRemove.IsEnabled = true;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnInsert_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int insertionItemIndex = Convert.ToInt32(txtInsertionItemIndex.Text);

                if (_isItemEmployee)
                {
                    _employeeList.Insert(insertionItemIndex, this.epcItem.Entity as Employee);

                    this.epcItem.Entity = GetNewEmployee();
                }
                else
                {
                    _personList.Insert(insertionItemIndex, this.epcItem.Entity as Person);

                    this.epcItem.Entity = GetNewPerson();
                }

                btnClear.IsEnabled = btnRemove.IsEnabled = true;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnRemove_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int removalItemIndex = Convert.ToInt32(txtRemovalItemIndex.Text);

                if (_isItemEmployee)
                {
                    _employeeList.RemoveAt(removalItemIndex);
                    btnClear.IsEnabled = btnRemove.IsEnabled = _employeeList.Count > 0;
                }
                else
                {
                    _personList.RemoveAt(removalItemIndex);
                    btnClear.IsEnabled = btnRemove.IsEnabled = _personList.Count > 0;
                }
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnGet_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                int getItemIndex = Convert.ToInt32(txtGetItemIndex.Text);
                Person item = null;

                if (_isItemEmployee)
                {
                    item = _employeeList[getItemIndex];
                }
                else
                {
                    item = _personList[getItemIndex];
                }
                this.epcItem.Entity = item;
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private void btnClearException_Click(object sender, RoutedEventArgs e)
        {
            txtException.Text = string.Empty;
        }

        private void btnRaiseINPC_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(txtSourcePropertyName.Text))
                {
                    MockData.INotifyPropertyChangedImplementer inpci = _dataGrid.ItemsSource as MockData.INotifyPropertyChangedImplementer;
                    if (inpci != null)
                    {
                        inpci.RaisePropertyChanged(txtSourcePropertyName.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtException.Text = ex.ToString();
            }
        }

        private Person GetNewPerson()
        {
            return new Person(
                firstName: "<FirstName>",
                lastName: "<LastName>",
                isResidentAlien: true,
                dateOfBirth: DateTime.Today - new TimeSpan(30 * 365, 0, 0, 0),
                cityOfResidence: "<CityOfResidence>",
                zipCode: 0);
        }

        private Employee GetNewEmployee()
        {
            return new Employee(
                        firstName: "<FirstName>",
                        lastName: "<LastName>",
                        isResidentAlien: true,
                        dateOfBirth: DateTime.Today - new TimeSpan(30 * 365, 0, 0, 0),
                        cityOfResidence: "<CityOfResidence>",
                        zipCode: 0,
                        hireDate: DateTime.Today - new TimeSpan(5 * 365, 0, 0, 0),
                        isEmployed: true,
                        employeeId: 1);
        }
    }
}