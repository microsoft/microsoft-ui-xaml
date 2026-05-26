// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type is a private api contract.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    public class PrivateApiContractAttribute : 
        Attribute,
        NewBuilders.ITypeBuilder
    {
        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IdlTypeInfo.IsPrivateIdlOnly = true;
        }
    }
}
