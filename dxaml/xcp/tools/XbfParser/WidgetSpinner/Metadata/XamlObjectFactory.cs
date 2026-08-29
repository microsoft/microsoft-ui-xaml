// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata.KnownTypes;
using Microsoft.Xaml.WidgetSpinner.Model;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    internal static class XamlObjectFactory
    {
        internal static XamlObject CreateObjectFromFullName(string fullName)
        {
            Func<XamlObject> constructor;

            if (!s_knownTypeConstructorsByFullName.TryGetValue(fullName, out constructor))
            {
                constructor = () => new XamlObject() { Type = XamlTypeRegistry.Instance.GetXamlTypeByFullName(fullName) };
            }

            return constructor();
        }

        internal static XamlObject CreateObjectFromStableXbfTypeIndex(StableXbfTypeIndex index)
        {
            Func<XamlObject> constructor;

            if (!s_knownTypeConstructorsByStableXbfIndex.TryGetValue(index, out constructor))
            {
                constructor = () => new XamlObject() { Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(index) };
            }

            return constructor();
        }

        internal static XamlObject CreateObjectFromXamlType(XamlType xamlType)
        {
            Func<XamlObject> constructor;

            if (!s_knownTypeConstructorsByFullName.TryGetValue(xamlType.FullName, out constructor))
            {
                constructor = () => new XamlObject() { Type = xamlType };
            }

            return constructor();
        }

        internal static XamlStaticResourceExtension CreateXamlStaticResourceExtension()
        {
            return new XamlStaticResourceExtension();
        }

        internal static XamlThemeResourceExtension CreateXamlThemeResourceExtension()
        {
            return new XamlThemeResourceExtension();
        }

        internal static XamlTemplateBindingExtension CreateXamlTemplateBindingExtension()
        {
            return new XamlTemplateBindingExtension();
        }

        internal static XamlNullExtension CreateXamlNullExtension()
        {
            return new XamlNullExtension();
        }

        internal static XamlCustomResourceExtension CreateXamlCustomResourceExtension()
        {
            return new XamlCustomResourceExtension();
        }

        private static readonly Dictionary<StableXbfTypeIndex, Func<XamlObject>> s_knownTypeConstructorsByStableXbfIndex = new Dictionary<StableXbfTypeIndex, Func<XamlObject>>()
        {
            {StableXbfTypeIndex.StaticResource, CreateXamlStaticResourceExtension },
            {StableXbfTypeIndex.ThemeResource, CreateXamlThemeResourceExtension },
            {StableXbfTypeIndex.TemplateBinding, CreateXamlTemplateBindingExtension },
            {StableXbfTypeIndex.NullExtension, CreateXamlNullExtension },
            {StableXbfTypeIndex.CustomResource, CreateXamlCustomResourceExtension },
        };

        private static readonly Dictionary<string, Func<XamlObject>> s_knownTypeConstructorsByFullName = new Dictionary<string, Func<XamlObject>>()
        {
            {StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[StableXbfTypeIndex.StaticResource].FullName, CreateXamlStaticResourceExtension },
            {StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[StableXbfTypeIndex.ThemeResource].FullName, CreateXamlThemeResourceExtension },
            {StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[StableXbfTypeIndex.TemplateBinding].FullName, CreateXamlTemplateBindingExtension },
            {StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[StableXbfTypeIndex.NullExtension].FullName, CreateXamlNullExtension },
            {StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[StableXbfTypeIndex.CustomResource].FullName, CreateXamlCustomResourceExtension },
        };
    }
}
