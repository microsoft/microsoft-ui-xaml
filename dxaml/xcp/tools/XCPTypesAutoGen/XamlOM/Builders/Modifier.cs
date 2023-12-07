// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies the real modifier of this type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class ModifierAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public Modifier Modifier
        {
            get;
            private set;
        }

        public ModifierAttribute(Modifier modifier)
        {
            Modifier = modifier;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            if (Modifier != Modifier.Public)
            {
                definition.IdlTypeInfo.IsExcluded = true;
            }
            definition.Modifier = (OM.Modifier)Modifier;
        }
    }
}
