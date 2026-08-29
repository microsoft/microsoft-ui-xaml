// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using XamlDom;

    internal class BoundPropertyAssignment : BindAssignment
    {
        public BoundPropertyAssignment(XamlDomMember bindMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
            : base(bindMember, bindUniverse, connectionIdElement)
        {
        }
    }
}