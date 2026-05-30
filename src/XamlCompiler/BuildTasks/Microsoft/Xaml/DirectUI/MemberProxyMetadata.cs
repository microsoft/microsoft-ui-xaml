// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Windows.Markup;
    using System.Xaml;
    using System.Xaml.Schema;

    internal class MemberProxyMetadata
    {
        public MemberProxyMetadata(string name, XamlType type)
        {
            this.IsReadPublic = true;
            this.IsWritePublic = true;
            this.Name = name;
            this.Type = type;
        }

        public string Name { get; private set; }
        public XamlMemberInvoker Invoker { get; private set; }
        public XamlValueConverter<XamlDeferringLoader> DeferringLoader { get; internal set; }
        public XamlValueConverter<TypeConverter> TypeConverter { get; private set; }
        public XamlValueConverter<ValueSerializer> ValueSerializer { get; private set; }
        public IList<XamlMember> DependsOn { get; private set; }
        public bool IsAmbient { get; private set; }
        public bool IsEvent { get; private set; }
        public bool IsReadPublic { get; private set; }
        public bool IsReadOnly { get; private set; }
        public bool IsUnknown { get; private set; }
        public bool IsWriteOnly { get; private set; }
        public bool IsWritePublic { get; private set; }
        public XamlType TargetType { get; private set; }
        public XamlType Type { get; private set; }
        public IList<string> XamlNamespaces { get; private set; }
    }
}