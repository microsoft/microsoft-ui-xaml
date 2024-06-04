// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace DumpXBF
{
    namespace FileFormat
    {
        namespace Metadata
        {
            /*
             * Header
             *
             */
            struct Header
            {
                // 4 byte magic number sequence [0x58,0x42,0x46,0x00]
                public byte[] magicNumber;

                // size of header metadata 
                public uint metadataSize;

                // size of node stream
                public uint nodeSize;

                // file version (major)
                public uint majorFileVersion;

                // file version (minor)
                public uint minorFileVersion;

                // offset to string table
                public ulong stringTableOffset;

                // offset to assembly table
                public ulong assemblyTableOffset;

                // offset to type namespaces table
                public ulong typeNamespaceTableOffset;

                // offset to type table
                public ulong typeTableOffset;

                // offset to property table
                public ulong propertyTableOffset;

                // offset to xml namespace table
                public ulong xmlNamespaceTableOffset;

                // hash
                public char[] hash;
            }

            /*
             * String Table
             * 
             */
            struct StringTable
            {
                public uint tableCount;
                public string[] stringEntry;
            }

            /*
             * Assembly Table
             * 
             */
            enum XamlTypeInfoProviderKind : uint
            {
                tpkUnknown,
                tpkNative,
                tpkManaged,
                tpkSystem,
                tpkParser,
                tpkAlternate,
            }

            struct AssemblyEntry
            {
                public XamlTypeInfoProviderKind providerKind;
                public uint stringId;
            }

            struct AssemblyTable
            {
                public uint tableCount;
                public AssemblyEntry[] assemblyEntry;
            }

            /*
             * TypeNamespace Table
             *
             */
            struct TypeNamespaceEntry
            {
                public uint assemblyId;
                public uint stringId;
            }

            struct TypeNamespaceTable
            {
                public uint tableCount;
                public TypeNamespaceEntry[] typeNamespaceEntry;
            }

            /*
             * Type Table
             * 
             */
            enum PersistedXamlTypeFlags : uint
            {
                TypeFlagsNone,
                TypeFlagsIsMarkupDirective
            }

            struct TypeEntry
            {
                public PersistedXamlTypeFlags typeFlags;
                public uint typeNamespaceId;
                public uint stringId;
            }

            struct TypeTable
            {
                public uint tableCount;
                public TypeEntry[] typeEntry;
            }

            /*
             * Property Table
             * 
             */
            [Flags]
            enum PersistedXamlPropertyFlags : uint
            {
                PropertyFlagsNone,
                PropertyFlagsIsXmlProperty = 0x01,
                PropertyFlagsIsMarkupDirective = 0x02,
                PropertyFlagsIsImplicitProperty = 0x04,
            }

            struct PropertyEntry
            {
                public PersistedXamlPropertyFlags propertyFlags;
                public uint typeId;
                public uint stringId;
            }

            struct PropertyTable
            {
                public uint tableCount;
                public PropertyEntry[] propertyEntry;
            }

            /*
             * Xml Namespace Table
             * 
             */
            struct XmlNamespaceEntry
            {
                public uint stringId;
            }

            struct XmlNamespaceTable
            {
                public uint tableCount;
                public XmlNamespaceEntry[] xmlNamespaceEntry;
            }
        }

        namespace NodeStream
        {
            /*
             * Xaml Node Stream
             * 
             */
            enum XamlNodeType : byte
            {
                xntNone,
                xntStartObject,
                xntEndObject,
                xntStartProperty,
                xntEndProperty,
                xntText,
                xntValue,
                xntNamespace,
                xntEndOfAttributes,
                xntEndOfStream,
                xntLineInfo,
                xntLineInfoAbsolute
            }

            //
            // preconverted types
            //
            enum PersistedXamlValueNodeType : byte
            {
                None,
                IsBoolFalse,
                IsBoolTrue,
                IsFloat,
                IsSigned,
                IsCString,
                IsKeyTime,
                IsThickness,
                IsLengthConverter,
                IsGridLength,
                IsColor,
                IsDuration
            }

            struct XamlLineInfo
            {
                public uint lineNumber;
                public uint linePosition;
            }

            struct XamlNodeInfo
            {
                public uint nodeId;
                public uint nodeFlags;
            }

            struct XamlNamespaceInfo
            {
                public XamlNodeInfo nodeInfo;
                public string name;
            }
        }
    }
}
