// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.Xaml.WidgetSpinner.Metadata.KnownTypes;
using System;
using System.Collections.Generic;
using System.Linq;

// Copyright (c) Microsoft Corporation.  All rights reserved.

namespace Microsoft.Xaml.WidgetSpinner.Metadata
{
    /// <summary>
    /// Singleton class for retrieving type information
    /// </summary>
    public class XamlTypeRegistry
    {
        public static XamlTypeRegistry Instance { get; } = new XamlTypeRegistry();

        private XamlTypeRegistry()
        {
            Reset();
        }

        /// <summary>
        /// Gets a type by its full name
        /// </summary>
        /// <returns>Returns a XamlType, first registering a new one if this is the first time the type with the specified name has been retrieved</returns>
        public XamlType GetXamlTypeByFullName(string fullName)
        {
            XamlType retval;
            if (!m_registeredTypes.TryGetValue(fullName, out retval))
            {
                XamlTypeInfo typeInfo;
                if (FullNameToTypeInfo.TryGetValue(fullName, out typeInfo))
                {
                    retval = GetXamlTypeFromTypeInfo(typeInfo);
                }
                else
                {
                    retval = new XamlType(fullName);
                    RegisterType(fullName, retval);
                }
            }
            return retval;
        }

        /// <summary>
        /// Gets a type by its StableXbfTypeIndex
        /// </summary>
        /// <returns>Returns a XamlType, first registering a new one if this is the first time the type with the specified index has been retrieved</returns>
        public XamlType GetXamlTypeByIndex(StableXbfTypeIndex index)
        {
            XamlTypeInfo xamlTypeInfo;
            if (!StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo.TryGetValue(index, out xamlTypeInfo))
            {
                throw new ArgumentException(string.Format("The StableXbfTypeIndex {0} does not match any known type", index), nameof(index));
            }

            return GetXamlTypeFromTypeInfo(xamlTypeInfo);
        }

        public XamlType GetXamlTypeFromTypeInfo(XamlTypeInfo xamlTypeInfo)
        {
            XamlType baseType = null;
            if (xamlTypeInfo.BaseTypeIndex != StableXbfTypeIndex.UnknownType)
            {
                baseType = GetXamlTypeByIndex(xamlTypeInfo.BaseTypeIndex);
            }
            var xamlType = new XamlType(xamlTypeInfo.FullName, baseType, xamlTypeInfo.Flags);
            RegisterType(xamlTypeInfo.FullName, xamlType);

            return xamlType;
        }

        public void RegisterType(string fullName, XamlType type)
        {
            m_registeredTypes[fullName] = type;
        }

        internal void Reset()
        {
            m_registeredTypes = new Dictionary<string, XamlType>()
            {
                { PrimitiveTypeNames.Enum, new XamlType(PrimitiveTypeNames.Enum) },
                { PrimitiveTypeNames.Signed, new XamlType(PrimitiveTypeNames.Signed) },
                { PrimitiveTypeNames.Bool, new XamlType(PrimitiveTypeNames.Bool) },
                { PrimitiveTypeNames.Float, new XamlType(PrimitiveTypeNames.Float) },
                { PrimitiveTypeNames.Thickness, new XamlType(PrimitiveTypeNames.Thickness) },
                { PrimitiveTypeNames.GridLength, new XamlType(PrimitiveTypeNames.GridLength) },
                { PrimitiveTypeNames.Color, new XamlType(PrimitiveTypeNames.Color) },
                { PrimitiveTypeNames.String, new XamlType(PrimitiveTypeNames.String) },
            };
        }

        private static readonly IReadOnlyDictionary<string, XamlTypeInfo> FullNameToTypeInfo =
            StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo.ToDictionary(x => x.Value.FullName, x => x.Value);

        private Dictionary<string, XamlType> m_registeredTypes;
    }
}
