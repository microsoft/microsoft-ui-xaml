// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies that the type was imported from a different library.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false)]
    public class ImportedAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        public string IdlFileName
        {
            get;
            set;
        }

        public ImportedAttribute()
        {
        }

        public ImportedAttribute(string idlFileName)
        {
            IdlFileName = idlFileName;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IsImported = true;
            definition.IdlTypeInfo.IsImported = true;

            if (!string.IsNullOrEmpty(IdlFileName))
            {
                definition.IdlTypeInfo.FileName = IdlFileName;
            }
            else
            {
                definition.IdlTypeInfo.FileName = definition.DeclaringNamespace.Name.ToLower() + ".idl";
            }
        }
    }
}
