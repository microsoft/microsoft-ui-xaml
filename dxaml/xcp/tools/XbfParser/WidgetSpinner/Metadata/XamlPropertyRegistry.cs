// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    /// <summary>
    /// Singleton class for retrieving property information
    /// </summary>
    public class XamlPropertyRegistry
    {
        public static XamlPropertyRegistry Instance { get; } = new XamlPropertyRegistry();

        private XamlPropertyRegistry()
        {
            Reset();
        }

        /// <summary>
        /// Looks up a property by name
        /// </summary>
        /// <param name="declaringType">The type that the property belongs to</param>
        /// <param name="name">The name of the property. Does not include the declaring type's name</param>
        /// <returns>A XamlProperty whose PropertyType is a placeholder is derived from the property name</returns>
        public XamlProperty GetPropertyByName(XamlType declaringType, string name)
        {
            return GetPropertyByName(declaringType, null, name);
        }

        /// <summary>
        /// Looks up a property by name
        /// </summary>
        /// <param name="declaringType">The type that the property belongs to</param>
        /// <param name="propertyType">The type of the property. Can be null if unknown, in which case a placeholder is derived from the property name.</param>
        /// <param name="name">The name of the property. Does not include the declaring type's name</param>
        /// <returns>A XamlProperty</returns>
        public XamlProperty GetPropertyByName(XamlType declaringType, XamlType propertyType, string name)
        {
            var fullName = (declaringType != null) ? declaringType.FullName + "." + name : name;

            XamlProperty retval;
            if (!m_registeredProperties.TryGetValue(fullName, out retval))
            {
                retval = new XamlProperty(name, declaringType)
                {
                    PropertyType =
                        propertyType ?? XamlTypeRegistry.Instance.GetXamlTypeByFullName($"TypeOfProperty_{fullName}"),
                    IsAttached = name.IndexOf('.') != -1
                };

                m_registeredProperties[name] = retval;
            }

            return retval;
        }

        /// <summary>
        /// Look up a property by StableXbfPropertyIndex
        /// </summary>
        /// <param name="index">The StablePropertyIndex of the property</param>
        /// <returns>A XamlProperty</returns>
        public XamlProperty GetPropertyByIndex(StableXbfPropertyIndex index)
        {
            XamlPropertyInfo xamlPropertyInfo;
            if (!StableXbfIndexMetadata.StableXbfPropertyIndexToPropertyInfo.TryGetValue(index, out xamlPropertyInfo))
            {
                throw new ArgumentException(string.Format("The StableXbfPropertyIndex {0} does not match any known property", index), nameof(index));
            }

            var fullName = xamlPropertyInfo.FullName;

            var splitName = fullName.Split('.');
            var propertyName = ((xamlPropertyInfo.Flags & XamlPropertyFlags.IsAttached) == XamlPropertyFlags.IsAttached)
                ? $"{splitName[splitName.Length - 2]}.{splitName[splitName.Length - 1]}"
                : $"{splitName[splitName.Length - 1]}";

            var declaringType = XamlTypeRegistry.Instance.GetXamlTypeByIndex(xamlPropertyInfo.DeclaringTypeIndex);
            // Not all built-in properties have a type that was assigned a StableXbfTypeIndex (e.g. SemanticView.ZoomedInView,
            // which is of type Windows.UI.Xaml.Controls.ISemanticZoomInformation)
            var xamlPropertyType = (xamlPropertyInfo.PropertyTypeIndex != StableXbfTypeIndex.UnknownType)
                ? XamlTypeRegistry.Instance.GetXamlTypeByIndex(xamlPropertyInfo.PropertyTypeIndex)
                : null;

            var xamlProperty = GetPropertyByName(declaringType, xamlPropertyType, propertyName);
            xamlProperty.IsVisualTreeProperty = (xamlPropertyInfo.Flags & XamlPropertyFlags.IsVisualTreeProperty) ==
                                                XamlPropertyFlags.IsVisualTreeProperty;

            return xamlProperty;
        }

        internal void Reset()
        {
            m_registeredProperties = new Dictionary<string, XamlProperty>();
        }

        private Dictionary<string, XamlProperty> m_registeredProperties;
    }
}
