// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that a type is imported from a different Windows library.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface | AttributeTargets.Delegate, Inherited = false)]
    //[Obsolete("Use ImportedAttribute instead.")]
    public class WindowsTypePatternAttribute :
        Attribute,
        NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder, NewBuilders.IOrderedBuilder
    {
        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IsImported = true;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.CoreCreationMethodName = "";
        }

        public int Order
        {
            get;
            set;
        }
    }
}
