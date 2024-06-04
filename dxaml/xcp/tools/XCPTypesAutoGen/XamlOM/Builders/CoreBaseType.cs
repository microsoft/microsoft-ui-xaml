// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies what the base type is for this type in the core.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public class CoreBaseTypeAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public Type BaseType
        {
            get;
            private set;
        }

        // TODO: Remove this when old code-gen gets removed.
        /// <summary>
        /// Allows us to specify the base type in the new code generator.
        /// </summary>
        public Type NewCodeGenBaseType
        {
            get;
            set;
        }

        public CoreBaseTypeAttribute(Type type)
        {
            BaseType = type;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.BaseClass = NewBuilders.ModelFactory.GetOrCreateClass(NewCodeGenBaseType ?? BaseType);
        }
    }
}
