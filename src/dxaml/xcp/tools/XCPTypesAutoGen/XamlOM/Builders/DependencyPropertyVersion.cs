// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the version for the DependencyProperty handle independently of the property itself.
    /// Use this when the property was already shipped in an earlier version but its DependencyProperty
    /// handle needs to be made public in a newer version.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class DependencyPropertyVersionAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        public int Version
        {
            get;
            private set;
        }

        public DependencyPropertyVersionAttribute(int version)
        {
            Version = version;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            OM.DependencyPropertyDefinition dp = definition as OM.DependencyPropertyDefinition;
            if (dp == null)
            {
                throw new InvalidOperationException(string.Format("[DependencyPropertyVersion] cannot be applied to property '{0}.{1}' because it does not generate a DependencyProperty. Remove the attribute or change the property kind.", source.DeclaringType.FullName, source.Name));
            }

            var modifierAtt = source.GetCustomAttribute<DependencyPropertyModifierAttribute>();
            if (modifierAtt != null && modifierAtt.Modifier == Modifier.Private)
            {
                throw new InvalidOperationException(string.Format("[DependencyPropertyVersion] cannot be applied to property '{0}.{1}' because its DependencyProperty is private. Versioning a private DependencyProperty handle is not meaningful.", source.DeclaringType.FullName, source.Name));
            }

            dp.DependencyPropertyVersion = Version;
        }
    }
}
