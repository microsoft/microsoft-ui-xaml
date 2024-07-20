// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    public enum XamlTypeInfoProviderKind
    {
        Unknown = 0,
        Native,
        Managed,
        System,
        Parser,
        Alternate,
        NumberOfItems
    }

    public struct XamlAssembly
    {
        public XamlTypeInfoProviderKind Kind { get; }
        public string Name { get; }

        internal XamlAssembly(XamlTypeInfoProviderKind kind, string name) : this()
        {
            Kind = kind;
            Name = name;
        }
    }
}
