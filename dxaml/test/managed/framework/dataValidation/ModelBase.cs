// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Collections;
using System.Runtime.CompilerServices;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Controls;
using MockDataAnnotations;

using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Framework.DataValidation
{
    public static class ValidationResultExtensions
    {
        public static IEnumerable<object> ToErrors(this IEnumerable<ValidationResult> results, string propertyName)
        {
            if (String.IsNullOrEmpty(propertyName))
            {
                return results;
            }

            return results.Where(e => e.MemberNames[0] == propertyName);
        }
    }

    public abstract class ModelBase : Microsoft.UI.Xaml.Data.INotifyDataErrorInfo, INotifyPropertyChanged
    {
        private Dictionary<string, object> _properties = new Dictionary<string, object>();
        protected object GetValue([CallerMemberName]string propertyName = "")
        {
            return _properties.ContainsKey(propertyName) ? _properties[propertyName] : null;
        }

        private TestObservableCollection<ValidationResult> _errors = new TestObservableCollection<ValidationResult>();
        public bool HasErrors { get { return _errors.Count > 0; } }

        public event EventHandler<Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs> ErrorsChanged;
        public event PropertyChangedEventHandler PropertyChanged;

        public IEnumerable<object> GetErrors(string propertyName)
        {
            return _errors.ToErrors(propertyName);
        }

        protected void AddError(ValidationResult result)
        {
            _errors.Add(result);
            ErrorsChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs(result.MemberNames.FirstOrDefault()));
        }

        private void OnValidate(string propertyName)
        {
            var val = GetValue(propertyName);
            var validationContext = new ValidationContext(this, propertyName);
            if (!Validator.TryValidateProperty( val,validationContext, out List<ValidationResult> results))
            {
                foreach (var error in results)
                {
                    AddError(error);
                }
            }
        }
            
        protected void SetValue(object newValue, [CallerMemberName]string propertyName = "")
        {
            var current = GetValue(propertyName);
            if (!newValue.Equals(current))
            {
                _properties[propertyName] = newValue;
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
                OnValidate(propertyName);
            }
        }
    }
}
