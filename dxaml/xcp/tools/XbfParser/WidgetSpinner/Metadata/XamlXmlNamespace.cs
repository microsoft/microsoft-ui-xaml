// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    public struct XamlXmlNamespace
    {
        public string NamespaceUri { get; }

        internal XamlXmlNamespace(string namespaceUri) : this()
        {
            NamespaceUri = namespaceUri;
        }
    }
}
