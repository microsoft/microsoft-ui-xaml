// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ConditionalControls
{
    public interface IEmployee : INotifyPropertyChanged
    {
        string FirstName { get; set; }
        string LastName { get; set; }
        int OfficeNumber1 { get;  set; }
        int OfficeNumber2 { get; set; }
        int OfficeNumber3 { get; set; }
        string GetNameV3();
    }

    internal class Employee : IEmployee
    {
        private string _firstName;
        private string _lastName;
        private int _officeNumber;

        public event PropertyChangedEventHandler PropertyChanged;

        public static string [] RandomNames =
        {
            "Steven", "Tommy", "Shane", "Sam", "Fabian", "Cristophe"
        };

        public Employee()
        {
        }

        public string FirstName
        {
            get { return _firstName; }
            set { if (value != _firstName) { _firstName = value; NotifyPropertyChanged(); } }
        }

        public string LastName
        {
            get { return _lastName; }
            set { if (value != _lastName) { _lastName = value; NotifyPropertyChanged(); } }
        }

        public string GetNameV3()
        {
            Test.EnsureVersion<V3Type>();
            return FirstName + " " + LastName;
        }

        public int OfficeNumber1
        {
            get
            {
                Test.EnsureVersion<V1Type>();
                return _officeNumber;
            }
            set
            {
                Test.EnsureVersion<V1Type>();
                if (value != _officeNumber)
                {
                    _officeNumber = 100 + value; NotifyPropertyChanged();
                }
            }
        }

        public int OfficeNumber2
        {
            get
            {
                Test.EnsureVersion<V2Type>();
                return _officeNumber;
            }
            set
            {
                Test.EnsureVersion<V2Type>();
                if (value != _officeNumber)
                {
                    _officeNumber = 200 + value; NotifyPropertyChanged();
                }
            }
        }

        public int OfficeNumber3
        {
            get
            {
                Test.EnsureVersion<V3Type>();
                return _officeNumber;
            }
            set
            {
                Test.EnsureVersion<V3Type>();
                if (value != _officeNumber)
                {
                    _officeNumber = 300 + value; NotifyPropertyChanged();
                }
            }
        }

        protected void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
