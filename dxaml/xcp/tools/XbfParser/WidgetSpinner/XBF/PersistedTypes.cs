// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    [Flags]
    internal enum PersistedXamlTypeFlags
    {
        None = 0x0,
        IsMarkupDirective = 0x1,
        IsUnknown = 0x2
    }

    [Flags]
    internal enum PersistedXamlPropertyFlags
    {
        None,
        IsXmlProperty = 0x01,
        IsMarkupDirective = 0x02,
        IsImplicitProperty = 0x04,
        IsCustomDependencyProperty = 0x08,
        IsUnknown = 0x10,
    }

    internal enum PersistedConstantType
    {
        None = 0,
        IsBoolFalse,
        IsBoolTrue,
        IsFloat,
        IsSigned,
        IsSharedString,
        IsThickness,
        IsGridLength,
        IsColor,
        IsUniqueString,
        IsNullString,
        IsEnum
    }

    internal struct PersistedXamlType
    {
        internal PersistedXamlTypeFlags Flags { get; }
        internal int NamespaceId { get; }
        internal string Name { get; }

        internal PersistedXamlType(PersistedXamlTypeFlags flags, int namespaceId, string name) : this()
        {
            Flags = flags;
            NamespaceId = namespaceId;
            Name = name;
        }
    }

    internal struct PersistedXamlProperty
    {
        internal PersistedXamlPropertyFlags Flags { get; }
        internal int DeclaringTypeId { get; }
        internal string Name { get; }

        internal PersistedXamlProperty(PersistedXamlPropertyFlags flags, int declaringTypeId, string name) : this()
        {
            Flags = flags;
            DeclaringTypeId = declaringTypeId;
            Name = name;
        }
    }

    internal struct PersistedXamlNode
    {
        internal ushort ObjectId { get; }
        internal bool IsTrusted { get; }

        public PersistedXamlNode(ushort objectId, bool isTrusted) : this()
        {
            ObjectId = objectId;
            IsTrusted = isTrusted;
        }
    }
}
