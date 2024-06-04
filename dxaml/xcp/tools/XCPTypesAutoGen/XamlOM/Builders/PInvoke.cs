// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies this method is a wrapper in the framework that invokes a corresponding method in the core.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property, Inherited = false)]
    public class PInvokeAttribute : Attribute, NewBuilders.IMethodBuilder, NewBuilders.IPropertyBuilder
    {
        public PInvokeAttribute()
        {
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.GenerateDefaultBody = true;
            definition.DelegateToCore = true;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.GenerateDefaultBody = true;
            definition.DelegateToCore = true;
        }
    }

    /// <summary>
    /// Specifies this method is a wrapper in the framework that invokes a corresponding method in the core.
    /// NOTE: This is the same as PInvokeAttribute, except the old code generator won't look for this attribute.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method | AttributeTargets.Property, Inherited = false)]
    public class DelegateToCoreAttribute : Attribute, NewBuilders.IMethodBuilder, NewBuilders.IPropertyBuilder
    {
        public DelegateToCoreAttribute()
        {
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.GenerateDefaultBody = true;
            definition.DelegateToCore = true;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.GenerateDefaultBody = true;
            definition.DelegateToCore = true;
        }
    }
}
