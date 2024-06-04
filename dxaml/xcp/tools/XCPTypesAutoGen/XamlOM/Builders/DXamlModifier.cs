// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies the modifier to use for this type in IDL.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum, Inherited = false)]
    public class DXamlModifierAttribute : Attribute
    {
        public Modifier Modifier
        {
            get;
            private set;
        }

        public DXamlModifierAttribute(Modifier modifier)
        {
            Modifier = modifier;
        }
    }
}
