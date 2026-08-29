// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System.Xaml;

    internal class XamlDomNamespace : XamlDomNode
    {
        private NamespaceDeclaration namespaceDeclaration;

        public XamlDomNamespace(NamespaceDeclaration namespaceDeclaration, string sourceFilePath)
            : base(sourceFilePath)
        {
            this.NamespaceDeclaration = namespaceDeclaration;
        }

        public NamespaceDeclaration NamespaceDeclaration
        {
            get
            {
                return this.namespaceDeclaration;
            }
            set
            {
                this.CheckSealed();
                this.namespaceDeclaration = value;
            }
        }
    }
}