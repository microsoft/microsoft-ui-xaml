// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies what the type in the core is for this parameter/property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Parameter, Inherited = false)]
    public class CoreTypeAttribute : 
        Attribute,
        NewBuilders.IPropertyBuilder
    {
        public Type PropertyType
        {
            get;
            private set;
        }

        // TODO: Remove when old code-gen gets removed.
        public Type NewCodeGenPropertyType
        {
            get;
            set;
        }

        public CoreTypeAttribute(Type type)
        {
            PropertyType = type;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.PropertyType.Type = NewBuilders.ModelFactory.GetOrCreateType(NewCodeGenPropertyType ?? PropertyType);
        }
    }
}
