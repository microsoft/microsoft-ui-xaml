// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies whether the type allows to be associated with multiple objects.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = true)]
    public class AllowsMultipleAssociationsAttribute : Attribute, NewBuilders.ITypeBuilder, NewBuilders.IPattern
    {
        public AllowsMultipleAssociationsAttribute()
        {
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.XamlTypeFlags.AllowsMultipleAssociations = true;
        }
    }
}
