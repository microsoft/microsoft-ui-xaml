// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies how a collection should be projected in IDL.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Method | AttributeTargets.Parameter, Inherited = false)]
    public class CollectionTypeAttribute : 
        Attribute, 
        NewBuilders.IPropertyBuilder, NewBuilders.IMethodBuilder, NewBuilders.IParameterBuilder
    {
        public Type NewCodeGenCollectionType
        {
            get;
            set;
        }

        public CollectionKind Kind
        {
            get;
            private set;
        }

        public CollectionTypeAttribute(CollectionKind kind)
        {
            Kind = kind;
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            if (NewCodeGenCollectionType != null)
            {
                definition.PropertyType.Type = definition.PropertyType.IdlInfo.Type = NewBuilders.ModelFactory.GetOrCreateType(NewCodeGenCollectionType);
            }
            else
            {
                OM.TypeDefinition elementType = NewBuilders.Helper.GetCollectionElementType((OM.ClassDefinition)definition.PropertyType.Type);
                definition.PropertyType.IdlInfo.Type = NewBuilders.Helper.GetCollectionType((OM.CollectionKind)Kind, elementType);
            }
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            OM.TypeDefinition elementType = NewBuilders.Helper.GetCollectionElementType((OM.ClassDefinition)definition.ReturnType.Type);
            definition.ReturnType.IdlInfo.Type = NewBuilders.Helper.GetCollectionType((OM.CollectionKind)Kind, elementType);
        }

        public void BuildNewParameter(OM.ParameterDefinition definition, ParameterInfo source)
        {
            OM.TypeDefinition elementType = NewBuilders.Helper.GetCollectionElementType((OM.ClassDefinition)definition.ParameterType.Type);
            definition.ParameterType.IdlInfo.Type = NewBuilders.Helper.GetCollectionType((OM.CollectionKind)Kind, elementType);
        }
    }
}
