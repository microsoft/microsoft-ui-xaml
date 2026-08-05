// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using Microsoft.UI.Xaml;

namespace ConditionalControls
{
    public static class AttachedProperties
    {
        public static Boolean GetAttachedBool(DependencyObject obj)
        {
            return (Boolean)obj.GetValue(AttachedBoolProperty);
        }

        public static void SetAttachedBool(DependencyObject obj, Boolean value)
        {
            obj.SetValue(AttachedBoolProperty, value);
        }

        private static DependencyProperty _AttachedBoolProperty = DependencyProperty.RegisterAttached(
                "AttachedBool", typeof(bool), typeof(AttachedProperties), new PropertyMetadata(null));

        public static DependencyProperty AttachedBoolProperty { get { return _AttachedBoolProperty; } }
    }
}
