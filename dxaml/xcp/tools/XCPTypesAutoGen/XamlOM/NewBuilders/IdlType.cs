// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies the type to use in IDL instead when dealing with type references.
    /// Example: DependencyPropertyProxy is an internal type. In IDL, we want to use DependencyProperty.
    /// Note: This is not supported for types in type parameters.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface, Inherited = false)]
    public class IdlTypeAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public Type IdlType
        {
            get;
            private set;
        }

        public IdlTypeAttribute(Type idlType)
        {
            IdlType = idlType;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            if (!source.IsValueType)
            {
                definition.IdlTypeInfo.IsExcluded = true;
            }

            definition.IdlClassInfo.IsSurrogate = true;
            definition.IdlClassInfo.SurrogateFor = (ClassDefinition)NewBuilders.ModelFactory.GetOrCreateType(IdlType);
        }
    }
}
