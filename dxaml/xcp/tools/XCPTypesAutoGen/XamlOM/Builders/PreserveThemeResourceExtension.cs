// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies whether ThemeResourceExtensions should be preserved as the effective value. If this attribute is not 
    /// applied to a property, a ThemeResourceExpression will be instantiated, and the effective value will evaluate to 
    /// the value resolved by the {ThemeResource}.
    /// Example: Setter.Style.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class PreserveThemeResourceExtensionAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public PreserveThemeResourceExtensionAttribute()
        {
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.XamlPropertyFlags.PreserveThemeResourceExtension = true;
        }
    }
}
