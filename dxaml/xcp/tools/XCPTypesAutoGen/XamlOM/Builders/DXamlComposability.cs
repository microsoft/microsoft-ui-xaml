// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies the composability of this type in IDL.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class DXamlComposabilityAttribute : Attribute, NewBuilders.IClassBuilder
    {
        public Modifier Modifier
        {
            get;
            private set;
        }

        public DXamlComposabilityAttribute(Modifier modifier)
        {
            Modifier = modifier;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            if (Modifier == Modifier.Private)
            {
                definition.IdlClassInfo.IsSealed = true;
            }
        }
    }
}
