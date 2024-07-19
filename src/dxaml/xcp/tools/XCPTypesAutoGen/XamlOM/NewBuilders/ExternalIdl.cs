// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type's IDL was imported from a different library, but the definition exists in the current library.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Enum, Inherited = false)]
    public class ExternalIdlAttribute : Attribute, NewBuilders.ITypeBuilder, NewBuilders.IClassBuilder
    {
        public string IdlFileName
        {
            get;
            set;
        }

        public string IdlTypeName
        {
            get;
            set;
        }

        public ExternalIdlAttribute()
        {
        }

        public ExternalIdlAttribute(string idlFileName)
        {
            IdlFileName = idlFileName;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IdlTypeInfo.IsImported = true;
            if (!string.IsNullOrEmpty(IdlFileName))
            {
                definition.IdlTypeInfo.FileName = IdlFileName;
            }
            else
            {
                definition.IdlTypeInfo.FileName = definition.DeclaringNamespace.Name.ToLower() + ".idl";
            }

            if (!string.IsNullOrEmpty(IdlTypeName))
            {
                definition.IdlTypeInfo.Name = IdlTypeName;
            }
        }

        // Run again for classes to deal with the fact that attributes don't execute in a guaranteed order.
        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            if (!string.IsNullOrEmpty(IdlTypeName))
            {
                definition.IdlTypeInfo.Name = IdlTypeName;
            }
        }
    }
}
