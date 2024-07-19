// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Metadata.KnownTypes;
using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.XBF;
using System;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.Xaml.WidgetSpinner.Reader
{
    internal partial class XbfReader : BinaryReader
    {
        // Strings are encoded in XBF as the number of UTF-16 code units followed by an array
        // of the specified number of UTF-16 code units.
        // This would be exactly the same as BinaryReader.ReadString() if the count were LEB128 encoded.
        internal string ReadXbfString()
        {
            // Internally XAML uses unsigned int to store string length whereas C# uses int,
            // so technically there can be loss of data. In practice, however, you're going
            // to run into other problems before that's an issue.
            var length = ReadInt32();
            if (length < 0)
            {
                throw new InvalidDataException("XBF stream encoded a string that is longer than can be represented by .Net");
            }
            var str = new string(ReadChars(length));

            return str;
        }

        internal List<T> ReadVector<T>(Func<XbfReader, T> elementReaderDelegate, bool use7BitEncodingForLength = false)
        {
            // Technically XBF stores vector counts as unsigned integers, but you'll probably run into other problems
            // if there are actually vectors stored in the XBF that are larger than Int32.MaxValue, so narrowing down to Int32
            // should be fine.
            var length = (use7BitEncodingForLength) ? Read7BitEncodedInt() : ReadInt32();
            if (length < 0)
            {
                throw new InvalidDataException("The XBF file contained a vector with size greater than Int32.MaxValue.");
            }
            var vector = new List<T>(length);
            for (var count = 0; count < length; count++)
            {
                vector.Add(elementReaderDelegate(this));
            }

            return vector;
        }

        internal PersistedXamlNode ReadPersistedXamlNode()
        {
            var value = ReadUInt16();
            var isTrusted = (value & 0x8000) == 0x8000;
            var objectId = (ushort)(value & ~0x8000);

            return new PersistedXamlNode(objectId, isTrusted);
        }

        internal Tuple<string, object> ReadConstant()
        {
            string constantTypeName;
            object constantValue;
            var constantType = (PersistedConstantType)ReadByte();

            switch (constantType)
            {
                case PersistedConstantType.IsEnum:
                    {
                        var typeIndex = (StableXbfTypeIndex)ReadUInt16();
                        var value = ReadUInt32();
                        var enumName = StableXbfIndexMetadata.StableXbfTypeIndexToTypeInfo[typeIndex].FullName;

                        constantTypeName = PrimitiveTypeNames.Enum;
                        constantValue = new EnumValue(enumName, value);
                    }
                    break;
                case PersistedConstantType.IsNullString:
                    {
                        constantTypeName = PrimitiveTypeNames.String;
                        constantValue = null;
                    }
                    break;
                case PersistedConstantType.IsBoolFalse:
                    {
                        constantTypeName = PrimitiveTypeNames.Bool;
                        constantValue = false;
                    }
                    break;
                case PersistedConstantType.IsBoolTrue:
                    {
                        constantTypeName = PrimitiveTypeNames.Bool;
                        constantValue = true;
                    }
                    break;
                case PersistedConstantType.IsColor:
                    {
                        var argb = ReadUInt32();

                        constantTypeName = PrimitiveTypeNames.Color;
                        constantValue = new Color(argb);
                    }
                    break;
                case PersistedConstantType.IsFloat:
                    {
                        var value = ReadSingle();

                        constantTypeName = PrimitiveTypeNames.Float;
                        constantValue = value;
                    }
                    break;
                case PersistedConstantType.IsGridLength:
                    {
                        var type = (GridUnitType)ReadUInt32();
                        var value = ReadSingle();

                        constantTypeName = PrimitiveTypeNames.GridLength;
                        constantValue = new GridLength(type, value);
                    }
                    break;
                case PersistedConstantType.IsSharedString:
                    {
                        var sharedString = ReadSharedString();

                        constantTypeName = PrimitiveTypeNames.String;
                        constantValue = sharedString;
                    }
                    break;
                case PersistedConstantType.IsSigned:
                    {
                        var value = ReadInt32();

                        constantTypeName = PrimitiveTypeNames.Signed;
                        constantValue = value;
                    }
                    break;
                case PersistedConstantType.IsThickness:
                    {
                        var left = ReadSingle();
                        var top = ReadSingle();
                        var right = ReadSingle();
                        var bottom = ReadSingle();

                        constantTypeName = PrimitiveTypeNames.Thickness;
                        constantValue = new Thickness(left, top, right, bottom);
                    }
                    break;
                case PersistedConstantType.IsUniqueString:
                    {
                        var value = ReadXbfString();

                        constantTypeName = PrimitiveTypeNames.String;
                        constantValue = value;
                    }
                    break;
                default:
                    {
                        throw new InvalidDataException("Unrecognized constant type encountered.");
                    }
            }

            return new Tuple<string, object>(constantTypeName, constantValue);
        }

        internal string ReadSharedString()
        {
            var persistedNode = ReadPersistedXamlNode();
            var sharedString = Metadata.StringTable[persistedNode.ObjectId];

            return sharedString;
        }

        internal XamlProperty ReadXamlProperty()
        {
            var persistedNode = ReadPersistedXamlNode();

            var property = persistedNode.IsTrusted ? XamlPropertyRegistry.Instance.GetPropertyByIndex((StableXbfPropertyIndex)persistedNode.ObjectId) : Metadata.PropertyTable[persistedNode.ObjectId];

            return property;
        }

        internal XamlType ReadXamlType()
        {
            var persistedNode = ReadPersistedXamlNode();

            var xamlType = persistedNode.IsTrusted ? XamlTypeRegistry.Instance.GetXamlTypeByIndex((StableXbfTypeIndex)persistedNode.ObjectId) : Metadata.TypeTable[persistedNode.ObjectId];

            return xamlType;
        }

        internal XamlPredicateAndArgs ReadXamlPredicateAndArgs()
        {
            var predicateType = ReadXamlType();
            var arguments = ReadSharedString();

            return new XamlPredicateAndArgs(predicateType, arguments);
        }

        internal CustomRuntimeData ReadCustomRuntimeData()
        {
            CustomRuntimeData data;

            var typeIndex = (CustomWriterRuntimeDataTypeIndex)Read7BitEncodedInt();

            switch (typeIndex)
            {
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v1:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v2:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v3:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v4:
                case CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v5:
                    {
                        data = VisualStateGroupCollectionCustomRuntimeData.CreateAndDeserializeRuntimeData(this, typeIndex);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v1:
                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v2:
                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v3:
                    {
                        data = ResourceDictionaryCustomRuntimeData.CreateAndDeserializeRuntimeData(this, typeIndex);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.Style_v1:
                case CustomWriterRuntimeDataTypeIndex.Style_v2:
                    {
                        data = StyleCustomRuntimeData.CreateAndDeserializeRuntimeData(this, typeIndex);
                    }
                    break;

                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v1:
                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v2:
                case CustomWriterRuntimeDataTypeIndex.DeferredElement_v3:
                    {
                        data = DeferredElementCustomRuntimeData.CreateAndDeserializeRuntimeData(this, typeIndex);
                    }
                    break;

                default:
                    {
                        throw new InvalidDataException(string.Format("Unrecognized CustomWriterRuntimeDataTypeIndex {0}", typeIndex));
                    }
            }

            return data;
        }

        internal StreamOffsetToken ReadStreamOffsetToken()
        {
            return new StreamOffsetToken(Read7BitEncodedInt());
        }
    }
}
