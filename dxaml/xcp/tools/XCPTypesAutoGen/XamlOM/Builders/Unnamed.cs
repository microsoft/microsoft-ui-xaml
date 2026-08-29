// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies this type should not have a name.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public class UnnamedAttribute : Attribute
    {
        public UnnamedAttribute()
        {
        }
    }
}
