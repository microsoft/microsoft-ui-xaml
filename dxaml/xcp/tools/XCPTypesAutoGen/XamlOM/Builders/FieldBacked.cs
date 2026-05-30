// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that a dependency property is backed by a field. This is the same as using OffsetFieldNameAttribute, except 
    /// we automatically generate a name for the field based on the dependency property name.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class FieldBackedAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public FieldBackedAttribute()
        {
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, System.Reflection.PropertyInfo source)
        {
            definition.FieldName = NewBuilders.Helper.GetFieldName(source);
        }
    }
}
