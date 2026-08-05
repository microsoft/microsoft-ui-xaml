// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.CompilerServices;

namespace BindTestbed
{
    internal class LanguageSpecific
    {
        public int IntField;
        public string StringField;

        public LanguageSpecific()
        {
            InitializeValues();
        }

        internal void InitializeValues()
        {
            StringField = "Simple string field";
            IntField = 2;
        }

        internal void UpdateValues()
        {
            this.IntField = IntField + 1 % 10;

            string ms = " " + DateTime.Now.Millisecond.ToString();
            this.StringField += ms;
        }

        // TODO: Move to a C#-specific model because indexers are not supported in WinRT.
        //public Employee this[int index] { get { return _reports[index]; } }
        //public Employee this[string name]
        //{
        //    get
        //    {
        //        return (from Employee e in _reports where e.FirstName == name select e).First();
        //    }
        //}

    }

    internal class LanguageSpecificDataErrorModel : INotifyDataErrorInfo, INotifyPropertyChanged
    {
        private int _intWithRange;
        [Range(0, 100, ErrorMessage = "IntWithRange is not within expected range")]
        public int IntWithRange
        {
            get { return _intWithRange; }
            set { SetValue(ref _intWithRange, value); }
        }

        private string _requiredString;
        [Required(ErrorMessage = "RequiredString is required, please enter a field")]
        public string RequiredString
        {
            get { return _requiredString; }
            set
            {
                SetValue(ref _requiredString, value);
            }
        }
        #region INPC/INDEI
        private void SetValue<T>(ref T currentValue, T newValue, [CallerMemberName] string propertyName = "")
        {
            if (!Object.Equals(currentValue, newValue))
            {
                currentValue = newValue;
                NotifyPropertyChanged(propertyName);
                OnPropertyChanged(newValue, propertyName);
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        private void NotifyPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged.Invoke(this, new PropertyChangedEventArgs(propertyName));
            }
        }
        Dictionary<string, List<ValidationResult>> _errors = new Dictionary<string, List<ValidationResult>>();
        public bool HasErrors
        {
            get
            {
                return _errors.Count > 0;
            }
        }

        public IEnumerable GetErrors(string propertyName)
        {
            return _errors[propertyName];
        }

        public event EventHandler<DataErrorsChangedEventArgs> ErrorsChanged;

        private void OnPropertyChanged(object value, string propertyName)
        {
            var results = new List<ValidationResult>();
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
            if (ErrorsChanged != null)
            {
                ErrorsChanged.Invoke(this, new DataErrorsChangedEventArgs(propertyName));
            }
        }

        private void ClearErrors(string propertyName)
        {
             List<ValidationResult> errors = null;
            if (_errors.TryGetValue(propertyName, out errors))
            {
                errors.Clear();
                if (ErrorsChanged != null)
                {
                    ErrorsChanged.Invoke(this, new DataErrorsChangedEventArgs(propertyName));
                }
            }
        }
        #endregion
    }
}