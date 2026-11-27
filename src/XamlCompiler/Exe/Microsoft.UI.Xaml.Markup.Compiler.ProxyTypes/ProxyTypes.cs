// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

// These types are only ever read as metadata by the XAML compiler (see DirectUI\TypeProxyMetadata.cs), their bodies never run.
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
