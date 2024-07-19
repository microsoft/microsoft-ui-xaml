// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    public struct XamlTypeNamespace
    {
        public XamlAssembly Assembly { get; }
        public string Name { get; }

        internal XamlTypeNamespace(XamlAssembly assembly, string name) : this()
        {
            Assembly = assembly;
            Name = name;
        }
    }
}
