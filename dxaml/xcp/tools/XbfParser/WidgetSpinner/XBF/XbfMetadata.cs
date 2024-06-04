// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Common;
using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class XbfMetadata
    {
        public Version XbfVersion { get; private set; }
        public ReadOnlyCollection<string> StringTable { get; private set; }
        public ReadOnlyCollection<XamlAssembly> AssemblyTable { get; private set; }
        public ReadOnlyCollection<XamlTypeNamespace> TypeNamespaceTable { get; private set; }
        public ReadOnlyCollection<XamlType> TypeTable { get; private set; }
        public ReadOnlyCollection<XamlProperty> PropertyTable { get; private set; }
        public ReadOnlyCollection<XamlXmlNamespace> XamlXmlNamespaceTable { get; private set; }

        internal XbfMetadata()
        {
        }

        internal static XbfMetadata Deserialize(XbfReader reader)
        {
            var metadata = new XbfMetadata();

            var metadataStart = reader.BaseStream.Position;

            var majorVersion = reader.ReadInt32();
            var minorVersion = reader.ReadInt32();
            metadata.XbfVersion = new Version(majorVersion, minorVersion);
            if (!metadata.XbfVersion.IsSupportedXbfVersion())
            {
                throw new InvalidDataException(string.Format("XBF v{0}.{1} is not supported by this library.", metadata.XbfVersion.Major, metadata.XbfVersion.Minor));
            }

            var stringTableOffset = reader.ReadInt64();
            var assemblyListOffset = reader.ReadInt64();
            var typeNamespaceListOffset = reader.ReadInt64();
            var typeListOffset = reader.ReadInt64();
            var propertyListOffset = reader.ReadInt64();
            var xamlXmlNamespaceListOffset = reader.ReadInt64();
            var xbfHash = reader.ReadBytes(64);

            if (reader.BaseStream.Position != metadataStart + stringTableOffset)
            {
                throw new InvalidDataException(string.Format("String table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + stringTableOffset, reader.BaseStream.Position));
            }
            var stringTable = reader.ReadVector((r) =>
                {
                    var str = r.ReadXbfString();
                    if (metadata.XbfVersion.XbfMetadataStringsAreNullTerminated())
                    {
                        // Consume the null terminator
                        r.ReadInt16();
                    }
                    return str;
                });

            if (reader.BaseStream.Position != metadataStart + assemblyListOffset)
            {
                throw new InvalidDataException(string.Format("Assembly table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + assemblyListOffset, reader.BaseStream.Position));
            }
            var assemblyTable = reader.ReadVector((r) =>
                {
                    var kind = (XamlTypeInfoProviderKind)r.ReadInt32();
                    var name = stringTable[r.ReadInt32()];

                    return new XamlAssembly(kind, name);
                });

            if (reader.BaseStream.Position != metadataStart + typeNamespaceListOffset)
            {
                throw new InvalidDataException(string.Format("TypeNamespace table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + typeNamespaceListOffset, reader.BaseStream.Position));
            }
            var typeNamespaceTable = reader.ReadVector((r) =>
                {
                    var assembly = assemblyTable[r.ReadInt32()];
                    var name = stringTable[r.ReadInt32()];

                    return new XamlTypeNamespace(assembly, name);
                });

            if (reader.BaseStream.Position != metadataStart + typeListOffset)
            {
                throw new InvalidDataException(string.Format("Type table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + typeListOffset, reader.BaseStream.Position));
            }
            // These will be resolved into XamlTypes once the full metadata has been deserialized
            var persistedTypeTable = reader.ReadVector((r) =>
                {
                    var flags = (PersistedXamlTypeFlags)r.ReadUInt32();
                    var namespaceId = r.ReadInt32();
                    var name = stringTable[r.ReadInt32()];

                    return new PersistedXamlType(flags, namespaceId, name);
                });

            if (reader.BaseStream.Position != metadataStart + propertyListOffset)
            {
                throw new InvalidDataException(string.Format("Property table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + propertyListOffset, reader.BaseStream.Position));
            }
            // These will be resolved into XamlProperties once the full metadata has been deserialized
            var persistedPropertyTable = reader.ReadVector((r) =>
                {
                    var flags = (PersistedXamlPropertyFlags)r.ReadUInt32();
                    var declaringTypeId = r.ReadInt32();
                    var name = stringTable[r.ReadInt32()];

                    return new PersistedXamlProperty(flags, declaringTypeId, name);
                });

            if (reader.BaseStream.Position != metadataStart + xamlXmlNamespaceListOffset)
            {
                throw new InvalidDataException(string.Format("XamlXmlNamespace table is not at expected offset (Expected: {0}, Actual: {1}", metadataStart + xamlXmlNamespaceListOffset, reader.BaseStream.Position));
            }
            var xamlXmlNamespaceTable = reader.ReadVector((r) =>
                {
                    var uri = stringTable[r.ReadInt32()];

                    return new XamlXmlNamespace(uri);
                });

            // Resolve our types and propertyes
            var typeTable = ResolvePersistedXamlTypes(persistedTypeTable, typeNamespaceTable, xamlXmlNamespaceTable);
            var propertyTable = ResolvePersistedXamlProperties(persistedPropertyTable, typeTable);

            metadata.StringTable = new ReadOnlyCollection<string>(stringTable);
            metadata.AssemblyTable = new ReadOnlyCollection<XamlAssembly>(assemblyTable);
            metadata.TypeNamespaceTable = new ReadOnlyCollection<XamlTypeNamespace>(typeNamespaceTable);
            metadata.TypeTable = new ReadOnlyCollection<XamlType>(typeTable);
            metadata.PropertyTable = new ReadOnlyCollection<XamlProperty>(propertyTable);
            metadata.XamlXmlNamespaceTable = new ReadOnlyCollection<XamlXmlNamespace>(xamlXmlNamespaceTable);

            return metadata;
        }

        private static List<XamlType> ResolvePersistedXamlTypes(List<PersistedXamlType> persistedTypeTable, List<XamlTypeNamespace> typeNamespaceTable, List<XamlXmlNamespace> xamlXmlNamespaceTable)
        {
            return persistedTypeTable.Select((persistedXamlType) =>
            {
                string owningNamespace;
                if ((persistedXamlType.Flags & PersistedXamlTypeFlags.IsUnknown) == PersistedXamlTypeFlags.IsUnknown)
                {
                    var baseXmlns = CrackConditionalXmlns(xamlXmlNamespaceTable[persistedXamlType.NamespaceId].NamespaceUri).Item1;
                    owningNamespace = baseXmlns.Substring("using:".Length);
                }
                else
                {
                    owningNamespace = typeNamespaceTable[persistedXamlType.NamespaceId].Name;
                }
                var xamlType = XamlTypeRegistry.Instance.GetXamlTypeByFullName(owningNamespace + '.' + persistedXamlType.Name);

                if ((persistedXamlType.Flags & PersistedXamlTypeFlags.IsMarkupDirective) == PersistedXamlTypeFlags.IsMarkupDirective)
                {
                    // Currently, only x:Null
                    if (persistedXamlType.Name != "Null")
                    {
                        throw new Exception("Encountered a markup directive type that is not the x:Null markup extension type.");
                    }
                    xamlType.IsMarkupExtension = true;
                }

                return xamlType;
            }).ToList();
        }

        private static List<XamlProperty> ResolvePersistedXamlProperties(List<PersistedXamlProperty> persistedPropertyTable, List<XamlType> typeTable)
        {
            return persistedPropertyTable.Select((persistedXamlProperty) =>
            {
                if ((persistedXamlProperty.Flags & PersistedXamlPropertyFlags.IsMarkupDirective) == PersistedXamlPropertyFlags.IsMarkupDirective)
                {
                    // What about stuff like xml:lang or xml:space?
                    return XamlPropertyRegistry.Instance.GetPropertyByName(null, "x:" + persistedXamlProperty.Name);
                }
                else
                {
                    var declaringType = typeTable[persistedXamlProperty.DeclaringTypeId];
                    return XamlPropertyRegistry.Instance.GetPropertyByName(declaringType, persistedXamlProperty.Name);
                }
            }).ToList();
        }

        private static Tuple<string, string> CrackConditionalXmlns(string xmlns)
        {
            var delimiterIndex = xmlns.IndexOf('?');
            return (delimiterIndex != -1) ? new Tuple<string, string>(xmlns.Substring(0, delimiterIndex), xmlns.Substring(delimiterIndex + 1)) : new Tuple<string, string>(xmlns, string.Empty);
        }
    }
}
