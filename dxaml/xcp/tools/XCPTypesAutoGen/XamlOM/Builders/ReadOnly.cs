// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the property is read-only.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class ReadOnlyAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.IsReadOnly = true;
            definition.IdlPropertyInfo.IsReadOnly = true;
        }
    }
}
