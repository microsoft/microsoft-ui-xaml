// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies a method return value is optional.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class OptionalReturnValueAttribute : Attribute, NewBuilders.IMethodBuilder
    {
        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.ReturnType.IsOptional = true;
        }
    }
}
