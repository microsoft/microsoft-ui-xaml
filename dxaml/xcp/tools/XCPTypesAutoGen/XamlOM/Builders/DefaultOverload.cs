// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Spceifies the default overload of a method.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class DefaultOverloadAttribute : Attribute, NewBuilders.IMethodBuilder
    {
        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.IsDefaultOverload = true;
        }
    }
}
