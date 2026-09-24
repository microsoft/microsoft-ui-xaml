// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using System.ComponentModel.DataAnnotations;

namespace BindTestbedModel
{
    public sealed class DOModel : DependencyObject
    {
        public DOModel()
        {
            YangObj = new YangContainer(this);
        }

        public void UpdateValues()
        {
            this.IntPropertyDP++;
            YinPropertyDP = 65;
        }

        public YangContainer YangObj
        {
            get;
            set;
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
        public static DependencyProperty IntPropertyDPProperty {  get { return _IntPropertyDPProperty; } }
        #endregion

        public double YinPropertyDP
        {
            get { return (double)GetValue(YinPropertyDPProperty); }
            set { SetValue(YinPropertyDPProperty, value); }
        }
        #region YinPropertyDP DP
        private const string YinPropertyDPName = "YinPropertyDP";
        private static readonly DependencyProperty _YinPropertyDPProperty =
            DependencyProperty.Register(YinPropertyDPName, typeof(double), typeof(DataModel), new PropertyMetadata(0, YinCallback));
        public static DependencyProperty YinPropertyDPProperty { get { return _YinPropertyDPProperty; } }
        #endregion


        public static void YinCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            DOModel inst = d as DOModel;
            double value = (double)e.NewValue;
            if (inst.YangObj.Value != value)
            {
                inst.YangObj.Value = value;
            }
        }
    }

    public sealed class YangContainer : INotifyPropertyChanged
    {
        private DOModel _parent;

        public YangContainer(DOModel parent)
        {
            _parent = parent;
        }

        private double _value;
        public double Value
        {
            get { return _value; }
            set
            {
                if (value != _value)
                {
                    _value = value;
                    NotifyPropertyChanged();
                    _parent.YinPropertyDP = value;
                }
            }
        }


        #region INPC

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged([CallerMemberName] string propertyName = "")
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
        #endregion
    }

    public sealed class DODataErrorModel : DependencyObject // TODO: add Microsoft.UI.Xaml.Data.INotifyDataErrorInfo support
    {
        static private DependencyProperty IntWithRangeProperty = DependencyProperty.Register("IntWithRange", typeof(int), typeof(DODataErrorModel), null);
        [Range(0, 100, ErrorMessage = "IntWithRange is not within expected range")]
        public int IntWithRange
        {
            get { return (int)GetValue(IntWithRangeProperty); }
            set { SetValue(IntWithRangeProperty, value); }
        }

        static private DependencyProperty RequiredStringProperty = DependencyProperty.Register("RequiredString", typeof(string), typeof(DODataErrorModel), null);
        [Required(ErrorMessage = "RequiredString is required, please enter a field")]
        public string RequiredString
        {
            get { return (string)GetValue(RequiredStringProperty); }
            set
            {
                SetValue(RequiredStringProperty, value);
            }
        }
        #region INPC/INDEI
        private void SetValue(DependencyProperty property, object newValue, [CallerMemberName] string propertyName = "")
        {
            var currentValue = GetValue(property);
            if (!Object.Equals(currentValue, newValue))
            {
                SetValue(property, newValue);
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

        // public event EventHandler<Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs> ErrorsChanged;

        private void OnPropertyChanged(object value, string propertyName)
        {
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
            // ErrorsChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs(propertyName));
        }

        private void ClearErrors(string propertyName)
        {
            if (_errors.TryGetValue(propertyName, out var errors))
            {
                errors.Clear();
                // ErrorsChanged?.Invoke(this, new Microsoft.UI.Xaml.Data.DataErrorsChangedEventArgs(propertyName));
            }
        }
#endregion
    }
}
