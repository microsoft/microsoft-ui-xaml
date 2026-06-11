// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Reflection;
using System.Windows.Markup;
using System.Xaml;
using System.Xaml.Schema;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    using Lmr;

    internal class ProxyDirectUIXamlType : DirectUIXamlType
    {
        private readonly TypeProxyMetadata _metadata;
        public ProxyDirectUIXamlType(TypeProxyMetadata metadata, DirectUISchemaContext schemaContext)
            : base(metadata.Name, null, schemaContext)
        {
            _metadata = metadata;
        }

        protected override XamlMember LookupAliasedProperty(XamlDirective directive)
        {
            return null;
        }

        protected override IList<XamlType> LookupAllowedContentTypes()
        {
            return null;
        }

        protected override XamlType LookupBaseType()
        {
            Type baseType = GetProxyType(_metadata.BaseTypeName);
            return SchemaContext.GetXamlType(baseType);
        }

        protected override XamlCollectionKind LookupCollectionKind()
        {
            return _metadata.CollectionKind;
        }

        protected override bool LookupConstructionRequiresArguments()
        {
            return _metadata.ConstructionRequiresArguments;
        }

        protected override XamlMember LookupContentProperty()
        {
            return null;
        }

        protected override IList<XamlType> LookupContentWrappers()
        {
            return null;
        }

        protected override XamlValueConverter<XamlDeferringLoader> LookupDeferringLoader()
        {
            return null;
        }

        protected override bool LookupIsConstructible()
        {
            return _metadata.IsConstructible;
        }

        protected override XamlTypeInvoker LookupInvoker()
        {
            return null;
        }

        protected override bool LookupIsMarkupExtension()
        {
            return _metadata.IsMarkupExtension;
        }

        protected override bool LookupIsNameScope()
        {
            return _metadata.IsNameScope;
        }

        protected override bool LookupIsNullable()
        {
            return _metadata.IsNullable;
        }

        protected override bool LookupIsUnknown()
        {
            return _metadata.IsUnknown;
        }

        protected override bool LookupIsWhitespaceSignificantCollection()
        {
            return _metadata.IsWhitespaceSignificantCollection;
        }

        protected override XamlType LookupKeyType()
        {
            return null;
        }

        protected override XamlType LookupItemType()
        {
            return null;
        }

        protected override XamlType LookupMarkupExtensionReturnType()
        {
            return null;
        }

        protected override IEnumerable<XamlMember> LookupAllAttachableMembers()
        {
            return null;
        }

        protected override IEnumerable<XamlMember> LookupAllMembers()
        {
            var allMembers = new List<XamlMember>();

            if (_metadata.MemberNamesAndMetadata != null)
            {
                var duiSchema = SchemaContext as DirectUISchemaContext;
                foreach (var memberName in _metadata.MemberNamesAndMetadata.Keys)
                {
                    XamlTypeName memberTypeName = _metadata.MemberNamesAndMetadata[memberName];
                    XamlType propertyType = duiSchema.GetXamlType(memberTypeName);

                    MemberProxyMetadata memMetaData = new MemberProxyMetadata(memberName, propertyType);
                    allMembers.Add(new ProxyDirectUIXamlMember(memMetaData, this));
                }
            }

            return allMembers;
        }

        protected override XamlMember LookupMember(string name, bool skipReadOnlyCheck)
        {
            XamlMember member = null;

            if (_metadata.MemberNamesAndMetadata != null)
            {
                XamlTypeName xamlTypeName;
                if (_metadata.MemberNamesAndMetadata.TryGetValue(name, out xamlTypeName))
                {
                    var duiSchema = SchemaContext as DirectUISchemaContext;
                    XamlType propertyType = duiSchema.GetXamlType(xamlTypeName);
                    MemberProxyMetadata memMetaData = new MemberProxyMetadata(name, propertyType);
                    member = new ProxyDirectUIXamlMember(memMetaData, this);
                }
            }
            return member;
        }

        protected override XamlMember LookupAttachableMember(string name)
        {
            return null;
        }

        protected override IList<XamlType> LookupPositionalParameters(int parameterCount)
        {
            return null;
        }

        protected override Type LookupUnderlyingType()
        {
            Type underlyingType = GetProxyType(_metadata.UnderlyingTypeName);
            return underlyingType;
        }

        protected override bool LookupIsPublic()
        {
            return _metadata.IsPublic;
        }

        protected override bool LookupIsXData()
        {
            return _metadata.IsXData;
        }

        protected override bool LookupIsAmbient()
        {
            return _metadata.IsAmbient;
        }

        protected override XamlValueConverter<TypeConverter> LookupTypeConverter()
        {
            return null;
        }

        protected override XamlValueConverter<ValueSerializer> LookupValueSerializer()
        {
            return null;
        }

        // Get LMR types at runtime
        // cannot use typeof().
        //
        private Type GetProxyType(string typeName)
        {
            var schema = SchemaContext as DirectUISchemaContext;
            Type type = null;
            foreach (XamlTypeUniverse typeUniverse in schema.DirectUISystem.XamlTypeUniverses)
            {
                Assembly asm = typeUniverse.GetXamlProxyAssembly();
                type = asm.GetType(typeName);
                if (type != null)
                {
                    break;
                }
            }
            return type;
        }

    }
}
