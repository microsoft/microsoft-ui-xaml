// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the method mutates the struct.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class MutatorAttribute : Attribute, NewBuilders.IMethodBuilder
    {
        public MutatorAttribute()
        {
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.IsMutator = true;
            definition.ReturnType = NewBuilders.Helper.GetReturnTypeRef(NewBuilders.Helper.GetClrType(definition.DeclaringType));
        }
    }
}
