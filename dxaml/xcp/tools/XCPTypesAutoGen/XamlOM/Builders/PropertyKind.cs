// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    public enum PropertyKind
    {
        Both = 0,
        DependencyPropertyOnly = 1,
        PropertyOnly = 2
    }

    /// <summary>
    /// Specifies what kind of property to represent.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class PropertyKindAttribute : Attribute
    {
        public PropertyKind Kind
        {
            get;
            private set;
        }

        public PropertyKindAttribute(PropertyKind kind)
        {
            Kind = kind;
        }
    }
}
