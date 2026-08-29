// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    public class XamlType
    {
        public XamlType BaseType { get; internal set; }

        public string FullName { get; }

        public bool IsCollection { get; internal set; } = false;

        public bool IsDictionary { get; internal set; } = false;

        public bool IsMarkupExtension { get; internal set; } = false;

        public XamlType(string fullName)
        {
            FullName = fullName;
        }

        public XamlType(string fullName, XamlType baseType, XamlTypeFlags typeFlags)
        {
            FullName = fullName;
            BaseType = baseType;
            IsCollection = typeFlags.HasFlag(XamlTypeFlags.IsCollection);
            IsDictionary = typeFlags.HasFlag(XamlTypeFlags.IsDictionary);
            IsMarkupExtension = typeFlags.HasFlag(XamlTypeFlags.IsMarkupExtension);
        }

        /// <summary>
        ///  Returns 'true' if 'other' XamlType is a derived type of
        /// 'this' XamlType
        /// </summary>
        public bool IsAssignableFrom(XamlType other)
        {
            var otherBaseType = other.BaseType;

            while (otherBaseType != null)
            {
                if (otherBaseType.Equals(this))
                {
                    return true;
                }
                otherBaseType = otherBaseType.BaseType;
            }

            return false;
        }

        /// <summary>
        ///  Returns 'true' if 'this' XamlType is a derived type of
        /// 'other' XamlType
        /// </summary>
        public bool IsDerivedFrom(XamlType other)
        {
            return other.IsAssignableFrom(this);
        }

        public override string ToString()
        {
            return FullName;
        }

        public override bool Equals(object obj)
        {
            var type = obj as XamlType;
            return type != null &&
                   FullName == type.FullName;
        }

        public override int GetHashCode()
        {
            return 733961487 + EqualityComparer<string>.Default.GetHashCode(FullName);
        }
    }
}
