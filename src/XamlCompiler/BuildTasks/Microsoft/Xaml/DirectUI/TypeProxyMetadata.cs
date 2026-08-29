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
            UnderlyingTypeName = typeof(ProxyTypes.TemplateBindingExtension).FullName,
            BaseTypeName = "System.Windows.Markup.MarkupExtension"
        };

        public static TypeProxyMetadata StaticResourceExtension = new TypeProxyMetadata()
        {
            Name = "StaticResourceExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = typeof(ProxyTypes.StaticResourceExtension).FullName,
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
            UnderlyingTypeName = typeof(ProxyTypes.ThemeResourceExtension).FullName,
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
            UnderlyingTypeName = typeof(ProxyTypes.NullExtension).FullName,
            BaseTypeName = "System.Windows.Markup.MarkupExtension"
        };

        public static TypeProxyMetadata CustomResourceExtension = new TypeProxyMetadata()
        {
            Name = "CustomResourceExtension",
            IsMarkupExtension = true,
            UnderlyingTypeName = typeof(ProxyTypes.CustomResourceExtension).FullName,
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
            UnderlyingTypeName = typeof(ProxyTypes.BindExtension).FullName,
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
            UnderlyingTypeName = typeof(ProxyTypes.Properties).FullName,
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
            UnderlyingTypeName = typeof(ProxyTypes.Property).FullName,
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
}

namespace Microsoft.UI.Xaml.Markup.Compiler.DirectUI.ProxyTypes
{

    public abstract class MarkupExtension
    {
        public abstract object ProvideValue(IServiceProvider serviceProvider);
    }

    public class NullExtension : ProxyTypes.MarkupExtension
    {
        public NullExtension() { }

        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }

    public class StaticResourceExtension : ProxyTypes.MarkupExtension
    {
        public StaticResourceExtension() { }

        public StaticResourceExtension(object resourceKey)
        {
            ResourceKey = resourceKey;
        }
        public object ResourceKey { get; set; }
        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }
    
    public class ThemeResourceExtension : ProxyTypes.MarkupExtension
    {
        public ThemeResourceExtension() { }

        public ThemeResourceExtension(object resourceKey)
        {
            ResourceKey = resourceKey;
        }
        public object ResourceKey { get; set; }
        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }

    public class CustomResourceExtension : ProxyTypes.MarkupExtension
    {
        public CustomResourceExtension() { }

        public CustomResourceExtension(object resourceKey)
        {
            ResourceKey = resourceKey;
        }
        public object ResourceKey { get; set; }
        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }

    public class TemplateBindingExtension : ProxyTypes.MarkupExtension
    {
        public TemplateBindingExtension() { }

        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }

    public class BindExtension : ProxyTypes.MarkupExtension
    {
        public BindExtension() { }
        public BindExtension(string path) { }

        public override object ProvideValue(System.IServiceProvider serviceProvider)
        {
            return null;
        }
    }

    public class Properties
    {
        public Properties() { }
    }

    public class Property
    {
        public Property() { }
    }
}
