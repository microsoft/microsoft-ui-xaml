// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using XamlDom;

    internal class BoundLoadAssignment : BindAssignment
    {
        public BoundLoadAssignment(XamlDomMember bindMember, BindUniverse bindUniverse, ConnectionIdElement connectionIdElement)
            : base(bindMember, bindUniverse, connectionIdElement)
        {
        }

        public override XamlType MemberType
        {
            // For x:Load, the member type is bool
            get { return this.bindMember.SchemaContext.GetXamlType(typeof(bool)); }
        }

        public override XamlType MemberDeclaringType
        {
            get { return this.ConnectionIdElement.Type; }
        }

        public override bool IsAttachable
        {
            // x:Load is not attachable, obviously
            get { return false; }
        }

        public override bool HasSetValueHelper
        {
            get { return false; }
        }

        public override bool HasDeferredValueProxy
        {
            get { return false; }
        }
    }
}