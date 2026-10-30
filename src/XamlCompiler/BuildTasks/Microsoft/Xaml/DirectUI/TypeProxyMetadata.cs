// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Xaml;
using System.Xaml.Schema;

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI
{
    internal class TypeProxyMetadata
    {
        public string Name = string.Empty;

        public XamlCollectionKind CollectionKind = XamlCollectionKind.None;
        public bool ConstructionRequiresArguments = false;

        public String UnderlyingTypeName;
        public String BaseTypeName;

        public bool IsConstructible = true;
        public bool IsMarkupExtension = false;
        public bool IsNameScope = false;
        public bool IsNullable = true;
        public bool IsUnknown = false;
        public bool IsWhitespaceSignificantCollection = false;
        public bool IsPublic = true;
        public bool IsXData = false;
        public bool IsAmbient = false;

        private TypeProxyMetadata() { }

        public Dictionary<String, XamlTypeName> MemberNamesAndMetadata;

        // Markup Extensions are a weak point in the API compat in SL 3 & 4
        // Binding and RelitiveSource really exist as classes but are not derived
        // from MarkupExtension (there is no MarkupExtension base class).
        // TemplateBinding and StaticResource don't  exists at all as objects
        // in the DirectUI OM and are faked by the DirectUI Schema.
        // [See: DirectUIXamlType.LookupIsMarkupExtension()]
        //
        public static TypeProxyMetadata TemplateBindingExtension = new TypeProxyMetadata()
        {
            Name = "TemplateBindingExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.TemplateBindingExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension"
        };

        public static TypeProxyMetadata StaticResourceExtension = new TypeProxyMetadata()
        {
            Name = "StaticResourceExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.StaticResourceExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension",
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "ResourceKey", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") }
            }
        };
        
        public static TypeProxyMetadata ThemeResourceExtension = new TypeProxyMetadata()
        {
            Name = "ThemeResourceExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.ThemeResourceExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension",
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "ResourceKey", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") }
            }
        };

        public static TypeProxyMetadata NullExtension = new TypeProxyMetadata()
        {
            Name = "NullExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.NullExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension"
        };

        public static TypeProxyMetadata CustomResourceExtension = new TypeProxyMetadata()
        {
            Name = "CustomResourceExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.CustomResourceExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension",
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "ResourceKey", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") }
            }
        };

        public static TypeProxyMetadata BindExtension = new TypeProxyMetadata()
        {
            Name = "BindExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = ProxyTypeNames.BindExtension,
            BaseTypeName = "System.Windows.Markup.MarkupExtension",
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "BindBack", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "Path", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "Mode", new XamlTypeName(KnownNamespaceUris.Wpf2006, "BindingMode") },
                { "Converter", new XamlTypeName(KnownNamespaceUris.Wpf2006, "IValueConverter") },
                { "ConverterParameter", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Object") },
                { "ConverterLanguage", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "FallbackValue", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Object") },
                { "TargetNullValue", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Object") },
                { "UpdateSourceTrigger", new XamlTypeName(KnownNamespaceUris.Wpf2006, "UpdateSourceTrigger") },
            }
        };

        public static TypeProxyMetadata Properties = new TypeProxyMetadata()
        {
            Name = "Properties",
            IsMarkupExtension = false,
            UnderlyingTypeName = ProxyTypeNames.Properties,
            BaseTypeName = "System.Object",
            CollectionKind = XamlCollectionKind.Collection,
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "DefaultValue", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Object") }
            }
        };

        public static TypeProxyMetadata Property = new TypeProxyMetadata()
        {
            Name = "Property",
            IsMarkupExtension = false,
            UnderlyingTypeName = ProxyTypeNames.Property,
            BaseTypeName = "System.Object",
            MemberNamesAndMetadata = new Dictionary<string, XamlTypeName>()
            {
                { "DefaultValue", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Object") },
                { "Name", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "Type", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "ChangedHandler", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "String") },
                { "ReadOnly", new XamlTypeName(XamlLanguage.Xaml2006Namespace, "Boolean") }
            }
        };
    }

    // Full names of the types in the embedded Microsoft.UI.Xaml.Markup.Compiler.ProxyTypes assembly (see XamlTypeUniverse.GetXamlProxyAssembly).
    internal static class ProxyTypeNames
    {
        public const string MarkupExtension = KnownNamespaces.ProxyTypes + ".MarkupExtension";
        public const string NullExtension = KnownNamespaces.ProxyTypes + ".NullExtension";
        public const string StaticResourceExtension = KnownNamespaces.ProxyTypes + ".StaticResourceExtension";
        public const string ThemeResourceExtension = KnownNamespaces.ProxyTypes + ".ThemeResourceExtension";
        public const string CustomResourceExtension = KnownNamespaces.ProxyTypes + ".CustomResourceExtension";
        public const string TemplateBindingExtension = KnownNamespaces.ProxyTypes + ".TemplateBindingExtension";
        public const string BindExtension = KnownNamespaces.ProxyTypes + ".BindExtension";
        public const string Properties = KnownNamespaces.ProxyTypes + ".Properties";
        public const string Property = KnownNamespaces.ProxyTypes + ".Property";
    }
}
