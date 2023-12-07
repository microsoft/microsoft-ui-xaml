// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Reflection;
    using System.Windows.Markup;
    using System.Xaml;
    using System.Xaml.Schema;

    internal class ProxyDirectUIXamlMember : DirectUIXamlMember
    {
        private readonly MemberProxyMetadata metadata;

        public ProxyDirectUIXamlMember(MemberProxyMetadata metadata, DirectUIXamlType declaringType)
            : base(metadata.Name, declaringType) 
        {
            this.metadata = metadata;
        }

        protected override XamlMemberInvoker LookupInvoker()
        {
            return this.metadata.Invoker;
        }

        protected override XamlValueConverter<XamlDeferringLoader> LookupDeferringLoader()
        {
            return this.metadata.DeferringLoader;
        }

        protected override IList<XamlMember> LookupDependsOn()
        {
            return this.metadata.DependsOn;
        }

        protected override bool LookupIsAmbient()
        {
            return this.metadata.IsAmbient;
        }

        protected override bool LookupIsEvent()
        {
            return this.metadata.IsEvent;
        }

        // Note: this returns whether the member itself is public or not. The visibility of the
        // declaring type is considered in the IsReadPublic property, not here.
        protected override bool LookupIsReadPublic()
        {
            return this.metadata.IsReadPublic;
        }

        protected override bool LookupIsReadOnly()
        {
            return this.metadata.IsReadOnly;
        }

        protected override bool LookupIsUnknown()
        {
            return this.metadata.IsUnknown;
        }

        protected override bool LookupIsWriteOnly()
        {
            return this.metadata.IsWriteOnly;
        }

        // Note: this returns whether the member itself is public or not. The visibility of the
        // declaring type is considered in the IsWritePublic property, not here.
        protected override bool LookupIsWritePublic()
        {
            return this.metadata.IsWritePublic;
        }

        protected override XamlType LookupTargetType()
        {
            return this.metadata.TargetType;
        }

        protected override XamlValueConverter<TypeConverter> LookupTypeConverter()
        {
            return this.metadata.TypeConverter;
        }

        protected override XamlValueConverter<ValueSerializer> LookupValueSerializer()
        {
            return this.metadata.ValueSerializer;
        }

        protected override XamlType LookupType()
        {
            return this.metadata.Type;
        }

        protected override MethodInfo LookupUnderlyingGetter()
        {
            return null;
        }

        protected override MethodInfo LookupUnderlyingSetter()
        {
            return null;
        }

        protected override MemberInfo LookupUnderlyingMember()
        {
            return null;
        }

        public override IList<string> GetXamlNamespaces()
        {
            return this.metadata.XamlNamespaces;
        }

        protected override IReadOnlyDictionary<char, char> LookupMarkupExtensionBracketCharacters()
        {
            if (DeclaringType.UnderlyingType.FullName == typeof(ProxyTypes.BindExtension).FullName && Name == "Path")
            {
                return new Dictionary<char, char>() { { '(', ')' }, { '[', ']' } };
            }
            return null;
        }
    }
}