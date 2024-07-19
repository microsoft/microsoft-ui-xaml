// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies whether the property should verify that the value does not have multiple associations.
    /// Examples: FrameworkElement.DataContext, Page.Frame, Button.Flyout.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class RequiresMultipleAssociationCheckAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public RequiresMultipleAssociationCheckAttribute()
        {
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.XamlPropertyFlags.RequiresMultipleAssociationCheck = true;
        }
    }
}
