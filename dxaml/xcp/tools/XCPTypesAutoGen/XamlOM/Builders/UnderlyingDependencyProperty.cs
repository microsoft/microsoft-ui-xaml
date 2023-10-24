// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the underlying DP. This is currently only used for FrameworkElement.NameProperty, which delegates to 
    /// the internal DependencyObject.NameProperty.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class UnderlyingDependencyPropertyAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        private Type _underlyingType;
        private string _underlyingDPName;

        public UnderlyingDependencyPropertyAttribute(Type underlyingType, string underlyingDPName)
        {
            _underlyingType = underlyingType;
            _underlyingDPName = underlyingDPName;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            OM.ClassDefinition underlyingType = NewBuilders.ModelFactory.GetOrCreateClass(_underlyingType);
            ((OM.DependencyPropertyDefinition)definition).UnderlyingDependencyProperty = (OM.DependencyPropertyDefinition)underlyingType.GetProperty(_underlyingDPName);
        }
    }
}
