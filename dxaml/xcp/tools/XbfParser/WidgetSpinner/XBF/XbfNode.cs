// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public enum XbfNodeType
    {
        PushScope = 1,
        PopScope = 2,
        AddNamespace = 3,
        PushConstant = 4,
        SetValue = 7,
        SetValueFromMarkupExtension = 32,
        AddToCollection = 8,
        AddToDictionary = 9,
        AddToDictionaryWithKey = 10,
        CheckPeerType = 11,
        SetConnectionId = 12,
        SetName = 13,
        GetResourcePropertyBag = 14,
        SetDeferredProperty = 17,
        SetCustomRuntimeData = 15,
        PushScopeAddNamespace = 18,
        PushScopeGetValue = 19,
        PushScopeCreateTypeBeginInit = 20,
        PushScopeCreateTypeWithConstantBeginInit = 21,
        PushScopeCreateTypeWithTypeConvertedConstantBeginInit = 22,
        CreateTypeBeginInit = 23,
        CreateTypeWithConstantBeginInit = 24,
        CreateTypeWithTypeConvertedConstantBeginInit = 25,
        SetValueConstant = 26,
        SetValueTypeConvertedConstant = 27,
        SetValueTypeConvertedResolvedType = 29,
        SetValueTypeConvertedResolvedProperty = 28,
        ProvideStaticResourceValue = 34,
        SetValueFromStaticResource = 30,
        ProvideThemeResourceValue = 35,
        SetValueFromThemeResource = 36,
        SetValueFromTemplateBinding = 31,
        EndInitPopScope = 33,
        BeginConditionalScope = 38,
        EndConditionalScope = 39,
        EndInitProvideValuePopScope = 40,

        // We never expect to see these node types appear in an XBF file
        None = 0,
        PushResolvedType = 5,
        PushResolvedProperty = 6,
        SetResourceDictionaryItems = 16,
        EndOfStream = 37,
        CreateType = 128,
        CreateTypeWithInitialValue = 129,
        BeginInit = 130,
        EndInit = 131,
        GetValue = 132,
        TypeConvertValue = 133,
        PushScopeCreateType = 134,
        PushScopeCreateTypeWithConstant = 135,
        PushScopeCreateTypeWithTypeConvertedConstant = 136,
        CreateTypeWithConstant = 137, // unclear when this might show up, but it is possible
        CreateTypeWithTypeConvertedConstant = 138,
        ProvideValue = 139, // can appear if a Binding is added to a ResourceDictionary and it's an older (minversion < RS5) XBF
        ProvideTemplateBindingValue = 140,
        SetDirectiveProperty = 141,
        StreamOffsetMarker = 142,
    }

    public struct LineInfo
    {
        public int Line { get; }
        public int Column { get; }

        internal LineInfo(int line, int column) : this()
        {
            Line = line;
            Column = column;
        }

        public override string ToString()
        {
            return $"Line: {Line}, Column: {Column}";

        }
    }

    public class XbfNode
    {
        public XbfNodeType NodeType { get; }
        public LineInfo LineInfo { get; private set; }

        public long NodeStreamOffset { get; }

        internal XbfNode(XbfNodeType nodeType, long nodeStreamOffset)
        {
            NodeType = nodeType;
            NodeStreamOffset = nodeStreamOffset;
        }

        public override string ToString()
        {
            return $"[{LineInfo}] {NodeType}";
        }

        internal static XbfNode Deserialize(XbfReader reader, List<Tuple<int, int, int>> lineStream)
        {
            var currentNodeStreamOffset = reader.BaseStream.Position;
            var nodeType = (XbfNodeType)reader.ReadByte();
            XbfNode node;

            switch (nodeType)
            {
                case XbfNodeType.PushScope:
                case XbfNodeType.PopScope:
                case XbfNodeType.AddToCollection:
                case XbfNodeType.AddToDictionary:
                case XbfNodeType.EndInitPopScope:
                case XbfNodeType.EndConditionalScope:
                    {
                        node = new XbfNode(nodeType, currentNodeStreamOffset);
                    }
                    break;

                case XbfNodeType.AddNamespace:
                case XbfNodeType.PushScopeAddNamespace:
                    {
                        var persistedNode = reader.ReadPersistedXamlNode();
                        var prefix = reader.ReadXbfString();
                        var xamlNamespace = reader.Metadata.XamlXmlNamespaceTable[persistedNode.ObjectId];
                        // Namespaces are returned as a tuple of the xmlns prefix and a XamlXmlNamespace object
                        node = new XbfNode<string, XamlXmlNamespace>(nodeType, currentNodeStreamOffset, prefix, xamlNamespace);
                    }
                    break;

                case XbfNodeType.PushConstant:
                case XbfNodeType.AddToDictionaryWithKey:
                case XbfNodeType.SetConnectionId:
                case XbfNodeType.SetName:
                case XbfNodeType.GetResourcePropertyBag:
                case XbfNodeType.ProvideStaticResourceValue:
                case XbfNodeType.ProvideThemeResourceValue:
                    {
                        var constant = reader.ReadConstant();
                        // Constants are returned as a tuple of the string representing the type of constant
                        // and a strongly typed value.
                        node = new XbfNode<string, object>(nodeType, currentNodeStreamOffset, constant.Item1, constant.Item2);
                    }
                    break;

                case XbfNodeType.SetValue:
                case XbfNodeType.SetValueFromMarkupExtension:
                case XbfNodeType.PushScopeGetValue:
                    {
                        var property = reader.ReadXamlProperty();

                        node = new XbfNode<XamlProperty>(nodeType, currentNodeStreamOffset, property);
                    }
                    break;

                case XbfNodeType.CheckPeerType:
                    {
                        var typeName = reader.ReadXbfString();

                        node = new XbfNode<string>(nodeType, currentNodeStreamOffset, typeName);
                    }
                    break;

                case XbfNodeType.SetDeferredProperty:
                    {
                        var property = reader.ReadXamlProperty();
                        var substreamIndex = reader.Read7BitEncodedInt();
                        var staticResourceCount = reader.Read7BitEncodedInt();
                        var themeResourceCount = reader.Read7BitEncodedInt();

                        var referencedStaticResources = new List<string>(staticResourceCount);
                        var referencedThemeResources = new List<string>(themeResourceCount);

                        for (var i = 0; i < staticResourceCount; i++)
                        {
                            referencedStaticResources.Add(reader.ReadSharedString());
                        }
                        for (var i = 0; i < themeResourceCount; i++)
                        {
                            referencedThemeResources.Add(reader.ReadSharedString());
                        }

                        node = new XbfNode<XamlProperty, int, List<string>, List<string>>(nodeType, currentNodeStreamOffset,
                            property, substreamIndex, referencedStaticResources, referencedThemeResources);
                    }
                    break;

                case XbfNodeType.SetCustomRuntimeData:
                    {
                        var substreamIndex = reader.Read7BitEncodedInt();

                        // This chunk (for resource references that need to be pre-resolved) exists for TH1 M1 compat.
                        // It's no longer used, but in order to avoid a format change GenXbf will still write a number 
                        // of zeroes, and the bytes still need to be consumed.
                        var staticResourcecount = reader.Read7BitEncodedInt();
                        var themeResourceCount = reader.Read7BitEncodedInt();
                        for (var i = 0; i < (staticResourcecount + themeResourceCount); i++)
                        {
                            reader.ReadPersistedXamlNode();
                        }

                        var customRuntimeData = reader.ReadCustomRuntimeData();

                        node = new XbfNode<CustomRuntimeData, int>(nodeType, currentNodeStreamOffset, customRuntimeData, substreamIndex);

                    }
                    break;

                case XbfNodeType.CreateTypeBeginInit:
                case XbfNodeType.PushScopeCreateTypeBeginInit:
                    {
                        var xamlType = reader.ReadXamlType();

                        node = new XbfNode<XamlType>(nodeType, currentNodeStreamOffset, xamlType);
                    }
                    break;

                case XbfNodeType.CreateTypeWithConstantBeginInit:
                case XbfNodeType.PushScopeCreateTypeWithConstantBeginInit:
                case XbfNodeType.CreateTypeWithTypeConvertedConstantBeginInit:
                case XbfNodeType.PushScopeCreateTypeWithTypeConvertedConstantBeginInit:
                    {
                        var xamlType = reader.ReadXamlType();
                        var constant = reader.ReadConstant();

                        node = new XbfNode<XamlType, string, object>(nodeType, currentNodeStreamOffset, xamlType, constant.Item1, constant.Item2);
                    }
                    break;

                case XbfNodeType.SetValueConstant:
                case XbfNodeType.SetValueTypeConvertedConstant:
                case XbfNodeType.SetValueFromStaticResource:
                case XbfNodeType.SetValueFromThemeResource:
                    {
                        var property = reader.ReadXamlProperty();
                        var constant = reader.ReadConstant();

                        node = new XbfNode<XamlProperty, string, object>(nodeType, currentNodeStreamOffset, property, constant.Item1, constant.Item2);
                    }
                    break;

                case XbfNodeType.SetValueTypeConvertedResolvedType:
                    {
                        var property = reader.ReadXamlProperty();
                        var value = reader.ReadXamlType();

                        node = new XbfNode<XamlProperty, XamlType>(nodeType, currentNodeStreamOffset, property, value);
                    }
                    break;

                case XbfNodeType.SetValueTypeConvertedResolvedProperty:
                case XbfNodeType.SetValueFromTemplateBinding:
                    {
                        var property = reader.ReadXamlProperty();
                        var value = reader.ReadXamlProperty();

                        node = new XbfNode<XamlProperty, XamlProperty>(nodeType, currentNodeStreamOffset, property, value);
                    }
                    break;

                case XbfNodeType.BeginConditionalScope:
                    {
                        node = new XbfNode<XamlPredicateAndArgs>(nodeType, currentNodeStreamOffset, reader.ReadXamlPredicateAndArgs());
                    }
                    break;

                default:
                    {
                        // Create a placeholder node for unexpected node types, but also raise a warning
                        // 1) orphaned ProvideValue can appear if a Binding is used as a resource in a ResourceDictionary
                        // 2) CreateTypeWithConstant can also appear, but it is currently unknown what markup triggers its appearance
                        ErrorService.Instance.RaiseWarning($"Unexpected node with type {nodeType} appeared in the node stream at offset {currentNodeStreamOffset} during deserialization. This is harmless as it is treated as a no-op, but may indicate a problem in the parser during XBF generation.");
                        node = new XbfNode(nodeType, currentNodeStreamOffset);
                    }
                    break;
            }

            node.LineInfo = ResolveLineInfo(currentNodeStreamOffset, lineStream);

            return node;
        }

        private static LineInfo ResolveLineInfo(long nodeStreamOffset, List<Tuple<int, int, int>> lineStream)
        {
            return new LineInfo();
        }
    }
}