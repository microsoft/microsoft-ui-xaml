// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Forces a type to be considered sealed, while still allowing internal types to derive from this type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class ForceSealedAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public ForceSealedAttribute()
        {
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            definition.IsSealed = true;
            definition.IdlClassInfo.IsSealed = true;
        }
    }
}
