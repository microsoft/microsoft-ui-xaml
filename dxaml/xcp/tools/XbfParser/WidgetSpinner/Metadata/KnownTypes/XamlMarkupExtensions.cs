// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Model;

namespace Microsoft.Xaml.WidgetSpinner.Metadata.KnownTypes
{
    /// <summary>
    /// Represents a StaticResource markup extension
    /// </summary>
    public class XamlStaticResourceExtension : XamlObject
    {
        public string ResourceKey
        {
            get
            {
                object retval;
                m_internalPropertyStore.TryGetValue(
                    XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.StaticResource_ResourceKey), out retval);

                return (string)retval;
            }
            internal set
            {
                SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.StaticResource_ResourceKey), value);
            }
        }

        internal XamlStaticResourceExtension()
        {
            Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.StaticResource);
        }
    }

    /// <summary>
    /// Represents a ThemeResource markup extension
    /// </summary>
    public class XamlThemeResourceExtension : XamlObject
    {
        public string ResourceKey
        {
            get
            {
                object retval;
                m_internalPropertyStore.TryGetValue(
                    XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.ThemeResource_ResourceKey), out retval);

                return (string)retval;
            }
            internal set
            {
                SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.ThemeResource_ResourceKey), value);
            }
        }

        internal XamlThemeResourceExtension()
        {
            Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.ThemeResource);
        }
    }

    /// <summary>
    /// Represents a TemplateBinding markup extension
    /// </summary>
    public class XamlTemplateBindingExtension : XamlObject
    {
        public XamlProperty Property
        {
            get
            {
                object retval;
                m_internalPropertyStore.TryGetValue(
                    XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.TemplateBinding_Property), out retval);

                return (XamlProperty)retval;
            }
            internal set
            {
                SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.TemplateBinding_Property), value);
            }
        }

        internal XamlTemplateBindingExtension()
        {
            Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.TemplateBinding);
        }
    }

    /// <summary>
    /// Represents a x:Null markup extension
    /// </summary>
    public class XamlNullExtension : XamlObject
    {
        internal XamlNullExtension()
        {
            Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.NullExtension);
        }
    }

    /// <summary>
    /// Represents a CustomResource markup extension
    /// </summary>
    public class XamlCustomResourceExtension : XamlObject
    {
        public string ResourceKey
        {
            get
            {
                object retval;
                m_internalPropertyStore.TryGetValue(
                    XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.CustomResource_ResourceKey), out retval);

                return (string)retval;
            }
            internal set
            {
                SetValue(XamlPropertyRegistry.Instance.GetPropertyByIndex(StableXbfPropertyIndex.CustomResource_ResourceKey), value);
            }
        }

        internal XamlCustomResourceExtension()
        {
            Type = XamlTypeRegistry.Instance.GetXamlTypeByIndex(StableXbfTypeIndex.CustomResource);
        }
    }
}
