// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.ComponentModel.DataAnnotations;

namespace BindTestbedModel
{
    public static class AttachedProperties
    {
        public static Boolean GetAttachedBool(TextBlock obj)
        {
            return (Boolean)obj.GetValue(AttachedBoolProperty);
        }
        public static void SetAttachedBool(TextBlock obj, Boolean value)
        {
            obj.SetValue(AttachedBoolProperty, value);
        }
        private static DependencyProperty _AttachedBoolProperty = DependencyProperty.RegisterAttached(
                "AttachedBool", typeof(bool), typeof(AttachedProperties), new PropertyMetadata(null));
        public static DependencyProperty AttachedBoolProperty { get { return _AttachedBoolProperty; } }

        public static IEmployee GetAttachedEmployee(TextBlock obj)
        {
            return (IEmployee)obj.GetValue(AttachedEmployeeProperty);
        }
        public static void SetAttachedEmployee(TextBlock obj, IEmployee value)
        {
            obj.SetValue(AttachedEmployeeProperty, value);
        }
        private static DependencyProperty _AttachedEmployeeProperty = DependencyProperty.RegisterAttached(
                "AttachedEmployee", typeof(IEmployee), typeof(AttachedProperties), new PropertyMetadata(null));
        public static DependencyProperty AttachedEmployeeProperty { get { return _AttachedEmployeeProperty; } }

        public static String GetAttachedString(DependencyObject obj)
        {
            var value = (String)obj.GetValue(AttachedStringProperty);
            return value ?? "";
        }
        public static void SetAttachedString(DependencyObject obj, String value)
        {
            obj.SetValue(AttachedStringProperty, value);
        }
        private static DependencyProperty _AttachedStringProperty = DependencyProperty.RegisterAttached(
                "AttachedString", typeof(string), typeof(AttachedProperties), new PropertyMetadata(null, AttachedStringChanged));
        [Required(ErrorMessage = "AttachedString is required, please enter a field")]
        public static DependencyProperty AttachedStringProperty { get { return _AttachedStringProperty; } }

        public static void AttachedStringChanged(Microsoft.UI.Xaml.DependencyObject dependencyObject, Microsoft.UI.Xaml.DependencyPropertyChangedEventArgs e)
        {

        }
    }
}
