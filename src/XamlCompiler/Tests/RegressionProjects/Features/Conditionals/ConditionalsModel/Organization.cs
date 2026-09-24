// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ConditionalControls
{
    public sealed class Organization
    {
        Random rand = new Random();
        IObservableCollection _employeesV1;
        IObservableCollection _employeesV3;

        public Organization()
        {   
        }

        public void Hire1()
        {
            Test.EnsureVersion<V1Type>();
            int officeNumber = rand.Next() % Employee.RandomNames.Count();
            var employee = new Employee() { FirstName = Employee.RandomNames[officeNumber], OfficeNumber1 = officeNumber };
            this.Employees.Add(employee);            
        }

        public void Hire2()
        {
            Test.EnsureVersion<V2Type>();
            int officeNumber = rand.Next() % Employee.RandomNames.Count();
            var employee = new Employee() { FirstName = Employee.RandomNames[officeNumber], OfficeNumber2 = officeNumber };
            this.Employees.Add(employee);            
        }

        public void Hire3()
        {
            Test.EnsureVersion<V3Type>();
            int officeNumber = rand.Next() % Employee.RandomNames.Count();
            var employee = new Employee() { FirstName = Employee.RandomNames[officeNumber], OfficeNumber3 = officeNumber };
            this.Employees.Add(employee);
            this.EmployeesV3.Add(employee);
        }

        public void Fire()
        {
            Test.EnsureVersion<V1Type>();
            int officeNumber = rand.Next() % Employee.RandomNames.Count();
            ((IList)this.Employees).RemoveAt(rand.Next() % ((IList)Employees).Count);
        }

        public IObservableCollection Employees
        {
            get { Test.EnsureVersion<V1Type>(); if (_employeesV1 == null) { _employeesV1 = new EmployeeCollection<V1Type>(); } return _employeesV1;  }
        }

        public IObservableCollection EmployeesV3
        {
            get { Test.EnsureVersion<V3Type>(); if (_employeesV3 == null) { _employeesV3 = new EmployeeCollection<V3Type>(); }  return _employeesV3; }
        }

        public string NullProperty
        {
            get { return null; }
        }

        public IEmployee NullEmployee
        {
            get { return null; }
        }

        public IEmployee EmployeeOfTheMonth
        {
            get { return this.Employees.FirstOrDefault(); }
        }

        public static bool IsNonEmpty(IObservableCollection collection)
        {
            return ((IList)collection).Count > 0;
        }
    }
}
