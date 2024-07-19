// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies what IDL file to generate this type in.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Struct | AttributeTargets.Enum, Inherited = false)]
    public class DXamlIdlGroupAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        public string IdlGroup
        {
            get;
            set;
        }

        public DXamlIdlGroupAttribute(string idlGroup)
        {
            IdlGroup = idlGroup;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IdlTypeInfo.Group = IdlGroup;
        }
    }
}
