// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    public class XamlProperty
    {
        public string FullName => (DeclaringType != null) ? DeclaringType.FullName + "." + Name : Name;

        public string Name { get; }

        public XamlType DeclaringType { get; }

        public XamlType PropertyType { get; internal set; }

        public bool IsAttached { get; internal set; }

        internal bool IsVisualTreeProperty { get; set; }

        public XamlProperty(string name, XamlType declaringType)
        {
            Name = name;
            DeclaringType = declaringType;
        }

        public override string ToString()
        {
            return FullName;
        }

        public override bool Equals(object obj)
        {
            var property = obj as XamlProperty;
            return property != null &&
                   FullName == property.FullName;
        }

        public override int GetHashCode()
        {
            return 733961487 + EqualityComparer<string>.Default.GetHashCode(FullName);
        }
    }
}
