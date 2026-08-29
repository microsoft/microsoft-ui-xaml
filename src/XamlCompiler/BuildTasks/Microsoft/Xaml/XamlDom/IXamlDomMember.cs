// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.XamlDom
{
    using System.Collections.Generic;
    using System.Xaml;

    internal interface IXamlDomMember
    {
        XamlDomItem Item { get; set; }
        IList<XamlDomItem> Items { get; }
        XamlMember Member { get; set; }
        XamlDomObject Parent { get; set; }
        XamlSchemaContext SchemaContext { get; set; }

        void Seal();
    }
}