// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the modifier for the dependency property to generate.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class DependencyPropertyModifierAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public Modifier Modifier
        {
            get;
            private set;
        }

        public DependencyPropertyModifierAttribute(Modifier modifier)
        {
            Modifier = modifier;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            OM.DependencyPropertyDefinition dp = definition as OM.DependencyPropertyDefinition;
            if (dp != null)
            {
                dp.DependencyPropertyModifier = (OM.Modifier)Modifier;
            }
        }
    }
}
