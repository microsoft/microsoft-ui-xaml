// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies what the base type is for this type in the old code generator.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class OldCodeGenBaseTypeAttribute : Attribute
    {
        public Type BaseType
        {
            get;
            private set;
        }

        public OldCodeGenBaseTypeAttribute(Type type)
        {
            BaseType = type;
        }
    }
}
