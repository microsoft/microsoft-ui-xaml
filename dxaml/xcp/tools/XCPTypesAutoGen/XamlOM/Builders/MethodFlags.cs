// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies low-level flags that control the behavior of this method.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class MethodFlagsAttribute : Attribute, NewBuilders.IMethodBuilder
    {
        /// <summary>
        /// Controls whether the method's implementation is virtual
        /// </summary>
        public bool IsImplVirtual { get; set; }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.IsImplVirtual = IsImplVirtual;
        }
    }
}
